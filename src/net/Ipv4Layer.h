#pragma once
#include "core/Demuxer.h"
#include "core/INetworkLayer.h"
#include "link/ILinkLayer.h"
#include <cstdint>
namespace net {
class Ipv4Layer : public core::INetworkLayer {
public:
    Ipv4Layer(ILinkLayer& link, uint32_t local_ip, core::Demuxer& demuxer);
    void send(uint8_t proto, uint32_t dst_ip, net::ByteView payload) override;
    void on_packet(net::ByteView raw);
private:
    ILinkLayer& link_;
    uint32_t local_ip_;
    core::Demuxer& demuxer_;
};
}
