#include "kv/kvservice.h"
#include "buffer.h"

namespace adachi::network::kv {
    const std::function<bool(adachi::io::Buffer&)> KVService::kDefaultHeadCheck = [](adachi::io::Buffer& buffer) -> bool {
        
    };
    const std::function<bool(adachi::io::Buffer&)> KVService::kDefaultShouldGet = [](adachi::io::Buffer& buffer) -> bool {
        return true;
    };
    const std::function<bool(adachi::io::Buffer&, void*)> KVService::kDefaultBodyCheck = [](adachi::io::Buffer& buffer, void* result) -> bool {
        return true;
    };
    const std::function<bool(adachi::io::Buffer&, void*, void*)> KVService::kDefaultShouldEcho = [](adachi::io::Buffer& buffer, void* result, void* echo) -> bool {
        return true;
    };

    KVService::KVService(std::function<bool(adachi::io::Buffer&)> headcheck
    , std::function<bool(adachi::io::Buffer&)> shouldget
    , std::function<bool(adachi::io::Buffer&, void* result)> bodycheck
    , std::function<bool(adachi::io::Buffer&, void* result, void* echo)> shouldecho)
    : headcheck_(headcheck)
    , shouldget_(shouldget)
    , bodycheck_(bodycheck)
    , shouldecho_(shouldecho) {}
    
    bool KVService::HeadCheck(adachi::io::Buffer& buffer) { return headcheck_(buffer);}
    bool KVService::ShouldGet(adachi::io::Buffer& buffer) { return shouldget_(buffer);}
    bool KVService::BodyCheck(adachi::io::Buffer& buffer, void* result) { return bodycheck_(buffer, result);}
    bool KVService::ShouldEcho(adachi::io::Buffer& buffer, void* result, void* echo) { return shouldecho_(buffer, result, echo);}
}