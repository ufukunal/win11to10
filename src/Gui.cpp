#include "StartMenuModel.h"
#include "TaskbarPreview.h"
#include "DockedBar.h"
#include "StartupWindow.h"
#include "StartupManager.h"
#include "InstalledApps.h"
#include "BootMetrics.h"
#include "BootHistory.h"
#include "StartupInventory.h"
#include "ServiceInventory.h"
#include "TrayHost.h"
#include "ClassicCalendar.h"
#include "PinManager.h"

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <objbase.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <cwctype>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr wchar_t kDashboardClass[] = L"NoyaPlanetDashboardWindowV1";
constexpr wchar_t kStartClass[] = L"NoyaPlanetStartPreviewWindowV1";
constexpr COLORREF kBackground = RGB(24, 34, 52);
constexpr COLORREF kForeground = RGB(238, 243, 249);
constexpr COLORREF kSecondary = RGB(184, 197, 214);

constexpr int kApply = 101;
constexpr int kRestore = 102;
constexpr int kRefresh = 103;
constexpr int kStartPreview = 104;
constexpr int kStartupSettings = 105;
constexpr int kVisualSettings = 106;
constexpr int kTaskbarPreview = 107;
constexpr int kExplorerOptions = 108;
constexpr int kStartHotkey = 9001;
constexpr int kBootHistoryMenu = 920;
constexpr int kStartupInventoryMenu = 921;
constexpr int kOpenWindowsStartupMenu = 922;
constexpr int kOpenExplorerOptionsMenu = 923;
constexpr int kEnableDockMenu = 924;
constexpr int kDisableDockMenu = 925;
constexpr int kServiceInventoryMenu = 926;
bool gStartHotkey = false;
constexpr int kSearch = 201;
constexpr int kAppList = 202;
constexpr int kPinSelected = 203;
constexpr int kUnpinSelected = 204;
constexpr int kTileBase = 300;

struct Dashboard {
  HWND window{};
  HWND title{};
  HWND subtitle{};
  HWND summary{};
  HWND effects{};
  HWND warning{};
  HWND apply{};
  HWND restore{};
  HWND refresh{};
  HWND preview{};
  HWND taskbar{};
  HWND startup{};
  HWND about{};
  HWND visualSettings{};
  HFONT bodyFont{};
  HFONT titleFont{};
};
struct PreviewItem {
  std::wstring title;
  std::wstring command;
};
struct StartPreview {
  HWND window{};
  HWND title{};
  HWND note{};
  HWND searchLabel{};
  HWND search{};
  HWND appList{};
  HWND pinButton{};
  HWND unpinButton{};
  HWND tilesLabel{};
  HWND tiles[4]{};
  HWND taskbarNote{};
  std::vector<std::size_t> visible;
  std::vector<PreviewItem> allApps;
  std::vector<std::wstring> pinnedTargets;
  HFONT bodyFont{};
  HFONT titleFont{};
};
Dashboard gDashboard;
StartPreview gPreview;
HBRUSH gBrush{};
UINT gTaskbarCreatedMessage = 0;
bool gTrayIconAdded = false;

HFONT MakeFont(int size, bool bold) {
  return CreateFontW(size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL,
                     FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

HWND CreateLabel(HWND parent, const wchar_t* text, HFONT font) {
  HWND label = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE |
                              SS_LEFT | SS_NOPREFIX,
                              0, 0, 10, 10, parent, nullptr,
                              GetModuleHandleW(nullptr), nullptr);
  SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  return label;
}

HWND CreateButton(HWND parent, const wchar_t* text, int id, HFONT font) {
  HWND button = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE |
                               WS_TABSTOP | BS_PUSHBUTTON,
                               0, 0, 10, 10, parent,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                               GetModuleHandleW(nullptr), nullptr);
  SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  return button;
}

void Place(HWND control, int x, int y, int width, int height) {
  if (control) MoveWindow(control, x, y, std::max(1, width), std::max(1, height), TRUE);
}

void LayoutDashboard(HWND hwnd) {
  RECT client{};
  GetClientRect(hwnd, &client);
  const int width = client.right;
  const int height = client.bottom;
  const int contentWidth = std::max(100, width - 56);
  Place(gDashboard.title, 28, 20, contentWidth, 46);
  Place(gDashboard.subtitle, 30, 72, contentWidth, 54);
  Place(gDashboard.summary, 30, 146, contentWidth, 110);
  Place(gDashboard.effects, 30, 272, contentWidth, 82);
  Place(gDashboard.warning, 30, 367, contentWidth, 74);
  const int buttonWidth = std::max(150, (contentWidth - 12) / 2);
  const int actionY = std::max(447, height - 190);
  Place(gDashboard.apply, 28, actionY, buttonWidth, 42);
  Place(gDashboard.restore, 40 + buttonWidth, actionY, buttonWidth, 42);
  Place(gDashboard.preview, 28, actionY + 50, buttonWidth, 38);
  Place(gDashboard.taskbar, 40 + buttonWidth, actionY + 50, buttonWidth, 38);
  Place(gDashboard.startup, 28, actionY + 96, buttonWidth, 38);
  Place(gDashboard.visualSettings, 40 + buttonWidth, actionY + 96, buttonWidth, 38);
  Place(gDashboard.refresh, 28, actionY + 142, buttonWidth, 36);
  Place(gDashboard.about, 40 + buttonWidth, actionY + 142, buttonWidth, 36);
}

