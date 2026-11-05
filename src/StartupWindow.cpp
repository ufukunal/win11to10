#include "StartupWindow.h"
#include "StartupManager.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kWindowClass[] = L"NoyaPlanetStartupManagerV1";
constexpr int kList = 401;
constexpr int kDisable = 402;
constexpr int kRestore = 403;
constexpr int kRefresh = 404;
constexpr int kSettings = 405;

struct WindowState {
  HWND window{};
  HWND caption{};
  HWND list{};
  HWND details{};
  HWND disable{};
  HWND restore{};
  HWND refresh{};
  HWND settings{};
  HFONT font{};
  HFONT heading{};
  HBRUSH background{};
  std::vector<StartupEntry> entries;
};
WindowState state;

HWND CreateText(HWND owner, const wchar_t* text, HFONT font) {
  auto child = CreateWindowExW(0, L"STATIC", text,
      WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
      0, 0, 10, 10, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
  SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  return child;
}
HWND CreateButton(HWND owner, int id, const wchar_t* text) {
  auto child = CreateWindowExW(0, L"BUTTON", text,
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
      0, 0, 10, 10, owner,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
      GetModuleHandleW(nullptr), nullptr);
  SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
  return child;
}

int SelectedIndex() {
  if (!state.list) return -1;
  const auto index = SendMessageW(state.list, LB_GETCURSEL, 0, 0);
  if (index == LB_ERR || index < 0 ||
      static_cast<std::size_t>(index) >= state.entries.size()) return -1;
  return static_cast<int>(index);
}

void UpdateSelection() {
  const int index = SelectedIndex();
  if (index < 0) {
    SetWindowTextW(state.details, L"Bir öğe seç. Değişiklikler yalnızca onayla yapılır.");
    EnableWindow(state.disable, FALSE);
    EnableWindow(state.restore, FALSE);
    return;
  }
  const auto& item = state.entries[static_cast<std::size_t>(index)];
  std::wstring description = item.name + L"\r\n";
  if (item.protectedEntry) {
    description += L"Korunan sistem veya uygulama girdisi; değiştirilemez.";
  } else {
    description += item.command;
    if (description.size() > 500) description.resize(500);
  }
  SetWindowTextW(state.details, description.c_str());
  EnableWindow(state.disable, !item.disabled && !item.protectedEntry);
  EnableWindow(state.restore, item.disabled && !item.protectedEntry);
}

void Refresh() {
  std::wstring error;
  state.entries = ListCurrentUserStartup(error);
  SendMessageW(state.list, LB_RESETCONTENT, 0, 0);
  for (const auto& item : state.entries) {
    const std::wstring label =
        std::wstring(item.disabled ? L"[Kapalı] " : L"[Açık] ") + item.name +
        (item.protectedEntry ? L" (korunan)" : L"");
    SendMessageW(state.list, LB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(label.c_str()));
  }
  UpdateSelection();
  if (!error.empty())
    MessageBoxW(state.window, error.c_str(), L"Başlangıç kayıtları",
                MB_ICONWARNING | MB_OK);
}

void Modify(bool disable) {
  const int index = SelectedIndex();
  if (index < 0) return;
  const auto item = state.entries[static_cast<std::size_t>(index)];
  if (item.protectedEntry || item.disabled == disable) return;
  const std::wstring question = L"Yalnızca bu kullanıcı için \"" + item.name +
     (disable ? L"\" otomatik başlangıçtan kaldırılsın mı?\r\n"
                L"Özgün kayıt yedeklenir ve geri alınabilir."
              : L"\" özgün başlangıç kaydı geri getirilsin mi?");
  if (MessageBoxW(state.window, question.c_str(), L"NoyaPLANET 11to10",
                  MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
  std::wstring error;
  if (!ChangeCurrentUserStartup(item.name, disable, error)) {
    MessageBoxW(state.window, error.c_str(), L"İşlem tamamlanamadı",
                MB_OK | MB_ICONERROR);
  }
  Refresh();
}

void Layout(HWND hwnd) {
  RECT rect{};
  GetClientRect(hwnd, &rect);
  const int w = rect.right, h = rect.bottom;
  const int width = std::max(100, w - 40);
  MoveWindow(state.caption, 20, 18, width, 55, TRUE);
  MoveWindow(state.list, 20, 82, width, std::max(100, h - 277), TRUE);
  MoveWindow(state.details, 20, h - 185, width, 65, TRUE);
  const int bw = std::max(100, (width - 24) / 4);
  MoveWindow(state.disable, 20, h - 104, bw, 42, TRUE);
  MoveWindow(state.restore, 28 + bw, h - 104, bw, 42, TRUE);
  MoveWindow(state.refresh, 36 + bw * 2, h - 104, bw, 42, TRUE);
  MoveWindow(state.settings, 44 + bw * 3, h - 104, bw, 42, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE:
      state.window = hwnd;
      state.font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
          FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      state.heading = CreateFontW(-23, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE,
          FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      state.background = CreateSolidBrush(RGB(25, 35, 50));
      state.caption = CreateText(hwnd,
          L"Kullanıcı başlangıç öğeleri\r\n"
          L"Yalnızca HKCU Run. Windows servisleri ve diğer kullanıcılar etkilenmez.",
          state.heading);
      state.list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
          WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_TABSTOP | WS_VSCROLL,
          0, 0, 10, 10, hwnd,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kList)),
          GetModuleHandleW(nullptr), nullptr);
      SendMessageW(state.list, WM_SETFONT,
                   reinterpret_cast<WPARAM>(state.font), TRUE);
      state.details = CreateText(hwnd, L"Öğe seçiniz.", state.font);
      state.disable = CreateButton(hwnd, kDisable, L"Devre dışı bırak");
      state.restore = CreateButton(hwnd, kRestore, L"Geri yükle");
      state.refresh = CreateButton(hwnd, kRefresh, L"Yenile");
      state.settings = CreateButton(hwnd, kSettings, L"Windows ayarları");
      Refresh();
      Layout(hwnd);
      return 0;
    case WM_SIZE:
      Layout(hwnd);
      return 0;
    case WM_GETMINMAXINFO:
      reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = POINT{720, 520};
      return 0;
    case WM_COMMAND:
      if (LOWORD(wParam) == kList && HIWORD(wParam) == LBN_SELCHANGE)
        UpdateSelection();
      if (HIWORD(wParam) == BN_CLICKED) {
        switch (LOWORD(wParam)) {
          case kDisable: Modify(true); break;
          case kRestore: Modify(false); break;
          case kRefresh: Refresh(); break;
          case kSettings:
            ShellExecuteW(hwnd, L"open", L"ms-settings:startupapps",
                          nullptr, nullptr, SW_SHOWNORMAL);
            break;
          default: break;
        }
      }
      return 0;
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      SetTextColor(dc, RGB(235, 240, 249));
      SetBkColor(dc, RGB(25, 35, 50));
      return reinterpret_cast<INT_PTR>(state.background);
    }
    case WM_DESTROY:
      if (state.font) DeleteObject(state.font);
      if (state.heading) DeleteObject(state.heading);
      if (state.background) DeleteObject(state.background);
      state = WindowState{};
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
}
}  // namespace

void ShowStartupManager(HWND owner) {
  if (IsWindow(state.window)) {
    SetForegroundWindow(state.window);
    return;
  }
  const auto instance = GetModuleHandleW(nullptr);
  WNDCLASSEXW cls{};
  cls.cbSize = sizeof(cls);
  cls.lpfnWndProc = WindowProc;
  cls.hInstance = instance;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.lpszClassName = kWindowClass;
  cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!GetClassInfoExW(instance, kWindowClass, &cls) &&
      !RegisterClassExW(&cls)) return;
  CreateWindowExW(WS_EX_APPWINDOW, kWindowClass,
      L"NoyaPLANET 11to10 | Başlangıç Yönetimi",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 850, 600,
      owner, nullptr, instance, nullptr);
}
}  // namespace noya
