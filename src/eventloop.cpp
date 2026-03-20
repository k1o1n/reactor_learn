#include <functional>
#include "eventloop.h"
#include "epoll.h"
#include <atomic>
#include "channel.h"
#include <vector>
#include <mutex>
#include <memory>
#include <iostream>
#include <sys/eventfd.h>
#include <cerror>
namespace adachi::tool {
    /// 非阻塞+LT模式
    EventLoop::EventLoop(int maxevents) 
        : epoll_(this, maxevents)
        , quit_(false)
        , looping_(false)
        , wakeupchannel_(this, eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
        , tid_(std::this_thread::get_id())
    {
        if (wakeupchannel_.Fd() >= 0) {
            wakeupchannel_.SetReadCallback([this](){
                if (this->wakeupchannel_.Fd() >= 0) {
                    uint64_t one;
                    int n = read(this->wakeupchannel_.Fd(), &one, sizeof(one));
                    if (n <= 0) {
                        std::cout << "[Error] wakeup fail: " << strerr(errno) << std::endl;
                    }
                }
            });
            wakeupchannel_.SetActive(adachi::io::Channel::kRead);
        }
    }

    void EventLoop::StopLoop() {
        quit_.store(true);
        WakeUp(); // epoll死睡是无法被唤醒的，有风险
        // std::unique_lock<std::mutex> lock(mtx_);
    }

    EventLoop::~EventLoop() {
        StopLoop();
    }

    void EventLoop::Loop() {
        quit_.store(false);
        looping_.store(true);
        std::vector<adachi::io::Channel*> active_list;
        while (!quit_.load()) {
            // std::lock_guard<std::mutex> lock(mtx_);
            int siz = epoll_.Poll(&active_list, 100, epoll_.epoll_list_.size());
            for (int idx = 0; idx < siz; ++idx) active_list[idx]->Handle();
            DoCrossThreadMission();
        }
        looping_.store(false);
    }


    bool EventLoop::AddChannel(adachi::io::Channel* channel) {
        return epoll_.AddChannel(channel);
    }
    bool EventLoop::UpdateChannel(adachi::io::Channel* channel) {
        return epoll_.UpdateChannel(channel);
    }
    bool EventLoop::DeleteChannel(adachi::io::Channel* channel) {
        return epoll_.DeleteChannel(channel);
    }

    void EventLoop::WakeUp() {
        if (wakeupchannel_.Fd() >= 0) {
            uint64_t one = 1;
            write(wakeupchannel_.Fd(), &one, sizeof(one));
        }
    }

    void EventLoop::DoCrossThreadMission() {
        std::vector<std::function<void()>> missions;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            std::swap(missions_, missions);
        }
        for (const std::function<void()>& func : missions) {
            func();
        }
    }

    void EventLoop::Submit(const std::function<void()>& cb) {
        if (tid_ == std::this_thread::get_id()) {
            cb();
        }
        else {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                missions_.emplace_back(cb);
            }
            WakeUp();
        }
    }
}
