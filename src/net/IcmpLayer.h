#pragma once
#include "core/INetworkLayer.h"
#include "core/INetworkProtocol.h"
namespace net {
class IcmpLayer : public core::INetworkProtocol {
public:
    explicit IcmpLayer(core::INetworkLayer& net_layer);
    void handle(const Ipv4Header& hdr, net::ByteView payload) override;
private:
    core::INetworkLayer& net_;
};
}
