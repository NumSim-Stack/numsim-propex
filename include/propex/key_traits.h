/**
 * @file key_traits.hpp
 * @brief Defines key traits for splitting and merging hierarchical keys.
 *
 * The `key_traits` class template provides a uniform interface for splitting
 * and merging registry keys. It allows flat and hierarchical registries to
 * share a common key-processing layer without changing their map structure.
 *
 * Custom specializations may be provided for structured key types or
 * alternative delimiters.
 *
 * @date 2025-10-16
 * @version 1.0
 * @author Peter Lenz
 */

#ifndef KEY_TRAITS_H
#define KEY_TRAITS_H

#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace numsim::propex {

/**
 * @brief Default key traits for string-like key types.
 *
 * The default specialization provides `split()` and `merge()` functions
 * for delimited string keys (e.g., `"object:property"`).
 *
 * The delimiter is `":"` by default but can be customized via the `Delimiter`
 * template parameter.
 *
 * @tparam KeyType    The type used as registry key (default: `std::string`).
 * @tparam Delimiter  The delimiter character used to separate subkeys.
 *
 * @code
 * using traits = propex::key_traits<std::string>;
 * auto [obj, prop] = traits::split("carA:speed");
 * auto merged = traits::merge("carA", "speed"); // "carA:speed"
 * @endcode
 */
template <typename KeyType, char Delimiter = ':'>
struct key_traits;

template <char Delimiter>
struct key_traits<std::string, Delimiter> {
    using key_type = std::string;
    using view_type = std::basic_string_view<typename key_type::value_type>;

    /**
     * @brief Splits a delimited key string into subkeys.
     *
     * This function splits a key like `"object:property"` into a vector
     * of subkeys. If no delimiter is found, the entire string is returned
     * as a single part.
     *
     * @param key The input key string.
     * @return A vector of subkey string views.
     */
    [[nodiscard]]
    static constexpr inline auto split(view_type key) noexcept {
        std::vector<view_type> parts;
        std::size_t start = 0;
        while (true) {
            std::size_t pos = key.find(Delimiter, start);
            if (pos == key.npos) {
                parts.emplace_back(key.substr(start));
                break;
            }
            parts.emplace_back(key.substr(start, pos - start));
            start = pos + 1;
        }
        return parts;
    }

    /**
     * @brief Merges subkeys into a single delimited key string.
     *
     * Joins multiple key fragments using the configured delimiter. All
     * arguments are expected to be string-like objects convertible to
     * `std::string_view`.
     *
     * @param args The subkeys to concatenate.
     * @return A merged `KeyType` key (e.g., `"object:property"`).
     */
    template <typename... Args>
    [[nodiscard]]
    static constexpr inline key_type merge(const Args&... args) {
        if constexpr (sizeof...(Args) == 0)
            return {};

        key_type result;
        ((result.append(args).append(1, Delimiter)), ...);
        if (!result.empty()) result.pop_back(); // remove trailing delimiter
        return result;
    }

    /**
     * @brief Returns the delimiter used by this trait.
     */
    [[nodiscard]]
    static constexpr inline char delimiter() noexcept { return Delimiter; }
};

} // namespace propex

#endif // KEY_TRAITS_H
