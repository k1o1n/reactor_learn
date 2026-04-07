#include "kv/kvservice.h"
#include "buffer.h"
#include "kv/protocolcodec.h"

namespace {
    using adachi::network::kv::protocol::KVRequest;
    using adachi::network::kv::protocol::KVResponse;
    using adachi::network::kv::protocol::Packet;
    using adachi::network::kv::protocol::PacketHead;
    using adachi::network::kv::protocol::ProtocolCodec;

    bool IsSupportedOpcode(uint16_t opcode) {
        return opcode == adachi::network::kv::protocol::kPing
            || opcode == adachi::network::kv::protocol::kPut
            || opcode == adachi::network::kv::protocol::kGet
            || opcode == adachi::network::kv::protocol::kDel;
    }
}

namespace adachi::network::kv {
    const std::function<bool(adachi::io::Buffer&)> KVService::kDefaultHeadCheck = [](adachi::io::Buffer& buffer) -> bool {
        if (buffer.Size() < sizeof(PacketHead)) return false;

        Packet packet;
        if (!ProtocolCodec::DecodeFlowToPacket(buffer.CReadData(), sizeof(PacketHead), &packet)) return true;

        return packet.head_.magic_ != protocol::kDefaultMagic
            || packet.head_.version_ != protocol::kDefaultVersion
            || packet.head_.msg_type_ != protocol::kRequest
            || packet.head_.body_length_ > protocol::kMaxBodyLength;
    };

    const std::function<bool(adachi::io::Buffer&)> KVService::kDefaultShouldGet = [](adachi::io::Buffer& buffer) -> bool {
        if (buffer.Size() < sizeof(PacketHead)) return false;

        Packet packet;
        if (!ProtocolCodec::DecodeFlowToPacket(buffer.CReadData(), sizeof(PacketHead), &packet)) return false;
        return buffer.Size() >= sizeof(PacketHead) + packet.head_.body_length_;
    };

    const std::function<bool(adachi::io::Buffer&, void*)> KVService::kDefaultBodyCheck = [](adachi::io::Buffer& buffer, void* result) -> bool {
        if (result == nullptr) return false;
        if (buffer.Size() < sizeof(PacketHead)) return false;

        Packet packet_head;
        if (!ProtocolCodec::DecodeFlowToPacket(buffer.CReadData(), sizeof(PacketHead), &packet_head)) return false;

        const size_t total_length = sizeof(PacketHead) + packet_head.head_.body_length_;
        if (buffer.Size() < total_length) return false;

        std::string raw;
        buffer.ReadBuffer(raw, total_length);

        auto* packet = static_cast<Packet*>(result);
        if (!ProtocolCodec::DecodeFlowToPacket(raw, packet)) return false;

        if (!IsSupportedOpcode(packet->head_.opcode_)) {
            packet->head_.status_ = protocol::kUnsupportedOpcode;
            return true;
        }

        KVRequest request;
        if (!ProtocolCodec::DecodePacketToKVRequest(packet, &request)) {
            packet->head_.status_ = protocol::kBadRequest;
        }
        return true;
    };

    const std::function<bool(adachi::io::Buffer&, void*, void*)> KVService::kDefaultShouldEcho = [](adachi::io::Buffer& buffer, void* result, void* echo) -> bool {
        (void)buffer;
        return result != nullptr && echo != nullptr;
    };

    KVService::KVService(std::function<bool(adachi::io::Buffer&)> headcheck
    , std::function<bool(adachi::io::Buffer&)> shouldget
    , std::function<bool(adachi::io::Buffer&, void* result)> bodycheck
    , std::function<bool(adachi::io::Buffer&, void* result, void* echo)> shouldecho)
    : headcheck_(headcheck)
    , shouldget_(shouldget)
    , bodycheck_(bodycheck)
    , shouldecho_(shouldecho) {}

    bool KVService::HeadCheck(adachi::io::Buffer& buffer) { return headcheck_(buffer); }
    bool KVService::ShouldGet(adachi::io::Buffer& buffer) { return shouldget_(buffer); }
    bool KVService::BodyCheck(adachi::io::Buffer& buffer, void* result) { return bodycheck_(buffer, result); }

    bool KVService::ShouldEcho(adachi::io::Buffer& buffer, void* result, void* echo) {
        if (!shouldecho_(buffer, result, echo)) return false;

        auto* request_packet = static_cast<Packet*>(result);
        auto* response_packet = static_cast<Packet*>(echo);
        if (request_packet == nullptr || response_packet == nullptr) return false;

        KVResponse response;
        response.status_ = request_packet->head_.status_;
        response.key_.clear();
        response.value_.clear();
        response.extrainfo_.clear();

        if (request_packet->head_.status_ == protocol::kOk) {
            KVRequest request;
            if (!ProtocolCodec::DecodePacketToKVRequest(request_packet, &request)) {
                response.status_ = protocol::kBadRequest;
            }
            else {
                switch (request.opcode_) {
                    case protocol::kPing:
                        response.status_ = protocol::kOk;
                        break;
                    case protocol::kPut:
                        kvstore.Put(&request, &response);
                        break;
                    case protocol::kGet:
                        kvstore.Get(&request, &response);
                        break;
                    case protocol::kDel:
                        kvstore.Del(&request, &response);
                        break;
                    default:
                        response.status_ = protocol::kUnsupportedOpcode;
                        break;
                }
            }
        }

        if (!ProtocolCodec::EncodeKVResponseToPacket(&response, request_packet->head_.opcode_, request_packet->head_.request_id_, response_packet)) {
            KVResponse fallback;
            fallback.status_ = protocol::kInternalError;
            fallback.key_.clear();
            fallback.value_.clear();
            fallback.extrainfo_.clear();
            return ProtocolCodec::EncodeKVResponseToPacket(&fallback, request_packet->head_.opcode_, request_packet->head_.request_id_, response_packet);
        }
        return true;
    }
}