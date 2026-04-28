#pragma once

template <typename T>
class singleton {

    using data_t   = T;
    using data_ptr = T*;
    using data_ref = T&;

    private:
    protected:
    singleton()  = default;
    ~singleton() = default;

    singleton(const singleton&)            = delete;
    singleton(singleton&&)                 = delete;
    singleton& operator=(const singleton&) = delete;
    singleton& operator=(singleton&&)      = delete;

    public:
    static T& instance() {
        static data_t instance;
        return instance;
    }
};

#define MAKE_SINGLETON(ClassName)                     \
    private:                                          \
    friend class singleton<ClassName>;                \
    ClassName()                            = default; \
    ~ClassName()                           = default; \
    ClassName(const ClassName&)            = delete;  \
    ClassName(ClassName&&)                 = delete;  \
    ClassName& operator=(const ClassName&) = delete;  \
    ClassName& operator=(ClassName&&)      = delete;
