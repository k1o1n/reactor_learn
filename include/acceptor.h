#ifndef ACCEPTOR
#define ACCEPTOR
#include <netinet/in.h>
#include "noncopyable.h"
#include <functional>
#include "socket.h"
#include "channel.h"
namespace adachi::network {
    class INetAddress;
}
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::network {
    class Acceptor : adachi::tool::NonCopyAble {
    public:
        Acceptor(adachi::tool::EventLoop* loop, const adachi::network::INetAddress &listenaddr, sa_family_t family = AF_INET);
        
        bool Listen(const int& backlog = 1024);

        bool IsListening();

        void SetNewconnetionCallback(std::function<void()> callback);

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