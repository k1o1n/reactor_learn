#ifndef ACCEPTOR
#define ACCEPTOR
#include <netinet/in.h>
#include "noncopyable.h"
#include <functional>
#include "socket.h"
#include "channel.h"
#include <cerrno>
namespace adachi::network {
    class INetAddress;
}
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::network {
    class INetAddress;
    class Acceptor : adachi::tool::NonCopyAble {
    public:
        Acceptor(adachi::tool::EventLoop* loop, const adachi::network::INetAddress &listenaddr);
        
        bool Listen(const int& backlog = 1024);

        bool IsListening();

        /// 设置接收到新fd之后会进行什么操作，分别为fd、INetAddress和出现的错误，
        /// 注意：INetAddress在回调函数执行完毕会被销毁，不要尝试直接使用指针绑定
        void SetNewconnectionCallback(std::function<void(int, INetAddress&, int)> callback);

        int Accept(INetAddress& addr) {
            return socket_.Accept(addr);
        }
    private:
        adachi::network::Socket socket_;
    public:
        adachi::io::Channel accept_channel_;
    private:
        adachi::tool::EventLoop* owner_;
        bool listen_check_ = false;
    };
}
#endif // ACCEPTOR