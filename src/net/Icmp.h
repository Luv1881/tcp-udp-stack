#pragma once
#include "common/Buffer.h"
#include "common/Checksum.h"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace net {

inline constexpr uint8_t kIcmpEchoRequest = 8;
inline constexpr uint8_t kIcmpEchoReply   = 0;

struct IcmpEcho {
    uint8_t  type;
    uint8_t  code = 0;
    uint16_t id;
    uint16_t seq;
    ByteView payload;

    static std::optional<IcmpEcho> parse(ByteView v) {
        if (v.size() < 8) return std::nullopt;
        IcmpEcho e;
        e.type    = v[0];
        e.code    = v[1];
        e.id      = (uint16_t(v[4]) << 8) | v[5];
        e.seq     = (uint16_t(v[6]) << 8) | v[7];
        e.payload = v.subspan(8);
        return e;
    }

    Buffer make_reply() const {
        Buffer buf(8 + payload.size());
        uint8_t* p = buf.data();
        p[0] = kIcmpEchoReply;
        p[1] = 0;
        p[2] = 0;            p[3] = 0;
        p[4] = id >> 8;      p[5] = id & 0xFF;
        p[6] = seq >> 8;     p[7] = seq & 0xFF;
        std::copy(payload.begin(), payload.end(), buf.begin() + 8);
        uint16_t ck = checksum(buf.view());
        p[2] = ck >> 8;      p[3] = ck & 0xFF;
        return buf;
    }
};

}
