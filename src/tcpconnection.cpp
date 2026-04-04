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
#include "logger.h"

namespace adachi::network {
    TcpConnection::TcpConnection(adachi::tool::EventLoop* loop, int fd, unsigned int read_buffer_size, unsigned int write_buffer_size) 
        
        : channel_(std::make_unique<adachi::io::Channel>(loop, fd))
        , status_(kConnecting)
        , socket_(std::make_unique<Socket>(fd))
        , read_buffer_(read_buffer_size)
        , write_buffer_(write_buffer_size)
        , onmessage_([](std::shared_ptr<adachi::network::TcpConnection> conn_ptr, adachi::io::Buffer& buffer){
            while (buffer.Size() >= sizeof(unsigned int)) {
                unsigned int len = ntohl(buffer.PeekUnsignedInt()); /// 数据为大端类型需要转换
                // unsigned int len = buffer.PeekUnsignedInt();
                if (len > 100000) {
                    ADACHI_LOG_INFO << "[Info] buffer size exceeded 100000 * sizeof(char).TcpConnection will be closed soonly\n";
                    conn_ptr->Close();
                    break;
                }
                else {
                    if (buffer.Size() >= len + sizeof(unsigned int)) {
                        std::string header;
                        std::string message;
                        buffer.ReadBuffer(header, sizeof(unsigned int));
                        buffer.ReadBuffer(message, len);

                        ADACHI_LOG_INFO << "receieve message: " << message << "\n";

                        message += " OK!";
                        unsigned int v = htonl(message.size());
                        std::string msgback;
                        msgback.assign(reinterpret_cast<char*>(&v), sizeof(v));

                        conn_ptr->Write(msgback + message);
                    }
                    else break;
                }
            }
        })
    {
        channel_->SetReadCallback([ptr = this]() {
            ptr->Read();
        });

        channel_->SetWriteCallback([ptr = this]() {
            ptr->WriteFd();
        });

        channel_->SetErrorCallback([ptr = this]() {
            ptr->Close();
        });

        channel_->SetCloseCallback([ptr = this]() {
            ptr->Close();
        });
    }

