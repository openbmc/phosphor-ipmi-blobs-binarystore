#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace conf
{

struct BinaryBlobConfig
{
    std::string blobBaseId;    // Required
    std::string sysFilePath;   // Required
    uint32_t offsetBytes = 0;  // Optional
    uint32_t maxSizeBytes = 0; // Optional
};

/**
 * @brief Parse parameters from a config json
 * @param j: input json object
 * @param config: output BinaryBlobConfig
 * @throws: exception if config doens't have required fields
 */
static inline void parseFromConfigFile(const json& j, BinaryBlobConfig& config)
{
    j.at("blobBaseId").get_to(config.blobBaseId);
    j.at("sysFilePath").get_to(config.sysFilePath);
    if (j.find("offsetBytes") != j.end())
    {
        j.at("offsetBytes").get_to(config.offsetBytes);
    }
    if (j.find("maxSizeBytes") != j.end())
    {
        j.at("maxSizeBytes").get_to(config.maxSizeBytes);
    }
}

} // namespace conf
