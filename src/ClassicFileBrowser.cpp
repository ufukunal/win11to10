#include "ClassicFileBrowser.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <shellapi.h>

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <iterator>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace noya {
namespace {
namespace fs = std::filesystem;
constexpr wchar_t kClassName[] = L"NoyaPlanetClassicFileBrowserV1";
constexpr int kBack = 501;
constexpr int kForward = 502;
constexpr int kUp = 503;
constexpr int kHome = 504;
constexpr int kRefresh = 505;
constexpr int kGo = 506;
constexpr int kAddress = 507;
constexpr int kFilter = 508;
constexpr int kFiles = 509;
constexpr int kOpenSelected = 510;
constexpr int kCopyPath = 511;
constexpr int kNativeExplorer = 512;
constexpr int kMaxFiles = 2500;
constexpr COLORREF kBackground = RGB(242, 245, 249);

struct FileItem {
  fs::path path;
  bool folder{};
  std::uintmax_t size{};
  bool sizeKnown{};
};
struct Browser {
  HWND window{};
  HWND address{};
  HWND filter{};
  HWND list{};
  HWND back{};
  HWND forward{};
  HWND up{};
  HWND home{};
  HWND go{};
  HWND refresh{};
  HWND open{};
  HWND copy{};
  HWND nativeExplorer{};
  HWND caption{};
  HWND status{};
  HFONT font{};
  HFONT heading{};
  HBRUSH background{};
  fs::path current;
  std::vector<fs::path> previous;
  std::vector<fs::path> next;
  std::vector<FileItem> files;
  std::vector<std::size_t> visible;
  std::wstring warning;
};
Browser browser;

std::wstring Lower(std::wstring_view s) {
  std::wstring text(s);
  std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(std::towlower(ch));
  });
  return text;
}

std::wstring Trim(std::wstring s) {
  const auto start = s.find_first_not_of(L" \t");
  if (start == std::wstring::npos) return {};
  const auto end = s.find_last_not_of(L" \t");
  return s.substr(start, end - start + 1);
}

std::wstring Text(HWND hwnd) {
  if (!IsWindow(hwnd)) return {};
  const int count = GetWindowTextLengthW(hwnd);
  if (count <= 0 || count > 32760) return {};
  std::wstring s(static_cast<std::size_t>(count) + 1, L'\0');
  const int actual = GetWindowTextW(hwnd, s.data(), count + 1);
  if (actual <= 0) return {};
  s.resize(static_cast<std::size_t>(actual));
  return s;
}

HWND Button(HWND hwnd, int id, const wchar_t* caption) {
  auto control = CreateWindowExW(0, L"BUTTON", caption,
      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
      0, 0, 20, 24, hwnd,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
      GetModuleHandleW(nullptr), nullptr);
  SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(browser.font), TRUE);
  return control;
}

HWND Label(HWND hwnd, const wchar_t* caption, HFONT font) {
  auto control = CreateWindowExW(0, L"STATIC", caption,
      WS_VISIBLE | WS_CHILD | SS_LEFT | SS_NOPREFIX,
      0, 0, 20, 24, hwnd, nullptr,
      GetModuleHandleW(nullptr), nullptr);
  SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  return control;
}

void SetStatus(const std::wstring& value) {
  SetWindowTextW(browser.status, value.c_str());
}

fs::path HomeFolder() {
  PWSTR folder = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &folder))) {
    fs::path result(folder);
    CoTaskMemFree(folder);
    folder = nullptr;
    std::error_code ec;
    if (fs::is_directory(result, ec)) return result;
  }
  if (folder) CoTaskMemFree(folder);
  wchar_t buffer[32768]{};
  const auto length = GetEnvironmentVariableW(
      L"USERPROFILE", buffer, static_cast<DWORD>(std::size(buffer)));
  if (length > 0 && length < std::size(buffer)) return fs::path(buffer);
  return fs::path(L"C:\\");
}

std::wstring FormatSize(std::uintmax_t bytes) {
  if (bytes < 1024) return std::to_wstring(bytes) + L" B";
  if (bytes < 1024 * 1024) return std::to_wstring(bytes / 1024) + L" KB";
  if (bytes < 1024ULL * 1024 * 1024)
    return std::to_wstring(bytes / (1024 * 1024)) + L" MB";
  return std::to_wstring(bytes / (1024ULL * 1024 * 1024)) + L" GB";
}

