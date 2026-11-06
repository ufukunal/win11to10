#include "LoginLaunch.h"
#include <windows.h>
#include <iostream>
#include <string>

int main() {
  wchar_t ci[8]{};
  if (GetEnvironmentVariableW(L"GITHUB_ACTIONS", ci, 8) == 0 ||
      std::wstring(ci) != L"true") {
    std::cerr << "Registry integration test requires disposable GitHub Actions runner.\n";
    return 2;
  }
  if (!noya::LoginLaunchModelSelfTest()) return 3;
  std::wstring error;
  auto before = noya::InspectLoginLaunch(error);
  if (before != noya::LoginLaunchState::Disabled) {
    std::wcerr << L"Unexpected pre-existing login entry: " << error << L"\n";
    return 4;
  }
  if (!noya::SetLoginLaunch(true, error)) {
    std::wcerr << error << L"\n";
    return 5;
  }
  const bool enabled = noya::InspectLoginLaunch(error) ==
                       noya::LoginLaunchState::Enabled;
  const bool removed = noya::SetLoginLaunch(false, error);
  const bool disabled = noya::InspectLoginLaunch(error) ==
                        noya::LoginLaunchState::Disabled;
  if (!enabled || !removed || !disabled) {
    std::wcerr << L"Login registration round trip failed: " << error << L"\n";
    return 6;
  }
  std::cout << "User login launch registration round trip passed.\n";
  return 0;
}
