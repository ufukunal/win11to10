#include "LoginLaunch.h"

#include <windows.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kName[] = L"NoyaPLANET11to10";
constexpr DWORD kMaxValueBytes = 65536;

struct Key {
  HKEY handle{};
  ~Key() { if (handle) RegCloseKey(handle); }
  Key() = default;
  Key(const Key&) = delete;
  Key& operator=(const Key&) = delete;
};

std::wstring CommandForPath(std::wstring_view path) {
  if (path.empty() || path.find(L'"') != std::wstring_view::npos) return {};
  return L"\"" + std::wstring(path) + L"\"";
}

bool OurCommand(std::wstring_view actual, std::wstring_view expected) {
  return !expected.empty() && actual == expected;
}

bool CurrentCommand(std::wstring& command, std::wstring& error) {
  std::array<wchar_t, 32768> path{};
  const DWORD count = GetModuleFileNameW(nullptr, path.data(),
                                         static_cast<DWORD>(path.size()));
  if (!count || count >= path.size()) {
    error = L"Uygulama dosyasının yolu okunamadı.";
    return false;
  }
  command = CommandForPath(std::wstring_view(path.data(), count));
  if (command.empty()) {
    error = L"Uygulama dosyasının yolu güvenli biçimde kullanılamıyor.";
    return false;
  }
  return true;
}

LoginLaunchState ReadEntry(HKEY key, std::wstring_view expected,
                           std::wstring& error) {
  DWORD type{}, bytes{};
  LONG code = RegQueryValueExW(key, kName, nullptr, &type, nullptr, &bytes);
  if (code == ERROR_FILE_NOT_FOUND) return LoginLaunchState::Disabled;
  if (code != ERROR_SUCCESS || bytes == 0 || bytes > kMaxValueBytes ||
      type != REG_SZ) {
    error = L"Mevcut başlangıç kaydı geçersiz veya okunamadı; üzerine yazılmadı.";
    return LoginLaunchState::Conflict;
  }
  std::vector<BYTE> data(bytes);
  code = RegQueryValueExW(key, kName, nullptr, &type, data.data(), &bytes);
  if (code != ERROR_SUCCESS || type != REG_SZ ||
      bytes < sizeof(wchar_t) || bytes % sizeof(wchar_t) != 0 ||
      data[bytes - 1] != 0 || data[bytes - 2] != 0) {
    error = L"Mevcut başlangıç kaydı çözümlenemedi; üzerine yazılmadı.";
    return LoginLaunchState::Conflict;
  }
  const auto* chars = reinterpret_cast<const wchar_t*>(data.data());
  const std::wstring value(chars, bytes / sizeof(wchar_t) - 1);
  if (value.find(L'\0') != std::wstring::npos ||
      !OurCommand(value, expected)) {
    error = L"Aynı adlı farklı bir başlangıç kaydı var; korunuyor.";
    return LoginLaunchState::Conflict;
  }
  return LoginLaunchState::Enabled;
}
}  // namespace

LoginLaunchState InspectLoginLaunch(std::wstring& error) {
  std::wstring command;
  if (!CurrentCommand(command, error)) return LoginLaunchState::Error;
  Key key;
  const LONG code = RegOpenKeyExW(HKEY_CURRENT_USER, kRun, 0,
                                   KEY_QUERY_VALUE, &key.handle);
  if (code == ERROR_FILE_NOT_FOUND) return LoginLaunchState::Disabled;
  if (code != ERROR_SUCCESS) {
    error = L"Başlangıç anahtarı okunamadı (Win32 " +
            std::to_wstring(code) + L").";
    return LoginLaunchState::Error;
  }
  return ReadEntry(key.handle, command, error);
}

bool SetLoginLaunch(bool enable, std::wstring& error) {
  std::wstring command;
  if (!CurrentCommand(command, error)) return false;
  Key key;
  const LONG code = RegCreateKeyExW(HKEY_CURRENT_USER, kRun, 0, nullptr,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                                    nullptr, &key.handle, nullptr);
  if (code != ERROR_SUCCESS) {
    error = L"Başlangıç anahtarı açılamadı (Win32 " +
            std::to_wstring(code) + L").";
    return false;
  }
  const LoginLaunchState state = ReadEntry(key.handle, command, error);
  if (state == LoginLaunchState::Conflict || state == LoginLaunchState::Error)
    return false;
  if (enable && state == LoginLaunchState::Enabled) return true;
  if (!enable && state == LoginLaunchState::Disabled) return true;

  LONG result{};
  if (enable) {
    result = RegSetValueExW(key.handle, kName, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()),
        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
  } else {
    result = RegDeleteValueW(key.handle, kName);
  }
  if (result != ERROR_SUCCESS) {
    error = L"Başlangıç kaydı değiştirilemedi (Win32 " +
            std::to_wstring(result) + L").";
    return false;
  }
  return true;
}

bool LoginLaunchModelSelfTest() {
  return CommandForPath(L"C:\\Program Files\\NoyaPLANET\\app.exe") ==
             L"\"C:\\Program Files\\NoyaPLANET\\app.exe\"" &&
         CommandForPath(L"") == L"" &&
         CommandForPath(L"bad\"path") == L"" &&
         OurCommand(L"\"C:\\app.exe\"", L"\"C:\\app.exe\"") &&
         !OurCommand(L"\"C:\\other.exe\"", L"\"C:\\app.exe\"");
}
}  // namespace noya
