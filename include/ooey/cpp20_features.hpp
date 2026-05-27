#pragma once

/**
 * @file cpp20_features.hpp
 * @brief Conditional compilation macros and utilities for C++20 features.
 * 
 * This header provides safe wrappers and macros for using C++20 features
 * while maintaining compatibility with C++17.
 */

// Check for C++20 support
#if __cplusplus >= 202002L
    #define OOEY_CPP20 1
#else
    #define OOEY_CPP20 0
#endif

// ============================================================================
// Concepts (C++20 feature)
// ============================================================================
#if OOEY_CPP20
    #include <concepts>
    
    /**
     * @brief Use C++20 concepts for compile-time type constraints.
     * Example: template<std::integral T> void foo(T value);
     */
    #define OOEY_REQUIRES(concept) requires concept
    
#else
    /**
     * @brief Fallback: empty macro for C++17 (no concepts support)
     */
    #define OOEY_REQUIRES(concept)
#endif

// ============================================================================
// Ranges (C++20 feature)
// ============================================================================
#if OOEY_CPP20
    #include <ranges>
    
    /**
     * @brief Use std::ranges algorithms for cleaner range-based operations.
     * Example: std::ranges::sort(container);
     */
    #define OOEY_USE_RANGES 1
    
#else
    /**
     * @brief Fallback: use std:: algorithms for C++17
     */
    #define OOEY_USE_RANGES 0
#endif

// ============================================================================
// Spaceship operator (C++20 feature)
// ============================================================================
#if OOEY_CPP20
    /**
     * @brief Three-way comparison operator (<=>) available in C++20.
     * Example: auto cmp = (a <=> b);  // Returns std::strong_ordering
     */
    #define OOEY_HAS_SPACESHIP 1
    
#else
    #define OOEY_HAS_SPACESHIP 0
#endif

// ============================================================================
// Designated Initializers (C++20)
// ============================================================================
#if OOEY_CPP20
    /**
     * @brief Use designated initializers for cleaner struct/aggregate initialization.
     * Example: MyStruct s{.field1 = value1, .field2 = value2};
     */
    #define OOEY_HAS_DESIGNATED_INIT 1
    
#else
    #define OOEY_HAS_DESIGNATED_INIT 0
#endif

// ============================================================================
// Likely/Unlikely attributes (C++20)
// ============================================================================
#if OOEY_CPP20 && defined(__has_cpp_attribute)
    #if __has_cpp_attribute(likely)
        #define OOEY_LIKELY [[likely]]
        #define OOEY_UNLIKELY [[unlikely]]
        #define OOEY_HAS_ATTR_LIKELY 1
    #else
        #define OOEY_LIKELY
        #define OOEY_UNLIKELY
        #define OOEY_HAS_ATTR_LIKELY 0
    #endif
#else
    #define OOEY_LIKELY
    #define OOEY_UNLIKELY
    #define OOEY_HAS_ATTR_LIKELY 0
#endif

// ============================================================================
// [[nodiscard]] with message (C++20)
// ============================================================================
#if OOEY_CPP20
    /**
     * @brief Use [[nodiscard("message")]] for more informative compiler warnings.
     * In C++17, [[nodiscard]] exists but without custom message.
     */
    #define OOEY_NODISCARD_MSG(msg) [[nodiscard(msg)]]
#else
    /**
     * @brief Fallback: use [[nodiscard]] without message for C++17
     */
    #define OOEY_NODISCARD_MSG(msg) [[nodiscard]]
#endif

// ============================================================================
// Std library improvements (C++20)
// ============================================================================
#if OOEY_CPP20
    /**
     * @brief In C++20, many std functions have better const-correctness.
     * Example: std::string::contains() instead of find() != npos
     */
    #define OOEY_HAS_STD_CONTAINS 1
    
    /**
     * @brief std::format is available in C++20 (if supported by compiler).
     */
    #if __has_include(<format>)
        #define OOEY_HAS_STD_FORMAT 1
        #include <format>
    #else
        #define OOEY_HAS_STD_FORMAT 0
    #endif
    
#else
    #define OOEY_HAS_STD_CONTAINS 0
    #define OOEY_HAS_STD_FORMAT 0
#endif

// ============================================================================
// Utility: Version checking at runtime
// ============================================================================

namespace ooey {

/**
 * @brief Get the C++ standard version at runtime.
 * @return __cplusplus value (e.g., 202002L for C++20, 201703L for C++17)
 */
constexpr long cpp_version() {
    return __cplusplus;
}

/**
 * @brief Check if compiled with C++20 or later.
 * @return true if C++20 or later, false otherwise
 */
constexpr bool is_cpp20_or_later() {
    return __cplusplus >= 202002L;
}

/**
 * @brief Check if compiled with C++17 only.
 * @return true if compiled with exactly C++17, false otherwise
 */
constexpr bool is_cpp17_only() {
    return __cplusplus >= 201703L && __cplusplus < 202002L;
}

} // namespace ooey
