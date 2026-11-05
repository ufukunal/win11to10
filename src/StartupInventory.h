#pragma once
#include <string>
#include <vector>

namespace noya {
struct StartupInventory {
  std::vector<std::wstring> currentUserRun;
  std::vector<std::wstring> allUsersRun;
  std::vector<std::wstring> currentUserShortcuts;
  std::vector<std::wstring> allUsersShortcuts;
  std::wstring diagnostics;
};
// Read-only inventory. "Registered" does not mean "enabled at boot".
// Individual changes are only supported by StartupManager for HKCU Run.
StartupInventory ScanStartupInventory();
bool StartupInventoryModelSelfTest();
}  // namespace noya
