#pragma once
#include <string>
#include <vector>
namespace noya {
struct InstalledApp {
  std::wstring title;
  std::wstring shortcut;
};

// Reads .lnk/.url shortcuts from current-user and common Start Menu folders.
// Only invoked when a Start menu preview is opened. No background indexer.
std::vector<InstalledApp> ScanInstalledApps();
bool InstalledAppModelSelfTest();
}  // namespace noya
