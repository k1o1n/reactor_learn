#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

enum class ClientRole {
    kActive,
    kSilent,
};

enum class CloseReason {
    kNone,
    kConnectError,
    kPeerClosed,
    kIoError,
    kStoppedByTester,
};

struct Options {
    std::string ip = "127.0.0.1";
    int port = 12345;
    int total_clients = 10000;
    int active_clients = 2000;
    int send_interval_ms = 3000;
    int active_phase_ms = 25000;
    int idle_min_ms = 8000;
    int idle_max_ms = 15000;
    int max_test_ms = 50000;
};

struct ClientState {
    int index = -1;
    int fd = -1;
    ClientRole role = ClientRole::kSilent;
    CloseReason close_reason = CloseReason::kNone;
    bool connecting = true;
    bool connected = false;
    bool closed = false;
    bool closed_during_active_phase = false;
    bool reply_seen = false;
    size_t write_offset = 0;
    std::string write_buffer;
    std::string read_buffer;
    int sent_messages = 0;
    int recv_messages = 0;
    int64_t connected_ms = -1;
    int64_t first_sent_ms = -1;
    int64_t last_sent_ms = -1;
    int64_t close_ms = -1;
    int64_t next_send_ms = -1;
};

struct Summary {
    int connect_failed = 0;
    int established = 0;
    int remaining_open = 0;
    int active_replied = 0;
    int active_closed_during_phase = 0;
    int active_closed_after_stop = 0;
    int active_closed_after_stop_in_range = 0;
    int active_never_sent = 0;
    int silent_closed = 0;
    int silent_closed_in_range = 0;
};

struct ProgressSnapshot {
    int established = 0;
    int active_open = 0;
    int silent_open = 0;
    int active_replied = 0;
    int active_closed_during_phase = 0;
    int active_closed_after_stop = 0;
    int silent_closed = 0;
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

int RequiredCount(int total, int percent) {
    if (total <= 0) {
        return 0;
    }
    int required = (total * percent + 99) / 100;
    return required > 0 ? required : 1;
}

bool SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

int64_t NowMs(const std::chrono::steady_clock::time_point& start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start)
        .count();
}

bool UpdateInterest(int epoll_fd, const ClientState& state, bool want_write) {
    if (state.fd < 0 || state.closed) {
        return true;
    }

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    if (state.connecting || want_write) {
        ev.events |= EPOLLOUT;
    }
    ev.data.u32 = static_cast<uint32_t>(state.index);
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, state.fd, &ev) == 0;
}

void CloseClient(int epoll_fd, ClientState& state, CloseReason reason, int64_t now_ms,
                 int64_t active_phase_end_ms) {
    if (state.closed) {
        return;
    }

    if (state.fd >= 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state.fd, nullptr);
        close(state.fd);
        state.fd = -1;
    }

    state.closed = true;
    state.close_reason = reason;
    state.close_ms = now_ms;
    if (state.role == ClientRole::kActive && now_ms < active_phase_end_ms) {
        state.closed_during_active_phase = true;
    }
}

std::string MakeFrame(const std::string& payload) {
    uint32_t len = htonl(static_cast<uint32_t>(payload.size()));
    std::string frame;
    frame.resize(sizeof(len));
    std::memcpy(frame.data(), &len, sizeof(len));
    frame += payload;
    return frame;
}

void QueueHeartbeat(ClientState& state, int64_t now_ms) {
    std::string payload = "heartbeat-" + std::to_string(state.index) + "-" +
        std::to_string(state.sent_messages);
    state.write_buffer += MakeFrame(payload);
    ++state.sent_messages;
    if (state.first_sent_ms < 0) {
        state.first_sent_ms = now_ms;
    }
    state.last_sent_ms = now_ms;
}

bool HandleConnect(int epoll_fd, ClientState& state, int64_t now_ms) {
    if (!state.connecting || state.fd < 0) {
        return true;
    }

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(state.fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        return false;
    }

    state.connecting = false;
    state.connected = true;
    state.connected_ms = now_ms;
    if (state.role == ClientRole::kActive) {
        state.next_send_ms = now_ms;
    }
    return UpdateInterest(epoll_fd, state, !state.write_buffer.empty());
}

bool FlushWrites(int epoll_fd, ClientState& state, int64_t now_ms) {
    if (state.fd < 0 || state.closed) {
        return false;
    }

    if (state.connecting && !HandleConnect(epoll_fd, state, now_ms)) {
        return false;
    }

    while (state.write_offset < state.write_buffer.size()) {
        ssize_t n = send(state.fd,
                         state.write_buffer.data() + state.write_offset,
                         state.write_buffer.size() - state.write_offset,
                         MSG_NOSIGNAL);
        if (n > 0) {
            state.write_offset += static_cast<size_t>(n);
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            break;
        }

        return false;
    }

    if (state.write_offset == state.write_buffer.size()) {
        state.write_buffer.clear();
        state.write_offset = 0;
        return UpdateInterest(epoll_fd, state, false);
    }

    return true;
}

