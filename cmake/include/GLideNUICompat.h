/*
 * Forced into the GLideNUI-wtl sources by the CMake build (Clang, MinGW-w64).
 *
 * GLideNUI-wtl and its copy of WTL 8.1 are written against MSVC's CRT and
 * ATL headers. With those, the C library comes in before WTL looks at it:
 * otherwise atlapp.h takes the secure CRT error codes for missing and defines
 * a partial set that hides ERANGE and EILSEQ from errno.h. WTL 8.1 also calls
 * min and max as the macros of <windows.h>, which libstdc++ turns off
 * (NOMINMAX); these function templates stand in for them.
 */
#pragma once

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <type_traits>

template <class A, class B>
inline typename std::common_type<A, B>::type min(A a, B b)
{
    return a < b ? a : b;
}

template <class A, class B>
inline typename std::common_type<A, B>::type max(A a, B b)
{
    return a < b ? b : a;
}
#endif
