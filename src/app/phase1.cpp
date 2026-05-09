#include "link/TunDevice.h"
#include "net/Ipv4Header.h"
#include "net/Icmp.h"
#include <cstdio>

static constexpr uint32_t kMyIp     = (10u << 24) | (0u << 16) | (0u << 8) | 2u;
static constexpr uint8_t  kProtoIcmp = 1;

static void print_ip(uint32_t ip, char* out) {
    std::sprintf(out, "%u.%u.%u.%u", ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

int main() {
    net::TunDevice tun("tun0");
    std::puts("[phase1] stack up — 10.0.0.2 via tun0");
    std::puts("[phase1] waiting for packets...\n");

    for (;;) {
        auto pkt = tun.read();
        auto ip  = net::Ipv4Header::parse(pkt.view());

        if (!ip) {
            std::printf("[RX]  dropped — bad IPv4 header (%zu bytes)\n", pkt.size());
            continue;
        }

        char src[16], dst[16];
        print_ip(ip->src, src);
        print_ip(ip->dst, dst);
        std::printf("[RX]  %s → %s  proto=%u  len=%u  ttl=%u\n",
                    src, dst, ip->protocol, ip->total_length, ip->ttl);

        if (ip->dst != kMyIp || ip->protocol != kProtoIcmp) {
            std::puts("[RX]  skipped — not for us or not ICMP");
            continue;
        }

        auto echo = net::IcmpEcho::parse(pkt.view().subspan(ip->ihl_bytes()));
        if (!echo || echo->type != net::kIcmpEchoRequest) {
            std::puts("[ICMP] dropped — not an echo request");
            continue;
        }

        std::printf("[ICMP] echo request  id=0x%04X  seq=%u  payload=%zu bytes\n",
                    echo->id, echo->seq, echo->payload.size());

        auto icmp_reply = echo->make_reply();

        net::Ipv4Header rip;
        rip.total_length = static_cast<uint16_t>(net::Ipv4Header::kSize + icmp_reply.size());
        rip.id           = ip->id;
        rip.protocol     = kProtoIcmp;
        rip.src          = kMyIp;
        rip.dst          = ip->src;

        auto out = rip.serialize();
        out.append(icmp_reply.view());
        tun.write(out.view());

        std::printf("[TX]   echo reply     id=0x%04X  seq=%u  → %s\n\n",
                    echo->id, echo->seq, src);
    }
}