std::wstring WindowsBuild() {
  wchar_t build[64]{};
  DWORD bytes = sizeof(build);
  if (RegGetValueW(HKEY_LOCAL_MACHINE,
                   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                   L"CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, build,
                   &bytes) != ERROR_SUCCESS) {
    return L"bilinmiyor";
  }
  return build;
}

// Read-only, user-invoked diagnostics; no ongoing background monitoring.
void ShowBootHistoryReport(HWND owner) {
  const auto history = noya::ReadBootHistory(8);
  if (history.milliseconds.empty()) {
    const std::wstring message = L"Açılış geçmişi okunamadı.\r\n" + history.warning;
    MessageBoxW(owner, message.c_str(), L"Açılış süresi analizi",
                MB_OK | MB_ICONINFORMATION);
    return;
  }
  const auto stats = noya::CalculateBootStats(history.milliseconds);
  std::wostringstream out;
  out << L"Son " << history.milliseconds.size()
      << L" kaydedilmiş açılış (Diagnostics-Performance 100)\r\n"
      << L"En son: " << stats.newestMs << L" ms\r\n"
      << L"Medyan: " << stats.medianMs << L" ms\r\n"
      << L"Ortalama: " << stats.meanMs << L" ms\r\n"
      << L"En kısa: " << stats.minimumMs << L" ms\r\n"
      << L"En uzun: " << stats.maximumMs << L" ms\r\n\r\n";
  for (std::size_t i = 0; i < history.milliseconds.size(); ++i) {
    out << (i + 1) << L". " << history.milliseconds[i] << L" ms\r\n";
  }
  out << L"\r\nBu kayıtlar aynı donanımda önce/sonra ölçümü"
         L" olmadan hızlanma kanıtlamaz.";
  if (!history.warning.empty()) out << L"\r\n" << history.warning;
  MessageBoxW(owner, out.str().c_str(), L"Açılış süresi analizi", MB_OK);
}

void ShowStartupInventoryReport(HWND owner) {
  const auto report = noya::ScanStartupInventory();
  std::wostringstream out;
  out << L"Başlangıç kaynakları (salt okunur)\r\n"
      << L"Bu liste Windows'un bütün başlangıç görevlerini kapsamaz.\r\n\r\n";
  const auto append = [&](const wchar_t* title, const auto& names) {
    out << title << L": " << names.size() << L" aday\r\n";
    const auto length = std::min<std::size_t>(names.size(), 8);
    for (std::size_t i = 0; i < length; ++i)
      out << L" - " << names[i] << L"\r\n";
    if (names.size() > length)
      out << L" ... (" << (names.size() - length) << L" diğer)\r\n";
    out << L"\r\n";
  };
  append(L"HKCU Run", report.currentUserRun);
  append(L"HKLM Run", report.allUsersRun);
  append(L"Kullanıcı Başlangıç klasörü", report.currentUserShortcuts);
  append(L"Ortak Başlangıç klasörü", report.allUsersShortcuts);
  out << L"Kayıtlı olması otomatik çalıştığı anlamına gelmez.\r\n"
         L"Programları kapatmak için Başlangıç Yöneticisini kullanın.\r\n";
  if (!report.diagnostics.empty()) out << report.diagnostics;
  MessageBoxW(owner, out.str().c_str(), L"Başlangıç envanteri", MB_OK);
}

void ShowServiceInventoryReport(HWND owner) {
  const auto result = noya::InspectWindowsServices();
  std::wostringstream out;
  out << L"Windows servisleri — salt okunur envanter\r\n"
      << L"Toplam: " << result.total << L"\r\n"
      << L"Çalışan: " << result.running << L"\r\n"
      << L"Çalışmayan: " << result.stopped << L"\r\n\r\n"
      << L"İlk çalışan servis örnekleri:\r\n";
  for (const auto& title : result.sampleRunningNames)
    out << L" - " << title << L"\r\n";
  if (!result.warning.empty()) out << L"\r\n" << result.warning << L"\r\n";
  out << L"\r\nÇalışıyor olması gereksiz olduğu anlamına gelmez. "
         L"Servisler devre dışı bırakılmaz ve değişiklik önerilmez.";
  MessageBoxW(owner, out.str().c_str(),
              L"NoyaPLANET — servis incelemesi", MB_OK | MB_ICONINFORMATION);
}

HMENU BuildDashboardMenu() {
  HMENU root = CreateMenu();
  HMENU tools = CreatePopupMenu();
  if (!root || !tools) {
    if (tools) DestroyMenu(tools);
    if (root) DestroyMenu(root);
    return nullptr;
  }
  AppendMenuW(tools, MF_STRING, kBootHistoryMenu, L"Açılış geçmişini göster");
  AppendMenuW(tools, MF_STRING, kStartupInventoryMenu,
              L"Başlangıç kaynaklarını listele");
  AppendMenuW(tools, MF_STRING, kServiceInventoryMenu,
              L"Windows servislerini incele (salt okunur)");
  AppendMenuW(tools, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(tools, MF_STRING, kOpenWindowsStartupMenu,
              L"Windows başlangıç ayarları");
  AppendMenuW(tools, MF_STRING, kOpenExplorerOptionsMenu,
              L"Dosya Gezgini seçenekleri");
  AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(tools), L"Sistem araçları");
  HMENU appearance = CreatePopupMenu();
  if (appearance) {
    AppendMenuW(appearance, MF_STRING, kEnableDockMenu,
                L"Ek görev çubuğunu yerleştir (deneysel)");
    AppendMenuW(appearance, MF_STRING, kDisableDockMenu,
                L"Ek görev çubuğunu kaldır");
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(appearance),
                L"Windows 10 görünümü");
  }
  return root;
}

