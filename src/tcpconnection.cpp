#include "noncopyable.h"
#include "socket.h"
#include "tcpconnection.h"
#include <string>
#include <memory>
#include "channel.h"
#include "eventloop.h"
#include <iostream>
#include <cstring>
#include <cerrno>

namespace adachi::network {
    TcpConnection::TcpConnection(adachi::tool::EventLoop* loop, int fd, unsigned int read_buffer_size, unsigned int write_buffer_size) 
        
        : channel_(std::make_unique<adachi::io::Channel>(loop, fd))
        , status_(kConnecting)
        , socket_(std::make_unique<Socket>(fd))
        , read_buffer_(read_buffer_size)
        , write_buffer_(write_buffer_size)
        , onmessage_([](const std::shared_ptr<TcpConnection> obj, adachi::io::Buffer& buffer){
            std::string message;
            buffer.ReadBuffer(message);
            std::cout << "[info] receive " << message.size() << " bytes" << std::endl;
            obj->Write(message + " OK");
        })
    {
        channel_->SetReadCallback([ptr = this]() {
            ptr->Read();
        });

        channel_->SetWriteCallback([ptr = this]() {
            ptr->WriteFd();
        });
    }
    /// 正常读操作完毕会默认调用onmessage进行解析
    void TcpConnection::Read() {
        if (status_ != kConnecting) return;
        int saveerrno;
        int n = read_buffer_.ReadFd(socket_->Fd(), saveerrno);

        if (n > 0) {
            onmessage_(shared_from_this(), read_buffer_);
        }
        else if (n == 0) {
            Close();
        } else {
            if (saveerrno == EAGAIN || saveerrno == EWOULDBLOCK || saveerrno == EINTR) {

            }
            else {
                std::cout << "[Error] TcpConnection Read failed: " << strerror(saveerrno) << std::endl;
                Close();
            }
        }
    }

    /// 用户可能跨线程调用，必须保证安全
    void TcpConnection::Write(std::string message) {
        if (channel_) {
            if (channel_->owner_) {
                if (channel_->owner_->IsInThread()) {
                    WriteInThread(std::move(message));
                }
                else {
                    channel_->owner_->Submit([this, msg = std::move(message)]() mutable {
                        WriteInThread(std::move(msg));
                    });
                }
            }
            else {
                std::cout << "[Error] TcpConnection::Write failed: channel->owner_ is nullptr" << std::endl;
            }
        }
        else {
            std::cout << "[Error] TcpConnection::Write failed: channel is nullptr" << std::endl;
        }
    }
    void TcpConnection::WriteFd() {
        int saveerrno;
        if (status_ == kDisConnected) return;
        int n = write_buffer_.WriteFd(Fd(), saveerrno);
        
        if (n < 0) {
            if (saveerrno != EAGAIN && saveerrno != EWOULDBLOCK && saveerrno != EINTR) {
                std::cout << "[Error] TcpConnection::WriteFd failed: " << strerror(saveerrno) << std::endl;
                Close();
            }
        }

        if (write_buffer_.Empty()) {
            if (channel_->Events() & adachi::io::Channel::kWrite) channel_->SetActive(channel_->Events() ^ adachi::io::Channel::kWrite);
            
            if (status_ == kDisConnecting) {
                status_ = kDisConnected;
                auto ptr = shared_from_this();
                if (close_callback_) close_callback_(ptr); // 上层关闭（如果有提供）
                channel_->RemoveFromLoop(); // 关闭所在epoll
                socket_->Close();
                Close();
            }
        }
    }
    void TcpConnection::Close() {
        if (channel_) {
            if (channel_->owner_) {
                if (channel_->owner_->IsInThread()) {
                    CloseInThread();
                }
                else {
                    channel_->owner_->Submit([this]() {
                        CloseInThread();
                    });
                }
            }
            else {
                std::cout << "[Error] TcpConnection::Write failed: channel->owner_ is nullptr" << std::endl;
            }
        }
        else {
            std::cout << "[Error] TcpConnection::Write failed: channel is nullptr" << std::endl;
        }
    }

    void TcpConnection::SaveLifeMechanism() {
        channel_->Tie(shared_from_this());
    }

    TcpConnection::~TcpConnection() {
        channel_->RemoveFromLoop();
    }

    /// 这个部分需要修改，确保io在本线程，任务在别处 
    void TcpConnection::SetOnMessage(const std::function<void(const std::shared_ptr<TcpConnection>, adachi::io::Buffer&)>& cb) {
        onmessage_ = cb;
    }

    void TcpConnection::SetCloseCallback(const std::function<void(const std::shared_ptr<TcpConnection>)>& cb) {
        close_callback_ = cb;
    }

    int TcpConnection::Fd() const {
        return socket_->Fd();
    }

    bool TcpConnection::IsWriteBufferEmpty() {
        return write_buffer_.Empty();
    }

    void TcpConnection::WriteInThread(std::string message) {
        int saveerrno;
        if (status_ != kConnecting) return;
        size_t _ = message.size();
        if (write_buffer_.Empty()) {
            int n = write(socket_->Fd(), message.c_str(), _);
            if (n >= 0) {
                if (static_cast<unsigned int>(n) != message.size()) {
                    write_buffer_.WriteBuffer(message.data() + n, message.size() - n);
                }
            }
            else {
                saveerrno = errno;
                if (saveerrno != EAGAIN && saveerrno != EWOULDBLOCK && saveerrno != EINTR) {
                    std::cout << "[Error] TcpConnection::write failed: " << strerror(saveerrno) << std::endl;
                    CloseInThread();
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
    }

    void TcpConnection::CloseInThread() {
        if (status_ == kDisConnected) return;
        if (write_buffer_.Empty()) {
            status_ = kDisConnected;
            auto ptr = shared_from_this();
            channel_->RemoveFromLoop(); // 关闭所在epoll
            socket_->Close();
            if (close_callback_) close_callback_(ptr); // 上层关闭（如果有提供）
        }
        else {
            status_ = kDisConnecting;
            channel_->SetActive(channel_->Events() | adachi::io::Channel::kWrite);
        }
    }
}
