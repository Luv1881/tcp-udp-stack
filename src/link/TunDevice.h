#pragma once
#include "link/ILinkLayer.h"
#include <string>

namespace net {

class TunDevice : public ILinkLayer {
public:
    explicit TunDevice(const std::string& name);
    ~TunDevice();
    TunDevice(const TunDevice&)            = delete;
    TunDevice& operator=(const TunDevice&) = delete;

    Buffer read();
    void   write(ByteView data) override;
    int    fd() const noexcept override { return fd_; }

private:
    int fd_ = -1;
};

}
