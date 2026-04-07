#include "kv/protocolcodec.h"
#include <arpa/inet.h>

namespace adachi::network::kv::protocol {

    namespace {
        void AppendBytes(std::string& dst, const void* src, size_t size) {
            if (size == 0) return;
            const size_t old_size = dst.size();
            dst.resize(old_size + size);
            memmove(dst.data() + old_size, src, size);
        }

        template <typename T>
        void AppendHostInteger(std::string& dst, T value) {
            ProtocolCodec::HToN(value);
            AppendBytes(dst, &value, sizeof(value));
        }

        template <typename T>
        bool ReadBodyInteger(std::string_view body, uint64_t& offset, T* value) {
            return ProtocolCodec::WriteFlowToHost(body.data(), body.size(), offset, value);
        }

        bool ReadBodyString(std::string_view body, uint64_t& offset, uint32_t len, std::string* value) {
            if (value == nullptr) return false;
            if (offset + len > body.size()) return false;
            value->assign(body.data() + offset, len);
            offset += len;
            return true;
        }
    }

    static_assert(sizeof(PacketHead) == 24, "sizeof(PacketHead) error");

    bool ProtocolCodec::IsBigEnd() {
        uint16_t value = 0x1234;
        return *reinterpret_cast<uint8_t*>(&value) == 0x12;
    }

    void ProtocolCodec::NToH(uint64_t& value) {
        if (!IsBigEnd()) {
            value = __builtin_bswap64(value);
        }
    }

    void ProtocolCodec::NToH(uint32_t& value) { value = ntohl(value); }
    void ProtocolCodec::NToH(uint16_t& value) { value = ntohs(value); }
    void ProtocolCodec::NToH(uint8_t& value) { (void)value; }

    void ProtocolCodec::HToN(uint64_t& value) {
        if (!IsBigEnd()) {
            value = __builtin_bswap64(value);
        }
    }

    void ProtocolCodec::HToN(uint32_t& value) { value = htonl(value); }
    void ProtocolCodec::HToN(uint16_t& value) { value = htons(value); }
    void ProtocolCodec::HToN(uint8_t& value) { (void)value; }

    bool ProtocolCodec::DecodeFlowToPacket(std::string_view src, Packet* store) {
        return DecodeFlowToPacket(src.data(), src.size(), store);
    }

    bool ProtocolCodec::DecodeFlowToPacket(const char* src, uint64_t len, Packet* store) {
        if (store == nullptr || src == nullptr) return false;
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
        if (src == nullptr || store == nullptr) return false;
        if (len < sizeof(PacketHead) + src->body_.size()) return false;

        uint64_t offset = 0;
        if (!WriteHostToFlow(src->head_.magic_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.version_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.msg_type_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.opcode_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.status_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.body_length_, offset, store, len)) return false;
        if (!WriteHostToFlow(src->head_.request_id_, offset, store, len)) return false;

        memmove(store + offset, src->body_.data(), src->body_.size());
        return true;
    }

    bool ProtocolCodec::EncodePacketToFlow(const Packet* src, std::string& store) {
        if (src == nullptr) return false;
        store.resize(sizeof(PacketHead) + src->body_.size());
        return EncodePacketToFlow(src, store.data(), store.size());
    }

    bool ProtocolCodec::EncodeKVRequestToPacket(const KVRequest* src, Packet* store) {
        if (src == nullptr || store == nullptr) return false;

        store->head_.magic_ = kDefaultMagic;
        store->head_.version_ = kDefaultVersion;
        store->head_.msg_type_ = kRequest;
        store->head_.opcode_ = src->opcode_;
        store->head_.status_ = kOk;
        store->head_.request_id_ = 0;
        store->body_.clear();

        switch (src->opcode_) {
            case kPing:
                break;
            case kPut: {
                const uint32_t key_length = static_cast<uint32_t>(src->key_.size());
                const uint32_t value_length = static_cast<uint32_t>(src->value_.size());
                if (key_length == 0 || key_length > kMaxKeyLength || value_length > kMaxValueLength) return false;
                AppendHostInteger(store->body_, key_length);
                AppendHostInteger(store->body_, value_length);
                AppendBytes(store->body_, src->key_.data(), src->key_.size());
                AppendBytes(store->body_, src->value_.data(), src->value_.size());
                break;
            }
            case kGet:
            case kDel: {
                const uint32_t key_length = static_cast<uint32_t>(src->key_.size());
                if (key_length == 0 || key_length > kMaxKeyLength) return false;
                AppendHostInteger(store->body_, key_length);
                AppendBytes(store->body_, src->key_.data(), src->key_.size());
                break;
            }
            default:
                return false;
        }

        if (store->body_.size() > kMaxBodyLength) return false;
        store->head_.body_length_ = static_cast<uint32_t>(store->body_.size());
        return true;
    }