    void TcpConnection::Activate() {
        if (channel_ == nullptr || channel_->owner_ == nullptr) {
            ADACHI_LOG_ERROR << "TcpConnection::Activate failed: channel owner is nullptr\n";
            return;
        }

        if (channel_->owner_->IsInThread()) {
            ActivateInThread();
        }
        else {
            auto weak_self = weak_from_this();
            channel_->owner_->Submit([weak_self]() {
                if (auto self = weak_self.lock()) {
                    self->ActivateInThread();
                }
            });
        }
    }
    /// 正常读操作完毕会默认调用onmessage进行解析
    void TcpConnection::Read() {
        if (status_ != kConnecting) return;
        int saveerrno;
        int n = read_buffer_.ReadFd(socket_->Fd(), saveerrno);

        if (n > 0) {

            /// 有shared指针管理，说明心跳机制正常运行，延长生命周期
            /// 没有shared指针管理，说明心跳机制没开启或者连接已经超时
            if (auto heartbeat = heartbeat_ptr_.lock()) {
                channel_->owner_->timerptr_->Insert(heartbeat);
            }

            if (auto self = weak_from_this().lock()) {
                onmessage_(self, read_buffer_);
            }
        }
        else if (n == 0) {
            // 这个地方后续可考虑修改为先处理完写缓冲区内容
            Close();
        } else {
            if (saveerrno == EAGAIN || saveerrno == EWOULDBLOCK || saveerrno == EINTR) {

            }
            else {
                ADACHI_LOG_ERROR << "[Error] TcpConnection Read failed: " << strerror(saveerrno) << "\n";
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
                    auto weak_self = weak_from_this();
                    channel_->owner_->Submit([weak_self, msg = std::move(message)]() mutable {
                        if (auto self = weak_self.lock()) {
                            self->WriteInThread(std::move(msg));
                        }
                    });
                }
            }
            else {
                ADACHI_LOG_ERROR << "TcpConnection::Write failed: channel->owner_ is nullptr" << "\n";
            }
        }
        else {
            ADACHI_LOG_ERROR << "TcpConnection::Write failed: channel is nullptr\n";
        }
    }
    void TcpConnection::WriteFd() {
        int saveerrno;
        if (status_ == kDisConnected) return;
        int n = write_buffer_.WriteFd(Fd(), saveerrno);
        
        if (n < 0) {
            if (saveerrno != EAGAIN && saveerrno != EWOULDBLOCK && saveerrno != EINTR) {
                ADACHI_LOG_ERROR << "TcpConnection::WriteFd failed: " << strerror(saveerrno) << "\n";
                Close();
            }
        }

        if (write_buffer_.Empty()) {
            if (channel_->Events() & adachi::io::Channel::kWrite) channel_->SetActive(channel_->Events() ^ adachi::io::Channel::kWrite);
            
            if (status_ == kDisConnecting) {
                status_ = kDisConnected;
                auto self = weak_from_this().lock();
                if (self && close_callback_) close_callback_(self); // 上层关闭（如果有提供）
                channel_->RemoveFromLoop(); // 关闭所在epoll
                socket_->Close();
            }
        }
    }
    void TcpConnection::Close() {
        if (close_requested_.exchange(true)) return;
        if (channel_) {
            if (channel_->owner_) {
                if (channel_->owner_->IsInThread()) {
                    CloseInThread();
                }
                else {
                    auto weak_self = weak_from_this();
                    channel_->owner_->Submit([weak_self]() {
                        if (auto self = weak_self.lock()) {
                            self->CloseInThread();
                        }
                    });
                }
            }
            else {
                ADACHI_LOG_ERROR << "TcpConnection::Write failed: channel->owner_ is nullptr\n";
            }
        }
        else {
            ADACHI_LOG_ERROR << "TcpConnection::Write failed: channel is nullptr\n";
        }
    }

    void TcpConnection::SaveLifeMechanism() {
        if (auto self = weak_from_this().lock()) {
            channel_->Tie(self);

            if (channel_->owner_->timerptr_) {
                std::shared_ptr<adachi::tool::HeartBeatObj> ptr = std::make_shared<adachi::tool::HeartBeatObj>(weak_from_this());
                heartbeat_ptr_ = ptr;
                channel_->owner_->timerptr_->Insert(ptr);
            }
        }
    }

    TcpConnection::~TcpConnection() {
        channel_->RemoveFromLoop();
    }

    /// 这个部分需要修改，确保io在本线程，任务在别处 
    void TcpConnection::SetOnMessage(const std::function<void(const std::shared_ptr<TcpConnection>, adachi::io::Buffer&)>& cb) {
        onmessage_ = cb;
    }

    void TcpConnection::SetCloseCallback(const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
        close_callback_ = cb;
    }

    int TcpConnection::Fd() const {
        return socket_->Fd();
    }

    bool TcpConnection::IsWriteBufferEmpty() {
        return write_buffer_.Empty();
    }

    void TcpConnection::ActivateInThread() {
        if (status_ != kConnecting) {
            return;
        }
        channel_->SetActive(adachi::io::Channel::kRead | adachi::io::Channel::kClose);
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
                    ADACHI_LOG_ERROR << "TcpConnection::write failed: " << strerror(saveerrno) << "\n";
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
            auto self = weak_from_this().lock();
            channel_->RemoveFromLoop(); // 关闭所在epoll
            socket_->Close();
            if (self && close_callback_) close_callback_(self); // 上层关闭（如果有提供）
        }
        else {
            status_ = kDisConnecting;
            channel_->SetActive(channel_->Events() | adachi::io::Channel::kWrite);
        }
    }
}
