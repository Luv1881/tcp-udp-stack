#pragma once
#include "common/Buffer.h"
#include "net/Ipv4Header.h"
namespace core {
class INetworkProtocol {
public:
    virtual void handle(const net::Ipv4Header& hdr, net::ByteView payload) = 0;
    virtual ~INetworkProtocol() = default;
};
}
