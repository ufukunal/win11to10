#include "StartupManager.h"

#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr wchar_t kRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kBackup[] = L"Software\\NoyaPLANET\\11to10\\DisabledStartup";

struct Guard {
  HKEY run{};
  HKEY backup{};
  std::wstring name;
  ~Guard() {
    if (run) {
      RegDeleteValueW(run, name.c_str());
      RegCloseKey(run);
    }
    if (backup) {
      RegDeleteValueW(backup, name.c_str());
      RegCloseKey(backup);
    }
  }
};

bool ReadValue(HKEY key, const std::wstring& name, DWORD& type,
               std::vector<BYTE>& value) {
  DWORD size = 0;
  if (RegQueryValueExW(key, name.c_str(), nullptr, &type,
                       nullptr, &size) != ERROR_SUCCESS) return false;
  value.resize(size);
  return RegQueryValueExW(key, name.c_str(), nullptr, &type,
                          value.data(), &size) == ERROR_SUCCESS;
}

bool TryRoundTrip(DWORD type) {
  Guard guard;
  guard.name = L"NoyaCI_RoundTrip_" + std::to_wstring(GetCurrentProcessId()) +
               L"_" + std::to_wstring(GetTickCount64()) + L"_" +
               std::to_wstring(type);
  if (RegCreateKeyExW(HKEY_CURRENT_USER, kRun, 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                      nullptr, &guard.run, nullptr) != ERROR_SUCCESS)
    return false;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, kBackup, 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                      nullptr, &guard.backup, nullptr) != ERROR_SUCCESS)
    return false;

  DWORD existingType{};
  std::vector<BYTE> existing;
  if (ReadValue(guard.run, guard.name, existingType, existing) ||
      ReadValue(guard.backup, guard.name, existingType, existing)) return false;

  const std::wstring command = type == REG_EXPAND_SZ
      ? L"%SystemRoot%\\System32\\notepad.exe" : L"notepad.exe";
  const DWORD bytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
  if (RegSetValueExW(guard.run, guard.name.c_str(), 0, type,
                     reinterpret_cast<const BYTE*>(command.c_str()),
                     bytes) != ERROR_SUCCESS) return false;

  std::wstring error;
  if (!noya::ChangeCurrentUserStartup(guard.name, true, error)) {
    std::wcerr << L"Could not disable: " << error << L"\n";
    return false;
  }

  DWORD backedType{};
  std::vector<BYTE> backed;
  if (ReadValue(guard.run, guard.name, existingType, existing) ||
      !ReadValue(guard.backup, guard.name, backedType, backed) ||
      backedType != type || backed.size() != bytes) return false;

  if (!noya::ChangeCurrentUserStartup(guard.name, false, error)) {
    std::wcerr << L"Could not restore: " << error << L"\n";
    return false;
  }
  DWORD restoredType{};
  std::vector<BYTE> restored;
  return ReadValue(guard.run, guard.name, restoredType, restored) &&
         restoredType == type && restored == backed &&
         !ReadValue(guard.backup, guard.name, existingType, existing);
}
}  // namespace

int main() {
  // This test MUST be enabled only on a disposable GitHub-hosted Windows
  // runner; it creates and deletes harmless test Run entries under HKCU.
  if (GetEnvironmentVariableW(L"GITHUB_ACTIONS", nullptr, 0) == 0) {
    std::cerr << "Live HKCU integration tests require GitHub Actions runner.\n";
    return 1;
  }
  const bool ok = TryRoundTrip(REG_SZ) && TryRoundTrip(REG_EXPAND_SZ);
  std::cout << (ok ? "Startup HKCU round-trip tests passed.\n"
                   : "Startup HKCU round-trip tests FAILED.\n");
  return ok ? 0 : 1;
}
