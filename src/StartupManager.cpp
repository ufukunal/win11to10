#include "StartupManager.h"

#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <memory>
#include <string>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kDisabled[] = L"Software\\NoyaPLANET\\11to10\\DisabledStartup";
constexpr DWORD kMaxValueBytes = 16384;
struct RegHandle {
  HKEY key{};
  ~RegHandle() { if (key) RegCloseKey(key); }
  RegHandle() = default;
  RegHandle(const RegHandle&) = delete;
  RegHandle& operator=(const RegHandle&) = delete;
};

std::wstring Lower(std::wstring_view text) {
  std::wstring out(text);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
  return out;
}

std::wstring WindowsError(std::wstring_view action, LONG error) {
  return std::wstring(action) + L" (Win32 " + std::to_wstring(error) + L")";
}

bool ReadRaw(HKEY key, const std::wstring& name, DWORD& type,
             std::vector<BYTE>& bytes, LONG& status) {
  DWORD size = 0;
  status = RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr, &size);
  if (status != ERROR_SUCCESS || size > kMaxValueBytes) return false;
  bytes.assign(std::max<DWORD>(size, 2), 0);
  DWORD readSize = static_cast<DWORD>(bytes.size());
  status = RegQueryValueExW(key, name.c_str(), nullptr, &type,
                            bytes.data(), &readSize);
  if (status != ERROR_SUCCESS || readSize > kMaxValueBytes) return false;
  bytes.resize(readSize);
  return true;
}

std::wstring DecodeCommand(const std::vector<BYTE>& bytes) {
  if (bytes.size() < sizeof(wchar_t) || bytes.size() % sizeof(wchar_t) != 0)
    return L"(dane okunamadı)";
  std::wstring text(bytes.size() / sizeof(wchar_t), L'\0');
  std::memcpy(text.data(), bytes.data(), bytes.size());
  const auto nullPos = text.find(L'\0');
  if (nullPos != std::wstring::npos) text.resize(nullPos);
  return text;
}

void AppendEntries(HKEY key, bool disabled, std::vector<StartupEntry>& result) {
  for (DWORD index = 0; index < 2000; ++index) {
    wchar_t name[1024]{};
    DWORD nameLength = static_cast<DWORD>(std::size(name));
    LONG status = RegEnumValueW(key, index, name, &nameLength,
                                nullptr, nullptr, nullptr, nullptr);
    if (status == ERROR_NO_MORE_ITEMS) break;
    if (status != ERROR_SUCCESS || nameLength == 0) continue;
    const std::wstring keyName(name, nameLength);
    DWORD type{};
    std::vector<BYTE> bytes;
    LONG readStatus{};
    if (!ReadRaw(key, keyName, type, bytes, readStatus)) continue;
    if (type != REG_SZ && type != REG_EXPAND_SZ) continue;
    result.push_back(StartupEntry{keyName, DecodeCommand(bytes),
                                  disabled, StartupNameProtected(keyName)});
  }
}
}  // namespace

bool StartupNameProtected(std::wstring_view name) {
  const auto lower = Lower(name);
  return lower.find(L"securityhealth") != std::wstring::npos ||
         lower.find(L"windowsdefender") != std::wstring::npos ||
         lower.find(L"windows security") != std::wstring::npos ||
         lower.find(L"microsoftsecurity") != std::wstring::npos ||
         lower.find(L"msseces") != std::wstring::npos ||
         lower.find(L"noyaplanet") != std::wstring::npos;
}

std::vector<StartupEntry> ListCurrentUserStartup(std::wstring& error) {
  std::vector<StartupEntry> output;
  RegHandle run;
  LONG code = RegOpenKeyExW(HKEY_CURRENT_USER, kRun, 0, KEY_READ, &run.key);
  if (code == ERROR_SUCCESS) AppendEntries(run.key, false, output);
  else if (code != ERROR_FILE_NOT_FOUND) error = WindowsError(L"Başlangıç kayıtları okunamadı", code);

  RegHandle disabled;
  code = RegOpenKeyExW(HKEY_CURRENT_USER, kDisabled, 0, KEY_READ,
                       &disabled.key);
  if (code == ERROR_SUCCESS) AppendEntries(disabled.key, true, output);
  else if (code != ERROR_FILE_NOT_FOUND && error.empty())
    error = WindowsError(L"Yedekler okunamadı", code);

  std::sort(output.begin(), output.end(), [](const StartupEntry& a,
                                            const StartupEntry& b) {
    return Lower(a.name) < Lower(b.name);
  });
  return output;
}

