#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace net {

using ByteView = std::span<const uint8_t>;
using MutByteView = std::span<uint8_t>;

class Buffer {
public:
    Buffer() = default;
    explicit Buffer(size_t n) : data_(n, 0) {}
    Buffer(const uint8_t* ptr, size_t n) : data_(ptr, ptr + n) {}
    explicit Buffer(ByteView v) : data_(v.begin(), v.end()) {}

    uint8_t* data() noexcept { return data_.data(); }
    const uint8_t* data() const noexcept { return data_.data(); }
    size_t size() const noexcept { return data_.size(); }
    bool empty() const noexcept { return data_.empty(); }

    void append(ByteView v) { data_.insert(data_.end(), v.begin(), v.end()); }
    void resize(size_t n) { data_.resize(n, 0); }

    ByteView view() const noexcept { return {data_.data(), data_.size()}; }
    MutByteView mut_view() noexcept { return {data_.data(), data_.size()}; }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    uint8_t& operator[](size_t i) { return data_[i]; }
    uint8_t operator[](size_t i) const { return data_[i]; }

private:
    std::vector<uint8_t> data_;
};

}