bool Scan(const fs::path& path, std::vector<FileItem>& found,
          std::wstring& warning) {
  std::error_code ec;
  if (!fs::is_directory(path, ec)) {
    warning = L"Klasör mevcut değil veya erişim izni bulunmuyor.";
    return false;
  }
  fs::directory_iterator iter(path, fs::directory_options::skip_permission_denied, ec);
  if (ec) {
    warning = L"Klasör içeriği okunamadı. Windows izinlerini kontrol edin.";
    return false;
  }
  const fs::directory_iterator end;
  for (int visited = 0; iter != end && visited < kMaxFiles; ++visited) {
    ec.clear();
    const auto& entry = *iter;
    const bool folder = entry.is_directory(ec);
    if (!ec) {
      FileItem info{};
      info.path = entry.path();
      info.folder = folder;
      if (!folder) {
        info.size = entry.file_size(ec);
        info.sizeKnown = !ec;
      }
      found.push_back(std::move(info));
    }
    iter.increment(ec);
    if (ec) {
      warning = L"Bazı öğeler okunamadı; liste kısmi olabilir.";
      break;
    }
  }
  if (found.size() >= kMaxFiles)
    warning = L"İlk 2500 öğe gösteriliyor; daha büyük klasörler kısaltıldı.";
  std::sort(found.begin(), found.end(), [](const FileItem& a, const FileItem& b) {
    if (a.folder != b.folder) return a.folder;
    return Lower(a.path.filename().wstring()) < Lower(b.path.filename().wstring());
  });
  return true;
}

void Render() {
  if (!IsWindow(browser.list)) return;
  const auto needle = Lower(Trim(Text(browser.filter)));
  browser.visible.clear();
  ListView_DeleteAllItems(browser.list);
  for (std::size_t i = 0; i < browser.files.size(); ++i) {
    const auto& file = browser.files[i];
    const auto title = file.path.filename().wstring();
    if (!needle.empty() && Lower(title).find(needle) == std::wstring::npos)
      continue;
    LVITEMW row{};
    row.mask = LVIF_TEXT | LVIF_PARAM;
    row.iItem = static_cast<int>(browser.visible.size());
    row.pszText = const_cast<wchar_t*>(title.c_str());
    row.lParam = static_cast<LPARAM>(i);
    const int inserted = ListView_InsertItem(browser.list, &row);
    if (inserted < 0) continue;
    browser.visible.push_back(i);
    const std::wstring type = file.folder ? L"Dosya klasörü"
        : (file.path.extension().empty() ? L"Dosya" :
           file.path.extension().wstring() + L" dosyası");
    ListView_SetItemText(browser.list, inserted, 1,
                         const_cast<wchar_t*>(type.c_str()));
    const auto size = file.folder ? L"" :
        (file.sizeKnown ? FormatSize(file.size) : L"—");
    ListView_SetItemText(browser.list, inserted, 2,
                         const_cast<wchar_t*>(size.c_str()));
  }
  SetStatus(std::to_wstring(browser.visible.size()) + L" öğe gösteriliyor.  " +
            browser.warning);
}

bool BrowseTo(fs::path destination, bool remember = true) {
  if (destination.empty()) return false;
  if (destination.is_relative()) destination = browser.current / destination;
  destination = destination.lexically_normal();
  std::vector<FileItem> found;
  std::wstring warning;
  if (!Scan(destination, found, warning)) {
    MessageBoxW(browser.window, warning.c_str(),
                L"Dosya Gezgini", MB_OK | MB_ICONWARNING);
    return false;
  }
  if (remember && !browser.current.empty() && destination != browser.current) {
    browser.previous.push_back(browser.current);
    if (browser.previous.size() > 60) browser.previous.erase(browser.previous.begin());
    browser.next.clear();
  }
  browser.current = destination;
  browser.files = std::move(found);
  browser.warning = std::move(warning);
  SetWindowTextW(browser.address, browser.current.c_str());
  SetWindowTextW(browser.filter, L"");
  EnableWindow(browser.back, !browser.previous.empty());
  EnableWindow(browser.forward, !browser.next.empty());
  EnableWindow(browser.up, browser.current.has_parent_path() &&
                         browser.current.parent_path() != browser.current);
  Render();
  return true;
}

const FileItem* Selected() {
  const int row = ListView_GetNextItem(browser.list, -1, LVNI_SELECTED);
  if (row < 0 || static_cast<std::size_t>(row) >= browser.visible.size()) return nullptr;
  const auto index = browser.visible[static_cast<std::size_t>(row)];
  return index < browser.files.size() ? &browser.files[index] : nullptr;
}

void OpenNative(const fs::path& path) {
  const auto result = ShellExecuteW(browser.window, L"open", path.c_str(),
                                    nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(result) <= 32)
    MessageBoxW(browser.window, L"Dosya Windows tarafından açılamadı.",
                L"NoyaPLANET 11to10", MB_OK | MB_ICONWARNING);
}

void OpenSelection() {
  const FileItem* item = Selected();
  if (!item) return;
  if (item->folder) {
    BrowseTo(item->path);
  } else {
    OpenNative(item->path);
  }
}

