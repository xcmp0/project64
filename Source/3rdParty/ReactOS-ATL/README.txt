ReactOS ATL: the ATL template headers of ReactOS (https://reactos.org),
sdk/lib/atl at commit e46f4bb1a1ea7c722f941231565144fe0c286d3e (2026-10-25).

Four files are changed, in blocks marked "Project64", where WTL and
Project64 expect what Microsoft's ATL does: in atlwin.h CWindowImpl gets a
default window class, CWndClassInfo::Register registers superclasses and
automatic class names, CWindow::GetDlgItem returns a CWindow and CWindow gets
the RECT overloads of MoveWindow, ScreenToClient and SetWindowPos, and
CDialogImplBaseT::DialogProc returns what a dialog procedure returns, and
CWindowImplRoot gets GetCurrentMessage, IsMsgHandled and SetMsgHandled; in
atlbase.h CAtlModule initializes and deletes m_csStaticDataInitAndTypeInfo
and CComModule gets GetModuleInstance and GetResourceInstance; in
atlsimpstr.h CSimpleStringT gets GetBufferSetLength; in atlcomcli.h the
__uuidof members of CComPtr are enabled for Clang.

Licenses are given in each file: LGPL-2.1-or-later (COPYING.LIB),
LGPL-2.0-or-later, GPL-2.0-or-later (COPYING) or public domain; files
without a header fall under the ReactOS licence (COPYING).

Project64 uses these headers in the Clang (MinGW) build of its Windows user
interface instead of Microsoft's ATL. Additions that WTL needs live in
Source/3rdParty/atlcompat.
