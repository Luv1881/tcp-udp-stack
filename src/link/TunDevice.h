#pragma once
#include "common/Buffer.h"
#include <string>

namespace net {

class TunDevice {
public:
    explicit TunDevice(const std::string& name);
    ~TunDevice();
    TunDevice(const TunDevice&) = delete;
    TunDevice& operator=(const TunDevice&) = delete;

    Buffer read();
    void write(ByteView data);
    int fd() const noexcept { return fd_; }

private:
    int fd_ = -1;
};

}
