#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>
#include "eventloop.h"
namespace adachi::tool {
    class Channel;
}
namespace adachi::io {
    class Channel {
    public:
        using callback = std::function<void()>;
        static const int kRead = 1;
        static const int kWrite = 2;
        static const int kError = 4;
        static const int kCallback = 8;
        Channel(int fd);

        void SetReadCallback(const callback&);

        void SetWriteCallback(const callback&);

        void SetErrorCallback(const callback&);

        void SetCloseCallback(const callback&);

        void SetActive(const int& status);

        callback read_callback_;
        callback write_callback_;
        callback error_callback_;
        callback close_callback_;

        const int Fd();

        const int Events();
    private:
        friend class Epoll;
        int fd_;
        int events_;
        int active_events_;
        adachi::tool::EventLoop* owner_;

        Channel* SetActiveEvents(int active_events);
    };
}
#endif // CHANNEL_H