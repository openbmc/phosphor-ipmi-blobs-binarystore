#pragma once

#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <sys/stat.h>

#include <string>

#include "binaryblobconfig.pb.h"

namespace blobs
{

namespace binaryblob
{

using namespace google::protobuf;

/**
 * @brief parse a BinaryBlobConfig object from config file
 * @param filePath: path for config file
 * @param config: output BinaryBlobConfig pointer
 * @return: true if parsed successfully
 */
static inline bool parseFromConfigFile(const std::string& filePath,
                                       BinaryBlobConfig* config)
{
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0)
    {
        return false;
    }

    io::FileInputStream input(fd);
    input.SetCloseOnDelete(true);

    return TextFormat::Parse(&input, config);
}

} // namespace binaryblob

} // namespace blobs
