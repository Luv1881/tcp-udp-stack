#pragma once
#include <cstdint>
#include "Buffer.h"

namespace net {

inline uint16_t checksum(ByteView data) {
    uint32_t sum = 0;
    const uint8_t* ptr = data.data();
    size_t len = data.size();

    while (len > 1) {
        sum += (static_cast<uint32_t>(ptr[0]) << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }
    if (len == 1)
        sum += static_cast<uint32_t>(*ptr) << 8;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return static_cast<uint16_t>(~sum);
}

inline uint16_t checksum(const Buffer& buf) {
    return checksum(buf.view());
}

}
