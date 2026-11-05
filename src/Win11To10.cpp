#include <windows.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
using Values = std::map<std::string, int>;

namespace {
enum class SetKind { Pointer, UiParameter, Animation };
struct Effect {
  const char* key;
  UINT get_code;
  UINT set_code;
  SetKind kind;
};

// Documented per-user SystemParametersInfo settings. No Explorer injection.
constexpr std::array<Effect, 15> effects{{
    {"window_animation", SPI_GETANIMATION, SPI_SETANIMATION, SetKind::Animation},
    {"menu_animation", SPI_GETMENUANIMATION, SPI_SETMENUANIMATION, SetKind::Pointer},
    {"menu_fade", SPI_GETMENUFADE, SPI_SETMENUFADE, SetKind::Pointer},
    {"combobox_animation", SPI_GETCOMBOBOXANIMATION, SPI_SETCOMBOBOXANIMATION, SetKind::Pointer},
    {"listbox_smooth_scrolling", SPI_GETLISTBOXSMOOTHSCROLLING, SPI_SETLISTBOXSMOOTHSCROLLING, SetKind::Pointer},
    {"selection_fade", SPI_GETSELECTIONFADE, SPI_SETSELECTIONFADE, SetKind::Pointer},
    {"tooltip_animation", SPI_GETTOOLTIPANIMATION, SPI_SETTOOLTIPANIMATION, SetKind::Pointer},
    {"tooltip_fade", SPI_GETTOOLTIPFADE, SPI_SETTOOLTIPFADE, SetKind::Pointer},
    {"cursor_shadow", SPI_GETCURSORSHADOW, SPI_SETCURSORSHADOW, SetKind::Pointer},
    {"drop_shadow", SPI_GETDROPSHADOW, SPI_SETDROPSHADOW, SetKind::Pointer},
    {"client_area_animation", SPI_GETCLIENTAREAANIMATION, SPI_SETCLIENTAREAANIMATION, SetKind::Pointer},
    {"ui_effects", SPI_GETUIEFFECTS, SPI_SETUIEFFECTS, SetKind::Pointer},
    {"drag_full_windows", SPI_GETDRAGFULLWINDOWS, SPI_SETDRAGFULLWINDOWS, SetKind::UiParameter},
    {"gradient_captions", SPI_GETGRADIENTCAPTIONS, SPI_SETGRADIENTCAPTIONS, SetKind::Pointer},
    {"font_smoothing", SPI_GETFONTSMOOTHING, SPI_SETFONTSMOOTHING, SetKind::UiParameter},
}};

bool validate(const Values& values, std::string& error) {
  if (values.size() != effects.size()) {
    error = "Unexpected or incomplete visual-effect settings.";
    return false;
  }
  for (const auto& effect : effects) {
    auto it = values.find(effect.key);
    if (it == values.end() || (it->second != 0 && it->second != 1)) {
      error = std::string("Invalid setting: ") + effect.key;
      return false;
    }
  }
  return true;
}

bool readEffects(Values& values, std::string& error) {
  Values result;
  for (const auto& e : effects) {
    BOOL value = FALSE;
    if (e.kind == SetKind::Animation) {
      ANIMATIONINFO animation{};
      animation.cbSize = sizeof(animation);
      if (!SystemParametersInfoW(e.get_code, sizeof(animation), &animation, 0)) {
        error = std::string("Cannot read ") + e.key + " (Win32 " +
                std::to_string(GetLastError()) + ")";
        return false;
      }
      value = animation.iMinAnimate != 0;
    } else if (!SystemParametersInfoW(e.get_code, 0, &value, 0)) {
      error = std::string("Cannot read ") + e.key + " (Win32 " +
              std::to_string(GetLastError()) + ")";
      return false;
    }
    result[e.key] = value ? 1 : 0;
  }
  values = std::move(result);
  return true;
}

bool writeEffects(const Values& values, std::string& error) {
  if (!validate(values, error)) return false;
  constexpr UINT flags = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE;
  for (const auto& e : effects) {
    const int value = values.at(e.key);
    BOOL success = FALSE;
    if (e.kind == SetKind::Animation) {
      ANIMATIONINFO animation{};
      animation.cbSize = sizeof(animation);
      animation.iMinAnimate = value;
      success = SystemParametersInfoW(e.set_code, sizeof(animation),
                                      &animation, flags);
    } else if (e.kind == SetKind::UiParameter) {
      success = SystemParametersInfoW(e.set_code, static_cast<UINT>(value),
                                      nullptr, flags);
    } else {
      success = SystemParametersInfoW(
          e.set_code, 0,
          reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(value)), flags);
    }
    if (!success) {
      error = std::string("Cannot set ") + e.key + " (Win32 " +
              std::to_string(GetLastError()) + ")";
      return false;
    }
  }
  return true;
}

Values performanceProfile(const Values& existing) {
  Values proposed = existing;
  for (auto& [key, value] : proposed)
    value = key == "font_smoothing" ? 1 : 0;
  return proposed;
}

