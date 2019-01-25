#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

class BinaryStoreBlobHandlerOpenTest : public BinaryStoreBlobHandlerTest
{
  protected:
    static inline std::string openTestBaseId = "/test/"s;
    static inline std::string openTestBlobId = "/test/blob0"s;
    static inline std::string openTestInvalidBlobId = "/invalid/blob0"s;
    static inline uint16_t openTestROFlags = OpenFlags::read;
    static inline uint16_t openTestRWFlags = OpenFlags::read | OpenFlags::write;
    static inline uint16_t openTestSessionId = 0;
};

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenCannotHandleId)
{
    EXPECT_FALSE(handler.open(openTestSessionId, openTestROFlags,
                              openTestInvalidBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, FailWhenStoreOpenReturnsFailure)
{
    auto store = defaultMockStore(openTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, openTestROFlags))
        .WillOnce(Return(false));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_FALSE(
        handler.open(openTestSessionId, openTestROFlags, openTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, SucceedWhenStoreOpenReturnsTrue)
{
    auto store = defaultMockStore(openTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, openTestROFlags))
        .WillOnce(Return(true));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_TRUE(
        handler.open(openTestSessionId, openTestROFlags, openTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, CloseFailForInvalidSession)
{
    uint16_t invalidSessionId = 1;
    EXPECT_FALSE(handler.close(invalidSessionId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, CloseFailWhenStoreCloseFails)
{
    auto store = defaultMockStore(openTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, openTestROFlags))
        .WillOnce(Return(true));
    EXPECT_CALL(*store, close()).WillOnce(Return(false));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_TRUE(
        handler.open(openTestSessionId, openTestROFlags, openTestBlobId));
    EXPECT_FALSE(handler.close(openTestSessionId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, CloseSucceedWhenStoreCloseSucceeds)
{
    auto store = defaultMockStore(openTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, openTestROFlags))
        .WillOnce(Return(true));
    EXPECT_CALL(*store, close()).WillOnce(Return(true));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_TRUE(
        handler.open(openTestSessionId, openTestROFlags, openTestBlobId));
    EXPECT_TRUE(handler.close(openTestSessionId));
}

TEST_F(BinaryStoreBlobHandlerOpenTest, ClosedSessionCannotBeReclosed)
{
    auto store = defaultMockStore(openTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, openTestROFlags))
        .WillOnce(Return(true));
    EXPECT_CALL(*store, close()).WillRepeatedly(Return(true));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_TRUE(
        handler.open(openTestSessionId, openTestROFlags, openTestBlobId));
    EXPECT_TRUE(handler.close(openTestSessionId));
    EXPECT_FALSE(handler.close(openTestSessionId));
    EXPECT_FALSE(handler.close(openTestSessionId));
}

} // namespace blobs
