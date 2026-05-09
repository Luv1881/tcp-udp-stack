#pragma once
#include "common/Buffer.h"
#include "common/Checksum.h"
#include <cstdint>
#include <optional>

namespace net {

struct Ipv4Header {
    uint8_t  version_ihl  = 0x45;
    uint8_t  dscp_ecn     = 0;
    uint16_t total_length = 0;
    uint16_t id           = 0;
    uint16_t flags_frag   = 0;
    uint8_t  ttl          = 64;
    uint8_t  protocol     = 0;
    uint32_t src          = 0;
    uint32_t dst          = 0;

    static constexpr size_t kSize = 20;

    uint8_t ihl_bytes() const noexcept { return (version_ihl & 0x0F) << 2; }

    static std::optional<Ipv4Header> parse(ByteView v) {
        if (v.size() < kSize) return std::nullopt;
        Ipv4Header h;
        h.version_ihl  = v[0];
        h.dscp_ecn     = v[1];
        h.total_length = (uint16_t(v[2]) << 8) | v[3];
        h.id           = (uint16_t(v[4]) << 8) | v[5];
        h.flags_frag   = (uint16_t(v[6]) << 8) | v[7];
        h.ttl          = v[8];
        h.protocol     = v[9];
        h.src = (uint32_t(v[12]) << 24) | (uint32_t(v[13]) << 16) | (uint32_t(v[14]) << 8) | v[15];
        h.dst = (uint32_t(v[16]) << 24) | (uint32_t(v[17]) << 16) | (uint32_t(v[18]) << 8) | v[19];
        if ((h.version_ihl >> 4) != 4)               return std::nullopt;
        if (h.ihl_bytes() < kSize)                   return std::nullopt;
        if (h.total_length < kSize)                  return std::nullopt;
        if (h.total_length > v.size())               return std::nullopt;
        if (checksum(v.subspan(0, kSize)) != 0x0000) return std::nullopt;
        return h;
    }

    Buffer serialize() const {
        Buffer buf(kSize);
        uint8_t* p = buf.data();
        p[0]  = version_ihl;
        p[1]  = dscp_ecn;
        p[2]  = total_length >> 8;    p[3]  = total_length & 0xFF;
        p[4]  = id >> 8;              p[5]  = id & 0xFF;
        p[6]  = flags_frag >> 8;      p[7]  = flags_frag & 0xFF;
        p[8]  = ttl;                  p[9]  = protocol;
        p[10] = 0;                    p[11] = 0;
        p[12] = src >> 24;            p[13] = (src >> 16) & 0xFF;
        p[14] = (src >> 8) & 0xFF;   p[15] = src & 0xFF;
        p[16] = dst >> 24;            p[17] = (dst >> 16) & 0xFF;
        p[18] = (dst >> 8) & 0xFF;   p[19] = dst & 0xFF;
        uint16_t ck = checksum(buf.view());
        p[10] = ck >> 8;              p[11] = ck & 0xFF;
        return buf;
    }
};

}
