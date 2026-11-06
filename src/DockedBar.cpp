#include "DockedBar.h"
#include "TaskbarPreview.h"
#include "ClassicCalendar.h"
#include "PinManager.h"
#include "StartMenuModel.h"
#include "PackagedApps.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kClass[] = L"NoyaPlanetSafeAppBarV1";
constexpr UINT kAppBarNotify = WM_APP + 92;
constexpr int kStart = 610;
constexpr int kRefresh = 611;
constexpr int kClose = 612;
constexpr int kClock = 613;
constexpr int kFirstWindow = 620;
constexpr int kMaxWindows = 6;
constexpr UINT kMenuRefresh = 1;
constexpr UINT kMenuFirstWindow = 100;
constexpr UINT kMenuFirstPin = 2000;
constexpr UINT kMenuSound = 20;
constexpr UINT kMenuNetwork = 21;
constexpr UINT kMenuPower = 22;
constexpr UINT kMenuWindowsSettings = 23;
constexpr std::size_t kWindowsPerMenu = 30;
constexpr std::size_t kMaxMenuWindows = 1020;  // 34 bounded pages.
constexpr int kHeight = 44;
constexpr COLORREF kBackground = RGB(32, 32, 32);

struct Dock {
  HWND hwnd{};
  HWND dashboard{};
  HWND start{};
  HWND refresh{};
  HWND close{};
  HWND clock{};
  std::array<HWND, kMaxWindows> apps{};
  std::vector<HWND> listed;
  int visibleSlots{};
  UINT dpi = 96;
  HFONT font{};
  HBRUSH background{};
  bool registered = false;
  bool positioning = false;
  bool hasPosition = false;
  RECT lastPosition{};
};
Dock dock;

// All AppBar geometry is expressed in 96-DPI logical units. GetDpiForWindow
// returns 96 for DPI-unaware hosts, avoiding incorrect double scaling.
int Scale(int value, UINT dpi) {
  return MulDiv(value, static_cast<int>(std::clamp(dpi, 72U, 480U)), 96);
}

int Px(int logical) { return Scale(logical, dock.dpi); }

APPBARDATA Data(HWND hwnd) {
  APPBARDATA data{};
  data.cbSize = sizeof(data);
  data.hWnd = hwnd;
  return data;
}

HWND Button(HWND owner, int id, const wchar_t* label) {
  HWND item = CreateWindowExW(0, L"BUTTON", label,
      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
      0, 0, 20, 30, owner,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
      GetModuleHandleW(nullptr), nullptr);
  SendMessageW(item, WM_SETFONT, reinterpret_cast<WPARAM>(dock.font), TRUE);
  return item;
}

BOOL CALLBACK Collect(HWND hwnd, LPARAM context) {
  auto* list = reinterpret_cast<std::vector<HWND>*>(context);
  if (hwnd == dock.hwnd || hwnd == dock.dashboard ||
      !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) ||
      GetWindowTextLengthW(hwnd) == 0 ||
      (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)) return TRUE;
  list->push_back(hwnd);
  return TRUE;
}

std::wstring Title(HWND hwnd) {
  const int count = GetWindowTextLengthW(hwnd);
  if (count <= 0 || count > 65535) return L"";
  std::wstring text(static_cast<std::size_t>(count) + 1, L'\0');
  const int got = GetWindowTextW(hwnd, text.data(), count + 1);
  if (got <= 0) return L"";
  text.resize(static_cast<std::size_t>(got));
  if (text.size() > 19) {
    text.resize(18);
    text += L"…";
  }
  return text;
}

void UpdateWindows() {
  if (!IsWindow(dock.hwnd)) return;
  dock.listed.clear();
  EnumWindows(Collect, reinterpret_cast<LPARAM>(&dock.listed));
  for (int i = 0; i < kMaxWindows; ++i) {
    const auto index = static_cast<std::size_t>(i);
    if (i >= dock.visibleSlots || index >= dock.listed.size()) {
      ShowWindow(dock.apps[index], SW_HIDE);
    } else {
      const auto text = Title(dock.listed[index]);
      SetWindowTextW(dock.apps[index], text.c_str());
      ShowWindow(dock.apps[index], SW_SHOWNA);
    }
  }
}

