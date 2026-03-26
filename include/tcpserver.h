#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "noncopyable.h"
#include <functional>
#include <memory>
#include "inetaddress.h"
namespace adachi::tool {
    class EventLoopThread;
    class EventLoop;
    class EventLoopThreadPool;
}
namespace adachi::network {
    class Acceptor;
}
namespace adachi::network {
    class TcpServer : adachi::tool::NonCopyAble {
    public:
        TcpServer(const INetAddress& listenaddr
        , std::function<void(adachi::tool::EventLoopThread*)> prework = [](adachi::tool::EventLoopThread*){}
        , int maxevents = 1024);
        void SetSubThreadNum(unsigned int num);
        void Start();
        void SetNewconnectionCallback(std::function<void(int, INetAddress&, int)>);
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
    };
}
#endif // TCPSERVER_H