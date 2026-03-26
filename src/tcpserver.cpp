#include "tcpserver.h"
#include "eventloop.h"
#include "eventloopthread.h"
#include "eventloopthreadpool.h"
#include "acceptor.h"
#include <iostream>
#include "inetaddress.h"

namespace adachi::network {
    TcpServer::TcpServer(const INetAddress& listenaddr
        , std::function<void(adachi::tool::EventLoopThread*)> prework
        , int maxevents)

        : listenaddr_(listenaddr)
        , pool_(std::make_shared<adachi::tool::EventLoopThreadPool>(prework, maxevents))
        , acceptor_thread_(std::make_unique<adachi::tool::EventLoopThread>(prework, maxevents))
        , baseloop_(acceptor_thread_->Start())
        , acceptor_(std::make_unique<adachi::network::Acceptor>(baseloop_, listenaddr))
    {
        SetSubThreadNum(1);
    }
    void TcpServer::SetSubThreadNum(unsigned int num) {
        pool_->SetSize(num);
    }
    void TcpServer::Start() {
        if (baseloop_) {
            acceptor_->Listen();
            // 设置回调函数
            pool_->Start();
        }
        else {
            std::cout << "[Error] TcpServer Start failed: acceptor_thread_->Start() return nullptr" << std::endl;
        }   
    }
    void TcpServer::SetNewconnectionCallback(std::function<void(int, INetAddress&, int)> callback) {
        acceptor_->SetNewconnectionCallback(callback);
    }   
}