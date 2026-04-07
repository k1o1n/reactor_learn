#include "kv/kvstore.h"
#include "kv/protocolcodec.h"

namespace adachi::network::kv {
	KVStore::KVStore() = default;

	void KVStore::Get(adachi::network::kv::protocol::KVRequest* request, adachi::network::kv::protocol::KVResponse* response) {
		if (request == nullptr || response == nullptr) return;

		std::lock_guard<std::mutex> lock(mtx_);
		response->key_ = request->key_;
		response->extrainfo_.clear();
		const auto iter = mp.find(request->key_);
		if (iter == mp.end()) {
			response->status_ = adachi::network::kv::protocol::kNotFound;
			response->value_.clear();
			return;
		}

		response->status_ = adachi::network::kv::protocol::kOk;
		response->value_ = iter->second;
	}

	void KVStore::Put(adachi::network::kv::protocol::KVRequest* request, adachi::network::kv::protocol::KVResponse* response) {
		if (request == nullptr || response == nullptr) return;

		std::lock_guard<std::mutex> lock(mtx_);
		mp[request->key_] = request->value_;
		response->status_ = adachi::network::kv::protocol::kOk;
		response->key_ = request->key_;
		response->value_.clear();
		response->extrainfo_.clear();
	}

	void KVStore::Del(adachi::network::kv::protocol::KVRequest* request, adachi::network::kv::protocol::KVResponse* response) {
		if (request == nullptr || response == nullptr) return;

		std::lock_guard<std::mutex> lock(mtx_);
		response->key_ = request->key_;
		response->value_.clear();
		response->extrainfo_.clear();
		response->status_ = mp.erase(request->key_) == 0
			? adachi::network::kv::protocol::kNotFound
			: adachi::network::kv::protocol::kOk;
	}
}