void RefreshDashboard(bool readBoot = false) {
  MEMORYSTATUSEX memory{};
  memory.dwLength = sizeof(memory);
  std::wostringstream details;
  if (GlobalMemoryStatusEx(&memory)) {
    const auto total = memory.ullTotalPhys / (1024ULL * 1024ULL);
    const auto available = memory.ullAvailPhys / (1024ULL * 1024ULL);
    details << L"RAM: " << total << L" MiB toplam, " << available
            << L" MiB kullanılabilir  |  Yük: %" << memory.dwMemoryLoad;
  } else {
    details << L"RAM bilgisi okunamadı.";
  }
  details << L"\r\nWindows derlemesi: " << WindowsBuild()
          << L"  |  Sistem çalışma süresi: "
          << (GetTickCount64() / 60000ULL) << L" dakika"
          << L"\r\nSistem çalışma süresi açılış süresi değildir.";
  if (readBoot) {
    const auto boot = noya::ReadLastBootEvent();
    if (boot.available) {
      details << L"\r\nSon kaydedilen açılış: " << (boot.milliseconds / 1000ULL)
              << L"." << ((boot.milliseconds % 1000ULL) / 100ULL)
              << L" saniye (Olay 100; güncel açılış olmayabilir).";
    } else {
      details << L"\r\nAçılış kaydı: " << boot.diagnostic;
    }
  } else {
    details << L"\r\nAçılış ölçümü için Yenile düğmesine basın.";
  }
  SetWindowTextW(gDashboard.summary, details.str().c_str());

  BOOL smoothing = FALSE;
  BOOL animations = FALSE;
  const bool smoothOk = SystemParametersInfoW(SPI_GETFONTSMOOTHING, 0,
                                              &smoothing, 0) != FALSE;
  const bool animationOk = [&] {
    ANIMATIONINFO info{};
    info.cbSize = sizeof(info);
    const bool ok = SystemParametersInfoW(SPI_GETANIMATION, sizeof(info),
                                           &info, 0) != FALSE;
    animations = info.iMinAnimate != 0;
    return ok;
  }();
  std::wstring text = L"Yazı tipi kenar düzeltmesi: ";
  text += smoothOk ? (smoothing ? L"Açık" : L"Kapalı") : L"Okunamadı";
  text += L"\r\nPencere animasyonları: ";
  text += animationOk ? (animations ? L"Açık" : L"Kapalı") : L"Okunamadı";
  text += L"\r\nDiğer görsel efektleri Windows sürümüne göre test edeceğiz.";
  SetWindowTextW(gDashboard.effects, text.c_str());
}

std::filesystem::path CliPath() {
  std::array<wchar_t, 32768> buffer{};
  DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                    static_cast<DWORD>(buffer.size()));
  if (length == 0 || length >= buffer.size()) return {};
  return std::filesystem::path(std::wstring(buffer.data(), length))
      .parent_path() / L"Win11To10.exe";
}

bool RunPerformanceCommand(const wchar_t* command, std::wstring& error) {
  const auto cli = CliPath();
  if (cli.empty() || !std::filesystem::exists(cli)) {
    error = L"Win11To10.exe yardımcı aracı bulunamadı.\r\n"
            L"Her iki EXE aynı klasörde olmalıdır.";
    return false;
  }

  std::wstring commandLine = L"\"" + cli.wstring() + L"\" " +
                             command + L" --confirm";
  std::vector<wchar_t> mutableLine(commandLine.begin(), commandLine.end());
  mutableLine.push_back(L'\0');
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  if (!CreateProcessW(cli.c_str(), mutableLine.data(), nullptr, nullptr,
                      FALSE, CREATE_NO_WINDOW, nullptr, cli.parent_path().c_str(),
                      &startup, &process)) {
    error = L"Yardımcı araç başlatılamadı. Win32 hata: " +
            std::to_wstring(GetLastError());
    return false;
  }

  DWORD waitResult = WaitForSingleObject(process.hProcess, INFINITE);
  DWORD exitCode = 1;
  const bool success = waitResult == WAIT_OBJECT_0 &&
                       GetExitCodeProcess(process.hProcess, &exitCode) &&
                       exitCode == 0;
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  if (!success) {
    error = L"İşlem tamamlanamadı. Çıkış kodu: " +
            std::to_wstring(exitCode) + L".\r\n"
            L"Görsel ayarları değiştirilmemiş veya geri alınmış olabilir.\r\n"
            L"Ayrıntı için konsol komutlarını ve yedek dosyasını kontrol et.";
  }
  return success;
}

