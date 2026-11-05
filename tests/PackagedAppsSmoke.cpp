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
    if (noya::CanResolveAppsFolderItem(apps[i].parsingName)) {
      ++parsed;
    } else if (i < 3) {
      std::wcout << L"Could not resolve (up to 160 characters): "
                 << apps[i].parsingName.substr(0, 160) << L"\n";
    }
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
