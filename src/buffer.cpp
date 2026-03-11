#include "buffer.h"
namespace adachi::io {
    Buffer::Buffer(int size) {
        buffer_.resize(size);
    }
}