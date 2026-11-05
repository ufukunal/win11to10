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

void ShowClassicCalendar(HWND parent) {
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
  const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  const int x = std::max(0, screenWidth - width - 32);
  const int y = std::max(0, screenHeight - height - 80);
  HWND created = CreateWindowExW(WS_EX_TOOLWINDOW, kCalendarClass,
      L"NoyaPLANET | Klasik Takvim", WS_OVERLAPPED | WS_CAPTION |
      WS_SYSMENU | WS_VISIBLE, x, y, width, height,
      parent, nullptr, instance, nullptr);
  if (created) ShowWindow(created, SW_SHOWNORMAL);
}
}  // namespace noya