void ActivateWindow(HWND chosen) {
  if (!IsWindow(chosen)) return;
  if (GetForegroundWindow() == chosen && !IsIconic(chosen))
    ShowWindow(chosen, SW_MINIMIZE);
  else {
    if (IsIconic(chosen)) ShowWindow(chosen, SW_RESTORE);
    SetForegroundWindow(chosen);
  }
}

std::wstring MenuTitle(HWND hwnd) {
  const int length = GetWindowTextLengthW(hwnd);
  if (length <= 0 || length > 65535) return L"";
  std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
  const int copied = GetWindowTextW(hwnd, value.data(), length + 1);
  if (copied <= 0) return L"";
  value.resize(static_cast<std::size_t>(copied));
  if (value.size() > 76) {
    value.resize(75);
    value += L"…";
  }
  return value;
}

UINT WindowMenuCommand(std::size_t index) {
  return index < kMaxMenuWindows ?
      kMenuFirstWindow + static_cast<UINT>(index) : 0;
}

std::wstring EscapeMenuLabel(std::wstring_view title) {
  std::wstring escaped;
  for (const wchar_t ch : title) {
    escaped.push_back(ch);
    if (ch == L'&') escaped.push_back(L'&');
  }
  return escaped;
}

std::vector<std::wstring> PinnedForMenu() {
  std::vector<std::wstring> pins;
  bool configured = false;
  std::wstring error;
  if (!LoadPinnedTargets(pins, configured, error)) return {};
  if (!configured) {
    const auto& apps = BuiltInApps();
    for (const std::size_t index : {0U, 4U, 1U, 3U}) {
      if (index < apps.size()) pins.emplace_back(apps[index].command);
    }
  }
  return pins;
}

std::wstring PinLabel(std::wstring_view target) {
  for (const auto& app : BuiltInApps()) {
    if (target == app.command) return std::wstring(app.title);
  }
  std::wstring label = std::filesystem::path(std::wstring(target)).stem().wstring();
  if (label.empty()) label = std::wstring(target);
  if (label.size() > 64) {
    label.resize(63);
    label += L"…";
  }
  return label;
}

bool OpenPin(HWND owner, const std::wstring& target) {
  if (!IsValidPinnedTarget(target)) return false;
  bool builtIn = false;
  for (const auto& app : BuiltInApps()) {
    if (target == app.command) { builtIn = true; break; }
  }
  if (!builtIn && CanResolveAppsFolderItem(target))
    return LaunchAppsFolderItem(owner, target);
  const HINSTANCE result = ShellExecuteW(owner, L"open", target.c_str(),
                                         nullptr, nullptr, SW_SHOWNORMAL);
  return reinterpret_cast<INT_PTR>(result) > 32;
}

const wchar_t* SystemSettingsUri(UINT command) {
  switch (command) {
    case kMenuSound: return L"ms-settings:sound";
    case kMenuNetwork: return L"ms-settings:network-status";
    case kMenuPower: return L"ms-settings:powersleep";
    case kMenuWindowsSettings: return L"ms-settings:";
    default: return nullptr;
  }
}

