#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    const char* server_ip = "127.0.0.1";
    int server_port = 12345;

    if (argc > 1) server_ip = argv[1];
    if (argc > 2) server_port = std::stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }

    std::string msg = "Hello from simple client!";
    if (send(sock, msg.c_str(), msg.length(), 0) == -1) {
        perror("send");
        close(sock);
        return 1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
    } else if (bytes_received == 0) {
        std::cout << "Server closed connection" << std::endl;
    } else {
        std::cout << "Received echo: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}
