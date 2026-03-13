#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>
#include <memory>
#include <sys/epoll.h>
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::io {
    /// Channel类可能会创建失败，需要手动调用GetOwner()检查是否绑定了一个loop
    class Channel {
    public:
        using callback = std::function<void()>;
        static const int kRead = EPOLLIN;
        static const int kWrite = EPOLLOUT;
        static const int kError = EPOLLERR;
        static const int kClose = EPOLLRDHUP;
        Channel(adachi::tool::EventLoop* loop, int fd);

        void SetReadCallback(const callback&);

        void SetWriteCallback(const callback&);

        void SetErrorCallback(const callback&);

        void SetCloseCallback(const callback&);
        /// 修改当前channel关注的事件
        Channel* SetActive(const int& status);
        /// 辅助函数，将表示被激活事件变量status赋值给内部激活事件检查变量
        Channel* SetActiveEvents(const int& status);

        void Handle();

        const int Fd();

        const int Events();

        void Tie(const std::shared_ptr<void>& obj);

        void RemoveFromLoop();

        const adachi::tool::EventLoop* GetOwner() const;
    private:
        callback read_callback_;
        callback write_callback_;
        callback error_callback_;
        callback close_callback_;
        std::weak_ptr<void> save_life_ptr_; // 指向tcpconnection的保活指针

        friend class Epoll;
        int fd_;
        int events_;
        int active_events_;
        adachi::tool::EventLoop* owner_; 
        bool tied_ = false;
    };
}
#endif // CHANNEL_H