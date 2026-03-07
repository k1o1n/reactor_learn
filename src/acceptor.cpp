#include "socket.h"
#include <netinet/in.h>
#include <iostream>
#include "channel.h"
#include <functional>
#include "inetaddress.h"
#include "eventloop.h"
namespace adachi::network {
    Acceptor::Acceptor(EventLoop* loop, const INetAddress &listenaddr, sa_family_t family) 
        : owner_(loop)
        , socket_(adachi::network::Socket::CreateNonBlockSocket())
        , listen_check_(false)
    {
        if (!socket_.BindAddress(listenaddr)) {
            std::cout << "[error] class Acceptor: BindAddress error" << std::endl;
        }
    }

    bool Acceptor::Listen(const int& backlog = 1024) {
        if (socket_.Listen(backlog)) {
            listen_check_ = false;
            return true;
        }
        return false;
    }

    bool Acceptor::IsListening() {
        return listen_check_;
    }

    void Acceptor::SetNewconnetionCallback(std::function<void()> callback) {
        newconnection_callback_ = callback;
    }
}
