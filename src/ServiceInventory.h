#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace noya {
struct ServiceInventory {
  unsigned total = 0;
  unsigned running = 0;
  unsigned stopped = 0;
  std::vector<std::wstring> sampleRunningNames;
  std::wstring warning;
};
// Uses only SC_MANAGER_ENUMERATE_SERVICE, no SERVICE_CHANGE_CONFIG.
ServiceInventory InspectWindowsServices();
bool ServiceSummarySelfTest();
}  // namespace noya