void ParseReplies(ClientState& state) {
    size_t offset = 0;
    while (state.read_buffer.size() - offset >= sizeof(uint32_t)) {
        uint32_t net_len = 0;
        std::memcpy(&net_len, state.read_buffer.data() + offset, sizeof(net_len));
        uint32_t payload_len = ntohl(net_len);
        if (payload_len > 1024 * 1024) {
            break;
        }
        if (state.read_buffer.size() - offset < sizeof(uint32_t) + payload_len) {
            break;
        }
        offset += sizeof(uint32_t) + payload_len;
        ++state.recv_messages;
        state.reply_seen = true;
    }

    if (offset > 0) {
        state.read_buffer.erase(0, offset);
    }
}

bool ReadReplies(ClientState& state) {
    char buffer[4096];
    while (true) {
        ssize_t n = recv(state.fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            state.read_buffer.append(buffer, static_cast<size_t>(n));
            ParseReplies(state);
            continue;
        }

        if (n == 0) {
            return false;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        }

        return false;
    }
}

void PrintUsage(const char* argv0) {
    std::cerr
        << "Usage: " << argv0
        << " <ip> <port> <total_clients> <active_clients> <send_interval_ms>"
        << " <active_phase_ms> <idle_min_ms> <idle_max_ms>" << std::endl;
}

int64_t EarliestTalkerCloseMs(const Options& options) {
    int64_t last_send_ms = options.active_phase_ms - options.send_interval_ms;
    if (last_send_ms < 0) {
        last_send_ms = 0;
    }
    return last_send_ms + options.idle_min_ms;
}

int64_t LatestTalkerCloseMs(const Options& options) {
    return options.active_phase_ms + options.idle_max_ms;
}

Summary BuildSummary(const std::vector<ClientState>& clients, int64_t active_phase_end_ms,
                     int idle_min_ms, int idle_max_ms) {
    Summary summary;
    for (const ClientState& client : clients) {
        if (client.connected) {
            ++summary.established;
        }
        if (!client.connected && client.close_reason == CloseReason::kConnectError) {
            ++summary.connect_failed;
        }
        if (client.connected && !client.closed) {
            ++summary.remaining_open;
        }

        if (client.role == ClientRole::kActive) {
            if (client.reply_seen) {
                ++summary.active_replied;
            }
            if (client.closed_during_active_phase) {
                ++summary.active_closed_during_phase;
            }
            if (client.first_sent_ms < 0) {
                ++summary.active_never_sent;
            }
            if (client.closed && client.close_ms >= active_phase_end_ms) {
                ++summary.active_closed_after_stop;
                int64_t idle_since = client.last_sent_ms >= 0 ? client.last_sent_ms : client.connected_ms;
                int64_t idle_duration = client.close_ms - idle_since;
                if (idle_duration >= idle_min_ms && idle_duration <= idle_max_ms) {
                    ++summary.active_closed_after_stop_in_range;
                }
            }
        } else {
            if (client.closed) {
                ++summary.silent_closed;
                int64_t idle_duration = client.close_ms - client.connected_ms;
                if (client.connected_ms >= 0 && idle_duration >= idle_min_ms && idle_duration <= idle_max_ms) {
                    ++summary.silent_closed_in_range;
                }
            }
        }
    }
    return summary;
}

void PrintSummary(const Summary& summary, const Options& options) {
    const int silent_clients = options.total_clients - options.active_clients;

    std::cout << "[summary] established=" << summary.established
              << " connect_failed=" << summary.connect_failed
              << " remaining_open=" << summary.remaining_open << std::endl;
    std::cout << "[summary] active_replied=" << summary.active_replied << "/"
              << options.active_clients
              << " active_closed_during_phase=" << summary.active_closed_during_phase
              << " active_closed_after_stop_in_range="
              << summary.active_closed_after_stop_in_range << "/"
              << options.active_clients
              << " active_never_sent=" << summary.active_never_sent << std::endl;
    std::cout << "[summary] silent_closed_in_range=" << summary.silent_closed_in_range << "/"
              << silent_clients
              << " silent_closed_total=" << summary.silent_closed << "/"
              << silent_clients << std::endl;
}

bool Passed(const Summary& summary, const Options& options) {
    const int silent_clients = options.total_clients - options.active_clients;
    const int min_active_reply = RequiredCount(options.active_clients, 95);
    const int min_active_close = RequiredCount(options.active_clients, 95);
    const int min_silent_close = RequiredCount(silent_clients, 90);

    return summary.connect_failed == 0 &&
        summary.remaining_open == 0 &&
        summary.active_closed_during_phase == 0 &&
        summary.active_never_sent == 0 &&
        summary.active_replied >= min_active_reply &&
        summary.active_closed_after_stop_in_range >= min_active_close &&
        summary.silent_closed_in_range >= min_silent_close;
}

