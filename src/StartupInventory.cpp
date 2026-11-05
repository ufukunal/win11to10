#include "StartupInventory.h"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace noya {
namespace {
namespace fs = std::filesystem;
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring Lower(std::wstring text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
  return text;
}

void CollectRun(HKEY root, std::vector<std::wstring>& names,
                std::wstring& diagnostics, const wchar_t* label) {
  HKEY handle{};
  const auto code = RegOpenKeyExW(root, kRunKey, 0, KEY_QUERY_VALUE, &handle);
  if (code == ERROR_FILE_NOT_FOUND) return;
  if (code != ERROR_SUCCESS) {
    diagnostics += std::wstring(label) + L": erişim yok (Win32 " +
                   std::to_wstring(code) + L").\r\n";
    return;
  }

  for (DWORD i = 0; i < 1024; ++i) {
    wchar_t buffer[2048]{};
    DWORD count = static_cast<DWORD>(std::size(buffer));
    const LONG status = RegEnumValueW(handle, i, buffer, &count,
                                      nullptr, nullptr, nullptr, nullptr);
    if (status == ERROR_NO_MORE_ITEMS) break;
    if (status == ERROR_SUCCESS && count != 0) {
      names.emplace_back(buffer, count);
    } else if (status == ERROR_MORE_DATA) {
      diagnostics += std::wstring(label) + L": çok uzun bir kayıt atlandı.\r\n";
    } else if (status != ERROR_SUCCESS) {
      diagnostics += std::wstring(label) + L": bazı kayıtlar okunamadı.\r\n";
    }
  }
  RegCloseKey(handle);
  std::sort(names.begin(), names.end());
}

void CollectShortcuts(const KNOWNFOLDERID& folder,
                      std::vector<std::wstring>& names,
                      std::wstring& diagnostics, const wchar_t* label) {
  PWSTR rawPath{};
  HRESULT result = SHGetKnownFolderPath(folder, KF_FLAG_DONT_VERIFY,
                                        nullptr, &rawPath);
  if (FAILED(result)) {
    diagnostics += std::wstring(label) + L": klasör adresi yok.\r\n";
    return;
  }
  const fs::path path(rawPath);
  CoTaskMemFree(rawPath);
  std::error_code error;
  if (!fs::is_directory(path, error)) return;
  fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, error);
  const fs::directory_iterator end;
  if (error) {
    diagnostics += std::wstring(label) + L": klasör okunamadı.\r\n";
    return;
  }
  for (unsigned i = 0; it != end && i < 512; ++i) {
    const auto item = it->path();
    const auto extension = Lower(item.extension().wstring());
    if (extension == L".lnk" || extension == L".url") {
      names.push_back(item.stem().wstring());
    }
    it.increment(error);
    if (error) {
      diagnostics += std::wstring(label) + L": klasörde erişim hatası.\r\n";
      break;
    }
  }
  std::sort(names.begin(), names.end());
}
}  // namespace

StartupInventory ScanStartupInventory() {
  StartupInventory result;
  CollectRun(HKEY_CURRENT_USER, result.currentUserRun,
             result.diagnostics, L"HKCU Run");
  CollectRun(HKEY_LOCAL_MACHINE, result.allUsersRun,
             result.diagnostics, L"HKLM Run");
  CollectShortcuts(FOLDERID_Startup, result.currentUserShortcuts,
                   result.diagnostics, L"Kullanıcı Başlangıç klasörü");
  CollectShortcuts(FOLDERID_CommonStartup, result.allUsersShortcuts,
                   result.diagnostics, L"Ortak Başlangıç klasörü");
  return result;
}

bool StartupInventoryModelSelfTest() {
  return Lower(L"EXAMPLE.LNK") == L"example.lnk" &&
         Lower(L"Test.URL") == L"test.url";
}
}  // namespace noya
