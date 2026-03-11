#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::io {
    class Channel {
    public:
        using callback = std::function<void()>;
        static const int kRead = 1;
        static const int kWrite = 2;
        static const int kError = 4;
        static const int kClose = 8;
        Channel(int fd);

        void SetReadCallback(const callback&);

        void SetWriteCallback(const callback&);

        void SetErrorCallback(const callback&);

        void SetCloseCallback(const callback&);

        Channel* SetActive(const int& status);

        Channel* SetActiveEvents(const int& status);

        void Handle();

        const int Fd();

        const int Events();
    private:
        callback read_callback_;
        callback write_callback_;
        callback error_callback_;
        callback close_callback_;
        
        friend class Epoll;
        int fd_;
        int events_;
        int active_events_;
        adachi::tool::EventLoop* owner_;
    };
}
#endif // CHANNEL_H