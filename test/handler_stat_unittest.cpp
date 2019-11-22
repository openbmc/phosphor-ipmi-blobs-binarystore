#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;

namespace blobs
{

class BinaryStoreBlobHandlerStatTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerStatTest()
    {
        addDefaultStore(statTestBaseId);
    }

    static inline std::string statTestBaseId = "/test/"s;
    static inline std::string statTestBlobId = "/test/blob0"s;
    static inline std::vector<uint8_t> statTestData = {0, 1, 2, 3, 4,
                                                       5, 6, 7, 8, 9};
    static inline std::vector<uint8_t> statTestDataToOverwrite = {10, 11, 12,
                                                                  13, 14};
    static inline std::vector<uint8_t> commitMetaUnused;

    static inline uint16_t statTestSessionId = 0;
    static inline uint16_t statTestNewSessionId = 1;
    static inline uint32_t statTestOffset = 0;

    void openAndWriteTestData()
    {
        uint16_t flags = OpenFlags::read | OpenFlags::write;
        EXPECT_TRUE(handler.open(statTestSessionId, flags, statTestBlobId));
        EXPECT_TRUE(
            handler.write(statTestSessionId, statTestOffset, statTestData));
    }

    void openWriteThenCommitData()
    {
        openAndWriteTestData();
        EXPECT_TRUE(handler.commit(statTestSessionId, commitMetaUnused));
    }
};

TEST_F(BinaryStoreBlobHandlerStatTest, InitialStatIsValid)
{
    BlobMeta meta;
    EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
    EXPECT_TRUE(meta.size == 0);

    /* Querying by blobId returns the same info */
    EXPECT_TRUE(handler.stat(statTestBlobId, &meta));
};

#if 0
TEST_F(BinaryStoreBlobHandlerStatTest, StatShowsCommittedState)
{
    openWriteThenCommitData();

    EXPECT_EQ(statTestData, handler.read(statTestSessionId, statTestOffset,
                                         statTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerStatTest, CommittedDataCanBeReopened)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.close(statTestSessionId));
    EXPECT_TRUE(
        handler.open(statTestNewSessionId, OpenFlags::read, statTestBlobId));
    EXPECT_EQ(statTestData, handler.read(statTestNewSessionId, statTestOffset,
                                         statTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerStatTest, OverwrittenDataCanBeCommitted)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.write(statTestSessionId, statTestOffset,
                              statTestDataToOverwrite));
    EXPECT_TRUE(handler.commit(statTestSessionId, commitMetaUnused));
    EXPECT_TRUE(handler.close(statTestSessionId));

    EXPECT_TRUE(
        handler.open(statTestNewSessionId, OpenFlags::read, statTestBlobId));
    EXPECT_EQ(statTestDataToOverwrite,
              handler.read(statTestNewSessionId, statTestOffset,
                           statTestDataToOverwrite.size()));
}

TEST_F(BinaryStoreBlobHandlerStatTest, UncommittedDataIsLostUponClose)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.write(statTestSessionId, statTestOffset,
                              statTestDataToOverwrite));
    EXPECT_TRUE(handler.close(statTestSessionId));

    EXPECT_TRUE(
        handler.open(statTestNewSessionId, OpenFlags::read, statTestBlobId));
    EXPECT_EQ(statTestData, handler.read(statTestNewSessionId, statTestOffset,
                                         statTestData.size()));
}
#endif

} // namespace blobs
