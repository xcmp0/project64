/*
 * atlbase.h for GCC and Clang: ReactOS's ATL (Source/3rdParty/ReactOS-ATL)
 * plus the parts of Microsoft's ATL 8 that WTL 10 and Project64 use.
 * This directory comes before ReactOS-ATL in the include path.
 */
#pragma once

#include_next <atlbase.h>
#include <atlsimpcoll.h>
#include <shlwapi.h>
#include <string.h>
#include <wchar.h>

#ifndef __ATLBASE_H__
#define __ATLBASE_H__
#endif

/* WTL 10 needs ATL 8 or later; claim no more, so WTL asks for no more */
#ifndef _ATL_VER
#define _ATL_VER 0x0800
#endif

#ifndef ATLVERIFY
#ifdef _DEBUG
#define ATLVERIFY(expr) ATLASSERT(expr)
#else
#define ATLVERIFY(expr) ((void)(expr))
#endif
#endif

#ifndef ATLASSUME
#define ATLASSUME(expr) ATLASSERT(expr)
#endif

#ifndef DECLARE_TRACE_CATEGORY
#ifdef _DEBUG
#define DECLARE_TRACE_CATEGORY(name) extern ATL::CTraceCategory name;
#else
#define DECLARE_TRACE_CATEGORY(name) const DWORD_PTR name = 0;
#endif
#endif

#ifndef ATLTRACE2
#define ATLTRACE2(...) ((void)0)
#endif

