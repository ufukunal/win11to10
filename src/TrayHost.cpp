#include "TrayHost.h"

#include <windows.h>
#include <shellapi.h>

#include <cwchar>

namespace noya {
namespace {
constexpr UINT kIconId = 1;

NOTIFYICONDATAW MakeData(HWND owner) {
  NOTIFYICONDATAW data{};
  data.cbSize = sizeof(data);
  data.hWnd = owner;
  data.uID = kIconId;
  return data;
}
}  // namespace

bool AddOwnTrayIcon(HWND owner) {
  NOTIFYICONDATAW data = MakeData(owner);
  data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  data.uCallbackMessage = kTrayCallbackMessage;
  data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  wcscpy_s(data.szTip, L"NoyaPLANET 11to10");
  if (!Shell_NotifyIconW(NIM_ADD, &data)) return false;
  data.uVersion = NOTIFYICON_VERSION_4;
  Shell_NotifyIconW(NIM_SETVERSION, &data);
  return true;
}

void RemoveOwnTrayIcon(HWND owner) {
  NOTIFYICONDATAW data = MakeData(owner);
  Shell_NotifyIconW(NIM_DELETE, &data);
}

void ShowOwnTrayMenu(HWND owner) {
  HMENU menu = CreatePopupMenu();
  if (!menu) return;
  AppendMenuW(menu, MF_STRING, kTrayShowDashboard, L"Kontrol panelini göster");
  AppendMenuW(menu, MF_STRING, kTrayOpenStart, L"Başlat önizlemesini aç");
  AppendMenuW(menu, MF_STRING, kTrayOpenTaskbar, L"Görev çubuğu önizlemesi");
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, kTrayExit, L"Uygulamadan çık");
  POINT cursor{};
  GetCursorPos(&cursor);
  SetForegroundWindow(owner);
  TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
                 cursor.x, cursor.y, 0, owner, nullptr);
  DestroyMenu(menu);
  PostMessageW(owner, WM_NULL, 0, 0);
}
}  // namespace noya
