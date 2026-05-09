#include <gtest/gtest.h>
#include "common/Buffer.h"

using namespace net;

TEST(BufferTest, DefaultEmpty) {
    Buffer b;
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.size(), 0u);
}

TEST(BufferTest, SizeConstruct) {
    Buffer b(8);
    EXPECT_EQ(b.size(), 8u);
    EXPECT_FALSE(b.empty());
    for (size_t i = 0; i < 8; ++i)
        EXPECT_EQ(b[i], 0);
}

TEST(BufferTest, PtrLenConstruct) {
    uint8_t raw[] = {1, 2, 3};
    Buffer b(raw, 3);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b[2], 3);
}

TEST(BufferTest, ByteViewConstruct) {
    uint8_t raw[] = {0xAB, 0xCD};
    ByteView v{raw, 2};
    Buffer b(v);
    EXPECT_EQ(b[0], 0xAB);
    EXPECT_EQ(b[1], 0xCD);
}

TEST(BufferTest, AppendByteView) {
    Buffer b(2);
    b[0] = 0x01; b[1] = 0x02;
    uint8_t extra[] = {0x03, 0x04};
    b.append(ByteView{extra, 2});
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b[2], 0x03);
    EXPECT_EQ(b[3], 0x04);
}

TEST(BufferTest, Resize) {
    Buffer b(2);
    b.resize(5);
    EXPECT_EQ(b.size(), 5u);
    EXPECT_EQ(b[4], 0);
}

TEST(BufferTest, ViewIsNonOwning) {
    Buffer b(3);
    b[0] = 7;
    ByteView v = b.view();
    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v.size(), 3u);
}

TEST(BufferTest, MutView) {
    Buffer b(2);
    MutByteView mv = b.mut_view();
    mv[0] = 0xFF;
    EXPECT_EQ(b[0], 0xFF);
}
