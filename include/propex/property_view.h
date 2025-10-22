/**
 * @file property_view.h
 * @brief Policy-based, non-owning handle for accessing and mutating property nodes.
 *
 * @details
 * `property_view<T, Node, Ownership>` provides a unified, type-safe interface
 * for accessing and mutating property nodes stored elsewhere (e.g., in a registry).
 *
 * It supports both:
 * - **Checked accessors** (`get_checked()`, `set_checked()`): throw if unbound.
 * - **Unchecked accessors** (`get()`, `set()`): assert if unbound (via `PROPERTYVIEW_ASSERT`).
 *
 * Ownership policies determine whether `get()` returns by value or by reference:
 * | Ownership Policy      | Returns Type | Description |
 * |------------------------|---------------|--------------|
 * | `ownership::by_value`   | `const T&`    | Direct storage |
 * | `ownership::by_reference` | `const T&` | External reference |
 * | `ownership::by_shared`   | `const T&`  | Shared ownership via `shared_ptr` |
 * | `ownership::by_atomic`   | `T`          | Copy via atomic access |
 *
 * ### Example
 * @code
 * using namespace numsim::propex;
 *
 * node<double, ownership::by_value> temperature_node(23.5);
 * property_view<double, node, ownership::by_value> temp(&temperature_node);
 *
 * // Checked read
 * double t = temp.get_checked(); // returns 23.5
 *
 * // Assignment (unchecked)
 * temp = 42.0;
 * @endcode
 *
 * @note `PROPERTYVIEW_ASSERT(expr)` must expand to a debug assertion (e.g. `assert(expr)`).
 *       Define it globally if not provided elsewhere.
 *
 * @see ownership::by_value, ownership::by_reference, ownership::by_shared, ownership::by_atomic
 */

#ifndef PROPEX_PROPERTY_VIEW_H
#define PROPEX_PROPERTY_VIEW_H

#include "ownership_policies.h"
#include <stdexcept>
#include <utility>

#ifndef PROPERTYVIEW_ASSERT
#include <cassert>
/// Default macro used for debug-checked access.
#define PROPERTYVIEW_ASSERT(expr) assert(expr)
#endif

namespace numsim::propex {

/**
 * @brief Forward declaration for a node type used with property_view.
 *
 * The `Node` template must accept `(T, Ownership)` as template parameters
 * and expose:
 *  - `auto get() const`
 *  - `template<class U> void set(U&&)`
 */
template <class T, template <class> class Ownership>
class node;

/**
 * @class property_view
 * @brief Thin non-owning interface for manipulating `node<T, Ownership>` instances.
 *
 * @tparam T          Stored value type.
 * @tparam Node       Node type template accepting `(T, Ownership)`.
 * @tparam Ownership  Ownership policy template (default: `ownership::by_value`).
 *
 * @ingroup propex-core
 */
template <class T,
         template <class, template <class> class> class Node,
         template <class> class Ownership = ownership::by_value>
class property_view {
public:
    /// Storage and trait helpers for interoperability with ownership policies.
    using make_storage   = ownership::make_storage<Ownership<T>>;

    /// Whether this ownership policy returns a reference (`true`) or a value (`false`).
    static constexpr inline bool returns_reference_v = ownership::returns_reference_v<Ownership<T>>;

    // -------------------------------------------------------------------------
    // Constructors and Assignment
    // -------------------------------------------------------------------------

    /// @brief Constructs an unbound view.
    constexpr property_view() noexcept = default;

    /// @brief Constructs a view bound to an existing node.
    constexpr explicit property_view(Node<T, Ownership>* n) noexcept : node_(n) {}

    /// @brief Move-constructs a view, transferring the node binding.
    constexpr property_view(property_view&& other) noexcept
        : node_(std::exchange(other.node_, nullptr)) {}

    /// @brief Move-assigns a view, releasing any existing binding.
    constexpr property_view& operator=(property_view&& other) noexcept {
        if (this != &other) node_ = std::exchange(other.node_, nullptr);
        return *this;
    }

    /// Deleted copy constructor.
    property_view(const property_view&) = delete;
    /// Deleted copy assignment.
    property_view& operator=(const property_view&) = delete;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /// @return `true` if the view is currently bound to a node.
    [[nodiscard]] constexpr bool valid() const noexcept { return node_ != nullptr; }

    // -------------------------------------------------------------------------
    // Write Operations
    // -------------------------------------------------------------------------

    /**
     * @brief Assigns a new value via the assignment operator.
     *
     * Equivalent to `set(value)`. Provided for ergonomic usage.
     */
    template <typename Value>
    constexpr property_view& operator=(Value&& value) {
        set(std::forward<Value>(value));
        return *this;
    }

    /// @brief Checked mutation — throws if unbound.
    constexpr void set_checked(const T& v) {
        if (!node_) throw std::runtime_error("property_view: null access");
        node_->set(v);
    }

    /// @brief Unchecked mutation — asserts in debug builds.
    constexpr void set(const T& v) noexcept {
        PROPERTYVIEW_ASSERT(node_);
        node_->set(v);
    }

    /// @brief Perfect-forwarding unchecked mutation.
    template <typename V>
    constexpr void set(V&& v) noexcept {
        PROPERTYVIEW_ASSERT(node_);
        node_->set(std::forward<V>(v));
    }

    // -------------------------------------------------------------------------
    // Read Operations
    // -------------------------------------------------------------------------

    /// @brief Checked access returning a reference (reference-returning policies only).
    [[nodiscard]] constexpr const T& get_checked() const
        requires (returns_reference_v)
    {
        if (!node_) throw std::runtime_error("property_view: null access");
        return node_->get();
    }

    /// @brief Checked access returning by value (value-returning policies only).
    [[nodiscard]] constexpr T get_checked() const
        requires (!returns_reference_v)
    {
        if (!node_) throw std::runtime_error("property_view: null access");
        return node_->get();
    }

    /// @brief Unchecked access returning a reference.
    [[nodiscard]] constexpr const T& get() const noexcept
        requires (returns_reference_v)
    {
        PROPERTYVIEW_ASSERT(node_);
        return node_->get();
    }

    /// @brief Unchecked access returning by value.
    [[nodiscard]] constexpr T get() const noexcept
        requires (!returns_reference_v)
    {
        PROPERTYVIEW_ASSERT(node_);
        return node_->get();
    }

private:
    /// @brief Non-owning pointer to the underlying node.
    Node<T, Ownership>* node_{nullptr};
};

} // namespace numsim::propex

#endif // PROPEX_PROPERTY_VIEW_H
