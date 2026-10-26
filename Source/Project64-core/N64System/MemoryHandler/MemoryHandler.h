#pragma once
#include <stdint.h>

// The recompilers call these through the vtable (Read32 first, then Write32).
// MSVC makes __interface methods pure virtual implicitly; other compilers
// need it written out.
__interface MemoryHandler
{
    virtual bool Read32(uint32_t Address, uint32_t & Value) = 0;
    virtual bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask) = 0;
};