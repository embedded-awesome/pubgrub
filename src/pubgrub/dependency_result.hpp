#pragma once

#include <pubgrub/concepts.hpp>

#include <string>
#include <variant>
#include <vector>

namespace pubgrub {

/**
 * @brief Structured result returned by providers that support dependency availability signalling.
 *
 * Analogous to Rust's `Dependencies::Available` / `Dependencies::Unavailable(M)`.
 *
 * Providers that wish to signal that a package's dependencies cannot be retrieved (e.g. due to
 * network unavailability, an unsupported format, or permission errors) should return
 * `dependency_result<Req>::unavailable(reason)` from their `get_dependencies` method.
 *
 * Example:
 * @code
 * pubgrub::dependency_result<MyReq> get_dependencies(const MyReq& req) const {
 *     if (!cache.has(req))
 *         return pubgrub::dependency_result<MyReq>::unavailable("not in cache");
 *     return pubgrub::dependency_result<MyReq>::available(cache.deps_of(req));
 * }
 * @endcode
 */
template <requirement Req>
class dependency_result {
public:
    struct available_tag {
        std::vector<Req> requirements;
    };
    struct unavailable_tag {
        std::string reason;
    };

private:
    using data_type = std::variant<available_tag, unavailable_tag>;
    data_type _data;

    explicit dependency_result(data_type data)
        : _data(std::move(data)) {}

public:
    /// Create a result representing successfully retrieved dependencies.
    static dependency_result available(std::vector<Req> reqs) {
        return dependency_result{data_type{available_tag{std::move(reqs)}}};
    }

    /// Create a result representing unavailable dependencies with a reason string.
    static dependency_result unavailable(std::string reason) {
        return dependency_result{data_type{unavailable_tag{std::move(reason)}}};
    }

    /// Returns true if the dependencies are available.
    bool is_available() const noexcept { return std::holds_alternative<available_tag>(_data); }

    /// Returns the list of requirements (only valid when @ref is_available() is true).
    const std::vector<Req>& requirements() const noexcept {
        return std::get<available_tag>(_data).requirements;
    }

    /// Returns the unavailability reason string (only valid when @ref is_available() is false).
    const std::string& reason() const noexcept {
        return std::get<unavailable_tag>(_data).reason;
    }
};

}  // namespace pubgrub
