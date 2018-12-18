#include "handler_unittest.hpp"

using ::testing::Return;
using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobChecksNameInvalid)
{
    auto bstore = std::make_unique<MockBinaryStore>();

    handler.addNewBinaryStore(std::move(bstore));

    // Verify canHandleBlob checks and returns false on an invalid name.
    EXPECT_FALSE(handler.canHandleBlob("asdf"s));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobCanOpenValidBlob)
{
    auto bstore = std::make_unique<MockBinaryStore>();

    EXPECT_CALL(*bstore, canHandleBlob("/test/path"s)).WillOnce(Return(true));
    handler.addNewBinaryStore(std::move(bstore));

    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_TRUE(handler.canHandleBlob("/test/path"s));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, GetBlobIdEqualsConcatenationsOfIds)
{
    std::string baseId0 = "/test/"s;
    std::string baseId1 = "/test1/"s;
    std::vector<std::string> idList0 = {"/test/"s, "/test/0"s};
    std::vector<std::string> idList1 = {"/test1/"s, "/test1/2"s};
    auto expectedIdList = idList0;
    expectedIdList.insert(expectedIdList.end(), idList1.begin(), idList1.end());

    auto bstore0 = std::make_unique<MockBinaryStore>();
    EXPECT_CALL(*bstore0, getBaseBlobId()).WillOnce(Return(baseId0));
    EXPECT_CALL(*bstore0, getBlobIds()).WillOnce(Return(idList0));
    handler.addNewBinaryStore(std::move(bstore0));

    auto bstore1 = std::make_unique<MockBinaryStore>();
    EXPECT_CALL(*bstore1, getBaseBlobId()).WillOnce(Return(baseId1));
    EXPECT_CALL(*bstore1, getBlobIds()).WillOnce(Return(idList1));
    handler.addNewBinaryStore(std::move(bstore1));

    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_EQ(expectedIdList, handler.getBlobIds());
}

} // namespace blobs
