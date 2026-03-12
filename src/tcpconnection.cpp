#include "noncopyable.h"
#include "socket.h"
#include "tcpconnection.h"
#include <string>
#include <memory>
#include "channel.h"
#include "eventloop.h"
#include <iostream>
namespace adachi::network {
    TcpConnection::TcpConnection(adachi::tool::EventLoop* loop, int fd, unsigned int read_buffer_size, unsigned int write_buffer_size) 
        : socket_(std::make_unique<Socket>(fd))
        , read_buffer_(read_buffer_size)
        , write_buffer_(write_buffer_size)
        , channel_(std::make_unique<adachi::io::Channel>(loop, fd))
        , onmessage_([this](const std::shared_ptr<TcpConnection>& obj, adachi::io::Buffer& buffer){
            std::string message;
            buffer.ReadBuffer(message);
            std::cout << "[info] receive " << message.size() << " bytes from fd: " << this->Fd() << std::endl;
        })
    {
        
    }
    int TcpConnection::Read(int& saveerrno) {
        int n = read_buffer_.ReadFd(socket_->Fd(), &saveerrno);

        if (n > 0) {
            onmessage_(shared_from_this(), read_buffer_);
        }
        else if (n == 0) {
            Close();
        } else {
            saveerrno = errno;
            if (saveerrno == EAGAIN || saveerrno == EWOULDBLOCK) {

            }
            else {
                Close();
            }
        }
        return n;
    }
    int TcpConnection::Write(const std::string& message, int& saveerrno) {
        size_t _ = message.size();
        int n = 0;
        if (write_buffer_.Empty()) {
            n = write(socket_->Fd(), message.c_str(), _);
            if (n >= 0) {
                if (n != message.size()) {
                    write_buffer_.WriteBuffer(message.data() + n, message.size() - n);
                }
            }
            else {
                saveerrno = errno;
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    Close();
                }
                else {
                    write_buffer_.WriteBuffer(message);
                }
            }
            
        }
        else {
            write_buffer_.WriteBuffer(message);
        }
        if (!write_buffer_.Empty()) {
            channel_->SetActive(channel_->Events() | adachi::io::Channel::kWrite);
        }
        return n;
    }
    int TcpConnection::WriteFd(int* saveerrno) {
        return write_buffer_.WriteFd(Fd(), saveerrno);
    }
    void TcpConnection::Close() {
        auto ptr = shared_from_this();
        if (close_callback_) close_callback_(ptr); // 上层关闭（如果有提供）
        channel_->RemoveFromLoop(); // 关闭所在epoll
        socket_->Close();
    }

    void TcpConnection::SaveLifeMechanism() {
        channel_->Tie(shared_from_this());
    }

    TcpConnection::~TcpConnection() {
        
    }

    void TcpConnection::SetOnMessage(const std::function<void(const std::shared_ptr<TcpConnection>&, adachi::io::Buffer&)>& cb) {
        onmessage_ = cb;
    }

    void TcpConnection::SetCloseCallback(const std::function<void(const std::shared_ptr<TcpConnection>&)>& cb) {
        close_callback_ = cb;
    }

    const int TcpConnection::Fd() const {
        return socket_->Fd();
    }
}
