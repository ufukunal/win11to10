#include "PackagedApps.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <iostream>

int main() {
  if (!noya::PackagedAppsModelSelfTest()) {
    std::cerr << "AppsFolder model test failed.\n";
    return 1;
  }
  const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(init)) return 1;
  const auto apps = noya::ScanAppsFolder();
  std::cout << "Windows AppsFolder entries visible to this test account: "
            << apps.size() << "\n";
  if (apps.empty()) {
    // Non-interactive build accounts can have no installed Store apps.
    // The Start menu still offers classic shortcuts and fixed Windows apps.
    CoUninitialize();
    return 0;
  }
  unsigned parsed = 0;
  const auto limit = std::min<std::size_t>(apps.size(), 15);
  for (std::size_t i = 0; i < limit; ++i) {
    PIDLIST_ABSOLUTE item{};
    const HRESULT hr = SHParseDisplayName(
        apps[i].parsingName.c_str(), nullptr, &item, 0, nullptr);
    if (SUCCEEDED(hr) && item) ++parsed;
    if (item) CoTaskMemFree(item);
  }
  CoUninitialize();
  if (parsed == 0) {
    std::cerr << "No enumerated AppsFolder item could be parsed for launch.\n";
    return 1;
  }
  std::cout << "Re-parsable AppsFolder launch targets: " << parsed
            << " of " << limit << " sampled.\n";
  return 0;
}
