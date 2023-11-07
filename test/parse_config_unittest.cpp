#include "parse_config.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using json = nlohmann::json;
using namespace conf;

TEST(ParseConfigTest, ExceptionWhenMissingRequiredFields)
{
    auto j = R"(
    {
      "blobBaseId": "/test/"
    }
  )"_json;

    BinaryBlobConfig config;

    EXPECT_THROW(parseFromConfigFile(j, config), std::exception);
}

TEST(ParseConfigTest, TestSimpleConfig)
{
    auto j = R"(
    {
      "blobBaseId": "/test/",
      "sysFilePath": "/sys/fake/path",
      "offsetBytes": 32,
      "maxSizeBytes": 2,
      "aliasBlobBaseId": "/test2/",
      "migrateToAlias": true
    }
  )"_json;

    BinaryBlobConfig config;

    EXPECT_NO_THROW(parseFromConfigFile(j, config));
    EXPECT_EQ(config.blobBaseId, "/test/");
    EXPECT_EQ(config.sysFilePath, "/sys/fake/path");
    EXPECT_EQ(config.offsetBytes, 32);
    EXPECT_EQ(config.maxSizeBytes, 2);
    EXPECT_EQ(config.aliasBlobBaseId, "/test2/");
    EXPECT_TRUE(config.migrateToAlias);
}

TEST(ParseConfigTest, TestConfigArray)
{
    auto j = R"(
    [{
      "blobBaseId": "/test/",
      "sysFilePath": "/sys/fake/path",
      "offsetBytes": 32,
      "maxSizeBytes": 32
     },
     {
       "blobBaseId": "/test/",
       "sysFilePath": "/another/path"
    },
     {
       "blobBaseId": "/test/",
       "sysFilePath": "/another/path",
       "offsetBytes": 32
    },
     {
       "blobBaseId": "/test/",
       "sysFilePath": "/another/path",
       "maxSizeBytes": 32
    }]
  )"_json;

    for (auto& element : j)
    {
        BinaryBlobConfig config;

        EXPECT_NO_THROW(parseFromConfigFile(element, config));
        EXPECT_EQ(config.blobBaseId, "/test/");
        EXPECT_TRUE(config.offsetBytes == std::nullopt ||
                    *config.offsetBytes == 32);
        EXPECT_TRUE(config.maxSizeBytes == std::nullopt ||
                    *config.maxSizeBytes == 32);
    }
}
