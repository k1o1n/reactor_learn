#ifndef ACCEPTOR
#define ACCEPTOR
#include <netinet/in.h>
#include "noncopyable.h"
#include <functional>
#include "socket.h"
#include "eventloop.h"
#include "channel.h"

namespace adachi::network {
    class INetAddress;
}

namespace adachi::network {
    class Acceptor : adachi::tool::NonCopyAble {
    public:
        Acceptor(adachi::tool::EventLoop* loop, const adachi::network::INetAddress &listenaddr, sa_family_t family = AF_INET);
        
        bool Listen(const int& backlog = 1024);

        bool IsListening();

        void SetNewconnetionCallback(std::function<void()> callback);

    private:
        adachi::network::Socket socket_;
        adachi::tool::EventLoop* owner_;
        bool listen_check_;
        adachi::io::Channel accept_channel_;
        std::function<void()> newconnection_callback_;
    };
}
#endif // ACCEPTOR