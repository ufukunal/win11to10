#include "TaskbarPreview.h"

#include <windows.h>
#include <array>
#include <algorithm>
#include <string>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kClassName[] = L"NoyaPlanetTaskbarPreviewV1";
constexpr int kStart = 400;
constexpr int kRefresh = 401;
constexpr int kWindowsBase = 410;
constexpr int kMaximumWindows = 6;
constexpr COLORREF kBackground = RGB(26, 37, 56);

struct State {
  HWND window{};
  HWND owner{};
  HWND caption{};
  HWND hint{};
  HWND start{};
  HWND refresh{};
  std::array<HWND, kMaximumWindows> buttons{};
  std::vector<HWND> windows;
  HFONT font{};
  HBRUSH background{};
};
State gState;

HWND Button(HWND owner, const wchar_t* name, int id) {
  HWND hwnd = CreateWindowExW(0, L"BUTTON", name,
      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 10, 10,
      owner, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
      GetModuleHandleW(nullptr), nullptr);
  SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gState.font), TRUE);
  return hwnd;
}

HWND Caption(HWND owner, const wchar_t* text) {
  HWND hwnd = CreateWindowExW(0, L"STATIC", text,
      WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 10, 10,
      owner, nullptr, GetModuleHandleW(nullptr), nullptr);
  SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gState.font), TRUE);
  return hwnd;
}

BOOL CALLBACK CollectWindow(HWND hwnd, LPARAM param) {
  auto* output = reinterpret_cast<std::vector<HWND>*>(param);
  if (hwnd == gState.window || hwnd == gState.owner ||
      !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
    return TRUE;
  }
  const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  if ((exStyle & WS_EX_TOOLWINDOW) != 0 || GetWindowTextLengthW(hwnd) <= 0) {
    return TRUE;
  }
  output->push_back(hwnd);
  return TRUE;
}

std::wstring WindowTitle(HWND hwnd) {
  const int count = GetWindowTextLengthW(hwnd);
  if (count <= 0) return L"";
  std::wstring title(static_cast<std::size_t>(count) + 1, L'\0');
  const int actual = GetWindowTextW(hwnd, title.data(), count + 1);
  title.resize(static_cast<std::size_t>(std::max(0, actual)));
  if (title.size() > 21) {
    title.resize(20);
    title += L"…";
  }
  return title;
}

void Refresh() {
  gState.windows.clear();
  EnumWindows(CollectWindow, reinterpret_cast<LPARAM>(&gState.windows));
  const std::size_t shown = std::min(
      gState.windows.size(), static_cast<std::size_t>(kMaximumWindows));
  for (std::size_t i = 0; i < gState.buttons.size(); ++i) {
    if (i >= shown) {
      ShowWindow(gState.buttons[i], SW_HIDE);
    } else {
      const auto text = WindowTitle(gState.windows[i]);
      SetWindowTextW(gState.buttons[i], text.c_str());
      ShowWindow(gState.buttons[i], SW_SHOW);
    }
  }
  std::wstring note = L"Etkin pencereler: " + std::to_wstring(gState.windows.size()) +
                      L"  |  En fazla 6 tanesi gösteriliyor. Yenile ile güncelle.";
  SetWindowTextW(gState.hint, note.c_str());
}

void Layout(HWND hwnd) {
  RECT rect{};
  GetClientRect(hwnd, &rect);
  const int width = rect.right;
  MoveWindow(gState.caption, 18, 12, std::max(1, width - 36), 26, TRUE);
  MoveWindow(gState.start, 18, 49, 98, 43, TRUE);
  MoveWindow(gState.refresh, width - 124, 49, 106, 43, TRUE);
  const int gap = 6;
  const int left = 124;
  const int available = std::max(200, width - 124 - 132);
  const int cellWidth = std::max(36, (available - (kMaximumWindows - 1) * gap) /
                                  kMaximumWindows);
  for (int i = 0; i < kMaximumWindows; ++i) {
    MoveWindow(gState.buttons[static_cast<std::size_t>(i)],
               left + i * (cellWidth + gap), 49, cellWidth, 43, TRUE);
  }
  MoveWindow(gState.hint, 18, 105, std::max(1, width - 36), 26, TRUE);
}

LRESULT CALLBACK Procedure(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE:
      gState.window = hwnd;
      gState.font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
          FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
          CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
      gState.background = CreateSolidBrush(kBackground);
      gState.caption = Caption(hwnd,
          L"Windows 10 görev çubuğu prototipi — gerçek görev çubuğunun yerine geçmez");
      gState.hint = Caption(hwnd, L"Pencereler okunuyor...");
      gState.start = Button(hwnd, L"⊞ Başlat", kStart);
      gState.refresh = Button(hwnd, L"Yenile", kRefresh);
      for (int i = 0; i < kMaximumWindows; ++i)
        gState.buttons[static_cast<std::size_t>(i)] =
            Button(hwnd, L"", kWindowsBase + i);
      Refresh();
      Layout(hwnd);
      return 0;
    case WM_SIZE:
      Layout(hwnd);
      return 0;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED) {
        const int id = LOWORD(wParam);
        if (id == kStart) {
          PostMessageW(gState.owner, kRequestStartMenuPreview, 0, 0);
        } else if (id == kRefresh) {
          Refresh();
        } else if (id >= kWindowsBase && id < kWindowsBase + kMaximumWindows) {
          const auto index = static_cast<std::size_t>(id - kWindowsBase);
          if (index < gState.windows.size() && IsWindow(gState.windows[index])) {
            const auto chosen = gState.windows[index];
            if (IsIconic(chosen)) ShowWindow(chosen, SW_RESTORE);
            SetForegroundWindow(chosen);
          }
        }
      }
      return 0;
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, RGB(235, 242, 249));
      SetBkColor(dc, kBackground);
      return reinterpret_cast<INT_PTR>(gState.background);
    }
    case WM_DESTROY:
      if (gState.font) DeleteObject(gState.font);
      if (gState.background) DeleteObject(gState.background);
      gState = State{};
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
}
}  // namespace

void ShowTaskbarPreview(HWND dashboardWindow) {
  if (IsWindow(gState.window)) {
    SetForegroundWindow(gState.window);
    return;
  }
  const HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSEXW cls{};
  cls.cbSize = sizeof(cls);
  cls.lpfnWndProc = Procedure;
  cls.hInstance = instance;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.lpszClassName = kClassName;
  cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!GetClassInfoExW(instance, kClassName, &cls)) {
    cls.lpfnWndProc = Procedure;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.lpszClassName = kClassName;
    if (!RegisterClassExW(&cls)) return;
  }
  gState.owner = dashboardWindow;
  const int width = 920;
  const int x = std::max(0, (GetSystemMetrics(SM_CXSCREEN) - width) / 2);
  const int y = std::max(0, GetSystemMetrics(SM_CYSCREEN) - 235);
  CreateWindowExW(WS_EX_TOOLWINDOW, kClassName,
      L"NoyaPLANET 11to10 | Görev Çubuğu Önizlemesi",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
      x, y, width, 175, dashboardWindow,
      nullptr, instance, nullptr);
}
}  // namespace noya
