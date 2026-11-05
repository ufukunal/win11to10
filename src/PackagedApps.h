#pragma once
#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

namespace noya {
struct PackagedApp {
  std::wstring title;
  // Shell namespace parsing name obtained from Windows, not an executable path.
  std::wstring parsingName;
};

// Enumerates Windows AppsFolder on demand, including modern packaged apps.
// Requires COM to have been initialized on the calling UI thread.
std::vector<PackagedApp> ScanAppsFolder();

// Uses Windows Shell to open a selected AppsFolder item by PIDL.
// Does not start apps during enumeration or acquire admin privileges.
bool CanResolveAppsFolderItem(std::wstring_view parsingName);
bool LaunchAppsFolderItem(HWND owner, std::wstring_view parsingName);
bool PackagedAppsModelSelfTest();
} // namespace noya
