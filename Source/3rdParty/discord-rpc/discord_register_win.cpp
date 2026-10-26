#include "discord_rpc.h"
#include "discord_register.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMCX
#define NOSERVICE
#define NOIME
#include <windows.h>
#include <psapi.h>
#include <cwchar>
#include <cstdio>

/**
 * Updated fixes for MinGW and WinXP
 * This block is written the way it does not involve changing the rest of the code
 * Checked to be compiling
 * 1) strsafe.h belongs to Windows SDK and cannot be added to MinGW
 * #include guarded, functions redirected to <string.h> substitutes
 * 2) RegSetKeyValueW and LSTATUS are not declared in <winreg.h>
 * The entire function is rewritten
 */
#ifdef __MINGW32__
/// strsafe.h fixes
static HRESULT StringCbPrintfW(LPWSTR pszDest, size_t cbDest, LPCWSTR pszFormat, ...)
{
    HRESULT ret;
    va_list va;
    va_start(va, pszFormat);
    cbDest /= 2; // Size is divided by 2 to convert from bytes to wide characters - causes segfault
                 // othervise
    ret = vsnwprintf(pszDest, cbDest, pszFormat, va);
    pszDest[cbDest - 1] = 0; // Terminate the string in case a buffer overflow; -1 will be returned
    va_end(va);
    return ret;
}
#else
#include <strsafe.h>
#endif // __MINGW32__

/// winreg.h fixes
#ifndef LSTATUS
#define LSTATUS LONG
#endif
#ifdef RegSetKeyValueW
#undefine RegSetKeyValueW
#endif
#define RegSetKeyValueW regset
static LSTATUS regset(HKEY hkey,
                      LPCWSTR subkey,
                      LPCWSTR name,
                      DWORD type,
                      const void* data,
                      DWORD len)
{
    return ERROR_SUCCESS;
}

static void Discord_RegisterW(const wchar_t* applicationId, const wchar_t* command)
{
    ;
}

extern "C" DISCORD_EXPORT void Discord_Register(const char* applicationId, const char* command)
{
    ;
}

extern "C" DISCORD_EXPORT void Discord_RegisterSteamGame(const char* applicationId,
                                                         const char* steamId)
{
    ;
}