void ConfirmAndRun(HWND hwnd, bool restoring) {
  const wchar_t* question = restoring
      ? L"İlk uygulamadan önce kaydedilmiş görsel efektleri geri yüklemek istiyor musun?"
      : L"Yalnızca yazı tipi kenar düzeltmesi açık bırakılarak seçili "
        L"görsel efektler kapatılsın mı?\r\n\r\n"
        L"İşlemden önce mevcut ayarlar yedeklenecek.";
  if (MessageBoxW(hwnd, question, L"NoyaPLANET 11to10",
                  MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) return;
  SetCursor(LoadCursorW(nullptr, IDC_WAIT));
  std::wstring error;
  bool success = RunPerformanceCommand(restoring ? L"restore" : L"apply-performance",
                                       error);
  SetCursor(LoadCursorW(nullptr, IDC_ARROW));
  RefreshDashboard();
  if (!success) {
    MessageBoxW(hwnd, error.c_str(), L"İşlem başarısız",
                MB_ICONERROR | MB_OK);
  } else {
    MessageBoxW(hwnd, restoring
       ? L"Kaydedilmiş görsel ayarları geri yükleme isteği tamamlandı."
       : L"Performans profili uygulama isteği tamamlandı.\r\n"
         L"Windows bazı efektleri desteklemeyebilir.",
       L"NoyaPLANET 11to10", MB_ICONINFORMATION | MB_OK);
  }
}

void OpenBuiltIn(HWND hwnd, const wchar_t* executable) {
  const HINSTANCE result = ShellExecuteW(hwnd, L"open", executable,
                                         nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(result) <= 32) {
    MessageBoxW(hwnd, L"Windows uygulaması başlatılamadı.",
                L"NoyaPLANET 11to10", MB_ICONERROR | MB_OK);
  }
}

void LaunchPreviewIndex(HWND hwnd, std::size_t index) {
  if (index >= gPreview.allApps.size()) return;
  const auto& app = gPreview.allApps[index];
  OpenBuiltIn(hwnd, app.command.c_str());
  PostMessageW(hwnd, WM_CLOSE, 0, 0); // Win10-like: Start closes on launch
}

void UpdatePinnedTiles() {
  for (std::size_t i = 0; i < 4; ++i) {
    if (!gPreview.tiles[i]) continue;
    if (i >= gPreview.pinnedTargets.size()) {
      SetWindowTextW(gPreview.tiles[i], L"(Boş kutucuk)");
      EnableWindow(gPreview.tiles[i], FALSE);
      continue;
    }
    const auto& target = gPreview.pinnedTargets[i];
    const auto app = std::find_if(gPreview.allApps.begin(), gPreview.allApps.end(),
        [&](const PreviewItem& item) { return item.command == target; });
    if (app == gPreview.allApps.end()) {
      SetWindowTextW(gPreview.tiles[i], L"(Uygulama bulunamadı)");
      EnableWindow(gPreview.tiles[i], FALSE);
    } else {
      SetWindowTextW(gPreview.tiles[i], app->title.c_str());
      EnableWindow(gPreview.tiles[i], TRUE);
    }
  }
}

void ChangePinnedSelection(bool pin) {
  const auto position = SendMessageW(gPreview.appList, LB_GETCURSEL, 0, 0);
  if (position == LB_ERR || position < 0 ||
      static_cast<std::size_t>(position) >= gPreview.visible.size()) {
    MessageBoxW(gPreview.window, L"Önce uygulama listesinden bir öğe seçin.",
                L"Başlat kutucukları", MB_OK | MB_ICONINFORMATION);
    return;
  }
  const auto& selected = gPreview.allApps[
      gPreview.visible[static_cast<std::size_t>(position)]];
  std::vector<std::wstring> next = gPreview.pinnedTargets;
  auto found = std::find(next.begin(), next.end(), selected.command);
  if (pin) {
    if (found != next.end()) return;
    if (next.size() >= 4) {
      MessageBoxW(gPreview.window,
                  L"Dört kutucuk dolu. Önce bir uygulamanın sabitlemesini kaldır.",
                  L"Başlat kutucukları", MB_OK | MB_ICONINFORMATION);
      return;
    }
    next.push_back(selected.command);
  } else {
    if (found == next.end()) return;
    next.erase(found);
  }
  std::wstring error;
  if (!noya::SavePinnedTargets(next, error)) {
    MessageBoxW(gPreview.window, error.c_str(),
                L"Sabitlenmiş uygulamalar kaydedilemedi", MB_OK | MB_ICONERROR);
    return;
  }
  gPreview.pinnedTargets = std::move(next);
  UpdatePinnedTiles();
}

void FilterApps() {
  if (!gPreview.appList || !gPreview.search) return;
  const int length = GetWindowTextLengthW(gPreview.search);
  std::wstring query(static_cast<std::size_t>(length) + 1, L'\0');
  GetWindowTextW(gPreview.search, query.data(), length + 1);
  query.resize(static_cast<std::size_t>(length));

  auto lower = [](std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return text;
  };
  const auto needle = lower(query);
  gPreview.visible.clear();
  SendMessageW(gPreview.appList, LB_RESETCONTENT, 0, 0);
  for (std::size_t i = 0; i < gPreview.allApps.size(); ++i) {
    if (lower(gPreview.allApps[i].title).find(needle) == std::wstring::npos) continue;
    gPreview.visible.push_back(i);
    SendMessageW(gPreview.appList, LB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(gPreview.allApps[i].title.c_str()));
  }
}

void LayoutPreview(HWND hwnd) {
  RECT client{};
  GetClientRect(hwnd, &client);
  const int width = client.right;
  const int height = client.bottom;
  const int usable = std::max(500, width - 32);
  const int listWidth = std::max(200, usable / 2 - 10);
  const int tileX = 28 + listWidth;
  const int tileWidth = std::max(170, usable - listWidth - 12);
  Place(gPreview.title, 22, 14, usable, 43);
  Place(gPreview.note, 23, 60, usable, 44);
  Place(gPreview.searchLabel, 22, 115, listWidth, 26);
  Place(gPreview.search, 22, 145, listWidth, 32);
  Place(gPreview.appList, 22, 188, listWidth, std::max(120, height - 350));
  Place(gPreview.pinButton, 22, height - 150,
        std::max(85, listWidth / 2 - 5), 38);
  Place(gPreview.unpinButton, 27 + listWidth / 2, height - 150,
        std::max(85, listWidth / 2 - 5), 38);
  Place(gPreview.tilesLabel, tileX, 115, tileWidth, 28);
  const int half = (tileWidth - 10) / 2;
  Place(gPreview.tiles[0], tileX, 151, half, 87);
  Place(gPreview.tiles[1], tileX + half + 10, 151, half, 87);
  Place(gPreview.tiles[2], tileX, 249, half, 87);
  Place(gPreview.tiles[3], tileX + half + 10, 249, half, 87);
  Place(gPreview.taskbarNote, 22, height - 99, usable, 61);
}

LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message,
                             WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      gPreview.window = hwnd;
      gPreview.bodyFont = MakeFont(-18, false);
      gPreview.titleFont = MakeFont(-29, true);
      gPreview.title = CreateLabel(hwnd, L"Başlat  |  Windows 10 önizlemesi",
                                   gPreview.titleFont);
      gPreview.note = CreateLabel(
          hwnd, L"Bu pencere gerçek Başlat menüsünün yerine geçmez.\r\n"
                L"Arama ve uygulama başlatma davranışları çalışır.",
          gPreview.bodyFont);
      gPreview.searchLabel = CreateLabel(hwnd, L"Uygulama ara:", gPreview.bodyFont);
      gPreview.search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                         0, 0, 10, 10, hwnd,
                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearch)),
                         GetModuleHandleW(nullptr), nullptr);
      SendMessageW(gPreview.search, WM_SETFONT,
                   reinterpret_cast<WPARAM>(gPreview.bodyFont), TRUE);
      gPreview.appList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                         LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT,
                         0, 0, 10, 10, hwnd,
                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAppList)),
                         GetModuleHandleW(nullptr), nullptr);
      SendMessageW(gPreview.appList, WM_SETFONT,
                   reinterpret_cast<WPARAM>(gPreview.bodyFont), TRUE);
      gPreview.pinButton = CreateButton(hwnd, L"Seçileni sabitle",
                                         kPinSelected, gPreview.bodyFont);
      gPreview.unpinButton = CreateButton(hwnd, L"Sabitlemeyi kaldır",
                                           kUnpinSelected, gPreview.bodyFont);
      gPreview.tilesLabel = CreateLabel(hwnd, L"Sabitlenmiş uygulamalar",
                                       gPreview.bodyFont);
      constexpr const wchar_t* tiles[]{
          L"Dosya Gezgini", L"Ayarlar", L"Not Defteri", L"Görev Yöneticisi"};
      for (int i = 0; i < 4; ++i)
        gPreview.tiles[i] = CreateButton(hwnd, tiles[i], kTileBase + i,
                                         gPreview.bodyFont);
      gPreview.taskbarNote = CreateLabel(
          hwnd, L"Görev çubuğu / sistem tepsisi henüz değiştirilmedi.\r\n"
                L"Başlat kısayolları yalnızca açıldığında taranır.",
          gPreview.bodyFont);
      for (const auto& builtIn : noya::BuiltInApps())
        gPreview.allApps.push_back(
            PreviewItem{std::wstring(builtIn.title), std::wstring(builtIn.command)});
      for (auto& installed : noya::ScanInstalledApps())
        gPreview.allApps.push_back(
            PreviewItem{std::move(installed.title), std::move(installed.shortcut)});
      std::wstring pinError;
      bool hadPins = false;
      if (!noya::LoadPinnedTargets(gPreview.pinnedTargets, hadPins, pinError)) {
        MessageBoxW(hwnd, pinError.c_str(), L"Sabitleme ayarları okunamadı",
                    MB_OK | MB_ICONWARNING);
        gPreview.pinnedTargets.clear();
      }
      if (!hadPins && pinError.empty()) {
        for (const auto index : {0, 4, 1, 3}) {
          const auto builtIn = noya::BuiltInApps()[index].command;
          gPreview.pinnedTargets.emplace_back(builtIn);
        }
      }
      UpdatePinnedTiles();
      FilterApps();
      RECT client{};
      GetClientRect(hwnd, &client);
      LayoutPreview(hwnd);
      return 0;
    }
    case WM_SIZE:
      LayoutPreview(hwnd);
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == kSearch && HIWORD(wParam) == EN_CHANGE) {
        FilterApps();
      } else if (id == kAppList && HIWORD(wParam) == LBN_DBLCLK) {
        const LRESULT selected = SendMessageW(gPreview.appList, LB_GETCURSEL, 0, 0);
        if (selected != LB_ERR && static_cast<std::size_t>(selected) < gPreview.visible.size())
          LaunchPreviewIndex(hwnd, gPreview.visible[static_cast<std::size_t>(selected)]);
      } else if (id == kPinSelected && HIWORD(wParam) == BN_CLICKED) {
        ChangePinnedSelection(true);
      } else if (id == kUnpinSelected && HIWORD(wParam) == BN_CLICKED) {
        ChangePinnedSelection(false);
      } else if (id >= kTileBase && id < kTileBase + 4 && HIWORD(wParam) == BN_CLICKED) {
        const auto pinnedIndex = static_cast<std::size_t>(id - kTileBase);
        if (pinnedIndex < gPreview.pinnedTargets.size()) {
          const auto& target = gPreview.pinnedTargets[pinnedIndex];
          const auto app = std::find_if(gPreview.allApps.begin(),
              gPreview.allApps.end(),
              [&](const PreviewItem& item) { return item.command == target; });
          if (app != gPreview.allApps.end())
            LaunchPreviewIndex(hwnd,
                static_cast<std::size_t>(app - gPreview.allApps.begin()));
        }
      }
      return 0;
    }
    case WM_VKEYTOITEM: {
      if (reinterpret_cast<HWND>(lParam) != gPreview.appList) return -1;
      if (LOWORD(wParam) == VK_RETURN) {
        const auto selected = SendMessageW(gPreview.appList, LB_GETCURSEL, 0, 0);
        if (selected != LB_ERR &&
            static_cast<std::size_t>(selected) < gPreview.visible.size()) {
          LaunchPreviewIndex(hwnd, gPreview.visible[
              static_cast<std::size_t>(selected)]);
        }
        return -2; // handled; no listbox incremental search
      }
      if (LOWORD(wParam) == VK_ESCAPE) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        return -2;
      }
      return -1;
    }
    case WM_CONTEXTMENU: {
      if (reinterpret_cast<HWND>(wParam) != gPreview.appList) break;
      const LRESULT selected = SendMessageW(gPreview.appList, LB_GETCURSEL, 0, 0);
      if (selected == LB_ERR) return 0;
      POINT at{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      if (lParam == -1) {
        RECT list{};
        GetWindowRect(gPreview.appList, &list);
        at = POINT{list.left + 20, list.top + 20};
      }
      HMENU menu = CreatePopupMenu();
      if (!menu) return 0;
      AppendMenuW(menu, MF_STRING, 1, L"Uygulamayı aç");
      AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
      AppendMenuW(menu, MF_STRING, 2, L"Başlat'a sabitle");
      AppendMenuW(menu, MF_STRING, 3, L"Sabitlemeyi kaldır");
      const int chosen = TrackPopupMenu(menu,
          TPM_RETURNCMD | TPM_RIGHTBUTTON,
          at.x, at.y, 0, hwnd, nullptr);
      DestroyMenu(menu);
      if (chosen == 1 &&
          static_cast<std::size_t>(selected) < gPreview.visible.size()) {
        LaunchPreviewIndex(hwnd, gPreview.visible[
            static_cast<std::size_t>(selected)]);
      } else if (chosen == 2) {
        ChangePinnedSelection(true);
      } else if (chosen == 3) {
        ChangePinnedSelection(false);
      }
      return 0;
    }
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, kForeground);
      SetBkColor(dc, kBackground);
      return reinterpret_cast<INT_PTR>(gBrush);
    }
    case WM_DESTROY:
      if (gPreview.bodyFont) DeleteObject(gPreview.bodyFont);
      if (gPreview.titleFont) DeleteObject(gPreview.titleFont);
      gPreview = StartPreview{};
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
}

