#include "socket.h"
#include <netinet/in.h>
#include <iostream>
#include "channel.h"
#include <functional>
#include "inetaddress.h"
#include "eventloop.h"
#include "acceptor.h"
namespace adachi::network {
    Acceptor::Acceptor(adachi::tool::EventLoop* loop, const INetAddress &listenaddr, sa_family_t family) 
        : owner_(loop)
        , socket_(adachi::network::Socket::CreateNonBlockSocket())
        , accept_channel_(loop, socket_.Fd())
    {
        if (!socket_.BindAddress(listenaddr)) {
            std::cout << "[error] class Acceptor: BindAddress error" << std::endl;
        }
    }

    bool Acceptor::Listen(const int& backlog) {
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
        accept_channel_.SetReadCallback(callback);
    }
}
