#include <stdexcept>
#include <memory>

namespace ownership {
template <class T>
struct by_value {
    T value;
    explicit by_value(const T& v) : value(v) {}
    T& get() { return value; }
    const T& get() const { return value; }
};

template <class T>
struct by_reference {
    T* ptr{};
    explicit by_reference(T& ref) : ptr(&ref) {}
    T& get() {
        if (!ptr) throw std::runtime_error("dangling reference");
        return *ptr;
    }
    const T& get() const {
        if (!ptr) throw std::runtime_error("dangling reference");
        return *ptr;
    }
};

template <class T>
struct by_shared {
    std::shared_ptr<T> ptr;
    explicit by_shared(std::shared_ptr<T> p) : ptr(std::move(p)) {}
    T& get() { return *ptr; }
    const T& get() const { return *ptr; }
};

template <class T>
struct by_atomic {
    std::atomic<T> value;
    explicit by_atomic(T v) : value(v) {}
    std::atomic<T>& get() { return value; }
    const std::atomic<T>& get() const { return value; }
    void set(T v) { value.store(v, std::memory_order_relaxed); }
};
}