void ShowAllWindowsMenu() {
  UpdateWindows();
  HMENU menu = CreatePopupMenu();
  if (!menu) return;
  AppendMenuW(menu, MF_STRING, kMenuRefresh, L"Pencere listesini yenile");

  // Reuse the same four user-owned pins as the independent Start flyout.
  // Read them on demand so pin/unpin changes appear next time this menu opens.
  const std::vector<std::wstring> pinned = PinnedForMenu();
  HMENU pins = CreatePopupMenu();
  if (pins) {
    if (pinned.empty()) {
      AppendMenuW(pins, MF_STRING | MF_GRAYED, 0, L"Sabitlenmiş uygulama yok");
    } else {
      for (std::size_t i = 0; i < pinned.size(); ++i) {
        const std::wstring label = EscapeMenuLabel(PinLabel(pinned[i]));
        AppendMenuW(pins, MF_STRING, kMenuFirstPin + static_cast<UINT>(i),
                    label.c_str());
      }
    }
    if (!AppendMenuW(menu, MF_POPUP,
                     reinterpret_cast<UINT_PTR>(pins), L"Sabitlenenler"))
      DestroyMenu(pins);
  }
  HMENU system = CreatePopupMenu();
  if (system) {
    AppendMenuW(system, MF_STRING, kMenuSound, L"Ses ayarları");
    AppendMenuW(system, MF_STRING, kMenuNetwork, L"Ağ ayarları");
    AppendMenuW(system, MF_STRING, kMenuPower, L"Güç ayarları");
    AppendMenuW(system, MF_STRING, kMenuWindowsSettings, L"Windows Ayarları");
    if (!AppendMenuW(menu, MF_POPUP,
                     reinterpret_cast<UINT_PTR>(system), L"Windows denetimleri"))
      DestroyMenu(system);
  }
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

  // A popup menu pumps a nested message loop: preserve the handle associated
  // with each command even when timer refreshes run while the menu is open.
  const std::vector<HWND> menuWindows = dock.listed;
  const std::size_t count = std::min(menuWindows.size(), kMaxMenuWindows);
  for (std::size_t first = 0; first < count; first += kWindowsPerMenu) {
    HMENU page = CreatePopupMenu();
    if (!page) break;
    const std::size_t last = std::min(count, first + kWindowsPerMenu);
    bool added = false;
    for (std::size_t i = first; i < last; ++i) {
      // '&' is a native menu accelerator marker; show literal app titles.
      const std::wstring escaped = EscapeMenuLabel(MenuTitle(menuWindows[i]));
      const UINT id = WindowMenuCommand(i);
      if (!escaped.empty() && id != 0 &&
          AppendMenuW(page, MF_STRING, id, escaped.c_str()))
        added = true;
    }
    if (!added) {
      DestroyMenu(page);
      continue;
    }
    const std::wstring pageName = L"Pencereler " +
        std::to_wstring(first + 1) + L"–" + std::to_wstring(last);
    if (!AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(page),
                     pageName.c_str()))
      DestroyMenu(page);  // Ownership transfers only after successful append.
  }
  if (menuWindows.size() > count) {
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0,
                L"İlk 1020 pencere gösteriliyor");
  }
  RECT button{};
  GetWindowRect(dock.refresh, &button);
  const UINT choice = TrackPopupMenu(menu,
      TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
      button.left, button.top, 0, dock.hwnd, nullptr);
  DestroyMenu(menu);
  if (choice == kMenuRefresh) {
    UpdateWindows();
  } else if (choice >= kMenuFirstPin &&
             choice < kMenuFirstPin + pinned.size()) {
    const auto& target = pinned[static_cast<std::size_t>(choice - kMenuFirstPin)];
    if (!OpenPin(dock.dashboard, target))
      MessageBoxW(dock.dashboard, L"Sabitlenmiş uygulama açılamadı.",
                  L"NoyaPLANET", MB_OK | MB_ICONWARNING);
  } else if (const wchar_t* settings = SystemSettingsUri(choice)) {
    const HINSTANCE result = ShellExecuteW(dock.dashboard, L"open", settings,
                                           nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
      MessageBoxW(dock.dashboard, L"Windows Ayarları açılamadı.",
                  L"NoyaPLANET", MB_OK | MB_ICONWARNING);
  } else if (choice >= kMenuFirstWindow &&
             choice < kMenuFirstWindow + count) {
    const std::size_t index = static_cast<std::size_t>(choice - kMenuFirstWindow);
    if (index < menuWindows.size()) {
      const HWND chosen = menuWindows[index];
      if (IsWindow(chosen)) {
        if (IsIconic(chosen)) ShowWindow(chosen, SW_RESTORE);
        SetForegroundWindow(chosen);
      }
    }
  }
}

void UpdateClock() {
  if (!IsWindow(dock.clock)) return;
  SYSTEMTIME now{};
  GetLocalTime(&now);
  wchar_t value[64]{};
  swprintf_s(value, L"%02u:%02u  %02u.%02u", now.wHour, now.wMinute,
             now.wDay, now.wMonth);
  SetWindowTextW(dock.clock, value);
}