bool ChangeCurrentUserStartup(std::wstring_view name, bool disable,
                              std::wstring& error) {
  const std::wstring valueName(name);
  if (valueName.empty() || valueName.size() > 1024) {
    error = L"Geçersiz başlangıç öğesi adı.";
    return false;
  }
  if (StartupNameProtected(valueName)) {
    error = L"Korunan Windows veya NoyaPLANET başlangıç kaydı değiştirilemez.";
    return false;
  }

  RegHandle run;
  RegHandle backup;
  LONG code = RegCreateKeyExW(HKEY_CURRENT_USER, kRun, 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE,
                              nullptr, &run.key, nullptr);
  if (code != ERROR_SUCCESS) {
    error = WindowsError(L"Başlangıç anahtarı açılamadı", code);
    return false;
  }
  code = RegCreateKeyExW(HKEY_CURRENT_USER, kDisabled, 0, nullptr,
                         REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE,
                         nullptr, &backup.key, nullptr);
  if (code != ERROR_SUCCESS) {
    error = WindowsError(L"Yedek anahtarı açılamadı", code);
    return false;
  }

  DWORD type{};
  std::vector<BYTE> value;
  LONG status{};
  if (disable) {
    DWORD alreadyType{};
    std::vector<BYTE> existing;
    LONG backupStatus{};
    if (ReadRaw(backup.key, valueName, alreadyType, existing, backupStatus) ||
        backupStatus != ERROR_FILE_NOT_FOUND) {
      error = L"Bu öğe için zaten bir yedek var; üzerine yazılmadı.";
      return false;
    }
    if (!ReadRaw(run.key, valueName, type, value, status)) {
      error = L"Başlangıç kaydı mevcut değil veya okunamadı.";
      return false;
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
      error = L"Bu kayıt türü güvenle yönetilemiyor.";
      return false;
    }
    code = RegSetValueExW(backup.key, valueName.c_str(), 0, type,
                          value.data(), static_cast<DWORD>(value.size()));
    if (code != ERROR_SUCCESS) {
      error = WindowsError(L"Özgün veri yedeklenemedi", code);
      return false;
    }
    code = RegFlushKey(backup.key);
    if (code != ERROR_SUCCESS) {
      RegDeleteValueW(backup.key, valueName.c_str());
      error = WindowsError(L"Yedek diske yazılamadı", code);
      return false;
    }
    code = RegDeleteValueW(run.key, valueName.c_str());
    if (code != ERROR_SUCCESS) {
      RegDeleteValueW(backup.key, valueName.c_str());
      error = WindowsError(L"Başlangıç kaydı devre dışı bırakılamadı", code);
      return false;
    }
    return true;
  }

  if (!ReadRaw(backup.key, valueName, type, value, status)) {
    error = L"Bu öğenin NoyaPLANET tarafından oluşturulmuş bir yedeği bulunamadı.";
    return false;
  }
  if (type != REG_SZ && type != REG_EXPAND_SZ) {
    error = L"Desteklenmeyen yedek veri türü.";
    return false;
  }
  DWORD currentType{};
  std::vector<BYTE> current;
  LONG currentStatus{};
  if (ReadRaw(run.key, valueName, currentType, current, currentStatus) ||
      currentStatus != ERROR_FILE_NOT_FOUND) {
    error = L"Özgün başlangıç konumunda aynı adlı bir kayıt zaten var.";
    return false;
  }
  code = RegSetValueExW(run.key, valueName.c_str(), 0, type, value.data(),
                        static_cast<DWORD>(value.size()));
  if (code != ERROR_SUCCESS) {
    error = WindowsError(L"Başlangıç kaydı geri yüklenemedi", code);
    return false;
  }
  code = RegFlushKey(run.key);
  if (code != ERROR_SUCCESS) {
    RegDeleteValueW(run.key, valueName.c_str());
    error = WindowsError(L"Geri yüklenen kayıt diske yazılamadı", code);
    return false;
  }
  code = RegDeleteValueW(backup.key, valueName.c_str());
  if (code != ERROR_SUCCESS) {
    // Keep the original backup if cleanup fails; restore is now active.
    error = WindowsError(L"Öğe geri yüklendi ancak yedek temizlenemedi", code);
    return false;
  }
  return true;
}

bool StartupModelSelfTest() {
  return StartupNameProtected(L"SecurityHealth") &&
         StartupNameProtected(L"WindowsDefender") &&
         StartupNameProtected(L"noyaplanet") &&
         !StartupNameProtected(L"Discord") &&
         !StartupNameProtected(L"Steam");
}
}  // namespace noya
