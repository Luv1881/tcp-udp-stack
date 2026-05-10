#pragma once
#include "common/Buffer.h"
#include <cstdint>
namespace core {
class INetworkLayer {
public:
    virtual void send(uint8_t proto, uint32_t dst_ip, net::ByteView payload) = 0;
    virtual ~INetworkLayer() = default;
};
}
