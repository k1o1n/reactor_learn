#include "tcpserver.h"
#include "eventloop.h"
#include "eventloopthread.h"
#include "eventloopthreadpool.h"
#include "acceptor.h"
#include <iostream>
#include "inetaddress.h"
#include "tcpconnection.h"

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
        SetNewconnectionCallback([](std::shared_ptr<adachi::network::TcpConnection>) {});
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

    void TcpServer::SetNewconnectionCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> callback) {
        acceptor_->SetNewconnectionCallback([server = this, callback = std::move(callback)](int fd, adachi::network::INetAddress& addr, int saveerrno) {
            if (fd >= 0) {
                // std::cout << "[info] receive a connection from " << addr.Ip() << " " << std::endl;
                addr.Ip();
                std::shared_ptr<adachi::network::TcpConnection> linkptr = std::make_shared<adachi::network::TcpConnection>(server->pool_->GetOneThread(), fd);
                linkptr->SaveLifeMechanism();
                
                server->tcpst_.insert(linkptr);

                linkptr->channel_->SetReadCallback([linkptr]() {
                    linkptr->Read();
                });
                linkptr->SetCloseCallback([server, closecallback = server->closecallback_](std::shared_ptr<TcpConnection> linkptr) {
                    //std::lock_guard<std::mutex> lock(server.mtx_);
                    closecallback(linkptr);
                    server->baseloop_->Submit([server, linkptr]() {
                        server->tcpst_.erase(linkptr);
                    }); /// 多线程操纵红黑树有危险，需要交由一个线程统一管理
                });
                linkptr->channel_->SetActive(linkptr->channel_->Events() | adachi::io::Channel::kClose);
                callback(linkptr);
            }
            else {
                std::cout << "[Error] connection failed: " << strerror(saveerrno) << std::endl;
            }
        });
    }   

    void TcpServer::SetCloseCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> callback) {
        closecallback_ = callback;
    }
}