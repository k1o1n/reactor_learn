#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include "noncopyable.h"
#include "socket.h"
#include <string>
#include "channel.h"
#include "buffer.h"
#include <memory>
#include <functional>
#include <cerrno>
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::network {
    /// 必须显式调用SaveLifeMechanism()保活机制才能开始使用，否则运行中可能出现问题，或者尝试使用工厂模式批量生产
    /// 表示一个tcp连接，提供一个OnMessage消息处理机制，同时后续可以设定CloseCallback关闭回调提供额外的操作。
    /// 默认关闭时只关闭eventloop，即调用EventLoop::DeleteChannel
    /// TcpConnection类由于持有Channel，创建后需要检查是否绑定成功Eventloop（调用Channel成员的GetOwner()检查）
    /// 默认创建后就会被加入到一个EventLoop中
    /// 设置该类的可写回调时必须显示调用该类的WriteFd函数，这涉及到该类的正常关闭
    class TcpConnection : adachi::tool::NonCopyAble, public std::enable_shared_from_this<TcpConnection> {
    public:
        TcpConnection(adachi::tool::EventLoop* loop, int fd, unsigned int read_buffer_size = 1024, unsigned int write_buffer_size = 1024);
        int Read(int&);
        /// 发送信息，未发送成功的信息存在缓冲区，并设置epoll开始关注可写事件
        int Write(const std::string& message, int& saveerrno);
        /// 尝试发送可写缓冲区信息到对应描述符
        int WriteFd(int* saveerrno);
        void Close();
        ~TcpConnection();
        void SaveLifeMechanism();
        std::unique_ptr<adachi::io::Channel> channel_;
        void SetOnMessage(const std::function<void(const std::shared_ptr<TcpConnection>&, adachi::io::Buffer&)>& cb);    
        /// 额外提供的关闭回调，如果不提供则不会进行任何操作，实际关闭时会传入被关闭对象的一个智能指针
        /// 提供额外关闭回调，请不要操作EventLoop或者tcpconnection本身的关闭操作，这些关闭操作将被自动执行
        void SetCloseCallback(const std::function<void(const std::shared_ptr<TcpConnection>&)>&);    
        int Fd() const;
        bool IsWriteBufferEmpty();

    private:
        enum connectionstate {
            kConnecting,
            kDisConnecting,
            kDisConnected
        } status_;
        std::unique_ptr<Socket> socket_;
        adachi::io::Buffer read_buffer_;
        adachi::io::Buffer write_buffer_;

        std::function<void(const std::shared_ptr<TcpConnection>&, adachi::io::Buffer&)> onmessage_;
        std::function<void(const std::shared_ptr<TcpConnection>&)> close_callback_;
    };
}
#endif // TCPCONNECTION