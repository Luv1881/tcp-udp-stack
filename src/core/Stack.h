#pragma once
#include "core/Demuxer.h"
#include "core/EventLoop.h"
#include "core/INetworkLayer.h"
#include "link/TunDevice.h"
#include "net/IcmpLayer.h"
#include "net/Ipv4Layer.h"
#include <cstdint>
#include <string>
namespace core {
class Stack {
public:
    Stack(const std::string& tun_name, uint32_t local_ip);
    void run();
    void stop();
    INetworkLayer& net() { return ipv4_; }
private:
    net::TunDevice tun_;
    Demuxer demuxer_;
    net::Ipv4Layer ipv4_;
    net::IcmpLayer icmp_;
    EventLoop loop_;
};
}