    bool ProtocolCodec::DecodePacketToKVRequest(const Packet* src, KVRequest* store) {
        if (src == nullptr || store == nullptr) return false;
        if (src->head_.msg_type_ != kRequest) return false;
        if (src->head_.body_length_ != src->body_.size()) return false;
        if (src->head_.body_length_ > kMaxBodyLength) return false;

        store->opcode_ = src->head_.opcode_;
        store->key_.clear();
        store->value_.clear();
        store->extrainfo_.clear();

        std::string_view body(src->body_);
        uint64_t offset = 0;
        switch (src->head_.opcode_) {
            case kPing:
                return body.empty();
            case kPut: {
                uint32_t key_length = 0;
                uint32_t value_length = 0;
                if (!ReadBodyInteger(body, offset, &key_length)) return false;
                if (!ReadBodyInteger(body, offset, &value_length)) return false;
                if (key_length == 0 || key_length > kMaxKeyLength || value_length > kMaxValueLength) return false;
                if (!ReadBodyString(body, offset, key_length, &store->key_)) return false;
                if (!ReadBodyString(body, offset, value_length, &store->value_)) return false;
                return offset == body.size();
            }
            case kGet:
            case kDel: {
                uint32_t key_length = 0;
                if (!ReadBodyInteger(body, offset, &key_length)) return false;
                if (key_length == 0 || key_length > kMaxKeyLength) return false;
                if (!ReadBodyString(body, offset, key_length, &store->key_)) return false;
                return offset == body.size();
            }
            default:
                return false;
        }
    }

    bool ProtocolCodec::DecodePacketToKVResponse(const Packet* src, KVResponse* store) {
        if (src == nullptr || store == nullptr) return false;

        store->status_ = src->head_.status_;
        store->key_.clear();
        store->value_.clear();
        store->extrainfo_.clear();

        switch (src->head_.opcode_) {
            case kPing:
            case kPut:
            case kDel:
                return src->body_.empty();
            case kGet: {
                if (src->head_.status_ != kOk) return src->body_.empty();
                std::string_view body(src->body_);
                uint64_t offset = 0;
                uint32_t value_length = 0;
                if (!ReadBodyInteger(body, offset, &value_length)) return false;
                if (value_length > kMaxValueLength) return false;
                if (!ReadBodyString(body, offset, value_length, &store->value_)) return false;
                return offset == body.size();
            }
            default:
                return src->body_.empty();
        }
    }

    bool ProtocolCodec::EncodeKVResponseToPacket(const KVResponse* src, uint16_t opcode, uint64_t request_id, Packet* store) {
        if (src == nullptr || store == nullptr) return false;

        store->head_.magic_ = kDefaultMagic;
        store->head_.version_ = kDefaultVersion;
        store->head_.msg_type_ = kResponse;
        store->head_.opcode_ = opcode;
        store->head_.status_ = src->status_;
        store->head_.request_id_ = request_id;
        store->body_.clear();

        if (opcode == kGet && src->status_ == kOk) {
            const uint32_t value_length = static_cast<uint32_t>(src->value_.size());
            if (value_length > kMaxValueLength) return false;
            AppendHostInteger(store->body_, value_length);
            AppendBytes(store->body_, src->value_.data(), src->value_.size());
        }

        if (store->body_.size() > kMaxBodyLength) return false;
        store->head_.body_length_ = static_cast<uint32_t>(store->body_.size());
        return true;
    }
}