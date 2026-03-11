#include "functional"
#include "eventloop.h"
#include "epoll.h"
#include <atomic>
#include "channel.h"
#include <vector>
#include <mutex>
namespace adachi::tool {
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
        while (!quit_.load()) {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<adachi::io::Channel*>* active_list;
            int siz = epoll_.Poll(active_list, -1, epoll_.epoll_list_.size());
            for (int idx = 0; idx < siz; ++idx) (*active_list)[idx]->Handle();
        }
        looping_.store(false);
    }
}
