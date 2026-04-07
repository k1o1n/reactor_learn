#ifndef KVSERVER_H
#define KVSERVER_H
#include <memory>
#include "tcpserver.h"
#include "noncopyable.h"
#include "kvservice.h"

namespace adachi::network::kv {
    /// 提供的kvservice服务必须是可重入的
    class KVServer : adachi::tool::NonCopyAble {
    public:
        KVServer(const INetAddress& listenaddr
        , KVService* ptr
        , unsigned int subthreadnum = 10
        , const adachi::io::TimerOpt& timeropt = {0, {0, 0}});
        ~KVServer();
    private:
        std::unique_ptr<adachi::network::TcpServer> tcpserver_;
        /// 提供的kvservice服务必须是可重入的
        KVService* kvservice_;
    };
}

#endif