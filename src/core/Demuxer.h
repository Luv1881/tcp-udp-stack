#pragma once
#include "core/INetworkProtocol.h"
#include <cstdint>
#include <unordered_map>
namespace core {
class Demuxer {
public:
    void add(uint8_t proto, INetworkProtocol* h) { handlers_[proto] = h; }
    void dispatch(const net::Ipv4Header& hdr, net::ByteView payload) const {
        auto it = handlers_.find(hdr.protocol);
        if (it != handlers_.end()) it->second->handle(hdr, payload);
    }
private:
    std::unordered_map<uint8_t, INetworkProtocol*> handlers_;
};
}
