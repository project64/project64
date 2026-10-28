#pragma once

#ifndef ASMJIT_ABI_NAMESPACE
#define ASMJIT_ABI_NAMESPACE _abi_1_9
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iterator>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>
// Build Mode
// ==========

// Detect ASMJIT_BUILD_DEBUG and ASMJIT_BUILD_RELEASE if not defined.
#if !defined(ASMJIT_BUILD_DEBUG) && !defined(ASMJIT_BUILD_RELEASE)
  #if !defined(NDEBUG)
    #define ASMJIT_BUILD_DEBUG
  #else
    #define ASMJIT_BUILD_RELEASE
  #endif
#endif

// Target Architecture Detection
// =============================

#if defined(_M_X64) || defined(__x86_64__)
  #define ASMJIT_ARCH_X86 64
#elif defined(_M_IX86) || defined(__X86__) || defined(__i386__)
  #define ASMJIT_ARCH_X86 32
#else
  #define ASMJIT_ARCH_X86 0
#endif

#if defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
# define ASMJIT_ARCH_ARM 64
#elif defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
  #define ASMJIT_ARCH_ARM 32
#else
  #define ASMJIT_ARCH_ARM 0
#endif

#if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
  #define ASMJIT_ARCH_MIPS 64
#elif defined(_MIPS_ARCH_MIPS32) || defined(_M_MRX000) || defined(__mips__)
  #define ASMJIT_ARCH_MIPS 32
#else
  #define ASMJIT_ARCH_MIPS 0
#endif

#define ASMJIT_ARCH_BITS (ASMJIT_ARCH_X86 | ASMJIT_ARCH_ARM | ASMJIT_ARCH_MIPS)
#if ASMJIT_ARCH_BITS == 0
  #undef ASMJIT_ARCH_BITS
  #if defined (__LP64__) || defined(_LP64)
    #define ASMJIT_ARCH_BITS 64
  #else
    #define ASMJIT_ARCH_BITS 32
  #endif
#endif

#if (defined(__ARMEB__))  || \
    (defined(__MIPSEB__)) || \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
  #define ASMJIT_ARCH_LE 0
  #define ASMJIT_ARCH_BE 1
#else
  #define ASMJIT_ARCH_LE 1
  #define ASMJIT_ARCH_BE 0
#endif

#if defined(ASMJIT_NO_FOREIGN)
  #if !ASMJIT_ARCH_X86 && !defined(ASMJIT_NO_X86)
    #define ASMJIT_NO_X86
  #endif

  #if !ASMJIT_ARCH_ARM && !defined(ASMJIT_NO_AARCH64)
    #define ASMJIT_NO_AARCH64
  #endif
#endif

// Compiler features detection macros.
#if defined(__clang__) && defined(__has_attribute)
  #define ASMJIT_CXX_HAS_ATTRIBUTE(NAME, CHECK) (__has_attribute(NAME))
#else
  #define ASMJIT_CXX_HAS_ATTRIBUTE(NAME, CHECK) (!(!(CHECK)))
#endif
#if !defined(ASMJIT_API)
  #define ASMJIT_API
#endif

#if !defined(ASMJIT_VARAPI)
  #define ASMJIT_VARAPI extern ASMJIT_API
#endif

//! \def ASMJIT_VIRTAPI
//!
//! This is basically a workaround. When using MSVC and marking class as DLL export everything gets exported, which
//! is unwanted in most projects. MSVC automatically exports typeinfo and vtable if at least one symbol of the class
//! is exported. However, GCC has some strange behavior that even if one or more symbol is exported it doesn't export
//! typeinfo unless the class itself is decorated with "visibility(default)" (i.e. ASMJIT_API).
#if !defined(_WIN32) && defined(__GNUC__)
  #define ASMJIT_VIRTAPI ASMJIT_API
#else
  #define ASMJIT_VIRTAPI
#endif

// Function attributes.
#if !defined(ASMJIT_BUILD_DEBUG) && defined(__GNUC__)
  #define ASMJIT_FORCE_INLINE inline __attribute__((__always_inline__))
#elif !defined(ASMJIT_BUILD_DEBUG) && defined(_MSC_VER)
  #define ASMJIT_FORCE_INLINE __forceinline
#else
  #define ASMJIT_FORCE_INLINE inline
#endif

#if defined(__GNUC__)
  #define ASMJIT_NOINLINE __attribute__((__noinline__))
  #define ASMJIT_NORETURN __attribute__((__noreturn__))
#elif defined(_MSC_VER)
  #define ASMJIT_NOINLINE __declspec(noinline)
  #define ASMJIT_NORETURN __declspec(noreturn)
#else
  #define ASMJIT_NOINLINE
  #define ASMJIT_NORETURN
#endif

// Type alignment (not allowed by C++11 'alignas' keyword).
#if defined(__GNUC__)
  #define ASMJIT_ALIGN_TYPE(TYPE, N) __attribute__((__aligned__(N))) TYPE
