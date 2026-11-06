#include "ScheduledStartup.h"

#include <windows.h>
#include <objbase.h>

#include <iostream>

int main() {
  if (!noya::ScheduledStartupModelSelfTest()) return 1;
  const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) return 2;
  const auto result = noya::ScanScheduledStartup();
  CoUninitialize();
  // A fresh CI image may have no accessible user logon tasks.
  // Read-only collection must stay bounded and must not require elevation.
  if (result.entries.size() > 4096) return 3;
  std::wcout << L"Discovered boot/logon tasks: " << result.entries.size() << L"\n";
  if (!result.diagnostics.empty()) std::wcout << result.diagnostics << L"\n";
  return 0;
}
