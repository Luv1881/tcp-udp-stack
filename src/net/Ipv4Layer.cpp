#include "net/Ipv4Layer.h"
#include "net/Ipv4Header.h"
namespace net {
Ipv4Layer::Ipv4Layer(ILinkLayer& link, uint32_t local_ip, core::Demuxer& demuxer)
    : link_(link), local_ip_(local_ip), demuxer_(demuxer) {}
void Ipv4Layer::on_packet(net::ByteView raw) {
    auto hdr = Ipv4Header::parse(raw);
    if (!hdr || hdr->dst != local_ip_) return;
    demuxer_.dispatch(*hdr, raw.subspan(hdr->ihl_bytes()));
}
void Ipv4Layer::send(uint8_t proto, uint32_t dst_ip, net::ByteView payload) {
    Ipv4Header hdr;
    hdr.total_length = static_cast<uint16_t>(Ipv4Header::kSize + payload.size());
    hdr.protocol = proto;
    hdr.src = local_ip_;
    hdr.dst = dst_ip;
    auto out = hdr.serialize();
    out.append(payload);
    link_.write(out.view());
}
}
