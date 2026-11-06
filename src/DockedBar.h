#pragma once
#include <windows.h>

namespace noya {
// Experimental, user-enabled Windows desktop appbar. Reserves its own
// screen strip using documented Shell APIs. Never hides Explorer/taskbar.
bool OpenDockedBar(HWND dashboard);
void CloseDockedBar();
bool IsDockedBarOpen();
HWND DockedBarWindow();
// Pure layout calculation smoke, no windows or system settings changed.
bool DockedBarLayoutSelfTest();
void OnDockedBarExplorerRestart();
}  // namespace noya
