#include <functional>
#include <iostream>
#include <atomic>
#include <thread>
#include <set>
#include <memory>
#include <vector>
#include "adachi_network.h"
class Ipv4TcpServer : adachi::tool::NonCopyAble {
public:
    Ipv4TcpServer(const adachi::network::INetAddress& addr) 
        : loop_()
        , acceptor_(&loop_, addr)
        , epoll_(&loop_)
    {
        acceptor_.SetNewconnetionCallback([this, &addr](){
            adachi::network::INetAddress addr;
            int n = this->acceptor_.Accept(addr);
            if (n >= 0) {

                std::shared_ptr<adachi::network::TcpConnection> _ptr;
                this->reg_.push_back(_ptr);
                this->reg_.back() = std::make_shared<adachi::network::TcpConnection>(n);
                this->reg_.back()->channel_.SetReadCallback([](){

                });
                this->reg_.back()->channel_.SetWriteCallback([](){
                    
                });
                this->reg_.back()->channel_.SetErrorCallback([](){

                });
                this->reg_.back()->channel_.SetCloseCallback([](){
                    
                });
            }
            this->reg_.back()->channel_.SetActive(adachi::io::Channel::kRead 
                | adachi::io::Channel::kWrite 
                | adachi::io::Channel::kError 
                | adachi::io::Channel:: kClose);
            
            this->epoll_.AddChannel(&this->reg_.back()->channel_);
        });
        acceptor_.accept_channel_.SetActive(adachi::io::Channel::kRead 
                | adachi::io::Channel::kWrite 
                | adachi::io::Channel::kError 
                | adachi::io::Channel::kClose);
        epoll_.AddChannel(&acceptor_.accept_channel_);
    }
    void Stop() {
        loop_.StopLoop();
    }
    void Loop() {
        while (loop_.Status());
        acceptor_.Listen();
        work_thread_ = std::thread([this]() {
            loop_.Loop();
        });
    }
    ~Ipv4TcpServer() {
        Stop();
        work_thread_.join();
    }
private:
    adachi::tool::EventLoop loop_;
    adachi::network::Acceptor acceptor_;
    adachi::io::Epoll epoll_;
    std::thread work_thread_;
    std::vector<std::shared_ptr<adachi::network::TcpConnection>> reg_;
};

int main() {
    adachi::network::INetAddress addr;
    addr.SetFamily(adachi::network::INetAddress::IPV4);
    addr.SetPort(12345);

    Ipv4TcpServer testserver(addr);
    testserver.Loop();
    int _;
    std::cin >> _;
    testserver.Stop();
    return 0;
}