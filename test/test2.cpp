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

    // 3. 彻底禁用拷贝构造和拷贝赋值
    MockSocket(const MockSocket&) = delete;
    MockSocket& operator=(const MockSocket&) = delete;

    // 4. 移动构造函数
    MockSocket(MockSocket&& other) noexcept {
        fd_ = other.fd_;
        other.fd_ = -1; // 关键：剥夺原对象资源
        std::cout << "  [移动构造] 触发！窃取资源, fd 变为 " << fd_ << ", 原对象变为空壳\n";
    }

    // 模拟工厂函数
    static MockSocket CreateNonBlockSocket() {
        std::cout << "    -> 进入 Create 函数内部\n";
        MockSocket local_sock(888); // 假设系统分配的 fd 是 888
        std::cout << "    -> 准备 return local_sock\n";
        return local_sock;
    }

private:
    int fd_;
};

int main() {
    std::cout << "========== 场景 1: 非裸调用 (auto mem = Create...) ==========\n";
    {
        auto mem = MockSocket::CreateNonBlockSocket();
        std::cout << "    -> mem 对象接收完毕，做了一些别的事...\n";
        std::cout << "    -> 准备离开 mem 的作用域，触发 mem 的析构\n";
    } 

    std::cout << "\n========== 场景 2: 裸调用 (Create... ;) ==========\n";
    {
        MockSocket::CreateNonBlockSocket();
        std::cout << "    -> 裸调用语句结束 (遇到了分号 ; )\n";
    }

    return 0;
}