/**
 * @file propex_node.h
 * @brief Concrete property node with policy-based ownership semantics.
 *
 * @details
 * A `node<T, Ownership>` stores a value of type `T` using an ownership policy
 * (e.g. `ownership::by_value`, `by_reference`, `by_shared`, `by_atomic`) and
 * exposes a minimal, uniform interface:
 *
 *  - `get()` — returns either `const T&` (reference-returning policies) or `T`
 *    by value (value-returning policies, e.g. atomic).
 *  - `set(U&&)` — assigns a new value using the policy’s `storage_traits`.
 *
 * The class is designed to be used behind non-owning handles such as
 * `property_view<T, node, Ownership>`, and inside registries/graphs.
 *
 * The exact get/set behavior is delegated to:
 *  - `ownership::make_storage<T, Ownership>` — constructs the policy storage
 *  - `ownership::storage_traits<Ownership>` — unified `get`/`set` adapters
 *
 * @note
 *  - For `ownership::by_atomic<T>`, `get()` returns a value (load) and
 *    `set()` performs an atomic store.
 *  - For `ownership::by_value`, `by_reference`, and `by_shared`, `get()` returns
 *    a `const T&` and `set()` writes through the underlying storage/reference.
 *
 * @see ownership::by_value, ownership::by_reference,
 *      ownership::by_shared, ownership::by_atomic,
 *      ownership::make_storage, ownership::storage_traits
 */

#pragma once
#include <typeindex>
#include <typeinfo>
#include <utility>
#include "ownership_policies.h"

namespace numsim::propex {

/**
 * @class node_base
 * @brief Abstract base for all property nodes (type erasure anchor).
 *
 * Provides a virtual destructor and a runtime query of the underlying
 * stored *value type* via @ref underlying_type().
 *
 * This enables heterogeneous containers (e.g., a registry) to hold nodes
 * of different `T` while still allowing RTTI-based inspection.
 */
class node_base {
public:
    /// Defaulted constructor.
    node_base() = default;

    /// Virtual destructor to allow polymorphic deletion.
    virtual ~node_base() = default;

    /**
     * @brief Returns the `std::type_index` of the underlying stored value type `T`.
     *
     * @return The type index of the `T` used by the concrete node.
     */
    [[nodiscard]] virtual std::type_index underlying_type() const noexcept = 0;
};


/**
 * @class node
 * @brief Concrete property node storing a `T` with an ownership policy.
 *
 * @tparam T          Stored value type.
 * @tparam Ownership  Ownership policy template (default: `ownership::by_value`).
 *
 * @details
 * The node delegates construction and access to the ownership utilities:
 *
 *  - `make_storage::make(...)` is used by constructors to initialize policy storage.
 *  - `storage_traits::get(storage_)` returns either `const T&` or `T` depending
 *    on the policy (selected by `returns_reference_v`).
 *  - `storage_traits::set(storage_, value)` performs the appropriate write.
 *
 * ### Examples
 * @code
 * using VNode = node<int, ownership::by_value>;
 * VNode nv(42);
 * int x = nv.get();      // const int& returned (binds to a temp here)
 * nv.set(7);
 *
 * using ANode = node<int, ownership::by_atomic>;
 * ANode na(1);
 * int v = na.get();      // loads atomically by value
 * na.set(2);             // stores atomically
 * @endcode
 */
template <class T, template <class> class Ownership = ownership::by_value>
class node final : public node_base {
public:
    /// Helper alias to construct the policy storage.
    using make_storage   = ownership::make_storage<Ownership<T>>;
    /// Helper alias to adapt get/set operations to the policy.
    using storage_traits = ownership::storage_traits<Ownership<T>>;
    /// Whether this policy returns `const T&` (`true`) or `T` by value (`false`).
    static constexpr inline auto returns_reference_v = ownership::returns_reference_v<Ownership<T>>;

    /// Deleted default constructor (node must be initialized with a value/reference).
    node() = delete;

    /**
     * @brief Constructs the node from a const lvalue (value-like policies).
     * @param v Initial value.
     *
     * Uses `make_storage::make(v)` so policies can customize construction
     * (e.g. `by_shared` will create a `std::shared_ptr<T>`).
     */
    template<typename V>
    explicit node(const V& v)
        : storage_(make_storage::make(v)) {}

    /**
     * @brief Constructs the node from a const lvalue (value-like policies).
     * @param v Initial value.
     *
     * Uses `make_storage::make(v)` so policies can customize construction
     * (e.g. `by_shared` will create a `std::shared_ptr<T>`).
     */
    explicit node(const T& v)
        : storage_(make_storage::make(v)) {}

    /**
     * @brief Constructs the node from a non-const lvalue (reference-like policies).
     * @param v Lvalue reference used for policies like `by_reference`.
     *
     * Uses `make_storage::make(v)`; for value-like policies this simply copies
     * the value, for `by_reference` it captures the reference.
     */
    explicit node(T& v)
        : storage_(make_storage::make(v)) {}

    /**
     * @brief Returns the `std::type_index` of the underlying stored type `T`.
     */
    [[nodiscard]] std::type_index underlying_type() const noexcept override {
        return type_index;
    }

    /**
     * @brief Read access — reference-returning policies.
     * @return `const T&`
     *
     * Only available when `returns_reference_v == true`. The reference remains
     * valid as long as the node and, for `by_reference`, the referred object live.
     */
    [[nodiscard]] constexpr inline auto& get() const noexcept
        requires (returns_reference_v)
    {
        return storage_traits::get(storage_);
    }

    /**
     * @brief Read access — value-returning policies (e.g., atomic).
     * @return `T` by value (e.g., atomic load)
     *
     * Only available when `returns_reference_v == false`.
     */
    [[nodiscard]] constexpr inline auto get() noexcept
        requires (!returns_reference_v)
    {
        return storage_traits::get(storage_);
    }

    /**
     * @brief Write access — forwards to the policy’s setter.
     * @tparam U Value-compatible type.
     * @param v  New value to assign.
     *
     * - For `by_value`/`by_shared`/`by_reference`, this typically writes through.
     * - For `by_atomic`, this performs an atomic store.
     */
    template<class U>
    constexpr inline void set(U&& v) noexcept {
        storage_traits::set(storage_, std::forward<U>(v));
    }

private:
    /// Policy storage for the value (e.g., raw `T`, `T*`, `std::shared_ptr<T>`, or `std::atomic<T>`).
    Ownership<T> storage_;

    /// Cached type index for fast `underlying_type()` lookups.
    static inline std::type_index type_index{typeid(T)};
};

} // namespace numsim::propex
