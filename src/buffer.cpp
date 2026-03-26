#include "buffer.h"
#include <cstring>
#include <sys/uio.h>
#include <unistd.h>
#include <cerrno>
namespace adachi::io {
    Buffer::Buffer(int size) {
        readptr_ = writeptr_ = 0;
        buffer_.resize(size);
    }
    int Buffer::ReadFd(int fd, int* saveerrno) {
        iovec vec[2]{};
        char extrabuf[65536];
        vec[0].iov_len = buffer_.size() - writeptr_;
        vec[0].iov_base = buffer_.data() + writeptr_;
        vec[1].iov_len = 65536;
        vec[1].iov_base = extrabuf;
        ssize_t n = readv(fd, vec, 2);
        if (n <= 0) {
            *saveerrno = errno;
            return n;
        }
        if (static_cast<std::size_t>(n) >= vec[0].iov_len) {
            writeptr_ = buffer_.size();
            MovePtr();
            std::size_t need = n - vec[0].iov_len;
            if (need + writeptr_ > buffer_.size()) Expand(need + writeptr_);
            std::memmove(buffer_.data() + writeptr_, extrabuf, need);
            writeptr_ += need;
        }
        else writeptr_ += n;
        return n;
    }
    int Buffer::WriteFd(int fd, int* saveerrno) {
        ssize_t n = write(fd, buffer_.data() + readptr_, sizeof(char) * (writeptr_ - readptr_));
        if (n < 0) {
            *saveerrno = errno;
            return n;
        }
        readptr_ += n;
        if (readptr_ == writeptr_) writeptr_ = readptr_ = 0;
        return n;
    }
    void Buffer::WriteBuffer(const std::string& message) {
        if (writeptr_ + message.size() > buffer_.size()) {
            MovePtr();
            if (writeptr_ + message.size() > buffer_.size()) buffer_.resize(writeptr_ + message.size());
        }
        memmove(buffer_.data() + writeptr_, message.data(), message.size());
        writeptr_ += message.size();
    }
    void Buffer::WriteBuffer(const char* message, unsigned int len) {
        if (writeptr_ + len > buffer_.size()) {
            MovePtr();
            if (writeptr_ + len > buffer_.size()) buffer_.resize(writeptr_ + len);
        }
        memmove(buffer_.data() + writeptr_, message, len);
        writeptr_ += len;
    }
    void Buffer::ReadBuffer(std::string& message) {
        message.assign(buffer_.data() + readptr_, writeptr_ - readptr_);
        readptr_ = writeptr_ = 0;
    }
    void Buffer::MovePtr() {
        if (readptr_) {
            std::memmove(buffer_.data(), buffer_.data() + readptr_, sizeof(char) * (writeptr_ - readptr_));
            writeptr_ -= readptr_;
            readptr_ = 0;
        }
    }
    void Buffer::Expand(unsigned int size) {
        MovePtr();
        if (size > buffer_.size()) {
            buffer_.resize(size);
        }
    }
    bool Buffer::Empty() {
        return readptr_ == writeptr_;
    }
}