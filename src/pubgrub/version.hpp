#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pubgrub {

/**
 * @brief Immutable semantic version: `major.minor.patch`.
 *
 * Provides the full ordering and basic arithmetic needed to implement a
 * `requirement` type on top of integer-based version ranges.
 *
 * Parsing:
 * @code
 * auto v = pubgrub::semver::parse("1.2.3");  // returns std::optional<semantic_version>
 * @endcode
 *
 * Comparison:
 * @code
 * semantic_version a{1, 0, 0};
 * semantic_version b{2, 0, 0};
 * assert(a < b);
 * @endcode
 */
struct semantic_version {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;

    constexpr auto operator<=>(const semantic_version&) const noexcept = default;

    /// Return a version with patch incremented by one.
    [[nodiscard]] constexpr semantic_version bump_patch() const noexcept {
        return {major, minor, patch + 1};
    }
    /// Return a version with minor incremented by one and patch reset to zero.
    [[nodiscard]] constexpr semantic_version bump_minor() const noexcept {
        return {major, minor + 1, 0};
    }
    /// Return a version with major incremented by one and minor+patch reset to zero.
    [[nodiscard]] constexpr semantic_version bump_major() const noexcept {
        return {major + 1, 0, 0};
    }

    /// Render as "major.minor.patch" string.
    [[nodiscard]] std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

namespace semver {

/**
 * @brief Parse a "major.minor.patch" version string.
 *
 * Returns `std::nullopt` when the input is not a valid three-part dotted integer version.
 */
[[nodiscard]] inline std::optional<semantic_version> parse(std::string_view s) noexcept {
    auto take_uint = [](std::string_view& sv) -> std::optional<std::uint32_t> {
        if (sv.empty() || sv[0] < '0' || sv[0] > '9') {
            return std::nullopt;
        }
        std::uint32_t value = 0;
        while (!sv.empty() && sv[0] >= '0' && sv[0] <= '9') {
            // Guard against overflow: ensure multiplying by 10 and adding up to 9
            // (the maximum digit value) won't overflow uint32_t.
            if (value > (UINT32_MAX - 9) / 10) {
                return std::nullopt;
            }
            value = value * 10 + static_cast<std::uint32_t>(sv[0] - '0');
            sv.remove_prefix(1);
        }
        return value;
    };

    auto take_dot = [](std::string_view& sv) -> bool {
        if (!sv.empty() && sv[0] == '.') {
            sv.remove_prefix(1);
            return true;
        }
        return false;
    };

    auto major_opt = take_uint(s);
    if (!major_opt || !take_dot(s)) {
        return std::nullopt;
    }
    auto minor_opt = take_uint(s);
    if (!minor_opt || !take_dot(s)) {
        return std::nullopt;
    }
    auto patch_opt = take_uint(s);
    if (!patch_opt || !s.empty()) {
        return std::nullopt;
    }
    return semantic_version{*major_opt, *minor_opt, *patch_opt};
}

/**
 * @brief Parse a "major.minor.patch" version string, throwing on failure.
 *
 * @throws std::invalid_argument if the string is not a valid semantic version.
 */
[[nodiscard]] inline semantic_version parse_strict(std::string_view s) {
    auto opt = parse(s);
    if (!opt) {
        throw std::invalid_argument(
            std::string("Not a valid semantic version: '") + std::string(s) + "'");
    }
    return *opt;
}

}  // namespace semver

}  // namespace pubgrub
