#include "timer/heartbeatobj.h"
#include "tcpconnection.h"
#include "logger.h"

namespace adachi::tool {
    HeartBeatObj::HeartBeatObj(std::weak_ptr<adachi::network::TcpConnection> tcp_weakptr) 
        : last_insert_pos_(-1)
        , tcp_weakptr_(tcp_weakptr)
    {
    }
    HeartBeatObj::~HeartBeatObj() {
        if (auto ptr = tcp_weakptr_.lock()) {
            ADACHI_LOG_INFO << ptr->addr_.Ip() << " will be closed because of heartbeat mechanism\n";
            ptr->Close();
        } 
    }
}