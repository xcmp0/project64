/*
 * atlcom.h for GCC and Clang: ReactOS's COM classes plus IObjectWithSiteImpl,
 * which WTL's print dialog uses.
 */
#pragma once

#include <atlbase.h>
#include_next <atlcom.h>
#include <ocidl.h>

namespace ATL
{
template <class T>
class ATL_NO_VTABLE IObjectWithSiteImpl :
    public IObjectWithSite
{
public:
    CComPtr<IUnknown> m_spUnkSite;

    STDMETHOD(SetSite)(IUnknown * pUnkSite)
    {
        m_spUnkSite = pUnkSite;
        return S_OK;
    }

    STDMETHOD(GetSite)(REFIID riid, void ** ppvSite)
    {
        if (ppvSite == NULL)
        {
            return E_POINTER;
        }
        *ppvSite = NULL;
        return m_spUnkSite ? m_spUnkSite->QueryInterface(riid, ppvSite) : E_FAIL;
    }
};
} // namespace ATL