#elif defined(_MSC_VER)
  #define ASMJIT_ALIGN_TYPE(TYPE, N) __declspec(align(N)) TYPE
#else
  #define ASMJIT_ALIGN_TYPE(TYPE, N) TYPE
#endif

//! \def ASMJIT_MAY_ALIAS
//!
//! Expands to `__attribute__((__may_alias__))` if supported.
#if defined(__GNUC__)
  #define ASMJIT_MAY_ALIAS __attribute__((__may_alias__))
#else
  #define ASMJIT_MAY_ALIAS
#endif

#if defined(__clang_major__) && __clang_major__ >= 4 && !defined(_DOXYGEN)
  // NOTE: Clang allows to apply this attribute to function arguments, which is what we want. Once GCC decides to
  // support this use, we will enable it for GCC as well. However, until that, it will be clang only, which is
  // what we need for static analysis.
  #define ASMJIT_NONNULL(FUNCTION_ARGUMENT) FUNCTION_ARGUMENT __attribute__((__nonnull__))
#else
  #define ASMJIT_NONNULL(FUNCTION_ARGUMENT) FUNCTION_ARGUMENT
#endif

//! \def ASMJIT_LIKELY(...)
//!
//! Condition is likely to be taken (mostly error handling and edge cases).

//! \def ASMJIT_UNLIKELY(...)
//!
//! Condition is unlikely to be taken (mostly error handling and edge cases).
#if defined(__GNUC__)
  #define ASMJIT_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
  #define ASMJIT_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
  #define ASMJIT_LIKELY(...) (__VA_ARGS__)
  #define ASMJIT_UNLIKELY(...) (__VA_ARGS__)
#endif

// Utilities.
#define ASMJIT_ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define ASMJIT_BEGIN_NAMESPACE                                              \
    namespace asmjit { inline namespace ASMJIT_ABI_NAMESPACE {                \
      __pragma(warning(push))                                                 \
      __pragma(warning(disable: 4127))  /* conditional expression is const */ \
      __pragma(warning(disable: 4201))  /* nameless struct/union */
#define ASMJIT_END_NAMESPACE                                                \
      __pragma(warning(pop))                                                  \
    }}
#endif

#define ASMJIT_BEGIN_SUB_NAMESPACE(NAMESPACE)                                 \
  ASMJIT_BEGIN_NAMESPACE                                                      \
  namespace NAMESPACE {

#define ASMJIT_END_SUB_NAMESPACE                                              \
  }                                                                           \
  ASMJIT_END_NAMESPACE
#define ASMJIT_NONCOPYABLE(Type)                                              \
    Type(const Type& other) = delete;                                         \
    Type& operator=(const Type& other) = delete;

//! \def ASMJIT_DEFINE_ENUM_FLAGS(T)
//!
//! Defines bit operations for enumeration flags.
#ifdef _DOXYGEN
  #define ASMJIT_DEFINE_ENUM_FLAGS(T)
#else
  #define ASMJIT_DEFINE_ENUM_FLAGS(T)                                         \
    static ASMJIT_FORCE_INLINE constexpr T operator~(T a) noexcept {          \
      return T(~(std::underlying_type<T>::type)(a));                          \
    }                                                                         \
                                                                              \
    static ASMJIT_FORCE_INLINE constexpr T operator|(T a, T b) noexcept {     \
      return T((std::underlying_type<T>::type)(a) |                           \
              (std::underlying_type<T>::type)(b));                            \
    }                                                                         \
    static ASMJIT_FORCE_INLINE constexpr T operator&(T a, T b) noexcept {     \
      return T((std::underlying_type<T>::type)(a) &                           \
              (std::underlying_type<T>::type)(b));                            \
    }                                                                         \
    static ASMJIT_FORCE_INLINE constexpr T operator^(T a, T b) noexcept {     \
      return T((std::underlying_type<T>::type)(a) ^                           \
              (std::underlying_type<T>::type)(b));                            \
    }                                                                         \
                                                                              \
    static ASMJIT_FORCE_INLINE T& operator|=(T& a, T b) noexcept {            \
      a = T((std::underlying_type<T>::type)(a) |                              \
            (std::underlying_type<T>::type)(b));                              \
      return a;                                                               \
    }                                                                         \
    static ASMJIT_FORCE_INLINE T& operator&=(T& a, T b) noexcept {            \
      a = T((std::underlying_type<T>::type)(a) &                              \
            (std::underlying_type<T>::type)(b));                              \
      return a;                                                               \
    }                                                                         \
    static ASMJIT_FORCE_INLINE T& operator^=(T& a, T b) noexcept {            \
      a = T((std::underlying_type<T>::type)(a) ^                              \
            (std::underlying_type<T>::type)(b));                              \
      return a;                                                               \
    }
#endif

