#include "InstalledApps.h"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace noya {
namespace {
namespace fs = std::filesystem;
std::wstring Lower(std::wstring_view text) {
  std::wstring out(text);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
  return out;
}

bool IsShortcut(const fs::path& path) {
  const auto extension = Lower(path.extension().wstring());
  return extension == L".lnk" || extension == L".url";
}

void ScanFolder(const KNOWNFOLDERID& id, std::vector<InstalledApp>& apps,
                std::set<std::wstring>& seen) {
  PWSTR raw = nullptr;
  const HRESULT hr = SHGetKnownFolderPath(id, KF_FLAG_DONT_VERIFY, nullptr, &raw);
  if (FAILED(hr)) return;
  const fs::path base(raw);
  CoTaskMemFree(raw);
  std::error_code ec;
  if (!fs::is_directory(base, ec)) return;

  fs::recursive_directory_iterator it(base, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end{};
  if (ec) return;
  while (it != end && apps.size() < 1500) {
    const auto path = it->path();
    ec.clear();
    const bool regular = it->is_regular_file(ec);
    if (!ec && regular && IsShortcut(path)) {
      std::wstring name = path.stem().wstring();
      if (!name.empty() && name.length() <= 150) {
        const std::wstring key = Lower(name);
        if (seen.insert(key).second) {
          // Shortcuts are launched by ShellExecute only after explicit click.
          apps.push_back(InstalledApp{std::move(name), path.wstring()});
        }
      }
    }
    it.increment(ec);
    if (ec) {
      // Do not traverse inaccessible folders or fail the whole Start menu.
      ec.clear();
      break;
    }
  }
}
}  // namespace

std::vector<InstalledApp> ScanInstalledApps() {
  std::vector<InstalledApp> apps;
  std::set<std::wstring> seen;
  ScanFolder(FOLDERID_Programs, apps, seen);
  ScanFolder(FOLDERID_CommonPrograms, apps, seen);
  std::sort(apps.begin(), apps.end(),
            [](const InstalledApp& a, const InstalledApp& b) {
              return Lower(a.title) < Lower(b.title);
            });
  return apps;
}

bool InstalledAppModelSelfTest() {
  return IsShortcut(std::filesystem::path(L"C:\\Apps\\Notepad.LNK")) &&
         IsShortcut(std::filesystem::path(L"C:\\Apps\\Installer.url")) &&
         !IsShortcut(std::filesystem::path(L"C:\\Apps\\config.txt")) &&
         Lower(L"ÖRNEK.EXE") != Lower(L"örnek.lnk");
}
}  // namespace noya
