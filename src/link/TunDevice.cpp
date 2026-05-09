#include "link/TunDevice.h"
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

namespace net {

TunDevice::TunDevice(const std::string& name) {
    fd_ = ::open("/dev/net/tun", O_RDWR);
    if (fd_ < 0) throw std::runtime_error("open /dev/net/tun");

    ifreq ifr{};
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

    if (::ioctl(fd_, TUNSETIFF, &ifr) < 0) {
        ::close(fd_);
        throw std::runtime_error("TUNSETIFF");
    }
}

TunDevice::~TunDevice() {
    if (fd_ >= 0) ::close(fd_);
}

Buffer TunDevice::read() {
    Buffer buf(1500);
    ssize_t n = ::read(fd_, buf.data(), buf.size());
    if (n < 0) throw std::runtime_error("tun read");
    buf.resize(static_cast<size_t>(n));
    return buf;
}

void TunDevice::write(ByteView data) {
    ::write(fd_, data.data(), data.size());
}

}
