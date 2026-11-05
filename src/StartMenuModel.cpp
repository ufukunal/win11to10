#include "StartMenuModel.h"
#include "TurkishSearch.h"

namespace noya {

const std::vector<LaunchTarget>& BuiltInApps() {
  static const std::vector<LaunchTarget> apps{
      {L"Dosya Gezgini", L"explorer.exe"},
      {L"Not Defteri", L"notepad.exe"},
      {L"Hesap Makinesi", L"calc.exe"},
      {L"Görev Yöneticisi", L"taskmgr.exe"},
      {L"Ayarlar", L"ms-settings:"},
      {L"Denetim Masası", L"control.exe"},
  };
  return apps;
}

std::vector<std::size_t> SearchApps(std::wstring_view query) {
  const auto& apps = BuiltInApps();
  std::vector<std::size_t> matching;
  for (std::size_t i = 0; i < apps.size(); ++i) {
    if (ContainsTurkish(apps[i].title, query)) {
      matching.push_back(i);
    }
  }
  return matching;
}

bool ModelSelfTest() {
  if (!TurkishSearchModelSelfTest()) return false;
  const auto all = SearchApps(L"");
  if (all.size() != BuiltInApps().size()) return false;
  if (SearchApps(L"DOSYA").size() != 1) return false;
  if (SearchApps(L"yöneticisi").size() != 1) return false;
  if (!SearchApps(L"zzzz_not_found_zzzz").empty()) return false;
  const auto settings = SearchApps(L"Ayarlar");
  return settings.size() == 1 &&
         BuiltInApps()[settings.front()].command == L"ms-settings:";
}
}  // namespace noya
