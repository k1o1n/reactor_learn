#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "noncopyable.h"
#include <functional>
#include <memory>
#include "inetaddress.h"
#include <unordered_set>

namespace adachi::tool {
    class EventLoopThread;
    class EventLoop;
    class EventLoopThreadPool;
}
namespace adachi::network {
    class Acceptor;
    class TcpConnection;
}

namespace adachi::network {
    class TcpServer : adachi::tool::NonCopyAble {
    public:
        TcpServer(const INetAddress& listenaddr
        , std::function<void(adachi::tool::EventLoopThread*)> prework = [](adachi::tool::EventLoopThread*){}
        , int maxevents = 1024);
        void SetSubThreadNum(unsigned int num);
        void Start();

        /// 成功建立一个TcpConnnection后需要执行什么内容
        /// 如：为TcpConnection设置读回调（OnMessage）处理Tcp粘包等问题
        void SetNewconnectionCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> callback);

        /// 关闭前需要额外提供什么操作
        void SetCloseCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)>);
    private:
        INetAddress listenaddr_;
    public:
        std::shared_ptr<adachi::tool::EventLoopThreadPool> pool_;
    private:
        std::unique_ptr<adachi::tool::EventLoopThread> acceptor_thread_;
    public:
        adachi::tool::EventLoop* baseloop_ = nullptr;
    private:
        std::unique_ptr<adachi::network::Acceptor> acceptor_;
        std::unordered_set<std::shared_ptr<adachi::network::TcpConnection>> tcpst_;

        std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> closecallback_ = [](std::shared_ptr<adachi::network::TcpConnection>) {};
    };
}
#endif // TCPSERVER_H