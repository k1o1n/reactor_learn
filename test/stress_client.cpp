#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>

struct ClientState {
    int fd;
    std::string msg_to_send;
    size_t bytes_sent;
    std::string msg_received;
    bool connected;

    ClientState(int _fd) : fd(_fd), bytes_sent(0), connected(false) {
        msg_to_send = "Hello from client " + std::to_string(fd) + "\n";
    }
    ~ClientState() {
        if (fd != -1) ::close(fd);
    }
};

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> <num_clients>" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = std::stoi(argv[2]);
    int num_clients = std::stoi(argv[3]);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    std::vector<ClientState*> clients;
    clients.reserve(num_clients);

    int clients_started = 0;
    int active_connections = 0;
    struct epoll_event events[10000]; // Batch events

    auto start_time = std::chrono::steady_clock::now();
    auto last_log = start_time;

    // Run loop
    while (true) {
        // 1. Start new clients in batches (flow control)
        // Only if we haven't started all clients yet
        if (clients_started < num_clients) {
            // Launch 1000 new connections per loop iteration
            for (int i = 0; i < 10000 && clients_started < num_clients; ++i) {
                int fd = socket(AF_INET, SOCK_STREAM, 0);
                if (fd == -1) {
                    perror("socket");
                    break; 
                }
                set_nonblocking(fd);

                ClientState* state = new ClientState(fd);
                struct epoll_event ev;
                ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
                ev.data.ptr = state;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                    perror("epoll_ctl add");
                    delete state; // closes fd
                } else {
                    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
                    if (ret == 0) {
                        state->connected = true;
                    } else if (ret == -1 && errno != EINPROGRESS) {
                        // Connection failed immediately
                        // Clean up
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        delete state;
                        // Start next one?
                        continue;
                    }
                    clients.push_back(state);
                    clients_started++;
                    active_connections++;
                }
            }
        }

        // 2. Wait for events
        int n = epoll_wait(epoll_fd, events, 10000, 10); // 10ms timeout
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            ClientState* state = (ClientState*)events[i].data.ptr;
            uint32_t ev = events[i].events;

            if (ev & (EPOLLERR | EPOLLHUP)) {
                // Connection closed or error
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, nullptr);
                // Note: We don't remove from 'clients' vector to keep indices simple, 
                // just mark as closed by setting fd to -1 in the state if we wanted to reuse.
                // But here we rely on the object's destructor to close fd. 
                // However, we just close it here and mark as inactive.
                ::close(state->fd);
                state->fd = -1; 
                active_connections--;
                continue;
            }

            if (ev & EPOLLOUT) {
                if (!state->connected) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    if (getsockopt(state->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
                        // Connect failed
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, nullptr);
                        ::close(state->fd);
                        state->fd = -1;
                        active_connections--;
                        continue;
                    }
                    state->connected = true;
                }

                // Send data if connected
                if (state->fd != -1 && state->bytes_sent < state->msg_to_send.size()) {
                    ssize_t sent = send(state->fd, state->msg_to_send.c_str() + state->bytes_sent,
                                        state->msg_to_send.size() - state->bytes_sent, 0);
                    if (sent > 0) {
                        state->bytes_sent += sent;
                    } else if (sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, nullptr);
                        ::close(state->fd);
                        state->fd = -1;
                        active_connections--;
                    }
                }
            }

            if (ev & EPOLLIN) {
                char buf[256];
                bool closed = false;
                while (true) {
                    if (state->fd == -1) break; 
                    ssize_t received = recv(state->fd, buf, sizeof(buf), 0);
                    if (received > 0) {
                        // Received data
                    } else if (received == 0) {
                        // Server closed
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, nullptr);
                        ::close(state->fd);
                        state->fd = -1;
                        active_connections--;
                        closed = true;
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // Try again later
                        } else {
                            // Error
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, nullptr);
                            ::close(state->fd);
                            state->fd = -1;
                            active_connections--;
                            closed = true;
                            break;
                        }
                    }
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 1) {
            std::cout << "[StressClient] Launching: " << clients_started 
                      << "/" << num_clients 
                      << " | Active: " << active_connections << std::endl;
            last_log = now;
        }

        // If we have started all clients, and active connections drop to 0, we are done
        // OR we can keep running if we want to hold connections open (but here we don't close them unless server does)
        if (clients_started == num_clients && active_connections == 0) {
            std::cout << "All connections closed by server/error." << std::endl;
            break; 
        }

        // Optional: Stop after some time to avoid infinite loop if server keeps connections open
        // if (std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() > 60) break;
    }

    // Cleanup
    // (OS cleans up memory on exit, but good practice)
    std::cout << "Cleaning up..." << std::endl;
    // Don't double close
    for (auto c : clients) {
        // fd is already closed if -1
        if (c->fd != -1) {
             ::close(c->fd); // Close explicitly
             c->fd = -1;
        }
    }
    // Delete objects
    for (auto c : clients) delete c; // Destructor called, checks fd != -1

    close(epoll_fd);
    return 0;
}
