#pragma once
#include <bit>
#include <cstdint>

namespace net {

constexpr uint16_t hton16(uint16_t v) {
    if constexpr (std::endian::native == std::endian::little)
        return static_cast<uint16_t>((v >> 8) | (v << 8));
    return v;
}

constexpr uint32_t hton32(uint32_t v) {
    if constexpr (std::endian::native == std::endian::little)
        return ((v & 0xFF000000u) >> 24) | ((v & 0x00FF0000u) >> 8)
             | ((v & 0x0000FF00u) << 8)  | ((v & 0x000000FFu) << 24);
    return v;
}

constexpr uint16_t ntoh16(uint16_t v) { return hton16(v); }
constexpr uint32_t ntoh32(uint32_t v) { return hton32(v); }

}
