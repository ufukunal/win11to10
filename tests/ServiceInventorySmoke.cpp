#include "ServiceInventory.h"

#include <iostream>

int main() {
  // No SERVICE_CHANGE_CONFIG permissions and no service modification.
  // Runs on a disposable GitHub Windows environment as a read-only smoke test.
  if (!noya::ServiceSummarySelfTest()) return 1;
  const auto snapshot = noya::InspectWindowsServices();
  if (snapshot.total == 0 ||
      snapshot.total != snapshot.running + snapshot.stopped) {
    std::wcerr << L"Services unavailable: " << snapshot.warning << L"\n";
    return 1;
  }
  std::cout << "Read-only Windows services inventory passed ("
            << snapshot.total << " entries).\n";
  return 0;
}