constexpr std::string_view magic = "WIN11TO10_VISUAL_V1";
std::string serialize(const Values& values) {
  std::ostringstream out;
  out << magic << '\n';
  for (const auto& [key, value] : values) out << key << '=' << value << '\n';
  return out.str();
}

bool parse(const std::string& data, Values& result, std::string& error) {
  std::istringstream in(data);
  std::string line;
  if (!std::getline(in, line)) {
    error = "Backup is empty.";
    return false;
  }
  if (!line.empty() && line.back() == '\r') line.pop_back();
  if (line != magic) {
    error = "Unknown backup format.";
    return false;
  }
  Values parsed;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const auto pos = line.find('=');
    if (pos == std::string::npos || line.find('=', pos + 1) != std::string::npos) {
      error = "Malformed backup line.";
      return false;
    }
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    if (key.empty() || key.find_first_not_of("abcdefghijklmnopqrstuvwxyz_") != std::string::npos ||
        (value != "0" && value != "1") ||
        !parsed.emplace(key, value == "1" ? 1 : 0).second) {
      error = "Duplicate or invalid backup setting.";
      return false;
    }
  }
  if (!validate(parsed, error)) return false;
  result = std::move(parsed);
  return true;
}

fs::path baselinePath() {
  std::array<wchar_t, 32768> local{};
  DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", local.data(),
                                         static_cast<DWORD>(local.size()));
  if (count == 0 || count >= local.size())
    throw std::runtime_error("Could not resolve LOCALAPPDATA.");
  return fs::path(local.data()) / L"Win11To10" / L"baseline-v1.txt";
}

bool readBaseline(Values& result, std::string& error) {
  std::ifstream file(baselinePath(), std::ios::binary);
  if (!file) {
    error = "Baseline backup does not exist or cannot be read.";
    return false;
  }
  std::string data((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  if (file.bad() || data.size() > 4096) {
    error = "Backup is unreadable or too large.";
    return false;
  }
  return parse(data, result, error);
}

bool saveBaselineOnce(const Values& original, std::string& error) {
  if (!validate(original, error)) return false;
  const fs::path path = baselinePath();
  if (fs::exists(path)) {
    Values saved;
    return readBaseline(saved, error);
  }
  fs::create_directories(path.parent_path());
  const std::string bytes = serialize(original);
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_EXISTS) {
      Values saved;
      return readBaseline(saved, error);
    }
    error = "Failed to create baseline file (Win32 " +
            std::to_string(GetLastError()) + ")";
    return false;
  }
  DWORD written = 0;
  BOOL ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()),
                      &written, nullptr);
  if (ok && written == bytes.size()) ok = FlushFileBuffers(file);
  CloseHandle(file);
  if (!ok || written != bytes.size()) {
    DeleteFileW(path.c_str());
    error = "Failed writing the complete baseline.";
    return false;
  }
  return true;
}

std::string utf8(const std::wstring& value) {
  if (value.empty()) return {};
  int n = WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
                             static_cast<int>(value.size()), nullptr, 0,
                             nullptr, nullptr);
  if (n <= 0) return "(encoding error)";
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
                      static_cast<int>(value.size()), out.data(), n,
                      nullptr, nullptr);
  return out;
}

void printRunKey(HKEY root, const char* label) {
  HKEY key = nullptr;
  constexpr wchar_t subkey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  LONG opened = RegOpenKeyExW(root, subkey, 0, KEY_READ, &key);
  if (opened != ERROR_SUCCESS) {
    std::cout << "  " << label << ": unavailable (Win32 " << opened << ")\n";
    return;
  }
  std::cout << "  " << label << " (Run key names; not necessarily enabled):\n";
  int count = 0;
  for (DWORD index = 0;; ++index) {
    wchar_t name[1024]{};
    DWORD length = static_cast<DWORD>(std::size(name));
    LONG status = RegEnumValueW(key, index, name, &length, nullptr,
                                nullptr, nullptr, nullptr);
    if (status == ERROR_NO_MORE_ITEMS) break;
    if (status == ERROR_SUCCESS) {
      std::cout << "    - " << utf8(std::wstring(name, length)) << '\n';
      ++count;
    } else if (status != ERROR_MORE_DATA) {
      std::cout << "    (Win32 enumeration error " << status << ")\n";
    }
  }
  if (count == 0) std::cout << "    (none)\n";
  RegCloseKey(key);
}

