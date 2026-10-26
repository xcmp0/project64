/*
 * Forced into every Project64 source file by the CMake build (GCC, Clang).
 *
 * The sources are written against MSVC, whose headers pull in much of the C
 * library and the SSE intrinsics implicitly, and which provides a few names of
 * its own. This header supplies the same, so the upstream files build
 * unchanged: the headers MSVC includes implicitly, and POSIX stand-ins for the
 * MSVC-only names the code uses.
 */
#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

#ifdef __cplusplus
#include <algorithm>
#endif

#ifndef _WIN32
#include <signal.h>
#include <stdarg.h>
#include <strings.h>

#ifdef __cplusplus
/* Defined in Common/Platform.cpp */
int _vscprintf(const char * format, va_list pargs);
#endif

#ifndef __debugbreak
#define __debugbreak() raise(SIGTRAP)
#endif

static inline void * _aligned_malloc(size_t size, size_t alignment)
{
    void * ptr = NULL;
    return posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;
}

static inline void _aligned_free(void * ptr)
{
    free(ptr);
}
#endif

/* MSVC source annotation (sal.h) */
#ifndef _Printf_format_string_
#define _Printf_format_string_
#endif

/* MSVC keyword */
#if !defined(_MSC_VER) && !defined(__interface)
#define __interface struct
#endif
