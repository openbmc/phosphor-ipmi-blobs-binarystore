#include "handler_unittest.hpp"

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::UnorderedElementsAreArray;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobChecksNameInvalidMock)
{
    auto bstore = std::make_unique<MockBinaryStore>();

    handler.addNewBinaryStore(std::move(bstore));

    // Verify canHandleBlob checks and returns false on an invalid name.
    EXPECT_FALSE(handler.canHandleBlob("asdf"s));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobCanOpenValidBlobMock)
{
    auto bstore = std::make_unique<MockBinaryStore>();

    EXPECT_CALL(*bstore, getBaseBlobId()).WillRepeatedly(Return("/test/"s));
    handler.addNewBinaryStore(std::move(bstore));

    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_TRUE(handler.canHandleBlob("/test/path"s));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, GetBlobIdEqualsConcatenationsOfIdsMock)
{
    std::string baseId0 = "/test/"s;
    std::string baseId1 = "/test1/"s;
    std::vector<std::string> idList0 = {"/test/"s, "/test/0"s};
    std::vector<std::string> idList1 = {"/test1/"s, "/test1/2"s};
    auto expectedIdList = idList0;
    expectedIdList.insert(expectedIdList.end(), idList1.begin(), idList1.end());

    auto bstore0 = std::make_unique<MockBinaryStore>();
    EXPECT_CALL(*bstore0, getBaseBlobId()).WillRepeatedly(Return(baseId0));
    EXPECT_CALL(*bstore0, getBlobIds()).WillOnce(Return(idList0));
    handler.addNewBinaryStore(std::move(bstore0));

    auto bstore1 = std::make_unique<MockBinaryStore>();
    EXPECT_CALL(*bstore1, getBaseBlobId()).WillRepeatedly(Return(baseId1));
    EXPECT_CALL(*bstore1, getBlobIds()).WillOnce(Return(idList1));
    handler.addNewBinaryStore(std::move(bstore1));

    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_EQ(expectedIdList, handler.getBlobIds());
}

TEST_F(BinaryStoreBlobHandlerBasicTest, GetBlobIdShowsBaseId)
{
    EXPECT_THAT(handler.getBlobIds(), IsEmpty());

    handler.addNewBinaryStore(
        BinaryStore::createFromConfig("/foo/"s, "/fake/path"s, 0, 0));

    // Verify getBlobId shows base id.
    EXPECT_THAT(handler.getBlobIds(), ElementsAreArray({"/foo/"s}));

    handler.addNewBinaryStore(
        BinaryStore::createFromConfig("/bar/"s, "/fake/path"s, 0, 0));

    EXPECT_THAT(handler.getBlobIds(),
                UnorderedElementsAreArray({"/foo/"s, "/bar/"s}));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobChecksNameInvalid)
{
    handler.addNewBinaryStore(
        BinaryStore::createFromConfig("/test/"s, "/fake/path"s, 0, 0));

    // Verify canHandleBlob checks and returns false on an invalid name.
    EXPECT_FALSE(handler.canHandleBlob("asdf"s));
    EXPECT_FALSE(handler.canHandleBlob("/"s));
    EXPECT_FALSE(handler.canHandleBlob("//"s));
    // Cannot handle the base id name
    EXPECT_FALSE(handler.canHandleBlob("/test/"s));
    EXPECT_FALSE(handler.canHandleBlob("/test"s));
    // Cannot handle nested name
    EXPECT_FALSE(handler.canHandleBlob("/test/this/blob"s));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobChecksNameValid)
{
    handler.addNewBinaryStore(
        BinaryStore::createFromConfig("/test/"s, "/fake/path"s, 0, 0));

    // Verify canHandleBlob can handle a valid blob under the path.
    EXPECT_TRUE(handler.canHandleBlob("/test/blob0"s));
    EXPECT_TRUE(handler.canHandleBlob("/test/test"s));
    EXPECT_TRUE(handler.canHandleBlob("/test/xyz.abc"s));
}

} // namespace blobs
