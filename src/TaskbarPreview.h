#pragma once
#include <windows.h>
namespace noya {
// Sent by the taskbar preview to the dashboard after a real user click.
inline constexpr UINT kRequestStartMenuPreview = WM_APP + 17;
void ShowTaskbarPreview(HWND dashboardWindow);
}  // namespace noya
