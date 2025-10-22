/**
 * @file ownership_policies.h
 * @brief Defines ownership policy types for the Propex property system.
 *
 * This header declares a set of template classes that define how property
 * values are owned, referenced, or shared within the Propex framework.
 * Each policy type provides a unified interface (`get()` / `set()`) but
 * implements distinct lifetime and access semantics.
 *
 * Ownership policies can be applied to any value type `T` and used within
 * Propex node and property abstractions.
 *
 * @date 2025-10-16
 * @version 1.0
 * @author Peter Lenz
 */

#ifndef OWNERSHIP_POLICIES_H
#define OWNERSHIP_POLICIES_H

#include <stdexcept>
#include <memory>
#include <atomic>

namespace ownership {

/**
 * @brief Owns a value by copy.
 *
 * The `by_value` ownership policy stores a value of type `T` directly inside
 * the node. Copies and assignments affect only this instance.
 *
 * @tparam T Value type.
 *
 * @code
 * ownership::by_value<int> val(42);
 * val.get() = 99; // modifies the stored value
 * @endcode
 */
template <class T>
struct by_value {
    /// The stored value.
    T value;

    /// Constructs a new instance with a copy of @p v.
    explicit by_value(const T& v) : value(v) {}

    /// Returns a mutable reference to the stored value.
    T& get() noexcept { return value; }

    /// Returns a const reference to the stored value.
    const T& get() const noexcept { return value; }
};

/**
 * @brief Owns a reference to an external value.
 *
 * The `by_reference` ownership policy holds a pointer to an existing value.
 * It does not take ownership and does not perform lifetime management.
 *
 * Attempting to access a dangling reference throws `std::runtime_error`.
 *
 * @tparam T Value type.
 *
 * @code
 * int x = 10;
 * ownership::by_reference<int> ref(x);
 * ref.get() = 20; // modifies x
 * @endcode
 */
template <class T>
struct by_reference {
    /// Pointer to the external value.
    T* ptr{};

    /// Constructs a new instance referring to @p ref.
    explicit by_reference(T& ref) noexcept : ptr(&ref) {}

    /// Returns a mutable reference to the referred value.
    /// @throws std::runtime_error if the pointer is null.
    T& get() {
        if (!ptr) throw std::runtime_error("dangling reference");
        return *ptr;
    }

    /// Returns a const reference to the referred value.
    /// @throws std::runtime_error if the pointer is null.
    const T& get() const {
        if (!ptr) throw std::runtime_error("dangling reference");
        return *ptr;
    }
};

/**
 * @brief Owns a shared pointer to a heap-allocated value.
 *
 * The `by_shared` ownership policy uses `std::shared_ptr<T>` to manage
 * the lifetime of the stored value. Copies of this policy share ownership
 * of the same underlying resource.
 *
 * @tparam T Value type.
 *
 * @code
 * auto p = std::make_shared<double>(3.14);
 * ownership::by_shared<double> sh(p);
 * *p = 6.28; // visible via sh.get()
 * @endcode
 */
template <class T>
struct by_shared {
    /// Shared pointer to the owned value.
    std::shared_ptr<T> ptr;

    /// Constructs a new instance with a given shared pointer.
    explicit by_shared(std::shared_ptr<T> p) noexcept : ptr(std::move(p)) {}

    /// Returns a mutable reference to the managed value.
    T& get() noexcept { return *ptr; }

    /// Returns a const reference to the managed value.
    const T& get() const noexcept { return *ptr; }
};

/**
 * @brief Owns a value stored in an atomic variable.
 *
 * The `by_atomic` ownership policy encapsulates a `std::atomic<T>` for
 * thread-safe concurrent access. Reading or writing the value is lock-free.
 *
 * The policy provides direct access to the underlying `std::atomic<T>` via
 * `get()` and a convenience setter via `set(T)`.
 *
 * @tparam T Value type (must be atomically assignable).
 *
 * @code
 * ownership::by_atomic<int> counter(0);
 * counter.get().fetch_add(1); // atomic increment
 * counter.set(42);            // store a new value
 * @endcode
 */
template <class T>
struct by_atomic {
    /// The atomically stored value.
    std::atomic<T> value;

    /// Constructs a new instance initializing the atomic with @p v.
    explicit by_atomic(T v) noexcept : value(v) {}

    /// Returns a reference to the underlying atomic object.
    std::atomic<T>& get() noexcept { return value; }

    /// Returns a const reference to the underlying atomic object.
    const std::atomic<T>& get() const noexcept { return value; }

    /// Atomically stores a new value.
    void set(T v) noexcept { value.store(v, std::memory_order_relaxed); }
};

template<class Ownership>
struct returns_reference : std::true_type{};

template<typename T>
struct returns_reference<by_atomic<T>> : std::false_type{};

template<class Ownership>
concept returns_reference_v = returns_reference<Ownership>::value;

/**
 * @brief Primary template — undefined, specialized per storage type.
 */
template<class Storage>
struct make_storage;

/**
 * @brief Generic fallback for value-like storages (by_value, by_atomic).
 */
template<template<class> class Ownership, class T>
struct make_storage<Ownership<T>> {
    static constexpr inline auto make(const T& v) {
        return Ownership<T>(v);
    }
};

/**
 * @brief Specialized factory for `by_shared<T>`.
 */
template<class T>
struct make_storage<by_shared<T>> {
    static inline auto make(std::shared_ptr<T>&& sp) noexcept {
        return by_shared<T>(std::move(sp));
    }
    static inline auto make(std::shared_ptr<T>const& sp) noexcept {
        return by_shared<T>(sp);
    }
    static inline auto make(const T& v) {
        return by_shared<T>(std::make_shared<T>(v));
    }
};

/**
 * @brief Specialized factory for `by_reference<T>`.
 */
template<class T>
struct make_storage<by_reference<T>> {
    static constexpr inline auto make(T& v) noexcept {
        return by_reference<T>(v);
    }
};

template<class Storage>
struct storage_traits;

// by_value<T>
template<class T>
struct storage_traits<by_value<T>> {
    static constexpr inline const T& get(const by_value<T>& s) noexcept { return s.get(); }
    template<class U>
    static constexpr inline void set(by_value<T>& s, U&& v) noexcept { s.get() = std::forward<U>(v); }
};

// by_reference<T>
template<class T>
struct storage_traits<by_reference<T>> {
    static inline const T& get(const by_reference<T>& s) { return s.get(); }
    template<class U>
    static inline void set(by_reference<T>& s, U&& v) { s.get() = std::forward<U>(v); }
};

// by_shared<T>
template<class T>
struct storage_traits<by_shared<T>> {
    static inline const T& get(const by_shared<T>& s) { return s.get(); }
    template<class U>
    static inline void set(by_shared<T>& s, U&& v) { s.get() = std::forward<U>(v); }
    static inline void set(by_shared<T>& s, std::shared_ptr<T>&& sp) noexcept { s.ptr = std::move(sp); }
};

// by_atomic<T>
template<class T>
struct storage_traits<by_atomic<T>> {
    static constexpr inline T get(const by_atomic<T>& s) noexcept { return s.value.load(std::memory_order_relaxed); }
    template<class U>
    static constexpr inline void set(by_atomic<T>& s, U&& v) noexcept { s.set(static_cast<T>(std::forward<U>(v))); }
};
} // namespace ownership

#endif // OWNERSHIP_POLICIES_H