ProgressSnapshot BuildProgressSnapshot(const std::vector<ClientState>& clients) {
    ProgressSnapshot snapshot;
    for (const ClientState& client : clients) {
        if (client.connected) {
            ++snapshot.established;
        }
        if (client.reply_seen) {
            ++snapshot.active_replied;
        }
        if (client.closed_during_active_phase) {
            ++snapshot.active_closed_during_phase;
        }
        if (client.role == ClientRole::kActive && client.closed && !client.closed_during_active_phase) {
            ++snapshot.active_closed_after_stop;
        }
        if (client.role == ClientRole::kSilent && client.closed) {
            ++snapshot.silent_closed;
        }
        if (!client.closed && client.connected) {
            if (client.role == ClientRole::kActive) {
                ++snapshot.active_open;
            } else {
                ++snapshot.silent_open;
            }
        }
    }
    return snapshot;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options options;
    if (argc == 9) {
        options.ip = argv[1];
        if (!ParseInt(argv[2], options.port) ||
            !ParseInt(argv[3], options.total_clients) ||
            !ParseInt(argv[4], options.active_clients) ||
            !ParseInt(argv[5], options.send_interval_ms) ||
            !ParseInt(argv[6], options.active_phase_ms) ||
            !ParseInt(argv[7], options.idle_min_ms) ||
            !ParseInt(argv[8], options.idle_max_ms)) {
            PrintUsage(argv[0]);
            return 1;
        }
    } else if (argc != 1) {
        PrintUsage(argv[0]);
        return 1;
    }

    if (options.total_clients <= 0 || options.active_clients < 0 ||
        options.active_clients > options.total_clients ||
        options.send_interval_ms <= 0 || options.active_phase_ms <= 0 ||
        options.idle_min_ms <= 0 || options.idle_max_ms < options.idle_min_ms) {
        std::cerr << "invalid arguments" << std::endl;
        return 1;
    }

    options.max_test_ms = options.active_phase_ms + options.idle_max_ms + 15000;

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(options.port);
    if (inet_pton(AF_INET, options.ip.c_str(), &server_addr.sin_addr) != 1) {
        std::cerr << "invalid ip: " << options.ip << std::endl;
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return 1;
    }

    std::vector<ClientState> clients(static_cast<size_t>(options.total_clients));
    auto start = std::chrono::steady_clock::now();
    const int64_t active_phase_end_ms = options.active_phase_ms;
    const int64_t hard_deadline_ms = options.max_test_ms;

    for (int index = 0; index < options.total_clients; ++index) {
        ClientState& client = clients[static_cast<size_t>(index)];
        client.index = index;
        client.role = index < options.active_clients ? ClientRole::kActive : ClientRole::kSilent;

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            client.close_reason = CloseReason::kConnectError;
            client.closed = true;
            continue;
        }

        if (!SetNonBlocking(fd)) {
            close(fd);
            client.close_reason = CloseReason::kConnectError;
            client.closed = true;
            continue;
        }

        client.fd = fd;

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
        ev.data.u32 = static_cast<uint32_t>(index);
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0) {
            close(fd);
            client.fd = -1;
            client.close_reason = CloseReason::kConnectError;
            client.closed = true;
            continue;
        }

        int ret = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if (ret == 0) {
            client.connecting = false;
            client.connected = true;
            client.connected_ms = 0;
            if (client.role == ClientRole::kActive) {
                client.next_send_ms = 0;
            }
            UpdateInterest(epoll_fd, client, false);
        } else if (errno != EINPROGRESS) {
            CloseClient(epoll_fd, client, CloseReason::kConnectError, 0, active_phase_end_ms);
        }
    }

    std::cout << "[INFO] total_clients=" << options.total_clients
              << " active_clients=" << options.active_clients
              << " silent_clients=" << (options.total_clients - options.active_clients)
              << " send_interval_ms=" << options.send_interval_ms
              << " active_phase_ms=" << options.active_phase_ms << std::endl;
    std::cout << "[INFO] success means: active_replied >= "
              << RequiredCount(options.active_clients, 95) << "/" << options.active_clients
              << ", active_closed_after_stop_in_range >= "
              << RequiredCount(options.active_clients, 95) << "/" << options.active_clients
              << ", silent_closed_in_range >= "
              << RequiredCount(options.total_clients - options.active_clients, 90) << "/"
              << (options.total_clients - options.active_clients)
              << ", active_closed_during_phase = 0, connect_failed = 0, remaining_open = 0"
              << std::endl;
    std::cout << "[INFO] talkers are expected to close roughly in ["
              << EarliestTalkerCloseMs(options) << "ms, "
              << LatestTalkerCloseMs(options) << "ms] from start"
              << std::endl;

    bool announced_stop = false;
    bool announced_active_reply = false;
    bool announced_silent_timeout = false;
    int64_t last_log_ms = -1000;
    std::vector<epoll_event> events(4096);

    while (true) {
        const int64_t now_ms = NowMs(start);

        if (!announced_stop && now_ms >= active_phase_end_ms) {
            announced_stop = true;
            std::cout << "[INFO] active phase ended, wait for heartbeat timeout; talkers should now start dropping before about "
                      << LatestTalkerCloseMs(options) << "ms" << std::endl;
        }

        for (int index = 0; index < options.active_clients; ++index) {
            ClientState& client = clients[static_cast<size_t>(index)];
            if (!client.connected || client.closed || client.connecting || now_ms >= active_phase_end_ms) {
                continue;
            }
            if (client.next_send_ms < 0 || now_ms < client.next_send_ms) {
                continue;
            }

            if (client.write_buffer.size() < 64 * 1024) {
                QueueHeartbeat(client, now_ms);
                client.next_send_ms = now_ms + options.send_interval_ms;
                if (!UpdateInterest(epoll_fd, client, true)) {
                    CloseClient(epoll_fd, client, CloseReason::kIoError, now_ms, active_phase_end_ms);
                }
            } else {
                client.next_send_ms = now_ms + options.send_interval_ms;
            }
        }

        int ready = epoll_wait(epoll_fd, events.data(), static_cast<int>(events.size()), 100);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < ready; ++i) {
            ClientState& client = clients[events[static_cast<size_t>(i)].data.u32];
            if (client.closed || client.fd < 0) {
                continue;
            }

            uint32_t event_mask = events[static_cast<size_t>(i)].events;
            bool ok = true;

            if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                ok = false;
            }

            if (ok && (event_mask & EPOLLOUT)) {
                ok = FlushWrites(epoll_fd, client, now_ms);
            }

            if (ok && (event_mask & EPOLLIN)) {
                ok = ReadReplies(client);
            }

            if (!ok) {
                CloseReason reason = client.connected ? CloseReason::kPeerClosed : CloseReason::kConnectError;
                CloseClient(epoll_fd, client, reason, now_ms, active_phase_end_ms);
            }
        }

        ProgressSnapshot snapshot = BuildProgressSnapshot(clients);
        const int required_active_reply = RequiredCount(options.active_clients, 95);
        const int required_silent_close = RequiredCount(options.total_clients - options.active_clients, 90);

        if (!announced_active_reply && snapshot.active_replied >= required_active_reply) {
            announced_active_reply = true;
            std::cout << "[MILESTONE] active traffic verified at t=" << now_ms
                      << "ms: active_replied=" << snapshot.active_replied << "/"
                      << options.active_clients << std::endl;
        }

        if (!announced_silent_timeout && snapshot.silent_closed >= required_silent_close) {
            announced_silent_timeout = true;
            std::cout << "[MILESTONE] silent heartbeat timeout verified at t=" << now_ms
                      << "ms: silent_closed=" << snapshot.silent_closed << "/"
                      << (options.total_clients - options.active_clients)
                      << "; now only waiting for talkers to stop and age out" << std::endl;
        }

        if (now_ms - last_log_ms >= 1000) {
            std::cout << "[progress] t=" << now_ms
                      << "ms established=" << snapshot.established
                      << " active_open=" << snapshot.active_open
                      << " silent_open=" << snapshot.silent_open
                      << " silent_closed=" << snapshot.silent_closed
                      << " active_replied=" << snapshot.active_replied
                      << " active_closed_after_stop=" << snapshot.active_closed_after_stop
                      << " active_closed_during_phase=" << snapshot.active_closed_during_phase
                      << std::endl;
            last_log_ms = now_ms;
        }

        bool all_closed = true;
        for (const ClientState& client : clients) {
            if (client.connected && !client.closed) {
                all_closed = false;
                break;
            }
        }

        if (all_closed || now_ms >= hard_deadline_ms) {
            break;
        }
    }

    const int64_t final_now_ms = NowMs(start);
    for (ClientState& client : clients) {
        if (client.connected && !client.closed) {
            CloseClient(epoll_fd, client, CloseReason::kStoppedByTester, final_now_ms, active_phase_end_ms);
        }
    }

    close(epoll_fd);

    Summary summary = BuildSummary(clients, active_phase_end_ms, options.idle_min_ms, options.idle_max_ms);
    PrintSummary(summary, options);

    if (!Passed(summary, options)) {
        std::cerr << "[FAIL] heartbeat mixed stress test did not meet expectations" << std::endl;
        return 2;
    }

    std::cout << "[PASS] heartbeat mixed stress test matched expected behavior" << std::endl;
    return 0;
}