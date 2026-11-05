#pragma once
#include <windows.h>

namespace noya {
// Opens a modeless, opt-in manager for current-user Run entries only.
void ShowStartupManager(HWND owner);
}  // namespace noya
