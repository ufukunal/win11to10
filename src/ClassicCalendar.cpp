#include "ClassicCalendar.h"

#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <string>

namespace noya {
namespace {
constexpr wchar_t kCalendarClass[] = L"NoyaPlanetClassicCalendarV1";
HWND calendarWindow{};
HFONT labelFont{};
HFONT titleFont{};
constexpr COLORREF kBackground = RGB(27, 38, 59);
HBRUSH background{};

LRESULT CALLBACK CalendarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      calendarWindow = hwnd;
      labelFont = CreateFontW(-17, 0, 0, 0, FW_NORMAL,
                 FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH, L"Segoe UI");
      titleFont = CreateFontW(-23, 0, 0, 0, FW_SEMIBOLD,
                 FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH, L"Segoe UI");
      background = CreateSolidBrush(kBackground);
      SYSTEMTIME now{};
      GetLocalTime(&now);
      wchar_t label[128]{};
      swprintf_s(label, L"Bugün: %02u.%02u.%04u", now.wDay, now.wMonth, now.wYear);
      HWND heading = CreateWindowExW(0, L"STATIC", label,
          WS_CHILD | WS_VISIBLE, 16, 16, 280, 32,
          hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
      SendMessageW(heading, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);

      HWND note = CreateWindowExW(0, L"STATIC",
          L"Yerel takvim. Windows bildirim alanını değiştirmez.",
          WS_CHILD | WS_VISIBLE, 16, 54, 296, 44,
          hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
      SendMessageW(note, WM_SETFONT, reinterpret_cast<WPARAM>(labelFont), TRUE);

      HWND month = CreateWindowExW(0, MONTHCAL_CLASSW, L"",
          WS_CHILD | WS_VISIBLE | MCS_NOTODAYCIRCLE,
          12, 108, 300, 220, hwnd, nullptr,
          GetModuleHandleW(nullptr), nullptr);
      if (!month) {
        MessageBoxW(hwnd, L"Windows takvim bileşeni yüklenemedi.",
                    L"NoyaPLANET 11to10", MB_ICONWARNING | MB_OK);
      } else {
        MonthCal_SetCurSel(month, &now);
      }
      return 0;
    }
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, RGB(238, 242, 249));
      SetBkColor(dc, kBackground);
      return reinterpret_cast<LRESULT>(background);
    }
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      if (labelFont) DeleteObject(labelFont);
      if (titleFont) DeleteObject(titleFont);
      if (background) DeleteObject(background);
      labelFont = nullptr;
      titleFont = nullptr;
      background = nullptr;
      calendarWindow = nullptr;
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}
}  // namespace

bool FitCalendar(RECT work, RECT reference, int width, int height,
                 int& x, int& y) {
  const int workWidth = work.right - work.left;
  const int workHeight = work.bottom - work.top;
  if (workWidth <= 0 || workHeight <= 0 || width <= 0 || height <= 0)
    return false;
  // Place above the app-owned taskbar clock, never outside its monitor.
  x = std::clamp(static_cast<int>(reference.right) - width,
                 static_cast<int>(work.left),
                 std::max(static_cast<int>(work.left),
                          static_cast<int>(work.right) - width));
  y = std::clamp(static_cast<int>(reference.top) - height,
                 static_cast<int>(work.top),
                 std::max(static_cast<int>(work.top),
                          static_cast<int>(work.bottom) - height));
  return true;
}

bool ClassicCalendarPlacementSelfTest() {
  int x{}, y{};
  if (!FitCalendar(RECT{-1920, -1080, 0, 0},
                   RECT{-220, -44, -100, 0}, 336, 405, x, y) ||
      x != -436 || y != -449) return false;
  if (!FitCalendar(RECT{0, 0, 320, 240},
                   RECT{230, 200, 310, 240}, 336, 405, x, y) ||
      x != 0 || y != 0) return false;
  return !FitCalendar(RECT{0, 0, 0, 0},
                      RECT{0, 0, 0, 0}, 336, 405, x, y);
}

void ShowClassicCalendarAt(HWND parent, HWND anchor) {
  if (IsWindow(calendarWindow)) {
    ShowWindow(calendarWindow, SW_SHOW);
    SetForegroundWindow(calendarWindow);
    return;
  }
  INITCOMMONCONTROLSEX controls{};
  controls.dwSize = sizeof(controls);
  controls.dwICC = ICC_DATE_CLASSES;
  if (!InitCommonControlsEx(&controls)) return;

  const HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSEXW cls{};
  cls.cbSize = sizeof(cls);
  cls.hInstance = instance;
  cls.lpfnWndProc = CalendarProc;
  cls.lpszClassName = kCalendarClass;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!GetClassInfoExW(instance, kCalendarClass, &cls) &&
      !RegisterClassExW(&cls)) return;

  const int width = 336;
  const int height = 405;
  HWND referenceWindow = IsWindow(anchor) ? anchor : parent;
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  const HMONITOR monitor = MonitorFromWindow(referenceWindow,
                                             MONITOR_DEFAULTTOPRIMARY);
  if (!GetMonitorInfoW(monitor, &monitorInfo)) return;
  RECT reference{};
  if (!GetWindowRect(referenceWindow, &reference)) return;
  int x{}, y{};
  if (!FitCalendar(monitorInfo.rcWork, reference, width, height, x, y))
    return;
  HWND created = CreateWindowExW(WS_EX_TOOLWINDOW, kCalendarClass,
      L"NoyaPLANET | Klasik Takvim", WS_OVERLAPPED | WS_CAPTION |
      WS_SYSMENU | WS_VISIBLE, x, y, width, height,
      parent, nullptr, instance, nullptr);
  if (created) ShowWindow(created, SW_SHOWNORMAL);
}

void ShowClassicCalendar(HWND parent) {
  ShowClassicCalendarAt(parent, parent);
}
}  // namespace noya
