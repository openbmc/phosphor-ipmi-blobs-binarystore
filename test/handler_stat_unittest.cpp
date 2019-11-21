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

TEST_F(BinaryStoreBlobHandlerStatTest, InitialStatIsValidQueriedWithBlobId)
{
    BlobMeta meta;

    /* Querying stat fails if there is no open session */
    EXPECT_FALSE(handler.stat(statTestSessionId, &meta));
    /* However stat completes if queried using blobId. Returning default. */
    EXPECT_TRUE(handler.stat(statTestBlobId, &meta));
    EXPECT_EQ(meta.size, 0);
};

TEST_F(BinaryStoreBlobHandlerStatTest, StatAfterOpenGetsCorrectSize)
{
    BlobMeta meta;

    openAndWriteTestData();
    EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
    EXPECT_EQ(meta.size, statTestData.size());
    EXPECT_TRUE((meta.blobState & OpenFlags::read) != 0);
    EXPECT_TRUE((meta.blobState & OpenFlags::write) != 0);

    EXPECT_TRUE(handler.stat(statTestBlobId, &meta));
    EXPECT_EQ(meta.size, statTestData.size());
    EXPECT_TRUE((meta.blobState & OpenFlags::read) != 0);
    EXPECT_TRUE((meta.blobState & OpenFlags::write) != 0);
}

TEST_F(BinaryStoreBlobHandlerStatTest, StatShowsCommittedState)
{
    BlobMeta meta;

    openWriteThenCommitData();
    EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
    EXPECT_EQ(meta.size, statTestData.size());
    EXPECT_TRUE((meta.blobState & OpenFlags::read) != 0);
    EXPECT_TRUE((meta.blobState & OpenFlags::write) != 0);
    EXPECT_TRUE((meta.blobState & StateFlags::committed) != 0);

    EXPECT_TRUE(handler.stat(statTestBlobId, &meta));
    EXPECT_EQ(meta.size, statTestData.size());
    EXPECT_TRUE((meta.blobState & OpenFlags::read) != 0);
    EXPECT_TRUE((meta.blobState & OpenFlags::write) != 0);
    EXPECT_TRUE((meta.blobState & StateFlags::committed) != 0);
}

} // namespace blobs
