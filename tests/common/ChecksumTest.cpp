#include <gtest/gtest.h>
#include "common/Checksum.h"
#include "common/Buffer.h"

using namespace net;

TEST(ChecksumTest, AllZerosIsMaxOnesComplement) {
    uint8_t data[] = {0, 0, 0, 0};
    EXPECT_EQ(checksum(ByteView{data, sizeof(data)}), 0xFFFF);
}

TEST(ChecksumTest, Rfc1071Example) {
    uint8_t data[] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
    EXPECT_EQ(checksum(ByteView{data, sizeof(data)}), 0x220D);
}

TEST(ChecksumTest, OddLength) {
    uint8_t data[] = {0x01};
    EXPECT_EQ(checksum(ByteView{data, sizeof(data)}), 0xFEFF);
}

TEST(ChecksumTest, VerificationYieldsZero) {
    uint8_t data[] = {0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00,
                      0x40, 0x06, 0x00, 0x00, 0xac, 0x10, 0x0a, 0x63,
                      0xac, 0x10, 0x0a, 0x0c};
    uint16_t csum = checksum(ByteView{data, sizeof(data)});
    data[10] = static_cast<uint8_t>(csum >> 8);
    data[11] = static_cast<uint8_t>(csum & 0xFF);
    EXPECT_EQ(checksum(ByteView{data, sizeof(data)}), 0x0000);
}

TEST(ChecksumTest, BufferOverload) {
    Buffer buf(4);
    EXPECT_EQ(checksum(buf), 0xFFFF);
}

TEST(ChecksumTest, KnownIpHeaderVerifies) {
    uint8_t hdr[] = {0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00,
                     0x40, 0x06, 0xb1, 0xe6, 0xac, 0x10, 0x0a, 0x63,
                     0xac, 0x10, 0x0a, 0x0c};
    EXPECT_EQ(checksum(ByteView{hdr, sizeof(hdr)}), 0x0000);
}
