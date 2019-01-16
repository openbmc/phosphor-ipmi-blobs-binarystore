#pragma once

#include "binarystore_mock.hpp"
#include "fake_sys_file.hpp"
#include "handler.hpp"

#include "binaryblob.pb.h"

#include <gtest/gtest.h>

using ::testing::Contains;

using namespace std::string_literals;

namespace blobs
{

class BinaryStoreBlobHandlerTest : public ::testing::Test
{
  protected:
    BinaryStoreBlobHandlerTest() = default;
    BinaryStoreBlobHandler handler;

    void AddDefaultBinaryStore(const std::string& baseId);
    binstore::FakeSysFile fakeFile;
};

void BinaryStoreBlobHandlerTest::AddDefaultBinaryStore(
    const std::string& baseId)
{
    handler.addNewBinaryStore(
        binstore::BinaryStore::createFromConfig(baseId, &fakeFile, 0));
    // Verify baseId shows in list of blob IDs
    EXPECT_THAT(handler.getBlobIds(), Contains(baseId));
}

class BinaryStoreBlobHandlerBasicTest : public BinaryStoreBlobHandlerTest
{
};

} // namespace blobs
