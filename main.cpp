#include "handler.hpp"
#include "parse_config.hpp"
#include "sys_file.hpp"

#include <fcntl.h>
#include <google/protobuf/message_lite.h>

#include <blobs-ipmid/blobs.hpp>
#include <memory>
#include <phosphor-logging/elog.hpp>

#include "binaryblobconfig.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is required by the blob manager.
 * TODO: move the declaration to blobs.hpp since all handlers need it
 */
std::unique_ptr<blobs::GenericBlobInterface> createHandler();

#ifdef __cplusplus
}
#endif

/* Configuration file path */
constexpr auto blobConfigPath = "/usr/share/binaryblob/config.textproto";

std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    using namespace phosphor::logging;

    blobs::binaryblob::BinaryBlobConfig config;
    if (!blobs::binaryblob::parseFromConfigFile(blobConfigPath, &config))
    {
        return nullptr;
    }

    // Construct binary blobs from config and add to handler
    auto handler = std::make_unique<blobs::BinaryStoreBlobHandler>();

    for (const auto& conf : config.entries())
    {
        auto file = std::make_unique<binstore::SysFileImpl>(
            conf.sysfile_path(), conf.offset_bytes());

        log<level::INFO>("Loading from config with",
                         entry("BASE_ID=%s", conf.blob_base_id().c_str()),
                         entry("FILE=%s", conf.sysfile_path().c_str()),
                         entry("MAX_SIZE=%llx", static_cast<unsigned long long>(
                                                    conf.max_size_bytes())));

        handler->addNewBinaryStore(binstore::BinaryStore::createFromConfig(
            conf.blob_base_id(), std::move(file), conf.max_size_bytes()));
    }

    return std::move(handler);
}
