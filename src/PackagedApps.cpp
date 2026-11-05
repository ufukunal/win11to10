#include "PackagedApps.h"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <algorithm>
#include <cwctype>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace noya {
namespace {
template<typename T>
class ComPtr {
 public:
  ComPtr() = default;
  ~ComPtr() { if (value_) value_->Release(); }
  ComPtr(const ComPtr&) = delete;
  ComPtr& operator=(const ComPtr&) = delete;
  T* get() const { return value_; }
  T** put() { return &value_; }
 private:
  T* value_{};
};

std::wstring Lower(std::wstring_view input) {
  std::wstring result(input);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
  return result;
}
bool Plausible(const PackagedApp& item) {
  return !item.title.empty() && item.title.size() <= 180 &&
         !item.parsingName.empty() && item.parsingName.size() <= 2048;
}
} // namespace

std::vector<PackagedApp> ScanAppsFolder() {
  std::vector<PackagedApp> result;
  ComPtr<IShellItem> folder;
  if (FAILED(SHCreateItemInKnownFolder(FOLDERID_AppsFolder, 0, nullptr,
                                      IID_PPV_ARGS(folder.put()))))
    return result;
  ComPtr<IEnumShellItems> enumerator;
  if (FAILED(folder.get()->BindToHandler(nullptr, BHID_EnumItems,
                                        IID_PPV_ARGS(enumerator.put()))))
    return result;

  std::set<std::wstring> seen;
  constexpr std::size_t kMaximumApps = 1500;
  for (std::size_t index = 0; index < kMaximumApps; ++index) {
    IShellItem* raw = nullptr;
    ULONG fetched = 0;
    if (enumerator.get()->Next(1, &raw, &fetched) != S_OK ||
        fetched != 1 || !raw) break;
    // raw is a fresh COM reference for each enumerated item.
    PWSTR label = nullptr;
    PWSTR parsing = nullptr;
    const HRESULT titleResult = raw->GetDisplayName(SIGDN_NORMALDISPLAY, &label);
    const HRESULT nameResult = raw->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING,
                                                    &parsing);
    if (SUCCEEDED(titleResult) && SUCCEEDED(nameResult) &&
        label != nullptr && parsing != nullptr) {
      PackagedApp app{label, parsing};
      if (Plausible(app) && seen.insert(Lower(app.parsingName)).second)
        result.push_back(std::move(app));
    }
    if (label) CoTaskMemFree(label);
    if (parsing) CoTaskMemFree(parsing);
    raw->Release();
  }
  std::sort(result.begin(), result.end(),
            [](const PackagedApp& a, const PackagedApp& b) {
              return Lower(a.title) < Lower(b.title);
            });
  return result;
}

bool LaunchAppsFolderItem(HWND owner, std::wstring_view parsingName) {
  if (parsingName.empty() || parsingName.size() > 2048) return false;
  // The stored name originates from the Shell namespace and is re-parsed
  // at launch time, so removed/uninstalled apps fail safely.
  const std::wstring text(parsingName);
  PIDLIST_ABSOLUTE pidl = nullptr;
  if (FAILED(SHParseDisplayName(text.c_str(), nullptr, &pidl, 0, nullptr)) ||
      pidl == nullptr) return false;
  SHELLEXECUTEINFOW execute{};
  execute.cbSize = sizeof(execute);
  execute.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI;
  execute.hwnd = owner;
  execute.lpVerb = L"open";
  execute.lpIDList = pidl;
  execute.nShow = SW_SHOWNORMAL;
  const bool result = ShellExecuteExW(&execute) != FALSE;
  CoTaskMemFree(pidl);
  return result;
}

bool PackagedAppsModelSelfTest() {
  const PackagedApp example{
      L"Örnek uygulama",
      L"shell:AppsFolder\\Example.Example_123!App"};
  return Plausible(example) && Lower(L"ÖRNEK") == L"örnek" &&
         !Plausible(PackagedApp{L"", L"shell:AppsFolder\\abc"}) &&
         !Plausible(PackagedApp{L"App", L""});
}
} // namespace noya
