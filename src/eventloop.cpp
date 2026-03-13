#include "functional"
#include "eventloop.h"
#include "epoll.h"
#include <atomic>
#include "channel.h"
#include <vector>
#include <mutex>
#include <memory>
namespace adachi::tool {
    /// 非阻塞+LT模式
    EventLoop::EventLoop(int maxevents) 
        : epoll_(this, maxevents)
        , quit_(false)
        , looping_(false)
    {
        
    }

    void EventLoop::StopLoop() {
        quit_.store(true);
        std::unique_lock<std::mutex> lock(mtx_);
    }

    EventLoop::~EventLoop() {
        StopLoop();
    }

    void EventLoop::Loop() {
        quit_.store(false);
        looping_.store(true);
        std::shared_ptr<std::vector<adachi::io::Channel*>> active_list = std::make_shared<std::vector<adachi::io::Channel*>>();
        while (!quit_.load()) {
            // std::lock_guard<std::mutex> lock(mtx_);
            int siz = epoll_.Poll(active_list.get(), 100, epoll_.epoll_list_.size());
            for (int idx = 0; idx < siz; ++idx) (*active_list)[idx]->Handle();
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
}