int VisibleAppButtons(int width, UINT dpi) {
  const int left = Scale(170, dpi);
  const int rightControls = Scale(119 + 60 + 16, dpi);
  const int gap = Scale(4, dpi);
  const int minButtonWidth = Scale(64, dpi);
  const int available = std::max(0, width - rightControls - left);
  return std::clamp((available + gap) / (minButtonWidth + gap),
                    0, kMaxWindows);
}

void PlaceChildren() {
  if (!IsWindow(dock.hwnd)) return;
  RECT rect{};
  GetClientRect(dock.hwnd, &rect);
  const int width = static_cast<int>(rect.right);
  const int top = std::max(0, (static_cast<int>(rect.bottom) - Px(34)) / 2);
  MoveWindow(dock.start, Px(6), top, Px(88), Px(34), TRUE);
  MoveWindow(dock.refresh, Px(100), top, Px(64), Px(34), TRUE);
  const int clockWidth = Px(119);
  const int closeWidth = Px(60);
  const int left = Px(170);
  const int rightLimit = width - clockWidth - closeWidth - Px(16);
  const int space = std::max(0, rightLimit - left);
  const int gap = Px(4);
  dock.visibleSlots = VisibleAppButtons(width, dock.dpi);
  const int buttonWidth = dock.visibleSlots > 0 ?
      (space - (dock.visibleSlots - 1) * gap) / dock.visibleSlots : 0;
  for (int i = 0; i < kMaxWindows; ++i) {
    HWND button = dock.apps[static_cast<std::size_t>(i)];
    if (i >= dock.visibleSlots) {
      ShowWindow(button, SW_HIDE);
    } else {
      MoveWindow(button, left + i * (buttonWidth + gap), top,
                 buttonWidth, Px(34), TRUE);
    }
  }
  // Prioritize Start and Close on very narrow or portrait displays.
  // The overflow menu remains available when there is room for its button.
  ShowWindow(dock.refresh, width >= Px(270) ? SW_SHOWNA : SW_HIDE);
  ShowWindow(dock.clock, width >= Px(360) ? SW_SHOWNA : SW_HIDE);
  MoveWindow(dock.clock, width - clockWidth - closeWidth - Px(12), top,
             clockWidth, Px(34), TRUE);
  MoveWindow(dock.close, width - closeWidth - Px(6), top,
             closeWidth, Px(34), TRUE);
}

