#ifndef EPOLL_H
#define EPOLL_H
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include "noncopyable.h"
#include <memory>
#include <algorithm>
#include "channel.h"
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::io {
    class Epoll : adachi::tool::NonCopyAble {
    public:
        Epoll(adachi::tool::EventLoop* loop, int maxevents = 1024);
        ~Epoll();

        int Poll(std::vector<Channel*>* active_list, int timeout = -1, int maxevents = 1024);

        bool AddChannel(Channel* channel);
        bool UpdateChannel(Channel* channel);
        bool DeleteChannel(Channel* channel);
    private:
        friend adachi::tool::EventLoop;
        adachi::tool::EventLoop* owner_;
        int maxevents_;
        int epoll_fd_;
        std::vector<epoll_event> epoll_list_;
    };
}
#endif // EPOLL_H