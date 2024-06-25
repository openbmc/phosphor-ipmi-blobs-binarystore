#include "binarystore.hpp"

#include "sys_file.hpp"

#include <pb_decode.h>
#include <pb_encode.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <blobs-ipmid/blobs.hpp>
#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <ipmid/handler.hpp>
#include <map>
#include <memory>
#include <optional>
#include <phosphor-logging/elog.hpp>
#include <stdplus/str/cat.hpp>
#include <string>
#include <vector>

#include "binaryblob.pb.n.h"

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

namespace binstore
{

using namespace phosphor::logging;

std::unique_ptr<BinaryStoreInterface> BinaryStore::createFromConfig(
    const std::string& baseBlobId, std::unique_ptr<SysFile> file,
    std::optional<uint32_t> maxSize, std::optional<std::string> aliasBlobBaseId)
{
    if (baseBlobId.empty() || !file)
    {
        log<level::ERR>("Unable to create binarystore from invalid config",
                        entry("BASE_ID=%s", baseBlobId.c_str()));
        return nullptr;
    }

    auto store =
        std::make_unique<BinaryStore>(baseBlobId, std::move(file), maxSize);

    if (!store->loadSerializedData(aliasBlobBaseId))
    {
        return nullptr;
    }

    return store;
}

std::unique_ptr<BinaryStoreInterface>
    BinaryStore::createFromFile(std::unique_ptr<SysFile> file, bool readOnly,
                                std::optional<uint32_t> maxSize)
{
    if (!file)
    {
        log<level::ERR>("Unable to create binarystore from invalid file");
        return nullptr;
    }

    auto store =
        std::make_unique<BinaryStore>(std::move(file), readOnly, maxSize);

    if (!store->loadSerializedData())
    {
        return nullptr;
    }

    return store;
}

template <typename S>
static constexpr auto pbDecodeStr = [](pb_istream_t* stream,
                                       const pb_field_iter_t*,
                                       void** arg) noexcept {
    static_assert(sizeof(*std::declval<S>().data()) == sizeof(pb_byte_t));
    auto& s = *reinterpret_cast<S*>(*arg);
    s.resize(stream->bytes_left);
    return pb_read(stream, reinterpret_cast<pb_byte_t*>(s.data()), s.size());
};

template <typename T>
static pb_callback_t pbStrDecoder(T& t) noexcept
{
    return {{.decode = pbDecodeStr<T>}, &t};
}

template <typename S>
static constexpr auto pbEncodeStr = [](pb_ostream_t* stream,
                                       const pb_field_iter_t* field,
                                       void* const* arg) noexcept {
    static_assert(sizeof(*std::declval<S>().data()) == sizeof(pb_byte_t));
    const auto& s = *reinterpret_cast<const S*>(*arg);
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(
               stream, reinterpret_cast<const pb_byte_t*>(s.data()), s.size());
};

template <typename T>
static pb_callback_t pbStrEncoder(const T& t) noexcept
{
    return {{.encode = pbEncodeStr<T>}, const_cast<T*>(&t)};
}

bool BinaryStore::loadSerializedData(std::optional<std::string> aliasBlobBaseId)
{
    /* Load blob from sysfile if we know it might not match what we have.
     * Note it will overwrite existing unsaved data per design. */
    if (commitState_ == CommitState::Clean ||
        commitState_ == CommitState::Uninitialized)
    {
        return true;
    }

    std::string protoBlobId;
    static constexpr auto blobcb = [](pb_istream_t* stream,
                                      const pb_field_iter_t*,
                                      void** arg) noexcept {
        std::string id;
        std::vector<std::uint8_t> data;
        binstore_binaryblobproto_BinaryBlob msg = {
            .blob_id = pbStrDecoder(id),
            .data = pbStrDecoder(data),
        };
        if (!pb_decode(stream, binstore_binaryblobproto_BinaryBlob_fields,
                       &msg))
        {
            return false;
        }
        reinterpret_cast<decltype(std::declval<BinaryStore>().blobs_)*>(*arg)
            ->emplace(id, data);
        return true;
    };

    try
    {
        /* Parse length-prefixed format to protobuf */
        boost::endian::little_uint64_t size = 0;
        file_->readToBuf(0, sizeof(size), reinterpret_cast<char*>(&size));
        auto proto = file_->readAsStr(sizeof(size), size);

        auto ist = pb_istream_from_buffer(
            reinterpret_cast<const pb_byte_t*>(proto.data()), proto.size());
        binstore_binaryblobproto_BinaryBlobBase msg = {
            .blob_base_id = pbStrDecoder(protoBlobId),
            .blobs = {{.decode = blobcb}, &blobs_},
        };
        blobs_.clear(); // Purge old contents before new append during decode
        if (!pb_decode(&ist, binstore_binaryblobproto_BinaryBlobBase_fields,
                       &msg))
        {
            /* Fail to parse the data, which might mean no preexsiting blobs
             * and is a valid case to handle. Simply init an empty binstore. */
            commitState_ = CommitState::Uninitialized;
        }
    }
    catch (const std::system_error& e)
    {
        /* Read causes unexpected system-level failure */
        log<level::ERR>("Reading from sysfile failed",
                        entry("ERROR=%s", e.what()));
        return false;
    }
    catch (const std::exception& e)
    {
        /* Non system error originates from junk value in 'size' */
        commitState_ = CommitState::Uninitialized;
    }

    if (commitState_ == CommitState::Uninitialized)
    {
        log<level::WARNING>("Fail to parse. There might be no persisted blobs",
                            entry("BASE_ID=%s", baseBlobId_.c_str()));
        return true;
    }

    if (baseBlobId_.empty() && !protoBlobId.empty())
    {
        baseBlobId_ = std::move(protoBlobId);
    }
    else if (protoBlobId == aliasBlobBaseId)
    {
        log<level::WARNING>("Alias blob id, rename blob id...",
                            entry("LOADED=%s", protoBlobId.c_str()),
                            entry("RENAMED=%s", baseBlobId_.c_str()));
        std::string tmpBlobId = baseBlobId_;
        baseBlobId_ = *aliasBlobBaseId;
        return setBaseBlobId(tmpBlobId);
    }
    else if (protoBlobId != baseBlobId_ && !readOnly_)
    {
        /* Uh oh, stale data loaded. Clean it and commit. */
        // TODO: it might be safer to add an option in config to error out
        // instead of to overwrite.
        log<level::ERR>("Stale blob data, resetting internals...",
                        entry("LOADED=%s", protoBlobId.c_str()),
                        entry("EXPECTED=%s", baseBlobId_.c_str()));
        blobs_.clear();
        return this->commit();
    }

    return true;
}

std::string BinaryStore::getBaseBlobId() const
{
    return baseBlobId_;
}

bool BinaryStore::setBaseBlobId(const std::string& baseBlobId)
{
    for (auto it = blobs_.begin(); it != blobs_.end();)
    {
        auto curr = it++;
        if (curr->first.starts_with(baseBlobId_))
        {
            auto nh = blobs_.extract(curr);
            nh.key() = stdplus::strCat(
                baseBlobId,
                std::string_view(curr->first).substr(baseBlobId_.size()));
            blobs_.insert(std::move(nh));
        }
    }
    baseBlobId_ = baseBlobId;
    return this->commit();
}

std::vector<std::string> BinaryStore::getBlobIds() const
{
    std::vector<std::string> result;
    result.reserve(blobs_.size() + 1);
    result.emplace_back(getBaseBlobId());
    for (const auto& kv : blobs_)
    {
        result.emplace_back(kv.first);
    }
    return result;
}

bool BinaryStore::openOrCreateBlob(const std::string& blobId, uint16_t flags)
{
    if (!(flags & blobs::OpenFlags::read))
    {
        log<level::ERR>("OpenFlags::read not specified when opening",
                        entry("BLOB_ID=%s", blobId.c_str()));
        return false;
    }

    if (!currentBlob_.empty())
    {
        log<level::ERR>("Already handling a different blob",
                        entry("EXPECTED=%s", currentBlob_.c_str()),
                        entry("RECEIVED=%s", blobId.c_str()));
        return false;
    }

    if (readOnly_ && (flags & blobs::OpenFlags::write))
    {
        log<level::ERR>("Can't open the blob for writing: read-only store",
                        entry("BLOB_ID=%s", blobId.c_str()));
        return false;
    }

    writable_ = flags & blobs::OpenFlags::write;

    /* If there are uncommitted data, discard them. */
    if (!this->loadSerializedData())
    {
        return false;
    }

    /* Iterate and find if there is an existing blob with this id.
     * blobsPtr points to a BinaryBlob container with STL-like semantics*/
    if (blobs_.find(blobId) != blobs_.end())
    {
        currentBlob_ = blobId;
        return true;
    }

    /* Otherwise, create the blob and append it */
    if (readOnly_)
    {
        return false;
    }

    blobs_.emplace(blobId, std::vector<std::uint8_t>{});
    currentBlob_ = blobId;
    commitState_ = CommitState::Dirty;
    return true;
}

bool BinaryStore::deleteBlob(const std::string&)
{
    return false;
}

std::vector<uint8_t> BinaryStore::read(uint32_t offset, uint32_t requestedSize)
{
    if (currentBlob_.empty())
    {
        log<level::ERR>("No open blob to read");
        return {};
    }

    const auto& data = blobs_.find(currentBlob_)->second;

    /* If it is out of bound, return empty vector */
    if (offset >= data.size())
    {
        log<level::ERR>("Read offset is beyond data size",
                        entry("MAX_SIZE=0x%x", data.size()),
                        entry("RECEIVED_OFFSET=0x%x", offset));
        return {};
    }

    auto s = data.begin() + offset;
    return {s, s + std::min<size_t>(requestedSize, data.size() - offset)};
}

std::vector<uint8_t> BinaryStore::readBlob(const std::string& blobId) const
{
    const auto blobIt = blobs_.find(blobId);
    if (blobIt == blobs_.end())
    {
        throw ipmi::HandlerCompletion(ipmi::ccUnspecifiedError);
    }
    return blobIt->second;
}

static binstore_binaryblobproto_BinaryBlobBase makeEncoder(
    const std::string& base,
    const std::map<std::string, std::vector<std::uint8_t>>& blobs) noexcept
{
    using Blobs = std::decay_t<decltype(blobs)>;
    static constexpr auto blobcb = [](pb_ostream_t* stream,
                                      const pb_field_iter_t* field,
                                      void* const* arg) noexcept {
        const auto& blobs = *reinterpret_cast<const Blobs*>(*arg);
        for (const auto& [id, data] : blobs)
        {
            binstore_binaryblobproto_BinaryBlob msg = {
                .blob_id = pbStrEncoder(id),
                .data = pbStrEncoder(data),
            };
            if (!pb_encode_tag_for_field(stream, field) ||
                !pb_encode_submessage(
                    stream, binstore_binaryblobproto_BinaryBlob_fields, &msg))
            {
                return false;
            }
        }
        return true;
    };
    return {
        .blob_base_id = pbStrEncoder(base),
        .blobs = {{.encode = blobcb},
                  const_cast<void*>(reinterpret_cast<const void*>(&blobs))},
    };
}

static std::size_t
    payloadCalcSize(const binstore_binaryblobproto_BinaryBlobBase& msg)
{
    pb_ostream_t nost = {};
    if (!pb_encode(&nost, binstore_binaryblobproto_BinaryBlobBase_fields, &msg))
    {
        throw std::runtime_error(
            std::format("Calculating msg size: {}", PB_GET_ERROR(&nost)));
    }
    // Proto is prepended with the size of the proto
    return nost.bytes_written + sizeof(boost::endian::little_uint64_t);
}

bool BinaryStore::write(uint32_t offset, const std::vector<uint8_t>& data)
{
    if (currentBlob_.empty())
    {
        log<level::ERR>("No open blob to write");
        return false;
    }

    if (!writable_)
    {
        log<level::ERR>("Open blob is not writable");
        return false;
    }

    auto& bdata = blobs_.find(currentBlob_)->second;
    if (offset > bdata.size())
    {
        log<level::ERR>("Write would leave a gap with undefined data. Return.");
        return false;
    }

    std::size_t oldsize = bdata.size(), reqSize = offset + data.size();
    if (reqSize > bdata.size())
    {
        bdata.resize(reqSize);
    }

    if (payloadCalcSize(makeEncoder(baseBlobId_, blobs_)) >
        maxSize.value_or(
            std::numeric_limits<std::decay_t<decltype(*maxSize)>>::max()))
    {
        log<level::ERR>("Write data would make the total size exceed the max "
                        "size allowed. Return.");
        bdata.resize(oldsize);
        return false;
    }

    commitState_ = CommitState::Dirty;
    std::copy(data.begin(), data.end(), bdata.data() + offset);
    return true;
}

bool BinaryStore::commit()
{
    if (readOnly_)
    {
        log<level::ERR>("ReadOnly blob, not committing");
        return false;
    }

    /* Store as little endian to be platform agnostic. Consistent with read. */
    auto msg = makeEncoder(baseBlobId_, blobs_);
    auto outSize = payloadCalcSize(msg);
    if (outSize >
        maxSize.value_or(
            std::numeric_limits<std::decay_t<decltype(*maxSize)>>::max()))
    {
        log<level::ERR>("Commit Data exceeded maximum allowed size");
        return false;
    }
    std::string buf(outSize, '\0');
    auto& size = *reinterpret_cast<boost::endian::little_uint64_t*>(buf.data());
    auto ost = pb_ostream_from_buffer(reinterpret_cast<pb_byte_t*>(buf.data()) +
                                          sizeof(size),
                                      buf.size() - sizeof(size));
    pb_encode(&ost, binstore_binaryblobproto_BinaryBlobBase_fields, &msg);
    size = ost.bytes_written;
    try
    {
        file_->writeStr(buf, 0);
    }
    catch (const std::exception& e)
    {
        commitState_ = CommitState::CommitError;
        log<level::ERR>("Writing to sysfile failed",
                        entry("ERROR=%s", e.what()));
        return false;
    };

    commitState_ = CommitState::Clean;
    return true;
}

bool BinaryStore::close()
{
    currentBlob_.clear();
    writable_ = false;
    commitState_ = CommitState::Dirty;
    return true;
}

/*
 * Sets |meta| with size and state of the blob. Returns |blobState| with
 * standard definition from phosphor-ipmi-blobs header blob.hpp, plus OEM
 * flag bits BinaryStore::CommitState.

enum StateFlags
{
    open_read = (1 << 0),
    open_write = (1 << 1),
    committing = (1 << 2),
    committed = (1 << 3),
    commit_error = (1 << 4),
};

enum CommitState
{
    Dirty = (1 << 8), // In-memory data might not match persisted data
    Clean = (1 << 9), // In-memory data matches persisted data
    Uninitialized = (1 << 10), // Cannot find persisted data
    CommitError = (1 << 11)    // Error happened during committing
};

*/
bool BinaryStore::stat(blobs::BlobMeta* meta)
{
    uint16_t blobState = blobs::StateFlags::open_read;
    if (writable_)
    {
        blobState |= blobs::StateFlags::open_write;
    }

    if (commitState_ == CommitState::Clean)
    {
        blobState |= blobs::StateFlags::committed;
    }
    else if (commitState_ == CommitState::CommitError)
    {
        blobState |= blobs::StateFlags::commit_error;
    }
    blobState |= commitState_;

    if (!currentBlob_.empty())
    {
        meta->size = blobs_.find(currentBlob_)->second.size();
    }
    else
    {
        meta->size = 0;
    }
    meta->blobState = blobState;

    return true;
}

} // namespace binstore
