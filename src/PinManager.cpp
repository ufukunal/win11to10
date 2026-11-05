#include "PinManager.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace noya {
namespace {
constexpr wchar_t kFolder[] = L"NoyaPLANET\\11to10";
constexpr wchar_t kFilename[] = L"start-pins-v1.txt";
constexpr std::size_t kMaxPins = 4;
constexpr std::size_t kMaxBytes = 16384;
constexpr char kHeader[] = "NOYAPLANET_START_PINS_V1";

std::filesystem::path PinnedFile() {
  std::array<wchar_t, 32768> local{};
  const DWORD n = GetEnvironmentVariableW(
      L"LOCALAPPDATA", local.data(), static_cast<DWORD>(local.size()));
  if (n == 0 || n >= local.size()) return {};
  return std::filesystem::path(local.data()) / kFolder / kFilename;
}

std::string ToUtf8(const std::wstring& value) {
  if (value.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
      value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (n <= 0) return {};
  std::string bytes(static_cast<std::size_t>(n), '\0');
  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
      value.c_str(), static_cast<int>(value.size()), bytes.data(), n,
      nullptr, nullptr) != n) return {};
  return bytes;
}

std::wstring FromUtf8(const std::string& bytes) {
  if (bytes.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
      bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
  if (n <= 0) return {};
  std::wstring text(static_cast<std::size_t>(n), L'\0');
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(),
      static_cast<int>(bytes.size()), text.data(), n) != n) return {};
  return text;
}

bool Validate(const std::vector<std::wstring>& targets) {
  if (targets.size() > kMaxPins) return false;
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (!IsValidPinnedTarget(targets[i])) return false;
    if (std::find(targets.begin(), targets.begin() +
                  static_cast<std::ptrdiff_t>(i), targets[i]) !=
                  targets.begin() + static_cast<std::ptrdiff_t>(i)) return false;
  }
  return true;
}

bool Parse(std::string_view bytes, std::vector<std::wstring>& targets) {
  if (bytes.size() > kMaxBytes) return false;
  const std::string header = std::string(kHeader) + "\n";
  if (!bytes.starts_with(header)) return false;
  std::vector<std::wstring> parsed;
  std::size_t pos = header.size();
  while (pos < bytes.size()) {
    const auto next = bytes.find('\n', pos);
    const auto length = next == std::string_view::npos
        ? bytes.size() - pos : next - pos;
    const std::string encoded(bytes.substr(pos, length));
    if (encoded.empty()) {
      if (next == std::string_view::npos) break;
      return false;
    }
    const auto item = FromUtf8(encoded);
    if (!IsValidPinnedTarget(item)) return false;
    parsed.push_back(item);
    if (parsed.size() > kMaxPins) return false;
    if (next == std::string_view::npos) break;
    pos = next + 1;
  }
  if (!Validate(parsed)) return false;
  targets = std::move(parsed);
  return true;
}
}  // namespace

bool IsValidPinnedTarget(std::wstring_view value) {
  if (value.empty() || value.size() > 2048) return false;
  for (const auto c : value) {
    if (c < 0x20 || c == 0x7f) return false;
  }
  return true;
}

bool LoadPinnedTargets(std::vector<std::wstring>& targets,
                       bool& hadConfiguration, std::wstring& error) {
  hadConfiguration = false;
  const auto path = PinnedFile();
  if (path.empty()) {
    error = L"LOCALAPPDATA erişilemiyor.";
    return false;
  }
  if (!std::filesystem::exists(path)) {
    targets.clear(); // no file => default preview pins can be used
    return true;
  }
  hadConfiguration = true;
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = L"Sabitlenmiş uygulamalar okunamadı.";
    return false;
  }
  const std::string bytes((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
  if (in.bad() || !Parse(bytes, targets)) {
    error = L"Kayıtlı sabitlenmiş uygulama listesi geçersiz.";
    return false;
  }
  return true;
}

bool SavePinnedTargets(const std::vector<std::wstring>& targets,
                       std::wstring& error) {
  if (!Validate(targets)) {
    error = L"Sabitlenecek öğe listesi geçersiz.";
    return false;
  }
  const auto path = PinnedFile();
  if (path.empty()) {
    error = L"LOCALAPPDATA erişilemiyor.";
    return false;
  }
  std::string contents = std::string(kHeader) + "\n";
  for (const auto& item : targets) {
    const auto encoded = ToUtf8(item);
    if (encoded.empty()) {
      error = L"Uygulama adı UTF-8 biçiminde kaydedilemiyor.";
      return false;
    }
    contents += encoded + "\n";
  }
  if (contents.size() > kMaxBytes) {
    error = L"Sabitlenmiş uygulama listesi çok büyük.";
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    error = L"Uygulama ayar klasörü oluşturulamadı.";
    return false;
  }

  // Keep existing file intact if writing/flushing fails.
  const auto temp = path.wstring() + L"." +
      std::to_wstring(GetCurrentProcessId()) + L".tmp";
  HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    error = L"Geçici sabitleme dosyası oluşturulamadı.";
    return false;
  }
  DWORD written{};
  const bool ok = WriteFile(file, contents.data(),
       static_cast<DWORD>(contents.size()), &written, nullptr) &&
       written == contents.size() && FlushFileBuffers(file);
  CloseHandle(file);
  if (!ok) {
    DeleteFileW(temp.c_str());
    error = L"Sabitleme dosyası yazılamadı.";
    return false;
  }
  if (!MoveFileExW(temp.c_str(), path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    DeleteFileW(temp.c_str());
    error = L"Sabitleme ayarları güvenli biçimde değiştirilemedi.";
    return false;
  }
  return true;
}

bool PinModelSelfTest() {
  if (!IsValidPinnedTarget(L"explorer.exe") ||
      IsValidPinnedTarget(L"bad\ncommand") ||
      IsValidPinnedTarget(L"")) return false;
  const std::vector<std::wstring> original{L"explorer.exe", L"ms-settings:",
                                          L"C:\\Uygulamalar\\Örnek.lnk"};
  if (!Validate(original)) return false;
  std::string contents = std::string(kHeader) + "\n";
  for (const auto& target : original) {
    contents += ToUtf8(target) + "\n";
  }
  std::vector<std::wstring> parsed;
  if (!Parse(contents, parsed) || parsed != original) return false;
  return !Parse("wrong\n", parsed) &&
         !Parse(contents + "explorer.exe\n", parsed);
}
}  // namespace noya
