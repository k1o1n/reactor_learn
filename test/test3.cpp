#include <iostream>

class MockSocket {
public:
    // 1. 普通构造函数
    explicit MockSocket(int fd) : fd_(fd) {
        std::cout << "  [普通构造] 创建 Socket 局部对象, fd = " << fd_ << "\n";
    }

    // 2. 析构函数
    ~MockSocket() {
        if (fd_ != -1) {
            std::cout << "  [析构函数] 关闭有效的 Socket, 执行 close(" << fd_ << ")\n";
        } else {
            std::cout << "  [析构函数] 销毁空壳对象 (fd = -1), 不执行 close\n";
        }
    }

    // 3. 显式提供【拷贝构造函数】（看看编译器会不会选它）
    MockSocket(const MockSocket& other) {
        fd_ = other.fd_; // 危险的浅拷贝，仅作测试
        std::cout << "  [拷贝构造] 触发！执行了拷贝, fd = " << fd_ << "\n";
    }

    // 4. 显式提供【移动构造函数】（看看编译器是不是优先选它）
    MockSocket(MockSocket&& other) noexcept {
        fd_ = other.fd_;
        other.fd_ = -1; // 剥夺原对象资源
        std::cout << "  [移动构造] 触发！窃取资源, fd 变为 " << fd_ << "\n";
    }

    // 模拟工厂函数
    static MockSocket CreateNonBlockSocket() {
        MockSocket local_sock(888);
        return local_sock;
    }

private:
    int fd_;
};

int main() {
    std::cout << "========== 验证：同时存在拷贝和移动时，返回值优先调用谁？ ==========\n";
    {
        auto mem = MockSocket::CreateNonBlockSocket();
        std::cout << "    -> mem 对象接收完毕，准备析构...\n";
    }
    return 0;
}