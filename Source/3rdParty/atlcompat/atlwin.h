/*
 * atlwin.h for GCC and Clang: ReactOS's windowing classes plus the message
 * map macros and helpers of Microsoft's ATL 8 that WTL 10, Project64 and
 * GLideN64's WTL 8.1 use.
 */
#pragma once

#include <atlbase.h>
#include_next <atlwin.h>
#include <olectl.h>

#ifndef __ATLWIN_H__
#define __ATLWIN_H__
#endif

namespace ATL
{
typedef CWindowImplBaseT<CWindow> CWindowImplBase;
typedef CDialogImplBaseT<CWindow> CDialogImplBase;

/* WM_COMMAND, WM_NOTIFY and the like, sent back to the control they came from */
inline LRESULT AtlReflectNotifications(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    HWND hWndChild = NULL;
    switch (uMsg)
    {
    case WM_COMMAND:
        if (lParam != 0)
        {
            hWndChild = (HWND)lParam;
        }
        break;
    case WM_NOTIFY:
        hWndChild = ((LPNMHDR)lParam)->hwndFrom;
        break;
    case WM_PARENTNOTIFY:
        switch (LOWORD(wParam))
        {
        case WM_CREATE:
        case WM_DESTROY:
            hWndChild = (HWND)lParam;
            break;
        default:
            hWndChild = ::GetDlgItem(hWnd, HIWORD(wParam));
            break;
        }
        break;
    case WM_DRAWITEM:
        if (wParam != 0)
        {
            hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
        }
        break;
    case WM_MEASUREITEM:
        if (wParam != 0)
        {
            hWndChild = ::GetDlgItem(hWnd, ((LPMEASUREITEMSTRUCT)lParam)->CtlID);
        }
        break;
    case WM_COMPAREITEM:
        if (wParam != 0)
        {
            hWndChild = ((LPCOMPAREITEMSTRUCT)lParam)->hwndItem;
        }
        break;
    case WM_DELETEITEM:
        if (wParam != 0)
        {
            hWndChild = ((LPDELETEITEMSTRUCT)lParam)->hwndItem;
        }
        break;
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
    case WM_HSCROLL:
    case WM_VSCROLL:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        hWndChild = (HWND)lParam;
        break;
    default:
        break;
    }
    if (hWndChild == NULL)
    {
        bHandled = FALSE;
        return 1;
    }
    return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
}

/* A reflected message the control does not handle gets the default handling */
inline BOOL AtlDefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult)
{
    switch (uMsg)
    {
    case OCM_COMMAND:
    case OCM_NOTIFY:
    case OCM_PARENTNOTIFY:
    case OCM_DRAWITEM:
    case OCM_MEASUREITEM:
    case OCM_COMPAREITEM:
    case OCM_DELETEITEM:
    case OCM_VKEYTOITEM:
    case OCM_CHARTOITEM:
    case OCM_HSCROLL:
    case OCM_VSCROLL:
    case OCM_CTLCOLORBTN:
    case OCM_CTLCOLORDLG:
    case OCM_CTLCOLOREDIT:
    case OCM_CTLCOLORLISTBOX:
    case OCM_CTLCOLORMSGBOX:
    case OCM_CTLCOLORSCROLLBAR:
    case OCM_CTLCOLORSTATIC:
        lResult = ::DefWindowProc(hWnd, uMsg - OCM__BASE, wParam, lParam);
        return TRUE;
    default:
        return FALSE;
    }
}

/* Notifications passed on to the parent window */
inline LRESULT AtlForwardNotifications(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    case WM_NOTIFY:
    case WM_PARENTNOTIFY:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
    case WM_HSCROLL:
    case WM_VSCROLL:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        return ::SendMessage(::GetParent(hWnd), uMsg, wParam, lParam);
    default:
        bHandled = FALSE;
        return 0;
    }
}
} // namespace ATL

#ifndef DECLARE_WND_CLASS
#define DECLARE_WND_CLASS(WndClassName) \
    DECLARE_WND_CLASS_EX(WndClassName, CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, COLOR_WINDOW)
#endif

#ifndef DECLARE_WND_SUPERCLASS
#define DECLARE_WND_SUPERCLASS(WndClassName, OrigWndClassName)                     \
    static ATL::CWndClassInfo & GetWndClassInfo()                                  \
    {                                                                              \
        static ATL::CWndClassInfo wc = {                                           \
            {sizeof(WNDCLASSEX), 0, StartWindowProc, 0, 0, NULL, NULL, NULL, NULL, \
             NULL, WndClassName, NULL},                                            \
            OrigWndClassName, NULL, NULL, TRUE, 0, _T("")};                        \
        return wc;                                                                 \
    }
#endif

#ifndef DECLARE_EMPTY_MSG_MAP
#define DECLARE_EMPTY_MSG_MAP()                                                 \
public:                                                                         \
    BOOL ProcessWindowMessage(HWND, UINT, WPARAM, LPARAM, LRESULT &, DWORD = 0) \
    {                                                                           \
        return FALSE;                                                           \
    }
#endif

#ifndef CHAIN_MSG_MAP_MEMBER
#define CHAIN_MSG_MAP_MEMBER(theChainMember)                                              \
    {                                                                                     \
        if (theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult))     \
            return TRUE;                                                                  \
    }
#endif

#ifndef CHAIN_MSG_MAP_ALT
#define CHAIN_MSG_MAP_ALT(theChainClass, msgMapID)                                                \
    {                                                                                             \
        if (theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID))   \
            return TRUE;                                                                          \
    }
#endif

#ifndef CHAIN_MSG_MAP_ALT_MEMBER
#define CHAIN_MSG_MAP_ALT_MEMBER(theChainMember, msgMapID)                                        \
    {                                                                                             \
        if (theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID))   \
            return TRUE;                                                                          \
    }
