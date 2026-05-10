#include "core/Stack.h"
namespace core {
Stack::Stack(const std::string& tun_name, uint32_t local_ip)
    : tun_(tun_name), demuxer_(), ipv4_(tun_, local_ip, demuxer_), icmp_(ipv4_), loop_() {
    demuxer_.add(1u, &icmp_);
}
void Stack::run() {
    loop_.run(tun_.fd(), [this](net::ByteView pkt) { ipv4_.on_packet(pkt); });
}
void Stack::stop() { loop_.stop(); }
}
