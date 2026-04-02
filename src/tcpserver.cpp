#include "tcpserver.h"
#include "eventloop.h"
#include "eventloopthread.h"
#include "eventloopthreadpool.h"
#include "acceptor.h"
#include <iostream>
#include "inetaddress.h"
#include "tcpconnection.h"
#include "logger.h"

namespace adachi::network {
    TcpServer::TcpServer(const INetAddress& listenaddr
        , const adachi::io::TimerOpt& timeropt
        , std::function<void(adachi::tool::EventLoopThread*)> prework
        , int maxevents)

        : listenaddr_(listenaddr)
        , pool_(std::make_shared<adachi::tool::EventLoopThreadPool>(timeropt, prework, maxevents))
        , acceptor_thread_(std::make_unique<adachi::tool::EventLoopThread>(timeropt, prework, maxevents))
        , baseloop_(acceptor_thread_->Start())
        , acceptor_(std::make_unique<adachi::network::Acceptor>(baseloop_, listenaddr))
        , isheartbeat_(timeropt.heartbeat_num_ > 0)
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
            ADACHI_LOG_ERROR << "TcpServer Start failed: acceptor_thread_->Start() return nullptr\n";
        }   
    }

    void TcpServer::SetNewconnectionCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> callback) {
        acceptor_->SetNewconnectionCallback([this, callback = std::move(callback), isheartbeat = isheartbeat_](int fd, adachi::network::INetAddress& addr, int saveerrno) {
            if (fd >= 0) {
                ADACHI_LOG_INFO << "receive a connection from " << addr.Ip() << "\n";
                addr.Ip();
                std::shared_ptr<adachi::network::TcpConnection> linkptr = std::make_shared<adachi::network::TcpConnection>(pool_->GetOneThread(), fd);
                linkptr->addr_ = addr;
                if (isheartbeat) {
                    linkptr->SaveLifeMechanism();
                }

                tcpst_.insert(linkptr);
                linkptr->SetCloseCallback([this, closecallback = closecallback_](std::shared_ptr<TcpConnection> linkptr) {
                    //std::lock_guard<std::mutex> lock(server.mtx_);
                    closecallback(linkptr);
                    baseloop_->Submit([this, linkptr]() {
                        tcpst_.erase(linkptr);
                    }); /// 多线程操纵红黑树有危险，需要交由一个线程统一管理
                });

                callback(linkptr);
            }
            else {
                strerror(saveerrno);
                ADACHI_LOG_ERROR << "connection failed: " << strerror(saveerrno) << "\n";
            }
        });
    }   

    void TcpServer::SetCloseCallback(std::function<void(std::shared_ptr<adachi::network::TcpConnection>)> callback) {
        closecallback_ = callback;
    }
}