#endif

#ifndef NOTIFY_ID_HANDLER
#define NOTIFY_ID_HANDLER(id, func)                                       \
    if (uMsg == WM_NOTIFY && (id) == ((LPNMHDR)lParam)->idFrom)           \
    {                                                                     \
        bHandled = TRUE;                                                  \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);           \
        if (bHandled)                                                     \
            return TRUE;                                                  \
    }
#endif

#ifndef REFLECT_NOTIFICATIONS
#define REFLECT_NOTIFICATIONS()                                                             \
    {                                                                                       \
        bHandled = TRUE;                                                                    \
        lResult = ATL::AtlReflectNotifications(hWnd, uMsg, wParam, lParam, bHandled);       \
        if (bHandled)                                                                       \
            return TRUE;                                                                    \
    }
#endif

#ifndef DEFAULT_REFLECTION_HANDLER
#define DEFAULT_REFLECTION_HANDLER()                                                \
    if (ATL::AtlDefaultReflectionHandler(hWnd, uMsg, wParam, lParam, lResult))      \
        return TRUE;
#endif

#ifndef FORWARD_NOTIFICATIONS
#define FORWARD_NOTIFICATIONS()                                                             \
    {                                                                                       \
        bHandled = TRUE;                                                                    \
        lResult = ATL::AtlForwardNotifications(hWnd, uMsg, wParam, lParam, bHandled);       \
        if (bHandled)                                                                       \
            return TRUE;                                                                    \
    }
#endif

/* Handlers of the messages that REFLECT_NOTIFICATIONS sends back to the control */
#ifndef REFLECTED_COMMAND_HANDLER
#define REFLECTED_COMMAND_HANDLER(id, code, func)                                     \
    if (uMsg == OCM_COMMAND && (id) == LOWORD(wParam) && (code) == HIWORD(wParam))     \
    {                                                                                  \
        bHandled = TRUE;                                                               \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);        \
        if (bHandled)                                                                  \
            return TRUE;                                                               \
    }
#define REFLECTED_COMMAND_ID_HANDLER(id, func)                                         \
    if (uMsg == OCM_COMMAND && (id) == LOWORD(wParam))                                 \
    {                                                                                  \
        bHandled = TRUE;                                                               \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);        \
        if (bHandled)                                                                  \
            return TRUE;                                                               \
    }
#define REFLECTED_COMMAND_CODE_HANDLER(code, func)                                     \
    if (uMsg == OCM_COMMAND && (code) == HIWORD(wParam))                               \
    {                                                                                  \
        bHandled = TRUE;                                                               \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);        \
        if (bHandled)                                                                  \
            return TRUE;                                                               \
    }
#define REFLECTED_COMMAND_RANGE_HANDLER(idFirst, idLast, func)                                    \
    if (uMsg == OCM_COMMAND && LOWORD(wParam) >= (idFirst) && LOWORD(wParam) <= (idLast))         \
    {                                                                                             \
        bHandled = TRUE;                                                                          \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);                   \
        if (bHandled)                                                                             \
            return TRUE;                                                                          \
    }
#define REFLECTED_COMMAND_RANGE_CODE_HANDLER(idFirst, idLast, code, func)                                                      \
    if (uMsg == OCM_COMMAND && (code) == HIWORD(wParam) && LOWORD(wParam) >= (idFirst) && LOWORD(wParam) <= (idLast))          \
    {                                                                                                                          \
        bHandled = TRUE;                                                                                                       \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);                                                \
        if (bHandled)                                                                                                          \
            return TRUE;                                                                                                       \
    }
#define REFLECTED_NOTIFY_HANDLER(id, cd, func)                                                        \
    if (uMsg == OCM_NOTIFY && (id) == ((LPNMHDR)lParam)->idFrom && (cd) == ((LPNMHDR)lParam)->code)    \
    {                                                                                                 \
        bHandled = TRUE;                                                                              \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);                                       \
        if (bHandled)                                                                                 \
            return TRUE;                                                                              \
    }
#define REFLECTED_NOTIFY_ID_HANDLER(id, func)                             \
    if (uMsg == OCM_NOTIFY && (id) == ((LPNMHDR)lParam)->idFrom)          \
    {                                                                     \
        bHandled = TRUE;                                                  \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);           \
        if (bHandled)                                                     \
            return TRUE;                                                  \
    }
#define REFLECTED_NOTIFY_CODE_HANDLER(cd, func)                           \
    if (uMsg == OCM_NOTIFY && (cd) == ((LPNMHDR)lParam)->code)            \
    {                                                                     \
        bHandled = TRUE;                                                  \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);           \
        if (bHandled)                                                     \
            return TRUE;                                                  \
    }
#define REFLECTED_NOTIFY_RANGE_HANDLER(idFirst, idLast, func)                                                 \
    if (uMsg == OCM_NOTIFY && ((LPNMHDR)lParam)->idFrom >= (idFirst) && ((LPNMHDR)lParam)->idFrom <= (idLast)) \
    {                                                                                                         \
        bHandled = TRUE;                                                                                      \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);                                               \
        if (bHandled)                                                                                         \
            return TRUE;                                                                                      \
    }
#define REFLECTED_NOTIFY_RANGE_CODE_HANDLER(idFirst, idLast, cd, func)                                                                              \
    if (uMsg == OCM_NOTIFY && (cd) == ((LPNMHDR)lParam)->code && ((LPNMHDR)lParam)->idFrom >= (idFirst) && ((LPNMHDR)lParam)->idFrom <= (idLast)) \
    {                                                                                                                                               \
        bHandled = TRUE;                                                                                                                            \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);                                                                                     \
        if (bHandled)                                                                                                                               \
            return TRUE;                                                                                                                            \
    }
#endif
