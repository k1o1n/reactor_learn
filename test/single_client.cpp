#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    std::string ip = "127.0.0.1";
    int port = 12345;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != -1) {
        std::cout << "link!" << std::endl;
        std::string msg = "Hello!";
        unsigned int head = msg.size();
        head = htonl(head);
        std::string strhead;
        strhead.assign(reinterpret_cast<char*>(&head), sizeof(head));
        msg = strhead + msg;
        write(fd, msg.c_str(), msg.size());
        std::cout << "send:" << msg << std::endl;

        iovec echo[2]{};
        char buf1[sizeof(unsigned int)]{};
        char buf2[1024]{};
        echo[0].iov_base = buf1;
        echo[0].iov_len = sizeof(buf1); 
        echo[1].iov_base = buf2;
        echo[1].iov_len = sizeof(buf2);
        int n = readv(fd, echo, 2);

        // std::string getmsg;

        std::cout << "get echo " << "(" << n << " bytes):" << (char*)echo[1].iov_base << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(9));

    std::string msg = "Hello!";
    unsigned int head = msg.size();
    head = htonl(head);
    std::string strhead;
    strhead.assign(reinterpret_cast<char*>(&head), sizeof(head));
    msg = strhead + msg;
    write(fd, msg.c_str(), msg.size());
    std::cout << "send:" << msg << std::endl;

    iovec echo[2]{};
    char buf1[sizeof(unsigned int)]{};
    char buf2[1024]{};
    echo[0].iov_base = buf1;
    echo[0].iov_len = sizeof(buf1); 
    echo[1].iov_base = buf2;
    echo[1].iov_len = sizeof(buf2);
    int n = readv(fd, echo, 2);

    // std::string getmsg;

    std::cout << "get echo " << "(" << n << " bytes):" << (char*)echo[1].iov_base << std::endl;

    {
        std::this_thread::sleep_for(std::chrono::seconds(12));

        std::string msg = "Hello!";
        unsigned int head = msg.size();
        head = htonl(head);
        std::string strhead;
        strhead.assign(reinterpret_cast<char*>(&head), sizeof(head));
        msg = strhead + msg;
        write(fd, msg.c_str(), msg.size());
        std::cout << "send:" << msg << std::endl;

        iovec echo[2]{};
        char buf1[sizeof(unsigned int)]{};
        char buf2[1024]{};
        echo[0].iov_base = buf1;
        echo[0].iov_len = sizeof(buf1); 
        echo[1].iov_base = buf2;
        echo[1].iov_len = sizeof(buf2);
        int n = readv(fd, echo, 2);

        // std::string getmsg;

        std::cout << "get echo " << "(" << n << " bytes):" << (char*)echo[1].iov_base << std::endl;
    }
    
    close(fd);
    
    return 0;
}