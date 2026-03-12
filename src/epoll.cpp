#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include "noncopyable.h"
#include <memory>
#include <algorithm>
#include "channel.h"
#include "epoll.h"
#include "eventloop.h"
namespace adachi::io {
    /// 默认设置成LT模式
    Epoll::Epoll(adachi::tool::EventLoop* loop, int maxevents) 
        : maxevents_(maxevents)
        , owner_(loop)
    {
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        epoll_list_.resize(maxevents_);
    }
    Epoll::~Epoll() {
        close(epoll_fd_);
    }

    int Epoll::Poll(std::vector<Channel*>* active_list, int timeout, int maxevents) {
        maxevents = std::min(maxevents, maxevents_);
        int n = epoll_wait(epoll_fd_, epoll_list_.data(), maxevents, timeout);

        active_list->clear();
        if (n > 0) {
            for (int idx = 0; idx < n; ++idx) {
                active_list->push_back((static_cast<Channel*>(epoll_list_[idx].data.ptr))->SetActiveEvents(epoll_list_[idx].events));
            }
        }
        return n;
    }

    bool Epoll::AddChannel(Channel* channel) {
        if (channel->owner_ == owner_) return UpdateChannel(channel);
        if (!channel->owner_) {
            channel->owner_ = owner_;

            epoll_event newevent;
            newevent.data.ptr = static_cast<void*>(channel);
            newevent.events = channel->events_;

            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->Fd(), &newevent) < 0) return false;
            return true;
        }
        return false;
    }
    bool Epoll::UpdateChannel(Channel* channel) {
        if (channel->owner_ != owner_) return false;
        epoll_event newevent;
        newevent.data.ptr = static_cast<void*>(channel);
        newevent.events = channel->events_;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->Fd(), &newevent) < 0) return false;
        return true;
    }
    bool Epoll::DeleteChannel(Channel* channel) {
        if (channel->owner_ != owner_) return false;
        epoll_event newevent;
        newevent.data.ptr = static_cast<void*>(channel);
        newevent.events = channel->events_;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->Fd(), &newevent) < 0) return false;
        channel->owner_ = nullptr;
        return true;
    }
}