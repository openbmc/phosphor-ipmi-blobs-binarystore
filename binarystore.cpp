#include "binarystore.hpp"

#include <unistd.h>

#include <algorithm>
#include <blobs-ipmid/blobs.hpp>
#include <cstdint>
#include <memory>
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

std::unique_ptr<BinaryStoreInterface>
    BinaryStore::createFromConfig(const std::string& baseBlobId,
                                  const std::string& sysfilePath,
                                  uint32_t offset, uint32_t maxSize)
{
    // TODO: implement sysFile parsing
    return std::make_unique<BinaryStore>(baseBlobId, 0, offset, maxSize);
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
        return false;
    }

    if (currentBlob_ && (currentBlob_->blob_id() != blobId))
    {
        /* Already handling a different blob */
        return false;
    }

    writable_ = flags & blobs::OpenFlags::write;

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

    return true;
}

bool BinaryStore::deleteBlob(const std::string& blobId)
{
    // TODO: implement
    return false;
}

std::vector<uint8_t> BinaryStore::read(uint32_t offset, uint32_t requestedSize)
{
    std::vector<uint8_t> result;

    if (!currentBlob_)
    {
        return result;
    }

    /* If it is out of bound, return empty vector */
    auto dataPtr = currentBlob_->mutable_data();

    if (offset >= dataPtr->size())
    {
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
        return false;
    }

    if (!writable_)
    {
        return false;
    }

    auto dataPtr = currentBlob_->mutable_data();

    if (offset > dataPtr->size())
    {
        /* Will leave a gap with undefined data */
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
    return false;
}

bool BinaryStore::close()
{
    currentBlob_ = nullptr;
    writable_ = false;
    return true;
}

bool BinaryStore::stat()
{
    return false;
}

} // namespace binstore
