#pragma once
#include "common/Buffer.h"
namespace net {
class ILinkLayer {
public:
    virtual void write(ByteView data) = 0;
    virtual int fd() const noexcept = 0;
    virtual ~ILinkLayer() = default;
};
}
