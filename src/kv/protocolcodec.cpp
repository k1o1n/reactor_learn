#include "protocolcodec.h"
#include <arpa/inet.h>

namespace adachi::network::kv::protocol {

    namespace {
        void AppendBytes(std::string& dst, const void* src, size_t size) {
            if (size == 0) return;
            const size_t old_size = dst.size();
            dst.resize(old_size + size);
            memmove(dst.data() + old_size, src, size);
        }
    }

    static_assert(sizeof(PacketHead) == 24, "sizeof(PacketHead) error");

    bool ProtocolCodec::IsBigEnd() {
        uint16_t _ = 0x1234;
        return *reinterpret_cast<uint8_t*>(&_) == 0x34; 
    }
    void ProtocolCodec::NToH(uint64_t& _) {
        if (!IsBigEnd()) {
            
        }
    }
    void ProtocolCodec::NToH(uint32_t& _) { _ = ntohl(_); return;}
    void ProtocolCodec::NToH(uint16_t& _) { _ = ntohs(_); return;}
    void ProtocolCodec::NToH(uint8_t& _) { return;}
    void ProtocolCodec::HToN(uint64_t& _) {
        if (!IsBigEnd()) {

        }
    }
    void ProtocolCodec::HToN(uint32_t& _) { _ = htonl(_); return;}
    void ProtocolCodec::HToN(uint16_t& _) { _ = htons(_); return;}
    void ProtocolCodec::HToN(uint8_t& _) {return;}

    bool ProtocolCodec::DecodeFlowToPacket(std::string_view src, Packet* store) {
        return DecodeFlowToPacket(src.data(), src.size(), store);
    }
    bool ProtocolCodec::DecodeFlowToPacket(const char* src, uint64_t len, Packet* store) {
        if (store == nullptr) return false;
        if (src == nullptr) return false;
        if (len < sizeof(PacketHead)) return false;
        uint64_t offset = 0;
        if (!WriteFlowToHost(src, len, offset, &store->head_.magic_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.version_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.msg_type_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.opcode_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.status_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.body_length_)) return false;
        if (!WriteFlowToHost(src, len, offset, &store->head_.request_id_)) return false;
        if (store->head_.body_length_ + sizeof(PacketHead) > len) return false;
        store->body_.assign(src + offset, store->head_.body_length_);
        return true;
    }
    bool ProtocolCodec::EncodePacketToFlow(const Packet* src, char* store, uint64_t len) {
        if (store == nullptr) return false;
        if (src == nullptr) return false;
        if (len < sizeof(PacketHead) + src->body_.size()) return false;
        uint64_t offset = 0;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        if (!WriteHostToFlow(src, offset, store, len)) return false;
        memmove(store + offset, src->body_.data(), src->body_.size());
        return true;
    }
    bool ProtocolCodec::EncodePacketToFlow(const Packet* src, std::string& store) {
        return EncodePacketToFlow(src, store.data(), store.size());
    }
    bool ProtocolCodec::EncodeKVRequestToPacket(const KVRequest* src, Packet* store) {
        if (src == nullptr) return false;
        if (store == nullptr) return false;
        if (src->opcode_ == kPing) {
            store->head_.opcode_ = src->opcode_;
            store->head_.magic_ = kDefaultMagic;
            store->head_.version_ = kDefaultVersion;
            store->head_.msg_type_ = kRequest;
            store->head_.status_ = kOk;
            store->head_.body_length_ = 0;
            store->body_.clear();
            return true;
        }
        if (src->opcode_ == kPut) {
            store->head_.opcode_ = src->opcode_;
            store->head_.magic_ = kDefaultMagic;
            store->head_.version_ = kDefaultVersion;
            store->head_.msg_type_ = kRequest;
            store->head_.status_ = kOk;
            store->body_.clear();
            uint32_t key_length = static_cast<uint32_t>(src->key_.size());
            uint32_t value_length = static_cast<uint32_t>(src->value_.size());

            store->body_.reserve(sizeof(key_length) + sizeof(value_length) + src->key_.size() + src->value_.size());
            AppendBytes(store->body_, &key_length, sizeof(key_length));
            AppendBytes(store->body_, &value_length, sizeof(value_length));
            AppendBytes(store->body_, src->key_.data(), src->key_.size());
            AppendBytes(store->body_, src->value_.data(), src->value_.size());
            store->head_.body_length_ = store->body_.size();
            return true;
        }
        if (src->opcode_ == kGet) {
            store->head_.opcode_ = src->opcode_;
            store->head_.magic_ = kDefaultMagic;
            store->head_.version_ = kDefaultVersion;
            store->head_.msg_type_ = kRequest;
            store->head_.status_ = kOk;
            store->body_.clear();
            uint32_t key_length = static_cast<uint32_t>(src->key_.size());
            
            AppendBytes(store->body_, &key_length, sizeof(key_length));
            AppendBytes(store->body_, src->key_.data(), src->key_.size());
            store->head_.body_length_ = store->body_.size();
            return true;
        }
        if (src->opcode_ == kDel) {
            store->head_.opcode_ = src->opcode_;
            store->head_.magic_ = kDefaultMagic;
            store->head_.version_ = kDefaultVersion;
            store->head_.msg_type_ = kRequest;
            store->head_.status_ = kOk;
            store->body_.clear();
            uint32_t key_length = static_cast<uint32_t>(src->key_.size());
            
            AppendBytes(store->body_, &key_length, sizeof(key_length));
            AppendBytes(store->body_, src->key_.data(), src->key_.size());
            store->head_.body_length_ = store->body_.size();
            return true;
        }
        return false;
    } 
    bool ProtocolCodec::DecodePacketToKVResponse(const Packet* src, KVResponse* store) {
        if (src == nullptr) return false;
        if (store == nullptr) return false;
        store->status_ = src->head_.status_;
        store->extrainfo_.clear();
        if (src->head_.opcode_ == kPing) {
            return true;
        }
        if (src->head_.opcode_ == kPut) {
            return true;
        }
        if (src->head_.opcode_ == kGet) {
            return true;
        }
        if (src->head_.opcode_ == kDel) {
            return true;
        }
        return true;
    }
}