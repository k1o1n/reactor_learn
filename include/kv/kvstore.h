#ifndef KVSTORE_H
#define KVSTORE_H
#include <unordered_map>
#include <string>
#include <mutex>
#include "kv/kvrequest.h"
#include "kv/kvresponse.h"

namespace adachi::network::kv {
    class KVStore {
    public:
        KVStore();

        void Get(adachi::network::kv::protocol::KVRequest*, adachi::network::kv::protocol::KVResponse*);
        void Put(adachi::network::kv::protocol::KVRequest*, adachi::network::kv::protocol::KVResponse*);
        void Del(adachi::network::kv::protocol::KVRequest*, adachi::network::kv::protocol::KVResponse*);
    private:
        std::unordered_map<std::string, std::string> mp;
        std::mutex mtx_;
    };
}

#endif