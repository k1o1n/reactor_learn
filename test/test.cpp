#include <functional>
#include <iostream>
#include <atomic>
#include <thread>
#include <set>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include "adachi_network.h"
class Ipv4TcpServer : adachi::tool::NonCopyAble {
public:
    Ipv4TcpServer(const adachi::network::INetAddress& addr) 
        : loop_()
        , acceptor_(&loop_, addr)
    {
        acceptor_.SetNewconnetionCallback([this, &addr](){
            adachi::network::INetAddress addr;
            int n = this->acceptor_.Accept(addr);
            if (n >= 0) {
                std::shared_ptr<adachi::network::TcpConnection> _ptr = std::make_shared<adachi::network::TcpConnection>(&this->loop_, n);
                if (_ptr->channel_->GetOwner() == NULL) {
                    std::cout << "[info] Tcp construction failed" << std::endl;
                }
                else {
                    std::cout << "[info] Tcp Construction success" << std::endl;
                }
                this->reg_.insert(_ptr);
                std::weak_ptr<adachi::network::TcpConnection> weak_conn = _ptr; // 防止泄露（传递shared_ptr会导致有一份shared_ptr指针一直指向channel导致内存无法正常释放）
                _ptr->channel_->SetReadCallback([weak_conn](){
                    if (auto conn = weak_conn.lock()) {
                        int saveerrno;
                        conn->Read(saveerrno);
                    }
                });
                _ptr->channel_->SetWriteCallback([weak_conn](){
                    if (auto conn = weak_conn.lock()) {
                        int saveerrno;
                        int n = conn->WriteFd(&saveerrno);

                        if (conn->IsWriteBufferEmpty()) {
                            auto events = conn->channel_->Events();
                            if (events & adachi::io::Channel::kWrite)
                                events ^= adachi::io::Channel::kWrite;
                            conn->channel_->SetActive(events);
                        }
                    }
                });
                _ptr->channel_->SetErrorCallback([weak_conn](){
                    if (auto conn = weak_conn.lock()) {
                        // 发生错误（如RST），记录错误并关闭连接
                        // 实际开发中可以在这里 log 一下 errno
                        conn->Close();
                    }
                });
                _ptr->channel_->SetCloseCallback([weak_conn](){
                    if (auto conn = weak_conn.lock()) {
                        // 对端关闭连接（检测到 EPOLLRDHUP 或 EPOLLHUP），执行关闭逻辑
                        conn->Close();
                    }
                });
                _ptr->channel_->SetActive(adachi::io::Channel::kRead 
                    | adachi::io::Channel::kWrite 
                    | adachi::io::Channel::kError 
                    | adachi::io::Channel:: kClose);
                _ptr->SetCloseCallback([this](const std::shared_ptr<adachi::network::TcpConnection>& obj){
                    this->reg_.erase(obj);
                });
                _ptr->SaveLifeMechanism();
                this->loop_.AddChannel(_ptr->channel_.get());
            }
        });
        acceptor_.accept_channel_.SetActive(adachi::io::Channel::kRead 
                | adachi::io::Channel::kWrite 
                | adachi::io::Channel::kError 
                | adachi::io::Channel::kClose);
        loop_.AddChannel(&acceptor_.accept_channel_);
    }
    void Stop() {
        loop_.StopLoop();
    }
    void Loop() {
        // while (loop_.Status());
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
    std::thread work_thread_;
    std::set<std::shared_ptr<adachi::network::TcpConnection>> reg_;
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