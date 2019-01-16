#include "binarystore.hpp"

#include <unistd.h>

#include <algorithm>
#include <blobs-ipmid/blobs.hpp>
#include <cstdint>
#include <memory>
#include <phosphor-logging/elog.hpp>
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

namespace internal
{

/* Helper methods to interconvert an uint64_t and bytes, LSB first.
   Convert bytes to uint64. Input should be exactly 8 bytes. */
uint64_t bytesToUint64(const std::string& bytes)
{
    if (bytes.size() != sizeof(uint64_t))
    {
        return 0;
    }

    return static_cast<uint64_t>(bytes[7]) << 56 |
           static_cast<uint64_t>(bytes[6]) << 48 |
           static_cast<uint64_t>(bytes[5]) << 40 |
           static_cast<uint64_t>(bytes[4]) << 32 |
           static_cast<uint64_t>(bytes[3]) << 24 |
           static_cast<uint64_t>(bytes[2]) << 16 |
           static_cast<uint64_t>(bytes[1]) << 8 |
           static_cast<uint64_t>(bytes[0]);
}

/* Convert uint64 to bytes, LSB first. */
std::string uint64ToBytes(uint64_t num)
{
    std::string result;
    for (size_t i = 0; i < sizeof(uint64_t); ++i)
    {
        result += static_cast<char>(num & 0xff);
        num >>= 8;
    }
    return result;
}

} // namespace internal

std::unique_ptr<BinaryStoreInterface>
    BinaryStore::createFromConfig(const std::string& baseBlobId,
                                  std::unique_ptr<SysFile> file,
                                  uint32_t maxSize)
{
    if (baseBlobId.empty() || !file)
    {
        log<level::ERR>("Unable to create binarystore from invalid config");
        return nullptr;
    }

    auto store =
        std::make_unique<BinaryStore>(baseBlobId, std::move(file), maxSize);

    store->blob_.set_blob_base_id(store->baseBlobId_);

    return std::move(store);
}

std::string BinaryStore::getBaseBlobId() const
{
    return baseBlobId_;
}

std::vector<std::string> BinaryStore::getBlobIds() const
{
    std::vector<std::string> result;
    result.push_back(baseBlobId_);

    for (const auto& blob : blob_.blobs())
    {
        result.push_back(blob.blob_id());
    }

    return result;
}

bool BinaryStore::openOrCreateBlob(const std::string& blobId, uint16_t flags)
{
    if (!(flags & blobs::OpenFlags::read))
    {
        log<level::ERR>("OpenFlags::read not specified when opening");
        return false;
    }

    if (currentBlob_ && (currentBlob_->blob_id() != blobId))
    {
        log<level::ERR>("Already handling a different blob",
                        entry("EXPECTED=%s", currentBlob_->blob_id().c_str()),
                        entry("RECEIVED=%s", blobId.c_str()));
        return false;
    }

    writable_ = flags & blobs::OpenFlags::write;

    // Load blob from sysfile if we know it might not match what we have.
    // Note it will overwrite existing unsaved data per design.
    if (commitState_ != CommitState::Clean)
    {
        log<level::NOTICE>("Try loading from persistent data");
        try
        {
            // Parse length-prefixed format to protobuf
            auto size =
                internal::bytesToUint64(file_->readAsStr(0, sizeof(uint64_t)));
            if (!blob_.ParseFromString(
                    file_->readAsStr(sizeof(uint64_t), size)))
            {
                // Fail to parse the data, which might mean no preexsiting data
                // and is a valid case to handle
                log<level::WARNING>(
                    "Fail to parse. There might be no persisted blobs");
            }
        }
        catch (const std::exception& e)
        {
            // read causes system-level failure
            log<level::ERR>("Reading from sysfile failed",
                            entry("ERROR=%s", e.what()));
        }
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
    currentBlob_ = blob_.add_blobs();
    currentBlob_->set_blob_id(blobId);

    log<level::NOTICE>("Created new blob");
    return true;
}

bool BinaryStore::deleteBlob(const std::string& blobId)
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
        log<level::ERR>("Read offset is beyond data size");
        return result;
    }

    uint32_t requestedEndPos = offset + requestedSize;

    result = std::vector<uint8_t>(
        dataPtr->begin() + offset,
        std::min(dataPtr->begin() + requestedEndPos, dataPtr->end()));
    return result;
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

    /* Copy (overwrite) the data */
    if (offset + data.size() > dataPtr->size())
    {
        dataPtr->resize(offset + data.size()); // not enough space, extend
    }
    std::copy(data.begin(), data.end(), dataPtr->begin() + offset);
    return true;
}

bool BinaryStore::commit()
{

    auto blobData = blob_.SerializeAsString();
    std::string commitData(internal::uint64ToBytes((uint64_t)blobData.size()));
    commitData += blobData;

    try
    {
        file_->writeStr(commitData, 0);
    }
    catch (const std::exception& e)
    {
        // TODO: logging
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

bool BinaryStore::stat()
{
    return false;
}

} // namespace binstore
