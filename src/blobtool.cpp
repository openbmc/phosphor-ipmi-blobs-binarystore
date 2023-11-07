#include "binarystore.hpp"
#include "parse_config.hpp"
#include "sys_file_impl.hpp"

#include <getopt.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdplus/print.hpp>

constexpr auto defaultBlobConfigPath = "/usr/share/binaryblob/config.json";

struct BlobToolConfig
{
    std::string configPath = defaultBlobConfigPath;
    std::string programName;
    std::string binStore;
    std::string blobName;
    size_t offsetBytes = 0;
    enum class Action
    {
        HELP,
        LIST,
        READ,
        MIGRATE,
    } action = Action::LIST;
} toolConfig;

void printUsage(const BlobToolConfig& cfg)
{
    stdplus::print(stderr,
                   "Usage: \n"
                   "{} [OPTIONS]\n"
                   "\t--list\t\tList all supported blobs. This is a default.\n"
                   "\t--read\t\tRead blob specified in --blob argument (which "
                   "becomes mandatory).\n"
                   "\t--config\tFILENAME\tPath to the configuration file. The "
                   "default is /usr/share/binaryblob/config.json.\n"
                   "\t--binary-store\tFILENAME\tPath to the binary storage. If "
                   "specified, configuration file is not used.\n"
                   "\t--blob\tSTRING\tThe name of the blob to read.\n"
                   "\t--offset\tNUMBER\tThe offset in the binary store file, "
                   "where the binary store actually starts.\n"
                   "\t--help\t\tPrint this help and exit\n",
                   cfg.programName);
}

bool parseOptions(int argc, char* argv[], BlobToolConfig& cfg)
{
    cfg.programName = argv[0];

    struct option longOptions[] = {
        {"help", no_argument, 0, 'h'},
        {"list", no_argument, 0, 'l'},
        {"read", no_argument, 0, 'r'},
        {"migrate", no_argument, 0, 'm'},
        {"config", required_argument, 0, 'c'},
        {"binary-store", required_argument, 0, 's'},
        {"blob", required_argument, 0, 'b'},
        {"offset", required_argument, 0, 'g'},
        {0, 0, 0, 0},
    };

    int optionIndex = 0;
    bool res = true;
    while (1)
    {
        int ret = getopt_long_only(argc, argv, "", longOptions, &optionIndex);

        if (ret == -1)
            break;

        switch (ret)
        {
            case 'h':
                cfg.action = BlobToolConfig::Action::HELP;
                break;
            case 'l':
                cfg.action = BlobToolConfig::Action::LIST;
                break;
            case 'r':
                cfg.action = BlobToolConfig::Action::READ;
                break;
            case 'c':
                cfg.configPath = optarg;
                break;
            case 's':
                cfg.binStore = optarg;
                break;
            case 'b':
                cfg.blobName = optarg;
                break;
            case 'g':
                cfg.offsetBytes = std::stoi(optarg);
                break;
            default:
                res = false;
                break;
        }
    }

    return res;
}

int main(int argc, char* argv[])
{
    parseOptions(argc, argv, toolConfig);
    if (toolConfig.action == BlobToolConfig::Action::HELP)
    {
        printUsage(toolConfig);
        return 0;
    }

    std::vector<std::unique_ptr<binstore::BinaryStoreInterface>> stores;
    if (!toolConfig.binStore.empty())
    {
        auto file = std::make_unique<binstore::SysFileImpl>(
            toolConfig.binStore, toolConfig.offsetBytes);
        if (!file)
        {
            stdplus::print(stderr, "Can't open binary store {}\n",
                           toolConfig.binStore);
            printUsage(toolConfig);
            return 1;
        }

        auto store =
            binstore::BinaryStore::createFromFile(std::move(file), true);
        stores.push_back(std::move(store));
    }
    else
    {
        std::ifstream input(toolConfig.configPath);
        json j;

        if (!input.good())
        {
            stdplus::print(stderr, "Config file not found:{}\n",
                           toolConfig.configPath);
            return 1;
        }

        try
        {
            input >> j;
        }
        catch (const std::exception& e)
        {
            stdplus::print(stderr, "Failed to parse config into json:\n{}\n",
                           e.what());
            return 1;
        }

        for (const auto& element : j)
        {
            conf::BinaryBlobConfig config;
            try
            {
                conf::parseFromConfigFile(element, config);
            }
            catch (const std::exception& e)
            {
                stdplus::print(
                    stderr, "Encountered error when parsing config file:\n{}\n",
                    e.what());
                return 1;
            }

            auto file = std::make_unique<binstore::SysFileImpl>(
                config.sysFilePath, config.offsetBytes);

            auto store = binstore::BinaryStore::createFromConfig(
                config.blobBaseId, std::move(file), config.maxSizeBytes,
                config.aliasBlobBaseId);

            if (toolConfig.action == BlobToolConfig::Action::MIGRATE)
            {
                if (config.migrateToAlias && config.aliasBlobBaseId.has_value())
                {
                    store->setBaseBlobId(config.aliasBlobBaseId.value());
                }
            }
            else
            {
                stores.push_back(std::move(store));
            }
        }
    }

    if (toolConfig.action == BlobToolConfig::Action::MIGRATE)
    {
        stdplus::print(stderr,
                       "Migrated all BinaryStore back to configured Alias");
        return 0;
    }

    if (toolConfig.action == BlobToolConfig::Action::LIST)
    {
        stdplus::print(stderr, "Supported Blobs: \n");
        for (const auto& store : stores)
        {
            const auto blobIds = store->getBlobIds();
            std::copy(
                blobIds.begin(), blobIds.end(),
                std::ostream_iterator<decltype(blobIds[0])>(std::cout, "\n"));
        }
        return 0;
    }
    if (toolConfig.action == BlobToolConfig::Action::READ)
    {
        if (toolConfig.blobName.empty())
        {
            stdplus::print(stderr,
                           "Must specify the name of the blob to read.\n");
            printUsage(toolConfig);
            return 1;
        }

        bool blobFound = false;

        for (const auto& store : stores)
        {
            const auto blobIds = store->getBlobIds();
            if (std::any_of(blobIds.begin(), blobIds.end(),
                            [](const std::string& bn) {
                                return bn == toolConfig.blobName;
                            }))
            {
                const auto blobData = store->readBlob(toolConfig.blobName);
                if (blobData.empty())
                {
                    stdplus::print(stderr, "No data read from {}\n",
                                   store->getBaseBlobId());
                    continue;
                }

                blobFound = true;

                std::copy(
                    blobData.begin(), blobData.end(),
                    std::ostream_iterator<decltype(blobData[0])>(std::cout));

                // It's assumed that the names of the blobs are unique within
                // the system.
                break;
            }
        }

        if (!blobFound)
        {
            stdplus::print(stderr, "Blob {} not found.\n", toolConfig.blobName);
            return 1;
        }
        return 0;
    }

    return 0;
}