void report() {
  SetConsoleOutputCP(CP_UTF8);
  MEMORYSTATUSEX state{};
  state.dwLength = sizeof(state);
  if (GlobalMemoryStatusEx(&state)) {
    constexpr unsigned long long mib = 1024ULL * 1024ULL;
    std::cout << "RAM total: " << state.ullTotalPhys / mib << " MiB\n"
              << "RAM available: " << state.ullAvailPhys / mib << " MiB\n"
              << "Physical memory load: " << state.dwMemoryLoad << "%\n";
  } else {
    std::cout << "RAM metrics unavailable.\n";
  }
  std::cout << "OS uptime: " << GetTickCount64() / 1000ULL
            << " seconds (not an actual boot benchmark)\n";
  wchar_t build[64]{};
  DWORD size = sizeof(build);
  if (RegGetValueW(HKEY_LOCAL_MACHINE,
      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
      L"CurrentBuildNumber", RRF_RT_REG_SZ, nullptr,
      build, &size) == ERROR_SUCCESS)
    std::cout << "Windows build: " << utf8(build) << '\n';
  std::cout << "Startup candidates, read-only and incomplete inventory:\n";
  printRunKey(HKEY_CURRENT_USER, "HKCU");
  printRunKey(HKEY_LOCAL_MACHINE, "HKLM");
}

void printEffects(const Values& values) {
  for (const auto& [key, value] : values)
    std::cout << "  " << key << ": " << (value ? "ON" : "OFF") << '\n';
}

bool snapshot(Values& out) {
  std::string error;
  if (readEffects(out, error)) return true;
  std::cerr << error << '\n';
  return false;
}

int apply() {
  Values before;
  if (!snapshot(before)) return 1;
  std::string error;
  if (!saveBaselineOnce(before, error)) {
    std::cerr << "No settings changed: " << error << '\n';
    return 1;
  }
  const Values proposed = performanceProfile(before);
  if (!writeEffects(proposed, error)) {
    std::cerr << "Application failed: " << error << '\n';
    std::string rollbackError;
    if (!writeEffects(before, rollbackError))
      std::cerr << "WARNING: rollback failed: " << rollbackError
                << ". Use restore --confirm.\n";
    else
      std::cerr << "Previous settings requested for rollback.\n";
    return 1;
  }
  std::cout << "Performance profile requested. Saved original preferences.\n"
            << "Current effects (Windows may ignore unsupported flags):\n";
  Values actual;
  if (snapshot(actual)) printEffects(actual);
  return 0;
}

int restore() {
  Values baseline;
  std::string error;
  if (!readBaseline(baseline, error)) {
    std::cerr << "Restore aborted: " << error << '\n';
    return 1;
  }
  Values previous;
  if (!snapshot(previous)) return 1;
  if (!writeEffects(baseline, error)) {
    std::cerr << "Restore failed: " << error << '\n';
    std::string rollbackError;
    if (!writeEffects(previous, rollbackError))
      std::cerr << "WARNING: partial restore may remain: " << rollbackError << '\n';
    return 1;
  }
  std::cout << "Original visual preferences requested for restoration.\n"
            << "The original baseline file was retained.\n";
  return 0;
}

bool selfTest() {
  Values example;
  for (const auto& e : effects)
    example[e.key] = 1;
  std::string error;
  if (!validate(example, error)) return false;
  const auto encoded = serialize(example);
  Values decoded;
  if (!parse(encoded, decoded, error) || example != decoded) return false;
  if (parse(encoded + "menu_fade=1\n", decoded, error)) return false;
  if (parse(encoded + "unknown=0\n", decoded, error)) return false;
  if (parse("INVALID\n", decoded, error)) return false;
  if (parse(encoded.substr(0, encoded.find("menu_fade=")) + "menu_fade=2\n",
            decoded, error)) return false;
  const auto best = performanceProfile(example);
  if (!validate(best, error)) return false;
  for (const auto& [name, value] : best)
    if (value != (name == "font_smoothing" ? 1 : 0)) return false;
  return true;
}

void usage() {
  std::cout <<
      "Win11To10 v0.1.0 — native performance foundation\n"
      "This version does NOT yet implement a Windows 10 shell.\n"
      "Usage:\n"
      "  Win11To10.exe analyze                       Read-only system report\n"
      "  Win11To10.exe status                        Read-only visual effects\n"
      "  Win11To10.exe apply-performance --confirm   Disable optional effects\n"
      "  Win11To10.exe restore --confirm             Restore original settings\n"
      "  Win11To10.exe self-test                     Non-destructive backup tests\n"
      "  Win11To10.exe help                          Show this text\n"
      "No security services, startup entries, or Windows updates are disabled.\n";
}
}  // namespace

int main(int argc, char* argv[]) {
  try {
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "help")) {
      usage();
      return 0;
    }
    const std::string cmd = argv[1];
    if (argc == 2 && cmd == "self-test") {
      if (!selfTest()) {
        std::cerr << "Self-test FAILED.\n";
        return 1;
      }
      std::cout << "Self-test passed.\n";
      return 0;
    }
    if (argc == 2 && cmd == "analyze") {
      report();
      return 0;
    }
    if (argc == 2 && cmd == "status") {
      Values values;
      if (!snapshot(values)) return 1;
      printEffects(values);
      return 0;
    }
    if ((cmd == "apply-performance" || cmd == "restore") &&
        argc == 3 && std::string(argv[2]) == "--confirm")
      return cmd == "apply-performance" ? apply() : restore();

    std::cerr << "Unknown command or missing --confirm. No changes made.\n";
    usage();
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
