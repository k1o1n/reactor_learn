#include "kv/kvserver.h"
#include "buffer.h"
#include "tcpconnection.h"
#include "kv/packet.h"
#include "kv/kvservice.h"
#include "kv/protocolcodec.h"
#include <memory>

namespace adachi::network::kv {
    KVServer::KVServer(const INetAddress& listenaddr
    , KVService* ptr
    , unsigned int subthreadnum
    , const adachi::io::TimerOpt& timeropt)
    : tcpserver_(std::make_unique<adachi::network::TcpServer>(listenaddr, timeropt))
    , kvservice_(ptr)

    {
        tcpserver_->SetSubThreadNum(subthreadnum);
        tcpserver_->SetNewconnectionCallback([this](std::shared_ptr<adachi::network::TcpConnection> newconn_ptr) {
            newconn_ptr->SetOnMessage([this](const std::shared_ptr<TcpConnection> conn_ptr, adachi::io::Buffer& buffer) {
                while (buffer.Size() >= sizeof(adachi::network::kv::protocol::PacketHead)) {
                    /// 协议头不符合规范，需要关闭
                    if (this->kvservice_->HeadCheck(buffer)) {
                        conn_ptr->Close();
                        return;
                    }

                    /// 还未收到一条完整命令
                    if (!this->kvservice_->ShouldGet(buffer)) {
                        return;
                    }

                    adachi::network::kv::protocol::Packet result;
                    /// 检查命令是否解析成功，并根据命令判断是否要close
                    if (!this->kvservice_->BodyCheck(buffer, &result)) {
                        conn_ptr->Close();
                        return;
                    }

                    adachi::network::kv::protocol::Packet echo;
                    /// 检查是否需要回复
                    if (this->kvservice_->ShouldEcho(buffer, &result, &echo)) {
                        std::string msg;
                        if (!adachi::network::kv::protocol::ProtocolCodec::EncodePacketToFlow(&echo, msg)) {
                            conn_ptr->Close();
                            return;
                        }
                        conn_ptr->Write(std::move(msg));
                    }
                }
            });
        });

        tcpserver_->Start();
    }

    KVServer::~KVServer() {
        
    }
}