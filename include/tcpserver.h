#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "noncopyable.h"
#include <functional>
#include <memory>
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
        TcpServer(std::function<void(adachi::tool::EventLoopThread*)> prework = [](adachi::tool::EventLoopThread* loop){}, int maxevents = 1024);
        void SetSubThreadNum(unsigned int num);
        void Start();
        void SetNewConnectionCallback(std::function<void()>);
    private:
        std::shared_ptr<adachi::tool::EventLoopThreadPool> pool_;
        std::unique_ptr<adachi::network::Acceptor> acceptor_;
    };
}
#endif // TCPSERVER_H