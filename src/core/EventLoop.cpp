#include "core/EventLoop.h"
#include <array>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
namespace core {
EventLoop::EventLoop() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1");
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd_ < 0) { ::close(epoll_fd_); throw std::runtime_error("timerfd_create"); }
    epoll_event ev{EPOLLIN, {.fd = timer_fd_}};
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ev);
}
EventLoop::~EventLoop() {
    ::close(timer_fd_);
    ::close(epoll_fd_);
}
void EventLoop::run(int watch_fd, std::function<void(net::ByteView)> on_packet) {
    epoll_event ev{EPOLLIN, {.fd = watch_fd}};
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, watch_fd, &ev);
    running_ = true;
    std::array<uint8_t, 65536> buf{};
    std::array<epoll_event, 8> evs{};
    while (running_) {
        int n = epoll_wait(epoll_fd_, evs.data(), static_cast<int>(evs.size()), -1);
        for (int i = 0; i < n; ++i) {
            int fd = evs[i].data.fd;
            if (fd == timer_fd_) {
                uint64_t exp{}; ::read(timer_fd_, &exp, sizeof(exp));
                fire_expired();
            } else {
                ssize_t r = ::read(fd, buf.data(), buf.size());
                if (r > 0) on_packet({buf.data(), static_cast<size_t>(r)});
            }
        }
    }
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, watch_fd, nullptr);
}
void EventLoop::stop() { running_ = false; }
TimerId EventLoop::after(std::chrono::milliseconds delay, std::function<void()> cb) {
    TimerId id = next_id_++;
    heap_.push({Clock::now() + delay, id, std::move(cb)});
    arm();
    return id;
}
void EventLoop::cancel(TimerId id) { cancelled_.insert(id); }
void EventLoop::arm() {
    while (!heap_.empty() && cancelled_.count(heap_.top().id)) heap_.pop();
    if (heap_.empty()) return;
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(heap_.top().expiry - Clock::now());
    if (ns.count() <= 0) ns = std::chrono::nanoseconds{1};
    itimerspec ts{};
    ts.it_value.tv_sec = ns.count() / 1'000'000'000;
    ts.it_value.tv_nsec = ns.count() % 1'000'000'000;
    timerfd_settime(timer_fd_, 0, &ts, nullptr);
}
void EventLoop::fire_expired() {
    auto now = Clock::now();
    while (!heap_.empty() && heap_.top().expiry <= now) {
        auto e = heap_.top(); heap_.pop();
        if (!cancelled_.count(e.id)) e.cb();
        cancelled_.erase(e.id);
    }
    arm();
}
}
