#pragma once
#include "common/Buffer.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
namespace core {
using TimerId = uint64_t;
using Clock = std::chrono::steady_clock;
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    void run(int watch_fd, std::function<void(net::ByteView)> on_packet);
    void stop();
    TimerId after(std::chrono::milliseconds delay, std::function<void()> cb);
    void cancel(TimerId id);
private:
    struct Entry {
        Clock::time_point expiry;
        TimerId id;
        std::function<void()> cb;
        bool operator>(const Entry& o) const { return expiry > o.expiry; }
    };
    void arm();
    void fire_expired();
    int epoll_fd_;
    int timer_fd_;
    bool running_ = false;
    TimerId next_id_ = 1;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap_;
    std::unordered_set<TimerId> cancelled_;
};
}
