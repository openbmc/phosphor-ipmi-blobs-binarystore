#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using std::uint32_t;
using json = nlohmann::json;

namespace conf
{

struct BinaryBlobConfig
{
    std::string blobBaseId;                     // Required
    std::string sysFilePath;                    // Required
    std::optional<uint32_t> offsetBytes;        // Optional
    std::optional<uint32_t> maxSizeBytes;       // Optional
    std::optional<std::string> aliasBlobBaseId; // Optional
    bool migrateToAlias = false;                // Optional
};

/**
 * @brief Parse parameters from a config json
 * @param j: input json object
 * @param config: output BinaryBlobConfig
 * @throws: exception if config doesn't have required fields
 */
static inline void parseFromConfigFile(const json& j, BinaryBlobConfig& config)
{
    j.at("blobBaseId").get_to(config.blobBaseId);
    j.at("sysFilePath").get_to(config.sysFilePath);
    if (j.contains("offsetBytes"))
    {
        j.at("offsetBytes").get_to(config.offsetBytes.emplace());
    }

    if (j.contains("maxSizeBytes"))
    {
        j.at("maxSizeBytes").get_to(config.maxSizeBytes.emplace());
    }

    if (j.contains("aliasBlobBaseId"))
    {
        j.at("aliasBlobBaseId").get_to(config.aliasBlobBaseId.emplace());
    }

    if (j.contains("migrateToAlias"))
    {
        config.migrateToAlias = j.at("migrateToAlias");
    }
}

} // namespace conf
