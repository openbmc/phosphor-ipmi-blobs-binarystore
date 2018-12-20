#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;

namespace blobs
{

class BinaryStoreBlobHandlerReadWriteTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerReadWriteTest()
    {
        AddDefaultBinaryStore(rwTestBaseId);
    }

    static inline std::string rwTestBaseId = "/test/"s;
    static inline std::string rwTestBlobId = "/test/blob0"s;
    static inline std::vector<uint8_t> rwTestData = {0, 1, 2, 3, 4,
                                                     5, 6, 7, 8, 9};
    static inline uint16_t rwTestSessionId = 0;
    static inline uint32_t rwTestOffset = 0;

    void openAndWriteTestData()
    {
        uint16_t flags = OpenFlags::read | OpenFlags::write;
        EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
        EXPECT_TRUE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
    }
};

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteFailForWrongSession)
{
    uint16_t flags = OpenFlags::read | OpenFlags::write, wrongSessionId = 1;
    EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
    EXPECT_FALSE(handler.write(wrongSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteFailForWrongFlag)
{
    uint16_t flags = OpenFlags::read;
    EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
    EXPECT_FALSE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteValidDataSucceeds)
{
    uint16_t flags = OpenFlags::read | OpenFlags::write;
    EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
    EXPECT_TRUE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadReturnEmptyForWrongSession)
{
    uint16_t flags = OpenFlags::read, wrongSessionId = 1;
    EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
    EXPECT_THAT(handler.read(wrongSessionId, rwTestOffset, rwTestData.size()),
                IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, AbleToReadDataWritten)
{
    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadBeyondEndReturnsEmpty)
{
    openAndWriteTestData();
    const size_t offsetBeyondEnd = rwTestData.size();
    EXPECT_THAT(
        handler.read(rwTestSessionId, offsetBeyondEnd, rwTestData.size()),
        IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadAtOffset)
{
    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));

    // Now read over the entire offset range for 1 byte
    for (size_t i = 0; i < rwTestData.size(); ++i)
    {
        EXPECT_THAT(handler.read(rwTestSessionId, i, 1),
                    ElementsAre(rwTestData[i]));
    }
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, PartialOverwriteDataWorks)
{
    // Construct a partially overwritten vector
    const size_t overwritePos = 8;
    std::vector<uint8_t> overwriteData = {8, 9, 10, 11, 12};
    std::vector<uint8_t> expectedOverWrittenData(
        rwTestData.begin(), rwTestData.begin() + overwritePos);
    expectedOverWrittenData.insert(expectedOverWrittenData.end(),
                                   overwriteData.begin(), overwriteData.end());

    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));
    EXPECT_TRUE(handler.write(rwTestSessionId, overwritePos, overwriteData));

    for (size_t i = 0; i < expectedOverWrittenData.size(); ++i)
    {
        EXPECT_THAT(handler.read(rwTestSessionId, i, 1),
                    ElementsAre(expectedOverWrittenData[i]));
    }
    EXPECT_THAT(
        handler.read(rwTestSessionId, expectedOverWrittenData.size(), 1),
        IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteWithGapShouldFail)
{
    const auto gapOffset = 10;
    uint16_t flags = OpenFlags::read | OpenFlags::write;
    EXPECT_TRUE(handler.open(rwTestSessionId, flags, rwTestBlobId));
    EXPECT_FALSE(handler.write(rwTestSessionId, gapOffset, rwTestData));
}

} // namespace blobs
