#include "StartMenuModel.h"
#include "TaskbarPreview.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <filesystem>
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
constexpr int kAboutSystem = 108;
constexpr int kSearch = 201;
constexpr int kAppList = 202;
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
struct StartPreview {
  HWND window{};
  HWND title{};
  HWND note{};
  HWND searchLabel{};
  HWND search{};
  HWND appList{};
  HWND tilesLabel{};
  HWND tiles[4]{};
  HWND taskbarNote{};
  std::vector<std::size_t> visible;
  HFONT bodyFont{};
  HFONT titleFont{};
};
Dashboard gDashboard;
StartPreview gPreview;
HBRUSH gBrush{};

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
  Place(gDashboard.summary, 30, 146, contentWidth, 90);
  Place(gDashboard.effects, 30, 258, contentWidth, 82);
  Place(gDashboard.warning, 30, 358, contentWidth, 74);
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

void RefreshDashboard() {
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
          << L"\r\nNot: Çalışma süresi, açılış hızını ölçmez.";
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
  const auto& apps = noya::BuiltInApps();
  if (index >= apps.size()) return;
  std::wstring command(apps[index].command);
  OpenBuiltIn(hwnd, command.c_str());
}

void FilterApps() {
  if (!gPreview.appList || !gPreview.search) return;
  const int length = GetWindowTextLengthW(gPreview.search);
  std::wstring query(static_cast<std::size_t>(length) + 1, L'\0');
  GetWindowTextW(gPreview.search, query.data(), length + 1);
  query.resize(static_cast<std::size_t>(length));

  gPreview.visible = noya::SearchApps(query);
  SendMessageW(gPreview.appList, LB_RESETCONTENT, 0, 0);
  for (const auto index : gPreview.visible) {
    const auto name = noya::BuiltInApps()[index].title;
    const std::wstring copy(name);
    SendMessageW(gPreview.appList, LB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(copy.c_str()));
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
  Place(gPreview.appList, 22, 188, listWidth, std::max(120, height - 286));
  Place(gPreview.tilesLabel, tileX, 115, tileWidth, 28);
  const int half = (tileWidth - 10) / 2;
  Place(gPreview.tiles[0], tileX, 151, half, 87);
  Place(gPreview.tiles[1], tileX + half + 10, 151, half, 87);
  Place(gPreview.tiles[2], tileX, 249, half, 87);
  Place(gPreview.tiles[3], tileX + half + 10, 249, half, 87);
  Place(gPreview.taskbarNote, 22, height - 78, usable, 61);
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
                         LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                         0, 0, 10, 10, hwnd,
                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAppList)),
                         GetModuleHandleW(nullptr), nullptr);
      SendMessageW(gPreview.appList, WM_SETFONT,
                   reinterpret_cast<WPARAM>(gPreview.bodyFont), TRUE);
      gPreview.tilesLabel = CreateLabel(hwnd, L"Sabitlenmiş uygulamalar",
                                       gPreview.bodyFont);
      constexpr const wchar_t* tiles[]{
          L"Dosya Gezgini", L"Ayarlar", L"Not Defteri", L"Görev Yöneticisi"};
      for (int i = 0; i < 4; ++i)
        gPreview.tiles[i] = CreateButton(hwnd, tiles[i], kTileBase + i,
                                         gPreview.bodyFont);
      gPreview.taskbarNote = CreateLabel(
          hwnd, L"Görev çubuğu / sistem tepsisi henüz değiştirilmedi.\r\n"
                L"Bu sürüm yalnızca bağımsız Başlat menüsü prototipidir.",
          gPreview.bodyFont);
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
      } else if (id >= kTileBase && id < kTileBase + 4 && HIWORD(wParam) == BN_CLICKED) {
        const std::array<std::size_t, 4> pinned{0, 4, 1, 3};
        LaunchPreviewIndex(hwnd, pinned[static_cast<std::size_t>(id - kTileBase)]);
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
  const int x = parent.left + 56;
  const int y = parent.top + 30;
  HWND preview = CreateWindowExW(
      WS_EX_APPWINDOW, kStartClass, L"NoyaPLANET 11to10 - Başlat menüsü önizlemesi",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      x, y, 720, 600, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (preview) ShowWindow(preview, SW_SHOWNORMAL);
}

LRESULT CALLBACK DashboardProc(HWND hwnd, UINT message,
                               WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      gDashboard.window = hwnd;
      gDashboard.bodyFont = MakeFont(-19, false);
      gDashboard.titleFont = MakeFont(-36, true);
      gDashboard.title = CreateLabel(hwnd, L"NoyaPLANET 11to10",
                                     gDashboard.titleFont);
      gDashboard.subtitle = CreateLabel(
          hwnd, L"Türkçe kontrol paneli  •  v0.2 arayüz prototipi\r\n"
                L"Windows 11 altyapısı korunur. Arayüz değişimleri aşamalı geliştiriliyor.",
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
      gDashboard.about = CreateButton(hwnd, L"Windows hakkında",
                                      kAboutSystem, gDashboard.bodyFont);
      RefreshDashboard();
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
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED) {
        switch (LOWORD(wParam)) {
          case kApply: ConfirmAndRun(hwnd, false); break;
          case kRestore: ConfirmAndRun(hwnd, true); break;
          case kRefresh: RefreshDashboard(); break;
          case kStartPreview: OpenPreview(hwnd); break;
          case kTaskbarPreview: noya::ShowTaskbarPreview(hwnd); break;
          case kStartupSettings: OpenBuiltIn(hwnd, L"ms-settings:startupapps"); break;
          case kVisualSettings: OpenBuiltIn(hwnd, L"SystemPropertiesPerformance.exe"); break;
          case kAboutSystem: OpenBuiltIn(hwnd, L"ms-settings:about"); break;
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
    case WM_CLOSE:
      if (IsWindow(gPreview.window)) DestroyWindow(gPreview.window);
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
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
    return noya::ModelSelfTest() ? 0 : 1;
  }
  gBrush = CreateSolidBrush(kBackground);
  if (!gBrush) return 1;
  if (!RegisterWindowClass(instance, kDashboardClass, DashboardProc) ||
      !RegisterWindowClass(instance, kStartClass, PreviewProc)) {
    DeleteObject(gBrush);
    return 1;
  }
  HWND window = CreateWindowExW(
      0, kDashboardClass, L"NoyaPLANET 11to10 - Kontrol Paneli",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      860, 735, nullptr, nullptr, instance, nullptr);
  if (!window) {
    DeleteObject(gBrush);
    return 1;
  }
  ShowWindow(window, show);
  UpdateWindow(window);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  DeleteObject(gBrush);
  return static_cast<int>(msg.wParam);
}
