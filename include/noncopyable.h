#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H
namespace adachi::tool {
    class NonCopyAble {
    protected:
        NonCopyAble() = default;
        ~NonCopyAble() = default;
    public:
        NonCopyAble(const NonCopyAble&) = delete;
        NonCopyAble& operator=(const NonCopyAble&) = delete;
    };
}
#endif // NONCOPYABLE