/**
 * @file registry.hpp
 * @brief Flat key–value registry for storing node objects by key.
 *
 * The `propex::registry` class provides a generic, map-like container
 * for storing `node_base`-derived objects under arbitrary string keys.
 * It supports both checked (`at()`) and unchecked (`find()`) access,
 * similar to `std::map` and `std::unordered_map`.
 *
 * @date 2025-10-16
 * @version 1.0
 * @author Peter Lenz
 */

#ifndef PROPEX_REGISTRY_HPP
#define PROPEX_REGISTRY_HPP

#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <utility>
#include "key_traits.h"

namespace numsim::propex {

/**
 * @brief Generic, flat registry for mapping keys to `node_base` instances.
 *
 * @tparam Key        The key type used to identify entries (default: `std::string`).
 * @tparam NodePtr    The smart pointer type used for node storage.
 * @tparam Map        The associative container template (default: `std::unordered_map`).
 * @tparam KeyTraits  A traits class that provides key composition logic.
 *
 * This registry stores all nodes in a single-level map. Keys can be hierarchical
 * strings (e.g., `"object:property"`) if your `KeyTraits` merges subkeys that way.
 *
 * ### Example
 * ```cpp
 * using namespace propex;
 * registry<std::string> reg;
 *
 * reg.add(std::make_unique<node<int>>(42), "carA", "speed");
 * auto* n = reg.find("carA:speed");
 * if (n) std::cout << n->type().name() << "\n";
 * ```
 */
template<
    class Key,
    class NodeType,
    template<class...>class NodePtr    = std::unique_ptr,
    template<class...> class Map       = std::unordered_map,
    template<class>class KeyTraits  = key_traits
    >
class registry {
public:
    using key_type     = Key;
    using node_pointer = NodePtr<NodeType>;
    using map_type     = Map<key_type, node_pointer>;
    using key_traits   = KeyTraits<Key>;

    /// Default constructor.
    constexpr registry() noexcept = default;

    /// Non-copyable.
    registry(const registry&) = delete;
    registry& operator=(const registry&) = delete;

    /// Movable.
    registry(registry&&) noexcept = default;
    registry& operator=(registry&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Insertion
    // -------------------------------------------------------------------------

    /**
     * @brief Inserts or replaces a node using one or more key fragments.
     *
     * Multiple fragments are combined via `KeyTraits::merge()`.
     *
     * @tparam Args... Key fragments that can be merged into a full key.
     * @param node Node pointer to insert (ownership transferred).
     * @param args Key fragments to merge into a key.
     */
    template<typename... Args>
    constexpr inline void add(node_pointer&& node, Args&&... args) {
        static_assert(sizeof...(Args) >= 1, "At least one key argument is required");
        const auto key = make_key(std::forward<Args>(args)...);
        data_[key] = std::move(node);
    }

    // -------------------------------------------------------------------------
    // Lookup (Unchecked)
    // -------------------------------------------------------------------------

    /**
     * @brief Finds a node by key without throwing.
     * @param key The lookup key.
     * @return A pointer to the node, or nullptr if not found.
     */
    [[nodiscard]]
    constexpr inline NodeType* find(const key_type& key) const noexcept {
        const auto it = data_.find(key);
        return (it != data_.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Checks whether a node with the given key exists.
     */
    [[nodiscard]]
    constexpr inline bool contains(const key_type& key) const noexcept {
        return data_.find(key) != data_.end();
    }

    // -------------------------------------------------------------------------
    // Lookup (Checked)
    // -------------------------------------------------------------------------

    /**
     * @brief Retrieves a node by key and throws if missing.
     * @param key The lookup key.
     * @throws std::out_of_range if the key is not found.
     * @return Reference to the stored node.
     */
    [[nodiscard]]
    constexpr inline NodeType& at(const key_type& key) const {
        const auto it = data_.find(key);
        if (it == data_.end())
            throw std::out_of_range("registry::at(): key not found");
        return *it->second;
    }

    // -------------------------------------------------------------------------
    // Erase and Clear
    // -------------------------------------------------------------------------

    /**
     * @brief Removes a node by key if present.
     * @param key The key to erase.
     * @return True if an element was erased.
     */
    constexpr inline bool erase(const key_type& key) noexcept {
        return data_.erase(key) > 0;
    }

    /**
     * @brief Removes all nodes from the registry.
     */
    constexpr inline void clear() noexcept { data_.clear(); }

    // -------------------------------------------------------------------------
    // Iteration / View
    // -------------------------------------------------------------------------

    /// @return A const reference to the underlying map container.
    [[nodiscard]]
    constexpr inline const map_type& data() const noexcept { return data_; }

    /// @return A mutable reference to the underlying map container.
    [[nodiscard]]
    constexpr inline map_type& data() noexcept { return data_; }

private:
    // Utility — merges key fragments using traits
    template<typename... Args>
    static constexpr key_type make_key(Args&&... args) {
        if constexpr (sizeof...(Args) == 1)
            return key_type(std::forward<Args>(args)...);
        else
            return key_traits::merge(std::forward<Args>(args)...);
    }

    map_type data_;
};

} // namespace propex


#endif // PROPEX_REGISTRY_H