bool Position() {
  if (!dock.registered || !IsWindow(dock.hwnd)) return false;
  if (dock.positioning) return true;  // ABM_SETPOS can notify this appbar.
  // Use the monitor hosting the owner. Do not modify the Windows taskbar;
  // ABM_QUERYPOS handles existing system appbars and keeps their area.
  HMONITOR monitor = MonitorFromWindow(dock.dashboard, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  if (!GetMonitorInfoW(monitor, &monitorInfo)) return false;
  // Start and Close must remain distinct, even on unusual tiny displays.
  if (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left < Px(160))
    return false;
  APPBARDATA data = Data(dock.hwnd);
  data.uEdge = ABE_BOTTOM;
  data.rc = monitorInfo.rcMonitor;
  data.rc.top = data.rc.bottom - Px(kHeight);
  if (!SHAppBarMessage(ABM_QUERYPOS, &data)) return false;
  data.rc.top = data.rc.bottom - Px(kHeight);
  if (dock.hasPosition && EqualRect(&dock.lastPosition, &data.rc)) return true;
  dock.positioning = true;
  const bool reserved = SHAppBarMessage(ABM_SETPOS, &data) != 0;
  const bool placed = reserved && SetWindowPos(dock.hwnd, HWND_TOPMOST,
               data.rc.left, data.rc.top,
               data.rc.right - data.rc.left, data.rc.bottom - data.rc.top,
               SWP_NOACTIVATE | SWP_SHOWWINDOW) != FALSE;
  if (placed) {
    dock.lastPosition = data.rc;
    dock.hasPosition = true;
  }
  dock.positioning = false;
  return placed;
}

LRESULT CALLBACK DockProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == kAppBarNotify) {
    if (wParam == ABN_POSCHANGED && !Position())
      PostMessageW(hwnd, WM_CLOSE, 0, 0);
    else if (wParam == ABN_FULLSCREENAPP) {
      SetWindowPos(hwnd, lParam ? HWND_BOTTOM : HWND_TOPMOST,
                   0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    return 0;
  }
  switch (message) {
    case WM_CREATE: {
      dock.hwnd = hwnd;
      dock.dpi = GetDpiForWindow(hwnd);
      dock.font = CreateFontW(-Px(16), 0, 0, 0, FW_NORMAL, FALSE,
          FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      dock.background = CreateSolidBrush(kBackground);
      dock.start = Button(hwnd, kStart, L"Başlat");
      dock.refresh = Button(hwnd, kRefresh, L"Tümü");
      dock.close = Button(hwnd, kClose, L"Kapat");
      // A real button supports mouse, keyboard and accessible naming.
      dock.clock = Button(hwnd, kClock, L"Takvim");
      for (int i = 0; i < kMaxWindows; ++i)
        dock.apps[static_cast<std::size_t>(i)] =
            Button(hwnd, kFirstWindow+i, L"");
      auto data = Data(hwnd);
      data.uCallbackMessage = kAppBarNotify;
      dock.registered = SHAppBarMessage(ABM_NEW, &data) != 0;
      if (!dock.registered) return -1;
      if (!Position()) return -1;
      UpdateClock();
      UpdateWindows();
      SetTimer(hwnd, 1, 30000, nullptr); // keep clock and open-window buttons current
      return 0;
    }
    case WM_SIZE:
      PlaceChildren();
      UpdateWindows();
      return 0;
    case WM_DPICHANGED: {
      const UINT newDpi = HIWORD(wParam);
      if (newDpi == dock.dpi || newDpi == 0) return 0;
      dock.dpi = newDpi;
      HFONT newFont = CreateFontW(-Px(16), 0, 0, 0, FW_NORMAL,
          FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      if (newFont) {
        for (HWND control : {dock.start, dock.refresh, dock.close, dock.clock})
          if (IsWindow(control)) SendMessageW(control, WM_SETFONT,
                                              reinterpret_cast<WPARAM>(newFont), TRUE);
        for (HWND control : dock.apps)
          if (IsWindow(control)) SendMessageW(control, WM_SETFONT,
                                              reinterpret_cast<WPARAM>(newFont), TRUE);
        if (dock.font) DeleteObject(dock.font);
        dock.font = newFont;
      }
      dock.hasPosition = false;
      if (!Position()) PostMessageW(hwnd, WM_CLOSE, 0, 0);
      PlaceChildren();
      UpdateWindows();
      return 0;
    }
    case WM_MOUSEACTIVATE:
      // Keep the application window in front when a taskbar button is clicked.
      // Otherwise the dock steals foreground focus before WM_COMMAND arrives.
      return MA_NOACTIVATE;
    case WM_DISPLAYCHANGE:
      // Monitor topology can change after docking. Release the appbar on failure.
      dock.hasPosition = false;
      if (!Position()) PostMessageW(hwnd, WM_CLOSE, 0, 0);
      return 0;
    case WM_ACTIVATE: {
      auto data = Data(hwnd);
      data.lParam = LOWORD(wParam) != WA_INACTIVE;
      SHAppBarMessage(ABM_ACTIVATE, &data);
      return 0;
    }
    case WM_WINDOWPOSCHANGED: {
      auto data = Data(hwnd);
      if (dock.registered) SHAppBarMessage(ABM_WINDOWPOSCHANGED, &data);
      return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    case WM_TIMER:
      if (wParam == 1) {
        UpdateClock();
        UpdateWindows();
      }
      return 0;
    case WM_COMMAND:
      if (HIWORD(wParam) != BN_CLICKED) break;
      if (LOWORD(wParam) == kStart) {
        PostMessageW(dock.dashboard, kRequestStartMenuPreview, 0, 0);
      } else if (LOWORD(wParam) == kRefresh) {
        ShowAllWindowsMenu();
      } else if (LOWORD(wParam) == kClock) {
        ShowClassicCalendarAt(dock.dashboard, dock.clock);
      } else if (LOWORD(wParam) == kClose) {
        DestroyWindow(hwnd);
      } else if (LOWORD(wParam) >= kFirstWindow &&
                 LOWORD(wParam) < kFirstWindow + kMaxWindows) {
        const auto index = static_cast<std::size_t>(LOWORD(wParam)-kFirstWindow);
        if (index < dock.listed.size() && IsWindow(dock.listed[index])) {
          ActivateWindow(dock.listed[index]);
        }
      }
      return 0;
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, RGB(243, 247, 251));
      SetBkColor(dc, kBackground);
      return reinterpret_cast<LRESULT>(dock.background);
    }
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      if (dock.registered) {
        auto data = Data(hwnd);
        SHAppBarMessage(ABM_REMOVE, &data);  // restore reserved work area
      }
      if (dock.font) DeleteObject(dock.font);
      if (dock.background) DeleteObject(dock.background);
      dock = Dock{};
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
  return DefWindowProcW(hwnd, message, wParam, lParam);
}
}  // namespace

bool DockedBarLayoutSelfTest() {
  return VisibleAppButtons(160, 96) == 0 &&
         VisibleAppButtons(360, 96) == 0 &&
         VisibleAppButtons(500, 96) == 2 &&
         VisibleAppButtons(800, 96) == 6 &&
         VisibleAppButtons(750, 144) == 2 &&
         VisibleAppButtons(1200, 144) == 6 &&
         Scale(44, 96) == 44 &&
         Scale(44, 144) == 66 &&
         Scale(44, 192) == 88 &&
         WindowMenuCommand(0) == kMenuFirstWindow &&
         WindowMenuCommand(99) == kMenuFirstWindow + 99 &&
         WindowMenuCommand(1019) == kMenuFirstWindow + 1019 &&
         WindowMenuCommand(1020) == 0 &&
         kMaxMenuWindows % kWindowsPerMenu == 0 &&
         SystemSettingsUri(kMenuSound) != nullptr &&
         SystemSettingsUri(0) == nullptr &&
         PinLabel(L"notepad.exe") == L"Not Defteri" &&
         EscapeMenuLabel(L"Save & Exit") == L"Save && Exit" &&
         EscapeMenuLabel(L"") == L"";
}

bool OpenDockedBar(HWND dashboard) {
  if (IsWindow(dock.hwnd)) {
    SetForegroundWindow(dock.hwnd);
    return true;
  }
  if (!IsWindow(dashboard)) return false;
  const HINSTANCE module = GetModuleHandleW(nullptr);
  WNDCLASSEXW cls{};
  cls.cbSize = sizeof(cls);
  cls.hInstance = module;
  cls.lpfnWndProc = DockProc;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.lpszClassName = kClass;
  cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!GetClassInfoExW(module, kClass, &cls) &&
      !RegisterClassExW(&cls)) return false;

  dock.dashboard = dashboard;
  HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
      kClass, L"NoyaPLANET 11to10 — ek görev çubuğu",
      WS_POPUP | WS_VISIBLE, 0, 0, 900,
      Scale(kHeight, GetDpiForWindow(dashboard)),
      dashboard, nullptr, module, nullptr);
  return IsWindow(hwnd) && dock.registered;
}

void CloseDockedBar() {
  if (IsWindow(dock.hwnd)) DestroyWindow(dock.hwnd);
}

bool IsDockedBarOpen() { return IsWindow(dock.hwnd) && dock.registered; }

HWND DockedBarWindow() { return IsDockedBarOpen() ? dock.hwnd : nullptr; }
void OnDockedBarExplorerRestart() {
  if (!IsWindow(dock.hwnd)) return;
  dock.hasPosition = false;
  auto data = Data(dock.hwnd);
  data.uCallbackMessage = kAppBarNotify;
  dock.registered = SHAppBarMessage(ABM_NEW, &data) != 0;
  if (!dock.registered || !Position()) {
    // Fail closed instead of leaving a broken topmost overlay.
    DestroyWindow(dock.hwnd);
  }
}
}  // namespace noya
