#include "binarystore.hpp"

#include "sys_file.hpp"

#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <blobs-ipmid/blobs.hpp>
#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <ipmid/handler.hpp>
#include <memory>
#include <optional>
#include <phosphor-logging/elog.hpp>
#include <stdplus/print.hpp>
#include <string>
#include <vector>

#include "binaryblob.pb.h"

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

bool BinaryStore::loadSerializedData(std::optional<std::string> aliasBlobBaseId)
{
    /* Load blob from sysfile if we know it might not match what we have.
     * Note it will overwrite existing unsaved data per design. */
    if (commitState_ == CommitState::Clean ||
        commitState_ == CommitState::Uninitialized)
    {
        return true;
    }

    log<level::NOTICE>("Try loading blob from persistent data",
                       entry("BASE_ID=%s", baseBlobId_.c_str()));
    try
    {
        /* Parse length-prefixed format to protobuf */
        boost::endian::little_uint64_t size = 0;
        file_->readToBuf(0, sizeof(size), reinterpret_cast<char*>(&size));

        if (!blob_.ParseFromString(file_->readAsStr(sizeof(uint64_t), size)))
        {
            /* Fail to parse the data, which might mean no preexsiting blobs
             * and is a valid case to handle. Simply init an empty binstore. */
            commitState_ = CommitState::Uninitialized;
        }

        // The new max size takes priority
        if (maxSize)
        {
            blob_.set_max_size_bytes(*maxSize);
        }
        else
        {
            blob_.clear_max_size_bytes();
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

    std::string alias = aliasBlobBaseId.value_or("");
    if (blob_.blob_base_id() == alias)
    {
        log<level::WARNING>("Alias blob id, rename blob id...",
                            entry("LOADED=%s", alias.c_str()),
                            entry("RENAMED=%s", baseBlobId_.c_str()));
        std::string tmpBlobId = baseBlobId_;
        baseBlobId_ = alias;
        return setBaseBlobId(tmpBlobId);
    }
    if (blob_.blob_base_id() != baseBlobId_ && !readOnly_)
    {
        /* Uh oh, stale data loaded. Clean it and commit. */
        // TODO: it might be safer to add an option in config to error out
        // instead of to overwrite.
        log<level::ERR>("Stale blob data, resetting internals...",
                        entry("LOADED=%s", blob_.blob_base_id().c_str()),
                        entry("EXPECTED=%s", baseBlobId_.c_str()));
        blob_.Clear();
        blob_.set_blob_base_id(baseBlobId_);
        return this->commit();
    }

    return true;
}

std::string BinaryStore::getBaseBlobId() const
{
    if (!baseBlobId_.empty())
    {
        return baseBlobId_;
    }

    return blob_.blob_base_id();
}

bool BinaryStore::setBaseBlobId(const std::string& baseBlobId)
{
    if (baseBlobId_.empty())
    {
        baseBlobId_ = blob_.blob_base_id();
    }

    std::string oldBlobId = baseBlobId_;
    size_t oldPrefixIndex = baseBlobId_.size();
    baseBlobId_ = baseBlobId;
    blob_.set_blob_base_id(baseBlobId_);
    auto blobsPtr = blob_.mutable_blobs();
    for (auto blob = blobsPtr->begin(); blob != blobsPtr->end(); blob++)
    {
        const std::string& blodId = blob->blob_id();
        if (blodId.starts_with(oldBlobId))
        {
            blob->set_blob_id(baseBlobId_ + blodId.substr(oldPrefixIndex));
        }
    }
    return this->commit();
}

std::vector<std::string> BinaryStore::getBlobIds() const
{
    std::vector<std::string> result;
    result.reserve(blob_.blobs().size() + 1);
    result.emplace_back(getBaseBlobId());
    std::for_each(
        blob_.blobs().begin(), blob_.blobs().end(),
        [&result](const auto& blob) { result.emplace_back(blob.blob_id()); });

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

    if (currentBlob_ && (currentBlob_->blob_id() != blobId))
    {
        log<level::ERR>("Already handling a different blob",
                        entry("EXPECTED=%s", currentBlob_->blob_id().c_str()),
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
    auto blobsPtr = blob_.mutable_blobs();
    auto blobIt =
        std::find_if(blobsPtr->begin(), blobsPtr->end(),
                     [&](const auto& b) { return b.blob_id() == blobId; });

    if (blobIt != blobsPtr->end())
    {
        currentBlob_ = &(*blobIt);
        return true;
    }

    /* Otherwise, create the blob and append it */
    if (readOnly_)
    {
        return false;
    }
    else
    {
        currentBlob_ = blob_.add_blobs();
        currentBlob_->set_blob_id(blobId);

        commitState_ = CommitState::Dirty;
        log<level::NOTICE>("Created new blob",
                           entry("BLOB_ID=%s", blobId.c_str()));
    }

    return true;
}

bool BinaryStore::deleteBlob(const std::string&)
{
    return false;
}

std::vector<uint8_t> BinaryStore::read(uint32_t offset, uint32_t requestedSize)
{
    std::vector<uint8_t> result;

    if (!currentBlob_)
    {
        log<level::ERR>("No open blob to read");
        return result;
    }

    auto dataPtr = currentBlob_->mutable_data();

    /* If it is out of bound, return empty vector */
    if (offset >= dataPtr->size())
    {
        log<level::ERR>("Read offset is beyond data size",
                        entry("MAX_SIZE=0x%x", dataPtr->size()),
                        entry("RECEIVED_OFFSET=0x%x", offset));
        return result;
    }

    uint32_t requestedEndPos = offset + requestedSize;

    result = std::vector<uint8_t>(
        dataPtr->begin() + offset,
        std::min(dataPtr->begin() + requestedEndPos, dataPtr->end()));
    return result;
}

std::vector<uint8_t> BinaryStore::readBlob(const std::string& blobId) const
{
    const auto blobs = blob_.blobs();
    const auto blobIt =
        std::find_if(blobs.begin(), blobs.end(),
                     [&](const auto& b) { return b.blob_id() == blobId; });

    if (blobIt == blobs.end())
    {
        throw ipmi::HandlerCompletion(ipmi::ccUnspecifiedError);
    }

    const auto blobData = blobIt->data();

    return std::vector<uint8_t>(blobData.begin(), blobData.end());
}

bool BinaryStore::write(uint32_t offset, const std::vector<uint8_t>& data)
{
    if (!currentBlob_)
    {
        log<level::ERR>("No open blob to write");
        return false;
    }

    if (!writable_)
    {
        log<level::ERR>("Open blob is not writable");
        return false;
    }

    auto dataPtr = currentBlob_->mutable_data();

    if (offset > dataPtr->size())
    {
        log<level::ERR>("Write would leave a gap with undefined data. Return.");
        return false;
    }

    bool needResize = offset + data.size() > dataPtr->size();

    // current size is the binary blob proto size + uint64 tracking the total
    // size of the binary blob.
    // currentSize = blob_size + x (uint64_t), where x = blob_size.
    size_t currentSize = blob_.SerializeAsString().size() +
                         sizeof(boost::endian::little_uint64_t);
    size_t sizeDelta = needResize ? offset + data.size() - dataPtr->size() : 0;

    if (maxSize && currentSize + sizeDelta > *maxSize)
    {
        log<level::ERR>("Write data would make the total size exceed the max "
                        "size allowed. Return.");
        return false;
    }

    commitState_ = CommitState::Dirty;
    /* Copy (overwrite) the data */
    if (needResize)
    {
        dataPtr->resize(offset + data.size()); // not enough space, extend
    }
    std::copy(data.begin(), data.end(), dataPtr->begin() + offset);
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
    auto blobData = blob_.SerializeAsString();
    boost::endian::little_uint64_t sizeLE = blobData.size();
    std::string commitData(reinterpret_cast<const char*>(sizeLE.data()),
                           sizeof(sizeLE));
    commitData += blobData;

    // This should never be true if it is blocked by the write command
    if (maxSize && sizeof(commitData) > *maxSize)
    {
        log<level::ERR>("Commit Data exceeded maximum allowed size");
        return false;
    }

    try
    {
        file_->writeStr(commitData, 0);
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
    currentBlob_ = nullptr;
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

    if (currentBlob_)
    {
        meta->size = currentBlob_->data().size();
    }
    else
    {
        meta->size = 0;
    }
    meta->blobState = blobState;

    return true;
}

} // namespace binstore