void OpenPreview(HWND owner) {
  if (IsWindow(gPreview.window)) {
    SetForegroundWindow(gPreview.window);
    return;
  }
  RECT parent{};
  GetWindowRect(owner, &parent);
  int x = parent.left + 56;
  int y = parent.top + 30;
  if (HWND docked = noya::DockedBarWindow()) {
    RECT bar{};
    GetWindowRect(docked, &bar);
    // Place the Start preview immediately above our optional docked bar.
    x = bar.left + 8;
    y = std::max(0L, bar.top - 610L);
  }
  HWND preview = CreateWindowExW(
      WS_EX_APPWINDOW, kStartClass, L"NoyaPLANET 11to10 - Başlat menüsü önizlemesi",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      x, y, 720, 600, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (preview) ShowWindow(preview, SW_SHOWNORMAL);
}

LRESULT CALLBACK DashboardProc(HWND hwnd, UINT message,
                               WPARAM wParam, LPARAM lParam) {
  if (gTaskbarCreatedMessage != 0 && message == gTaskbarCreatedMessage) {
    gTrayIconAdded = noya::AddOwnTrayIcon(hwnd);
    noya::OnDockedBarExplorerRestart();
    return 0;
  }
  switch (message) {
    case WM_CREATE: {
      gDashboard.window = hwnd;
      gDashboard.bodyFont = MakeFont(-19, false);
      gDashboard.titleFont = MakeFont(-36, true);
      gDashboard.title = CreateLabel(hwnd, L"NoyaPLANET 11to10",
                                     gDashboard.titleFont);
      gDashboard.subtitle = CreateLabel(
          hwnd, L"Türkçe kontrol paneli  •  v0.5 geliştirme sürümü\r\n"
                L"Windows 11 altyapısı korunur. Başlat için Ctrl+Alt+S (uygunsa).",
          gDashboard.bodyFont);
      gDashboard.summary = CreateLabel(hwnd, L"Sistem bilgileri okunuyor...",
                                        gDashboard.bodyFont);
      gDashboard.effects = CreateLabel(hwnd, L"Efekt durumu okunuyor...",
                                        gDashboard.bodyFont);
      gDashboard.warning = CreateLabel(
          hwnd, L"Önemli: Bu sürüm Windows 10 görev çubuğunu veya sistem tepsisini "
                L"henüz değiştirmez.\r\n"
                L"Yalnızca görsel efektlerde geri alınabilir değişiklik yapar.",
          gDashboard.bodyFont);
      gDashboard.apply = CreateButton(hwnd, L"Performans profilini uygula",
                                      kApply, gDashboard.bodyFont);
      gDashboard.restore = CreateButton(hwnd, L"Orijinal ayarları geri yükle",
                                        kRestore, gDashboard.bodyFont);
      gDashboard.preview = CreateButton(hwnd, L"Windows 10 Başlat önizlemesi",
                                        kStartPreview, gDashboard.bodyFont);
      gDashboard.taskbar = CreateButton(hwnd, L"Windows 10 görev çubuğu önizlemesi",
                                        kTaskbarPreview, gDashboard.bodyFont);
      gDashboard.startup = CreateButton(hwnd, L"Windows başlangıç uygulamaları",
                                        kStartupSettings, gDashboard.bodyFont);
      gDashboard.refresh = CreateButton(hwnd, L"Sistem bilgilerini yenile",
                                        kRefresh, gDashboard.bodyFont);
      gDashboard.visualSettings = CreateButton(hwnd, L"Windows görsel efekt ayarları",
                                               kVisualSettings, gDashboard.bodyFont);
      gDashboard.about = CreateButton(hwnd, L"Dosya Gezgini seçenekleri",
                                      kExplorerOptions, gDashboard.bodyFont);
      RefreshDashboard();
      gStartHotkey = RegisterHotKey(hwnd, kStartHotkey,
                                   MOD_CONTROL | MOD_ALT, 'S') != FALSE;
      gTaskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
      gTrayIconAdded = noya::AddOwnTrayIcon(hwnd);
      LayoutDashboard(hwnd);
      return 0;
    }
    case WM_SIZE:
      LayoutDashboard(hwnd);
      return 0;
    case WM_GETMINMAXINFO: {
      auto* data = reinterpret_cast<MINMAXINFO*>(lParam);
      data->ptMinTrackSize = POINT{690, 690};
      return 0;
    }
    case noya::kTrayCallbackMessage:
      switch (LOWORD(lParam)) {
        case WM_CONTEXTMENU:
        case WM_RBUTTONUP:
          noya::ShowOwnTrayMenu(hwnd);
          break;
        case WM_LBUTTONDBLCLK:
        case NIN_SELECT:
          ShowWindow(hwnd, SW_SHOW);
          SetForegroundWindow(hwnd);
          break;
        default: break;
      }
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case kEnableDockMenu:
          if (noya::IsDockedBarOpen()) {
            SetForegroundWindow(noya::DockedBarWindow());
            return 0;
          }
          if (MessageBoxW(hwnd,
              L"Windows görev çubuğu korunacak.\r\n"
              L"Ek olarak ekranın altında yer ayıran ayrı bir NoyaPLANET "
              L"görev çubuğu yerleştirilecek.\r\n\r\n"
              L"İstediğiniz zaman Kapat veya Windows 10 görünümü > "
              L"Ek görev çubuğunu kaldır ile kapatabilirsiniz.\r\n\r\n"
              L"Deneysel özelliği şimdi etkinleştirmek istiyor musunuz?",
              L"NoyaPLANET 11to10 — ek görev çubuğu",
              MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES &&
              !noya::OpenDockedBar(hwnd)) {
            MessageBoxW(hwnd, L"Görev çubuğu yerleştirilemedi. Windows'un "
                 L"orijinal görev çubuğu değiştirilmedi.",
                 L"Yerleştirme hatası", MB_OK | MB_ICONWARNING);
          }
          return 0;
        case kDisableDockMenu: noya::CloseDockedBar(); return 0;
        case kBootHistoryMenu: ShowBootHistoryReport(hwnd); return 0;
        case kStartupInventoryMenu: ShowStartupInventoryReport(hwnd); return 0;
        case kServiceInventoryMenu: ShowServiceInventoryReport(hwnd); return 0;
        case kOpenWindowsStartupMenu:
          OpenBuiltIn(hwnd, L"ms-settings:startupapps");
          return 0;
        case kOpenExplorerOptionsMenu:
          ShellExecuteW(hwnd, L"open", L"control.exe",
                        L"folders", nullptr, SW_SHOWNORMAL);
          return 0;
        case noya::kTrayShowDashboard:
          ShowWindow(hwnd, SW_RESTORE);
          SetForegroundWindow(hwnd);
          return 0;
        case noya::kTrayOpenStart: OpenPreview(hwnd); return 0;
        case noya::kTrayOpenTaskbar: noya::ShowTaskbarPreview(hwnd); return 0;
        case noya::kTrayCalendar: noya::ShowClassicCalendar(hwnd); return 0;
        case noya::kTrayExit: PostMessageW(hwnd, WM_CLOSE, 0, 0); return 0;
        default: break;
      }
      if (HIWORD(wParam) == BN_CLICKED) {
        switch (LOWORD(wParam)) {
          case kApply: ConfirmAndRun(hwnd, false); break;
          case kRestore: ConfirmAndRun(hwnd, true); break;
          case kRefresh: RefreshDashboard(true); break;
          case kStartPreview: OpenPreview(hwnd); break;
          case kTaskbarPreview: noya::ShowTaskbarPreview(hwnd); break;
          case kStartupSettings: noya::ShowStartupManager(hwnd); break;
          case kVisualSettings: OpenBuiltIn(hwnd, L"SystemPropertiesPerformance.exe"); break;
          case kExplorerOptions:
            if (reinterpret_cast<INT_PTR>(ShellExecuteW(
                    hwnd, L"open", L"control.exe", L"folders",
                    nullptr, SW_SHOWNORMAL)) <= 32)
              MessageBoxW(hwnd, L"Explorer seçenekleri açılamadı.",
                          L"NoyaPLANET 11to10", MB_ICONWARNING | MB_OK);
            break;
          default: break;
        }
      }
      return 0;
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, kForeground);
      SetBkColor(dc, kBackground);
      return reinterpret_cast<INT_PTR>(gBrush);
    }
    case noya::kRequestStartMenuPreview:
      OpenPreview(hwnd);
      return 0;
    case WM_HOTKEY:
      if (wParam == kStartHotkey) {
        OpenPreview(hwnd);
        return 0;
      }
      return DefWindowProcW(hwnd, message, wParam, lParam);
    case WM_CLOSE:
      noya::CloseDockedBar();
      if (IsWindow(gPreview.window)) DestroyWindow(gPreview.window);
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      if (gTrayIconAdded) {
        noya::RemoveOwnTrayIcon(hwnd);
        gTrayIconAdded = false;
      }
      if (gStartHotkey) {
        UnregisterHotKey(hwnd, kStartHotkey);
        gStartHotkey = false;
      }
      if (gDashboard.bodyFont) DeleteObject(gDashboard.bodyFont);
      if (gDashboard.titleFont) DeleteObject(gDashboard.titleFont);
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
}

