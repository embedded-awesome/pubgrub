#pragma once

#include <neo/invoke.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>

namespace pubgrub {

namespace detail {

template <typename B>
concept boolean = requires(const B b) {
    {b ? 0 : 0};
};

template <typename T>
concept equality_comparable = requires(const T& value) {
    { value == value } -> boolean;
    { value != value } -> boolean;
};

template <typename From, typename To>
concept decays_to = std::same_as<std::decay_t<From>, To>;

template <typename Iter, typename Type>
concept iterator_of
    = std::input_iterator<Iter> && std::convertible_to<std::iter_value_t<Iter>, Type>;

template <typename R, typename T>
concept range_of = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

template <typename Opt, typename Type>
concept optional_like = boolean<Opt> && requires(const Opt what) {
    { *what } -> std::convertible_to<Type>;
};

template <typename Allocator, typename Type>
using rebind_alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<Type>;

template <typename T>
constexpr decltype(auto) key_of_impl(const T& value) noexcept {
    return neo::invoke(&T::key, value);
}

}  // namespace detail

template <typename T>
concept key = std::totally_ordered<T> && std::semiregular<T>;

template <typename T>
concept keyed = requires(const T item) {
    { neo::invoke(&std::remove_cvref_t<T>::key, item) }
    noexcept;
    requires key<std::remove_cvref_t<decltype(detail::key_of_impl(item))>>;
};

struct key_of_fn {
    constexpr decltype(auto) operator()(keyed auto const& value) const noexcept {
        return detail::key_of_impl(value);
    }
};

inline constexpr key_of_fn key_of;

template <keyed T>
using key_type_t = std::remove_cvref_t<decltype(key_of(std::declval<T&&>()))>;

template <typename T>
concept requirement = keyed<T> && requires(const T req) {
    { req.implied_by(req) } -> detail::boolean;
    { req.excludes(req) } -> detail::boolean;
    { req.intersection(req) } -> detail::optional_like<T>;
    { req.union_(req) } -> detail::optional_like<T>;
    { req.difference(req) } -> detail::optional_like<T>;
};

template <typename Iter>
concept requirement_iterator = std::input_iterator<Iter> && requirement<std::iter_value_t<Iter>>;

template <typename R>
concept requirement_range
    = std::ranges::input_range<R> && requirement<std::ranges::range_value_t<R>>;

/**
 * @brief Core provider concept: must be able to pick a best candidate and retrieve its
 *        dependencies.
 */
template <typename Provider, typename Req>
concept provider = requirement<Req> && requires(const Provider provider, const Req requirement) {
    { provider.best_candidate(requirement) } -> detail::boolean;
    { *provider.best_candidate(requirement) } -> std::convertible_to<const Req&>;
    { provider.requirements_of(requirement) } -> detail::range_of<Req>;
};

/**
 * @brief Optional extension: provider can signal that it wants to cancel the solve early.
 *
 * When the method returns true, the solver throws @ref pubgrub::solver_cancelled.
 *
 * Example:
 * @code
 * bool should_cancel() const { return timed_out(); }
 * @endcode
 */
template <typename Provider>
concept cancellable_provider = requires(const Provider& p) {
    { p.should_cancel() } -> detail::boolean;
};

/**
 * @brief Optional extension: provider can assign a priority to each undecided package.
 *
 * The solver will decide the package with the *highest* priority first.  This lets users
 * replicate PubGrub-rs' strategy of preferring packages involved in many conflicts or with few
 * matching versions.
 *
 * @param req           The candidate requirement for the undecided package.
 * @param conflict_count Number of conflicts this package has been involved in so far.
 *
 * The return value must support operator< (be less-than comparable).
 *
 * Example:
 * @code
 * int prioritize(const MyReq& req, std::size_t conflicts) const {
 *     return static_cast<int>(conflicts) - static_cast<int>(available_versions(req).size());
 * }
 * @endcode
 */
template <typename Provider, typename Req>
concept prioritizable_provider = provider<Provider, Req>
    && requires(const Provider& p, const Req& req, std::size_t count) {
           p.prioritize(req, count);
           { p.prioritize(req, count) < p.prioritize(req, count) } -> detail::boolean;
       };

/**
 * @brief Optional extension: provider returns a dependency_result from `get_dependencies`,
 *        which can signal that dependencies are unavailable with a custom reason string.
 *
 * When used, the solver calls `get_dependencies` instead of `requirements_of`.
 * The `requirements_of` method is still required by the base @ref provider concept.
 *
 * The return type must provide:
 *   - `bool is_available() const`   – true if deps were retrieved
 *   - `requirements()`              – iterable range of Req (valid when is_available())
 *   - `reason()`                    – convertible to std::string_view (valid when !is_available())
 *
 * See @ref pubgrub::dependency_result for a ready-made return type.
 *
 * Example:
 * @code
 * pubgrub::dependency_result<MyReq> get_dependencies(const MyReq& req) const {
 *     if (!network_ok)
 *         return pubgrub::dependency_result<MyReq>::unavailable("network unavailable");
 *     return pubgrub::dependency_result<MyReq>::available(lookup(req));
 * }
 * @endcode
 */
template <typename Provider, typename Req>
concept provider_with_dependency_info = provider<Provider, Req>
    && requires(const Provider& p, const Req& req) {
           p.get_dependencies(req);
           { p.get_dependencies(req).is_available() } -> detail::boolean;
           p.get_dependencies(req).requirements();
           p.get_dependencies(req).reason();
       };

}  // namespace pubgrub
