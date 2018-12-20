#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StartsWith;
using ::testing::UnorderedElementsAreArray;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

class BinaryStoreBlobHandlerOpenTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerOpenTest()
    {
        AddDefaultBinaryStore(openTestBaseId);
    }
    static inline std::string openTestBaseId = "/test/"s;
    static inline std::string openTestBlobId = "/test/blob0"s;
    static inline std::string openTestInvalidBlobId = "/invalid/blob0"s;
};

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenCannotHandleId)
{
    uint16_t flags = OpenFlags::read, sessionId = 0;
    EXPECT_FALSE(handler.open(sessionId, flags, openTestInvalidBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenNoReadFlag)
{
    uint16_t flags = OpenFlags::write, sessionId = 0;
    EXPECT_FALSE(handler.open(sessionId, flags, openTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, SucceedForValidBlobId)
{
    uint16_t flags = OpenFlags::read, sessionId = 0;
    EXPECT_TRUE(handler.open(sessionId, flags, openTestBlobId));
    EXPECT_TRUE(handler.close(sessionId));
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
