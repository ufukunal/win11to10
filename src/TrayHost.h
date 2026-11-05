#pragma once
#include <windows.h>

namespace noya {
// Only OUR icon is created. Other applications' notification icons remain
// managed by Windows Explorer and are never intercepted or enumerated.
inline constexpr UINT kTrayCallbackMessage = WM_APP + 55;
inline constexpr UINT kTrayShowDashboard = 710;
inline constexpr UINT kTrayOpenStart = 711;
inline constexpr UINT kTrayOpenTaskbar = 712;
inline constexpr UINT kTrayExit = 713;

bool AddOwnTrayIcon(HWND owner);
void RemoveOwnTrayIcon(HWND owner);
void ShowOwnTrayMenu(HWND owner);
}  // namespace noya
