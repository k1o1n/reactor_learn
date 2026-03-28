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
    /// 在高并发场景下Handle成员函数可能有问题
    class Channel {
    public:
        using callback = std::function<void()>;
        static constexpr int kRead = EPOLLIN;
        static constexpr int kWrite = EPOLLOUT;
        static constexpr int kError = EPOLLERR;
        static constexpr int kClose = EPOLLRDHUP | EPOLLHUP;
        Channel(adachi::tool::EventLoop* loop, int fd);

        void SetReadCallback(const callback&);

        void SetWriteCallback(const callback&);

        void SetErrorCallback(const callback&);

        void SetCloseCallback(const callback&);
        /// 修改当前channel关注的事件
        /// 必须确认当前channel属于某个EventLoop，否则调用无效，仅修改内部表示当前channel希望关注的事件的变量
        Channel* SetActive(const int& status);
        /// 辅助函数，将表示被激活事件变量status赋值给内部激活事件检查变量
        Channel* SetActiveEvents(const int& status);

        /// 可能运行中途出现了错误的情况，尚未处理，需要进行加锁判断，在执行回调函数之前要判断tcp连接情况
        void Handle();

        int Fd();

        int Events();

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
    public:
        adachi::tool::EventLoop* owner_; 
    private:
        bool tied_ = false;
    };
}
#endif // CHANNEL_H