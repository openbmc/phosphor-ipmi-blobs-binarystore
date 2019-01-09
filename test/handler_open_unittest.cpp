#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StartsWith;
using ::testing::UnorderedElementsAreArray;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenCannotHandleId)
{
    uint16_t flags = OpenFlags::read, sessionId = 0;
    EXPECT_FALSE(handler.open(sessionId, flags, "/invalid/blob"s));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenStoreOpenReturnsFailureMock)
{
    auto testBaseId = "/test/"s;
    auto testBlobId = "/test/blob0"s;
    uint16_t flags = OpenFlags::read, sessionId = 0;
    auto bstore = std::make_unique<MockBinaryStore>();

    EXPECT_CALL(*bstore, getBaseBlobId()).WillRepeatedly(Return(testBaseId));
    EXPECT_CALL(*bstore, openOrCreateBlob(_, flags)).WillOnce(Return(false));

    handler.addNewBinaryStore(std::move(bstore));

    EXPECT_FALSE(handler.open(sessionId, flags, testBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, SucceedWhenStoreOpenReturnsTrueMock)
{
    auto testBaseId = "/test/"s;
    auto testBlobId = "/test/blob0"s;
    uint16_t flags = OpenFlags::read, sessionId = 0;
    auto bstore = std::make_unique<MockBinaryStore>();

    EXPECT_CALL(*bstore, getBaseBlobId()).WillRepeatedly(Return(testBaseId));
    EXPECT_CALL(*bstore, openOrCreateBlob(_, flags)).WillOnce(Return(true));

    handler.addNewBinaryStore(std::move(bstore));

    EXPECT_TRUE(handler.open(sessionId, flags, testBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, FailForNonMatchingBasePath)
{
    auto testBaseId = "/test/"s;
    auto testBlobId = "/invalid/blob"s;
    uint16_t flags = OpenFlags::read, sessionId = 0;
    auto bstore =
        BinaryStore::createFromConfig(testBaseId, "/fake/systempath"s, 0, 0);
    handler.addNewBinaryStore(std::move(bstore));

    EXPECT_FALSE(handler.open(sessionId, flags, testBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, OpenCloseSucceedForValidBlobId)
{
    auto testBaseId = "/test/"s;
    auto testBlobId = "/test/blob0"s;
    uint16_t flags = OpenFlags::read, sessionId = 0;
    auto bstore =
        BinaryStore::createFromConfig(testBaseId, "/fake/systempath"s, 0, 0);
    handler.addNewBinaryStore(std::move(bstore));

    EXPECT_FALSE(handler.close(sessionId)); // Haven't open
    EXPECT_TRUE(handler.open(sessionId, flags, testBlobId));
    EXPECT_TRUE(handler.close(sessionId));
    EXPECT_FALSE(handler.close(sessionId)); // Already closed
}

TEST_F(BinaryStoreBlobHandlerOpenTest, OpenSuccessShowsBlobId)
{
    auto testBaseId = "/test/"s;
    auto testBlobId = "/test/blob0"s;
    uint16_t flags = OpenFlags::read, sessionId = 0;
    auto bstore =
        BinaryStore::createFromConfig(testBaseId, "/fake/systempath"s, 0, 0);
    handler.addNewBinaryStore(std::move(bstore));

    EXPECT_TRUE(handler.open(sessionId, flags, testBlobId));
    EXPECT_THAT(handler.getBlobIds(),
                UnorderedElementsAreArray({testBaseId, testBlobId}));
}

} // namespace blobs