bool RegisterWindowClass(HINSTANCE instance, const wchar_t* name,
                         WNDPROC procedure) {
  WNDCLASSEXW cls{};
  cls.cbSize = sizeof(cls);
  cls.hInstance = instance;
  cls.lpfnWndProc = procedure;
  cls.lpszClassName = name;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  cls.hbrBackground = gBrush;
  return RegisterClassExW(&cls) != 0;
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int show) {
  if (commandLine && std::wstring_view(commandLine) == L"--self-test") {
    return noya::ModelSelfTest() && noya::StartupModelSelfTest() && noya::InstalledAppModelSelfTest() && noya::BootMetricsModelSelfTest() && noya::BootHistoryModelSelfTest() && noya::StartupInventoryModelSelfTest() && noya::PinModelSelfTest() && noya::ServiceSummarySelfTest() ? 0 : 1;
  }
  // One tray icon and one dashboard per user session. No resident service.
  HANDLE mutex = CreateMutexW(nullptr, FALSE,
                              L"Local\\NoyaPLANET11to10-GuiInstanceV1");
  if (!mutex) return 1;
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    if (HWND existing = FindWindowW(kDashboardClass, nullptr)) {
      ShowWindow(existing, SW_RESTORE);
      SetForegroundWindow(existing);
    }
    CloseHandle(mutex);
    return 0;
  }
  const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(com)) {
    CloseHandle(mutex);
    return 1;
  }
  gBrush = CreateSolidBrush(kBackground);
  if (!gBrush) {
    CoUninitialize();
    CloseHandle(mutex);
    return 1;
  }
  if (!RegisterWindowClass(instance, kDashboardClass, DashboardProc) ||
      !RegisterWindowClass(instance, kStartClass, PreviewProc)) {
    DeleteObject(gBrush);
    CoUninitialize();
    CloseHandle(mutex);
    return 1;
  }
  HWND window = CreateWindowExW(
      0, kDashboardClass, L"NoyaPLANET 11to10 - Kontrol Paneli",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      860, 735, nullptr, nullptr, instance, nullptr);
  if (!window) {
    DeleteObject(gBrush);
    CoUninitialize();
    CloseHandle(mutex);
    return 1;
  }
  if (HMENU menu = BuildDashboardMenu()) {
    SetMenu(window, menu);
    DrawMenuBar(window);
  }
  ShowWindow(window, show);
  UpdateWindow(window);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  DeleteObject(gBrush);
  CoUninitialize();
  CloseHandle(mutex);
  return static_cast<int>(msg.wParam);
}
