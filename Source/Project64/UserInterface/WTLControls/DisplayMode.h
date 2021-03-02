#pragma once

#include <type_traits>

enum class DisplayMode
{
    None = 0,
    ShowHexIdent = 1 << 0,
    ZeroExtend = 1 << 1,
    AllHex = ShowHexIdent | ZeroExtend,
};

inline DisplayMode operator |(DisplayMode lhs, DisplayMode rhs)
{
    using T = std::underlying_type<DisplayMode>::type;
    return static_cast<DisplayMode>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline DisplayMode& operator |=(DisplayMode& lhs, DisplayMode rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline DisplayMode operator &(DisplayMode lhs, DisplayMode rhs)
{
    using T = std::underlying_type<DisplayMode>::type;
    return static_cast<DisplayMode>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline DisplayMode& operator &=(DisplayMode& lhs, DisplayMode rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline DisplayMode operator ^(DisplayMode lhs, DisplayMode rhs)
{
    using T = std::underlying_type<DisplayMode>::type;
    return static_cast<DisplayMode>((static_cast<T>(lhs) ^ static_cast<T>(rhs)) & static_cast<T>(DisplayMode::AllHex));
}

inline DisplayMode& operator ^=(DisplayMode& lhs, DisplayMode rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

inline DisplayMode operator ~(DisplayMode lhs)
{
    using T = std::underlying_type<DisplayMode>::type;
    return static_cast<DisplayMode>(~static_cast<T>(lhs) & static_cast<T>(DisplayMode::AllHex));
}
