#include "net/IcmpLayer.h"
#include "net/Icmp.h"
namespace net {
IcmpLayer::IcmpLayer(core::INetworkLayer& net_layer) : net_(net_layer) {}
void IcmpLayer::handle(const Ipv4Header& hdr, net::ByteView payload) {
    auto echo = IcmpEcho::parse(payload);
    if (!echo || echo->type != kIcmpEchoRequest) return;
    auto reply = echo->make_reply();
    net_.send(1u, hdr.src, reply.view());
}
}
