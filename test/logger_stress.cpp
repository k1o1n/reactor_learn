#include "logger.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

struct Options {
    int thread_count = 8;
    int duration_seconds = 30;
    int payload_bytes = 128;
};

bool ParseInt(const char* text, int& value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

void PrintUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " [thread_count] [duration_seconds] [payload_bytes]" << std::endl;
}

std::string BuildPayload(int thread_id, int payload_bytes) {
    std::string prefix = " thread=" + std::to_string(thread_id) + " payload=";
    if (payload_bytes <= static_cast<int>(prefix.size())) {
        return prefix;
    }

    std::string payload = prefix;
    payload.append(static_cast<size_t>(payload_bytes - static_cast<int>(prefix.size())), 'x');
    return payload;
}

void WriteBurstLogs(int thread_id,
                    int duration_seconds,
                    const std::string& payload,
                    std::atomic<bool>& start_flag,
                    std::atomic<int>& ready_threads,
                    std::atomic<bool>& stop_flag,
                    std::atomic<unsigned long long>& produced_logs) {
    ready_threads.fetch_add(1, std::memory_order_relaxed);
    while (!start_flag.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(duration_seconds);
    unsigned long long index = 0;
    while (!stop_flag.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline) {
        const auto checksum = static_cast<unsigned long long>(thread_id) * 131 + index;
        switch (index % 3) {
            case 0:
                ADACHI_LOG_INFO << " log_index=" << index << payload << " checksum=" << checksum;
                break;
            case 1:
                ADACHI_LOG_WARNING << " log_index=" << index << payload << " checksum=" << checksum;
                break;
            default:
                ADACHI_LOG_ERROR << " log_index=" << index << payload << " checksum=" << checksum;
                break;
        }
        produced_logs.fetch_add(1, std::memory_order_relaxed);
        ++index;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    Options options;
    if (argc > 4) {
        PrintUsage(argv[0]);
        return 1;
    }
    if (argc >= 2 && !ParseInt(argv[1], options.thread_count)) {
        PrintUsage(argv[0]);
        return 1;
    }
    if (argc >= 3 && !ParseInt(argv[2], options.duration_seconds)) {
        PrintUsage(argv[0]);
        return 1;
    }
    if (argc >= 4 && !ParseInt(argv[3], options.payload_bytes)) {
        PrintUsage(argv[0]);
        return 1;
    }

    if (options.thread_count <= 0 || options.duration_seconds <= 0 || options.payload_bytes <= 0) {
        std::cerr << "invalid arguments" << std::endl;
        return 1;
    }

    const std::string runtime_log_path = "pid" + std::to_string(getpid()) + ".log";

    std::cout << "[INFO] thread_count=" << options.thread_count
              << " duration_seconds=" << options.duration_seconds
              << " payload_bytes=" << options.payload_bytes
              << " log_path=" << runtime_log_path
              << " macros=ADACHI_LOG_INFO|ADACHI_LOG_WARNING|ADACHI_LOG_ERROR"
              << std::endl;

    std::filesystem::remove(runtime_log_path);

    std::atomic<bool> start_flag{false};
    std::atomic<bool> stop_flag{false};
    std::atomic<int> ready_threads{0};
    std::atomic<unsigned long long> produced_logs{0};

    auto total_begin = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(options.thread_count));

    for (int thread_id = 0; thread_id < options.thread_count; ++thread_id) {
        workers.emplace_back(WriteBurstLogs,
                             thread_id,
                             options.duration_seconds,
                             BuildPayload(thread_id, options.payload_bytes),
                             std::ref(start_flag),
                             std::ref(ready_threads),
                             std::ref(stop_flag),
                             std::ref(produced_logs));
    }

    while (ready_threads.load(std::memory_order_acquire) < options.thread_count) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto enqueue_begin = std::chrono::steady_clock::now();
    start_flag.store(true, std::memory_order_release);

    for (std::thread& worker : workers) {
        worker.join();
    }

    stop_flag.store(true, std::memory_order_relaxed);

    auto enqueue_end = std::chrono::steady_clock::now();
    auto enqueue_ms = std::chrono::duration_cast<std::chrono::milliseconds>(enqueue_end - enqueue_begin).count();
    if (enqueue_ms == 0) {
        enqueue_ms = 1;
    }

    double enqueue_seconds = static_cast<double>(enqueue_ms) / 1000.0;
    const unsigned long long total_logs = produced_logs.load(std::memory_order_relaxed);
    std::cout << "[enqueue] produced_logs=" << total_logs
              << " duration_ms=" << enqueue_ms
              << " logs_per_sec=" << static_cast<unsigned long long>(total_logs / enqueue_seconds)
              << std::endl;

    auto total_end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_begin).count();
    if (total_ms == 0) {
        total_ms = 1;
    }

    std::uintmax_t file_size = 0;
    if (std::filesystem::exists(runtime_log_path)) {
        file_size = std::filesystem::file_size(runtime_log_path);
    }

    double total_seconds = static_cast<double>(total_ms) / 1000.0;
    double mb_written = static_cast<double>(file_size) / (1024.0 * 1024.0);

    std::cout << "[flush] file_size_bytes=" << file_size
              << " duration_ms=" << total_ms
              << " logs_per_sec=" << static_cast<unsigned long long>(total_logs / total_seconds)
              << " mb_per_sec=" << (mb_written / total_seconds)
              << std::endl;
    std::cout << "[PASS] logger stress completed" << std::endl;
    return 0;
}