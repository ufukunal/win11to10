#pragma once
#include <string>
#include <vector>

namespace noya {
// Read-only logon/boot scheduled-task inventory. Tasks are never changed.
struct ScheduledStartupInventory {
  std::vector<std::wstring> entries;
  std::wstring diagnostics;
};
ScheduledStartupInventory ScanScheduledStartup();
bool ScheduledStartupModelSelfTest();
}  // namespace noya
