#ifndef KVSERVICE_H
#define KVSERVICE_H
#include <memory>
#include "buffer.h"
#include "packet.h"
#include <functional>
#include "kv/kvstore.h"

namespace adachi::network::kv {
    class KVService {
    public:
        static const std::function<bool(adachi::io::Buffer&)> kDefaultHeadCheck;
        static const std::function<bool(adachi::io::Buffer&)> kDefaultShouldGet;
        static const std::function<bool(adachi::io::Buffer&, void*)> kDefaultBodyCheck;
        static const std::function<bool(adachi::io::Buffer&, void*, void*)> kDefaultShouldEcho;
        
        KVService(std::function<bool(adachi::io::Buffer&)> headcheck = kDefaultHeadCheck
        , std::function<bool(adachi::io::Buffer&)> shouldget = kDefaultShouldGet
        , std::function<bool(adachi::io::Buffer&, void* result)> bodycheck = kDefaultBodyCheck
        , std::function<bool(adachi::io::Buffer&, void* result, void* echo)> shouldecho = kDefaultShouldEcho);
        
        /// 假设当前buffer的大小已经超过了协议头的大小（即可以容纳一个协议头）
        /// 对协议头内容进行检查，查看是否合法
        /// 不合法将强制关闭连接
        bool HeadCheck(adachi::io::Buffer&);
        bool ShouldGet(adachi::io::Buffer&);
        bool BodyCheck(adachi::io::Buffer&, void* result);
        bool ShouldEcho(adachi::io::Buffer&, void* result, void* echo);
    private:
        std::function<bool(adachi::io::Buffer&)> headcheck_;
        std::function<bool(adachi::io::Buffer&)> shouldget_;
        std::function<bool(adachi::io::Buffer&, void* result)> bodycheck_;
        std::function<bool(adachi::io::Buffer&, void* result, void* echo)> shouldecho_;

        KVStore kvstore;
    };
}

#endif