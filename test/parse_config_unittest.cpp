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
      "maxSizeBytes": 2
    }
  )"_json;

    BinaryBlobConfig config;

    EXPECT_NO_THROW(parseFromConfigFile(j, config));
    EXPECT_EQ(config.blobBaseId, "/test/");
    EXPECT_EQ(config.sysFilePath, "/sys/fake/path");
    EXPECT_EQ(config.offsetBytes, 32);
    EXPECT_EQ(config.maxSizeBytes, 2);
}

TEST(ParseConfigTest, TestConfigArray)
{
    auto j = R"(
    [{
      "blobBaseId": "/test/",
      "sysFilePath": "/sys/fake/path",
      "offsetBytes": 32,
      "maxSizeBytes": 2
     },
     {
       "blobBaseId": "/test/",
       "sysFilePath": "/another/path"
    }]
  )"_json;

    for (auto& element : j)
    {
        BinaryBlobConfig config;

        EXPECT_NO_THROW(parseFromConfigFile(element, config));
        EXPECT_EQ(config.blobBaseId, "/test/");
    }
}