void CopySelectionPath() {
  const FileItem* item = Selected();
  if (!item || !OpenClipboard(browser.window)) return;
  const std::wstring text = item->path.wstring();
  const auto bytes = (text.size()+1)*sizeof(wchar_t);
  HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!data) { CloseClipboard(); return; }
  void* target = GlobalLock(data);
  if (!target) {
    GlobalFree(data);
    CloseClipboard();
    return;
  }
  std::memcpy(target, text.c_str(), bytes);
  GlobalUnlock(data);
  EmptyClipboard();
  if (!SetClipboardData(CF_UNICODETEXT, data)) GlobalFree(data);
  CloseClipboard();
}

void Layout(HWND hwnd) {
  RECT rect{};
  GetClientRect(hwnd, &rect);
  const int dpi = static_cast<int>(GetDpiForWindow(hwnd));
  auto S = [dpi](int n) { return MulDiv(n, dpi, 96); };
  const int w = rect.right, h = rect.bottom;
  const int margin = S(16), gap = S(6), button = S(74);
  MoveWindow(browser.caption, margin, S(10), std::max(1, w-2*margin), S(35), TRUE);
  const HWND buttons[]{browser.back, browser.forward, browser.up,
                        browser.home, browser.refresh};
  int x = margin;
  for (const HWND control : buttons) {
    MoveWindow(control, x, S(54), button, S(36), TRUE);
    x += button+gap;
  }
  const int addressX = margin;
  const int goWidth = S(64);
  const int addressWidth = std::max(S(130), w-2*margin-goWidth-gap);
  MoveWindow(browser.address, addressX, S(99), addressWidth, S(34), TRUE);
  MoveWindow(browser.go, addressX+addressWidth+gap, S(99), goWidth, S(34), TRUE);
  MoveWindow(browser.filter, margin, S(142), std::max(S(100),w-2*margin), S(32), TRUE);
  MoveWindow(browser.list, margin, S(184),
             std::max(S(100), w-2*margin), std::max(S(140),h-S(300)), TRUE);
  const int bottom = std::max(S(390), h-S(102));
  MoveWindow(browser.open, margin, bottom, S(144), S(38), TRUE);
  MoveWindow(browser.copy, margin+S(150), bottom, S(145), S(38), TRUE);
  MoveWindow(browser.nativeExplorer, margin+S(301), bottom, S(205), S(38), TRUE);
  MoveWindow(browser.status, margin, bottom+S(48),
             std::max(S(120), w-2*margin), S(32), TRUE);
}

LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      browser.window = hwnd;
      browser.background = CreateSolidBrush(kBackground);
      browser.font = CreateFontW(-18, 0, 0, 0, FW_NORMAL,
          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      browser.heading = CreateFontW(-24, 0, 0, 0, FW_SEMIBOLD,
          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
          DEFAULT_PITCH, L"Segoe UI");
      browser.caption = Label(hwnd, L"Klasik Dosya Gezgini — bağımsız tarayıcı",
                              browser.heading);
      browser.back = Button(hwnd, kBack, L"Geri");
      browser.forward = Button(hwnd, kForward, L"İleri");
      browser.up = Button(hwnd, kUp, L"Üst");
      browser.home = Button(hwnd, kHome, L"Belgeler");
      browser.refresh = Button(hwnd, kRefresh, L"Yenile");
      browser.go = Button(hwnd, kGo, L"Git");
      browser.open = Button(hwnd, kOpenSelected, L"Seçileni aç");
      browser.copy = Button(hwnd, kCopyPath, L"Yolu kopyala");
      browser.nativeExplorer = Button(hwnd, kNativeExplorer, L"Windows Gezgini'nde aç");
      browser.address = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
          WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10,
          hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAddress)),
          GetModuleHandleW(nullptr), nullptr);
      browser.filter = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
          WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10,
          hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFilter)),
          GetModuleHandleW(nullptr), nullptr);
      SendMessageW(browser.address, WM_SETFONT, reinterpret_cast<WPARAM>(browser.font), TRUE);
      SendMessageW(browser.filter, WM_SETFONT, reinterpret_cast<WPARAM>(browser.font), TRUE);
      SendMessageW(browser.filter, EM_SETCUEBANNER, TRUE,
                   reinterpret_cast<LPARAM>(L"Geçerli klasörde ara..."));
      browser.list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
          WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT |
          LVS_SINGLESEL | LVS_SHOWSELALWAYS,
          0, 0, 10, 10, hwnd,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFiles)),
          GetModuleHandleW(nullptr), nullptr);
      if (!browser.list) return -1;
      SendMessageW(browser.list, WM_SETFONT, reinterpret_cast<WPARAM>(browser.font), TRUE);
      ListView_SetExtendedListViewStyle(browser.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
      LVCOLUMNW column{};
      column.mask = LVCF_TEXT | LVCF_WIDTH;
      column.pszText = const_cast<wchar_t*>(L"Ad");
      column.cx = 370;
      ListView_InsertColumn(browser.list, 0, &column);
      column.pszText = const_cast<wchar_t*>(L"Tür");
      column.cx = 180;
      ListView_InsertColumn(browser.list, 1, &column);
      column.pszText = const_cast<wchar_t*>(L"Boyut");
      column.cx = 120;
      ListView_InsertColumn(browser.list, 2, &column);
      browser.status = Label(hwnd, L"", browser.font);
      BrowseTo(HomeFolder(), false);
      Layout(hwnd);
      return 0;
    }
    case WM_SIZE:
      Layout(hwnd);
      return 0;
    case WM_GETMINMAXINFO:
      reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = POINT{680, 520};
      return 0;
    case WM_COMMAND:
      if (LOWORD(wParam) == kFilter && HIWORD(wParam) == EN_CHANGE) {
        Render();
        return 0;
      }
      if (HIWORD(wParam) != BN_CLICKED) break;
      switch (LOWORD(wParam)) {
        case kBack: {
          if (browser.previous.empty()) break;
          auto old = browser.current;
          auto destination = browser.previous.back();
          if (BrowseTo(destination, false)) {
            browser.previous.pop_back();
            browser.next.push_back(std::move(old));
          }
          EnableWindow(browser.back, !browser.previous.empty());
          EnableWindow(browser.forward, !browser.next.empty());
          break;
        }
        case kForward: {
          if (browser.next.empty()) break;
          auto old = browser.current;
          auto destination = browser.next.back();
          if (BrowseTo(destination, false)) {
            browser.next.pop_back();
            browser.previous.push_back(std::move(old));
          }
          EnableWindow(browser.back, !browser.previous.empty());
          EnableWindow(browser.forward, !browser.next.empty());
          break;
        }
        case kUp: {
          const auto parent = browser.current.parent_path();
          if (!parent.empty() && parent != browser.current) BrowseTo(parent);
          break;
        }
        case kHome: BrowseTo(HomeFolder()); break;
        case kRefresh: BrowseTo(browser.current, false); break;
        case kGo: BrowseTo(Trim(Text(browser.address))); break;
        case kOpenSelected: OpenSelection(); break;
        case kCopyPath: CopySelectionPath(); break;
        case kNativeExplorer: OpenNative(browser.current); break;
        default: break;
      }
      return 0;
    case WM_NOTIFY: {
      const auto* info = reinterpret_cast<const NMHDR*>(lParam);
      if (info && info->idFrom == kFiles && info->code == NM_DBLCLK) {
        OpenSelection();
        return 0;
      }
      break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
      HDC dc = reinterpret_cast<HDC>(wParam);
      if (message == WM_CTLCOLORSTATIC) {
        SetBkColor(dc, kBackground);
        SetTextColor(dc, RGB(28, 44, 64));
        return reinterpret_cast<LRESULT>(browser.background);
      }
      return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      if (browser.font) DeleteObject(browser.font);
      if (browser.heading) DeleteObject(browser.heading);
      if (browser.background) DeleteObject(browser.background);
      browser = Browser{};
      return 0;
    default:
      break;
  }
  return DefWindowProcW(hwnd, message, wParam, lParam);
}
}  // namespace

void ShowClassicFileBrowser(HWND dashboard) {
  if (IsWindow(browser.window)) {
    SetForegroundWindow(browser.window);
    return;
  }
  INITCOMMONCONTROLSEX init{};
  init.dwSize = sizeof(init);
  init.dwICC = ICC_LISTVIEW_CLASSES;
  if (!InitCommonControlsEx(&init)) return;
  const HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSEXW window{};
  window.cbSize = sizeof(window);
  window.hInstance = instance;
  window.lpfnWndProc = Proc;
  window.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  window.lpszClassName = kClassName;
  window.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
  if (!GetClassInfoExW(instance, kClassName, &window) &&
      !RegisterClassExW(&window)) return;
  HWND created = CreateWindowExW(WS_EX_APPWINDOW, kClassName,
      L"NoyaPLANET 11to10 — Klasik Dosya Gezgini",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
      dashboard, nullptr, instance, nullptr);
  if (created) ShowWindow(created, SW_SHOWNORMAL);
}

bool ClassicFileBrowserModelSelfTest() {
  return Lower(L"Folder.TXT") == L"folder.txt" &&
         Trim(L"  C:\\Test  ") == L"C:\\Test" &&
         FormatSize(1000) == L"1000 B" &&
         FormatSize(2048) == L"2 KB";
}
}  // namespace noya
