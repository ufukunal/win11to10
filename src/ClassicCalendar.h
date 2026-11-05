#pragma once
#include <windows.h>

namespace noya {
// Own, on-demand classic calendar window. It does NOT replace Windows 11
// taskbar clock, system notification flyout or third-party tray behavior.
void ShowClassicCalendar(HWND parent);
}  // namespace noya
