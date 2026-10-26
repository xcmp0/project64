/* MSVC's <intrin.h>: GCC and Clang declare the x86 intrinsics in <x86intrin.h>. */
#pragma once

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif
