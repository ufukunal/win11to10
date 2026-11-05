#pragma once
#include <windows.h>
namespace noya {
// Native, read-only browsing window. It does not hook or replace Explorer.
// Opening a selected file invokes ShellExecute only after user interaction.
void ShowClassicFileBrowser(HWND dashboard);
bool ClassicFileBrowserModelSelfTest();
} // namespace noya
