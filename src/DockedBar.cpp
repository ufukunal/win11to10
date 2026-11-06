#include "DockedBar.h"
#include "TaskbarPreview.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kClass[] = L"NoyaPlanetSafeAppBarV1";
constexpr UINT kAppBarNotify = WM_APP + 92;
constexpr int kStart = 610;
constexpr int kRefresh = 611;
constexpr int kClose = 612;
constexpr int kFirstWindow = 620;
constexpr int kMaxWindows = 6;
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
  HFONT font{};
  HBRUSH background{};
  bool registered = false;
};
Dock dock;

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
    if (index >= dock.listed.size()) {
      ShowWindow(dock.apps[index], SW_HIDE);
    } else {
      const auto text = Title(dock.listed[index]);
      SetWindowTextW(dock.apps[index], text.c_str());
      ShowWindow(dock.apps[index], SW_SHOWNA);
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

void PlaceChildren() {
  if (!IsWindow(dock.hwnd)) return;
  RECT rect{};
  GetClientRect(dock.hwnd, &rect);
  const int width = std::max(static_cast<int>(rect.right), 360);
  const int margin = 6;
  MoveWindow(dock.start, margin, 5, 88, 34, TRUE);
  MoveWindow(dock.refresh, 100, 5, 64, 34, TRUE);
  const int clockWidth = 119;
  const int closeWidth = 60;
  const int left = 170;
  const int rightLimit = width - clockWidth - closeWidth - 16;
  const int space = std::max(120, rightLimit - left);
  const int gap = 4;
  const int buttonWidth = std::max(32, (space - (kMaxWindows - 1)*gap) / kMaxWindows);
  for (int i = 0; i < kMaxWindows; ++i) {
    MoveWindow(dock.apps[static_cast<std::size_t>(i)],
        left + i*(buttonWidth + gap), 5, buttonWidth, 34, TRUE);
  }
  MoveWindow(dock.clock, width - clockWidth - closeWidth - 12, 5,
             clockWidth, 34, TRUE);
  MoveWindow(dock.close, width - closeWidth - 6, 5, closeWidth, 34, TRUE);
}

bool Position() {
  if (!dock.registered || !IsWindow(dock.hwnd)) return false;
  // Use the monitor hosting the owner. Do not modify the Windows taskbar;
  // ABM_QUERYPOS handles existing system appbars and keeps their area.
  HMONITOR monitor = MonitorFromWindow(dock.dashboard, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  if (!GetMonitorInfoW(monitor, &monitorInfo)) return false;
  APPBARDATA data = Data(dock.hwnd);
  data.uEdge = ABE_BOTTOM;
  data.rc = monitorInfo.rcMonitor;
  data.rc.top = data.rc.bottom - kHeight;
  if (!SHAppBarMessage(ABM_QUERYPOS, &data)) return false;
  data.rc.top = data.rc.bottom - kHeight;
  if (!SHAppBarMessage(ABM_SETPOS, &data)) return false;
  return SetWindowPos(dock.hwnd, HWND_TOPMOST,
               data.rc.left, data.rc.top,
               data.rc.right-data.rc.left, data.rc.bottom-data.rc.top,
               SWP_NOACTIVATE | SWP_SHOWWINDOW) != FALSE;
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
      dock.font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE,
          FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      dock.background = CreateSolidBrush(kBackground);
      dock.start = Button(hwnd, kStart, L"Başlat");
      dock.refresh = Button(hwnd, kRefresh, L"Yenile");
      dock.close = Button(hwnd, kClose, L"Kapat");
      dock.clock = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
          0, 0, 110, 36, hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
      SendMessageW(dock.clock, WM_SETFONT,
                   reinterpret_cast<WPARAM>(dock.font), TRUE);
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
      return 0;
    case WM_MOUSEACTIVATE:
      // Keep the application window in front when a taskbar button is clicked.
      // Otherwise the dock steals foreground focus before WM_COMMAND arrives.
      return MA_NOACTIVATE;
    case WM_DISPLAYCHANGE:
      // Monitor topology can change after docking. Release the appbar on failure.
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
        UpdateWindows();
      } else if (LOWORD(wParam) == kClose) {
        DestroyWindow(hwnd);
      } else if (LOWORD(wParam) >= kFirstWindow &&
                 LOWORD(wParam) < kFirstWindow + kMaxWindows) {
        const auto index = static_cast<std::size_t>(LOWORD(wParam)-kFirstWindow);
        if (index < dock.listed.size() && IsWindow(dock.listed[index])) {
          auto chosen = dock.listed[index];
          // Windows 10 taskbar convention: clicking the foreground window
          // minimizes it; clicking a minimized or background window activates it.
          if (GetForegroundWindow() == chosen && !IsIconic(chosen)) {
            ShowWindow(chosen, SW_MINIMIZE);
          } else {
            if (IsIconic(chosen)) ShowWindow(chosen, SW_RESTORE);
            SetForegroundWindow(chosen);
          }
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
      WS_POPUP | WS_VISIBLE, 0, 0, 900, kHeight,
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
  auto data = Data(dock.hwnd);
  data.uCallbackMessage = kAppBarNotify;
  dock.registered = SHAppBarMessage(ABM_NEW, &data) != 0;
  if (!dock.registered || !Position()) {
    // Fail closed instead of leaving a broken topmost overlay.
    DestroyWindow(dock.hwnd);
  }
}
}  // namespace noya