namespace ATL
{
/* A resource given by name or by number */
class _U_STRINGorID
{
public:
    _U_STRINGorID(LPCTSTR lpString) :
        m_lpstr(lpString)
    {
    }
    _U_STRINGorID(UINT nID) :
        m_lpstr(MAKEINTRESOURCE(nID))
    {
    }
    LPCTSTR m_lpstr;
};

/* The CRT's bounded copies; ATL raises on failure, these assert and return the error */
namespace Checked
{
inline errno_t memcpy_s(void * dst, size_t dstSize, const void * src, size_t count)
{
    errno_t err = ::memcpy_s(dst, dstSize, src, count);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t memmove_s(void * dst, size_t dstSize, const void * src, size_t count)
{
    errno_t err = ::memmove_s(dst, dstSize, src, count);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t strcpy_s(char * dst, size_t size, const char * src)
{
    errno_t err = ::strcpy_s(dst, size, src);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t wcscpy_s(wchar_t * dst, size_t size, const wchar_t * src)
{
    errno_t err = ::wcscpy_s(dst, size, src);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t strncpy_s(char * dst, size_t size, const char * src, size_t count)
{
    errno_t err = ::strncpy_s(dst, size, src, count);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t wcsncpy_s(wchar_t * dst, size_t size, const wchar_t * src, size_t count)
{
    errno_t err = ::wcsncpy_s(dst, size, src, count);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t strcat_s(char * dst, size_t size, const char * src)
{
    errno_t err = ::strcat_s(dst, size, src);
    ATLASSERT(err == 0);
    return err;
}
inline errno_t wcscat_s(wchar_t * dst, size_t size, const wchar_t * src)
{
    errno_t err = ::wcscat_s(dst, size, src);
    ATLASSERT(err == 0);
    return err;
}
#ifdef _UNICODE
inline errno_t tcscpy_s(wchar_t * dst, size_t size, const wchar_t * src)
{
    return wcscpy_s(dst, size, src);
}
inline errno_t tcsncpy_s(wchar_t * dst, size_t size, const wchar_t * src, size_t count)
{
    return wcsncpy_s(dst, size, src, count);
}
inline errno_t tcscat_s(wchar_t * dst, size_t size, const wchar_t * src)
{
    return wcscat_s(dst, size, src);
}
#else
inline errno_t tcscpy_s(char * dst, size_t size, const char * src)
{
    return strcpy_s(dst, size, src);
}
inline errno_t tcsncpy_s(char * dst, size_t size, const char * src, size_t count)
{
    return strncpy_s(dst, size, src, count);
}
inline errno_t tcscat_s(char * dst, size_t size, const char * src)
{
    return strcat_s(dst, size, src);
}
#endif
} // namespace Checked

/* Elements on the stack up to t_nFixedBytes, on the heap beyond */
template <typename T, int t_nFixedBytes = 128>
class CTempBuffer
{
public:
    CTempBuffer() :
        m_p(NULL)
    {
    }
    explicit CTempBuffer(size_t nElements) :
        m_p(NULL)
    {
        Allocate(nElements);
    }
    ~CTempBuffer()
    {
        FreeHeap();
    }
    operator T *() const
    {
        return m_p;
    }
    T * operator->() const
    {
        return m_p;
    }
    T * Allocate(size_t nElements)
    {
        return AllocateBytes(nElements * sizeof(T));
    }
    T * AllocateBytes(size_t nBytes)
    {
        FreeHeap();
        m_p = nBytes > t_nFixedBytes ? static_cast<T *>(malloc(nBytes)) : reinterpret_cast<T *>(m_abFixedBuffer);
        return m_p;
    }

private:
    CTempBuffer(const CTempBuffer &);
    CTempBuffer & operator=(const CTempBuffer &);

    void FreeHeap()
    {
        if (m_p != NULL && m_p != reinterpret_cast<T *>(m_abFixedBuffer))
        {
            free(m_p);
        }
        m_p = NULL;
    }

    T * m_p;
    BYTE m_abFixedBuffer[t_nFixedBytes];
};

/* Version of the common controls in use (5 or 6) */
inline HRESULT AtlGetCommCtrlVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
    HINSTANCE hModule = ::LoadLibrary(TEXT("comctl32.dll"));
    if (hModule == NULL)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    HRESULT hr = S_OK;
    DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)(void *)::GetProcAddress(hModule, "DllGetVersion");
    if (pfnDllGetVersion != NULL)
    {
        DLLVERSIONINFO dvi = {sizeof(DLLVERSIONINFO)};
        hr = pfnDllGetVersion(&dvi);
        if (SUCCEEDED(hr))
        {
            *pdwMajor = dvi.dwMajorVersion;
            *pdwMinor = dvi.dwMinorVersion;
        }
    }
    else
    {
        *pdwMajor = 4;
        *pdwMinor = 0;
    }
    ::FreeLibrary(hModule);
    return hr;
}

/* Extended dialog template records (only the layouts) */
class _DialogSplitHelper
{
public:
    struct DLGTEMPLATEEX
    {
        WORD dlgVer;
        WORD signature;
        DWORD helpID;
        DWORD exStyle;
        DWORD style;
        WORD cDlgItems;
        short x;
        short y;
        short cx;
        short cy;
    };
    struct DLGITEMTEMPLATEEX
    {
        DWORD helpID;
        DWORD exStyle;
        DWORD style;
        short x;
        short y;
        short cx;
        short cy;
        DWORD id;
    };
};
} // namespace ATL

/* Windows Vista names of the SDK headers that MinGW-w64 lacks */
#ifndef PSM_SETNEXTTEXTW
#define PSM_SETNEXTTEXTW (WM_USER + 121)
#define PSM_SHOWWIZBUTTONS (WM_USER + 138)
#define PSM_ENABLEWIZBUTTONS (WM_USER + 139)
#define PSM_SETBUTTONTEXTW (WM_USER + 140)
#endif
#ifndef PSM_SETNEXTTEXT
#ifdef _UNICODE
#define PSM_SETNEXTTEXT PSM_SETNEXTTEXTW
#define PSM_SETBUTTONTEXT PSM_SETBUTTONTEXTW
#endif
#endif
#ifndef PSH_AEROWIZARD
#define PSH_AEROWIZARD 0x00004000
#endif
#ifndef PSH_RESIZABLE
#define PSH_RESIZABLE 0x04000000
#endif
#ifndef PSH_HEADERBITMAP
#define PSH_HEADERBITMAP 0x08000000
#endif
#ifndef PSH_NOMARGIN
#define PSH_NOMARGIN 0x10000000
#endif
#ifndef LM_GETIDEALSIZE
#define LM_GETIDEALSIZE (WM_USER + 0x301)
#endif

/* ATL 3 string conversions; with Unicode, T and W strings are the same */
#ifndef USES_CONVERSION
#define USES_CONVERSION ((void)0)
#endif
#ifdef _UNICODE
#ifndef T2W
#define T2W(s) ((LPWSTR)(s))
#define T2CW(s) ((LPCWSTR)(s))
#define T2OLE(s) ((LPOLESTR)(s))
#define T2COLE(s) ((LPCOLESTR)(s))
#define OLE2T(s) ((LPTSTR)(s))
#endif
#endif
