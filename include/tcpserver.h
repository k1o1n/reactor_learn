#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "noncopyable.h"
#include <functional>
#include <memory>
#include "inetaddress.h"
#include <unordered_set>
#include "timer/timer.h"

namespace adachi::tool {
    class EventLoopThread;
    class EventLoop;
    class EventLoopThreadPool;
}

namespace adachi::network {
    class Acceptor;
    class TcpConnection;
}

namespace adachi::io {
    class TimerOpt;
}

namespace adachi::network {
    /// WARNING: TcpServer 的生命周期必须长于所有 TcpConnection，否则其内部回调会引发 Use-After-Free 悬垂指针崩溃！
    /// 其中timeropt参数为心跳机制，{时间轮片数，{每tick秒，每tick纳秒}}，将时间轮片数设定为0表示关闭心跳机制
    class TcpServer : adachi::tool::NonCopyAble, public std::enable_shared_from_this<TcpServer> {
    public:
        TcpServer(const INetAddress& listenaddr
        , const adachi::io::TimerOpt& timeropt = {0, {0, 0}}
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