#include "BootComparison.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace noya {
namespace {
namespace fs = std::filesystem;
constexpr std::string_view kHeader = "NOYAPLANET_BOOT_BASELINE_V1";
constexpr auto kFileName = L"boot-baseline-v1.txt";

fs::path BaselinePath() {
  std::array<wchar_t, 32768> buffer{};
  const auto count = GetEnvironmentVariableW(
      L"LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size()));
  if (!count || count >= buffer.size()) return {};
  return fs::path(buffer.data()) / L"NoyaPLANET" / L"11to10" / kFileName;
}

bool ParseInteger(std::string_view token, std::uint64_t& out) {
  if (token.empty() || token.size() > 20) return false;
  std::uint64_t value{};
  const auto [end, err] = std::from_chars(token.data(),
                                          token.data()+token.size(), value);
  if (err != std::errc{} || end != token.data()+token.size()) return false;
  out = value;
  return true;
}

bool ParseBaseline(std::string_view text, BootBaseline& result) {
  const auto prefix = std::string(kHeader) + "\n";
  if (!text.starts_with(prefix) || text.size() > 1024) return false;
  const auto next = text.substr(prefix.size());
  const auto break1 = next.find('\n');
  if (break1 == std::string_view::npos) return false;
  const auto break2 = next.find('\n', break1+1);
  if (break2 == std::string_view::npos) return false;
  const auto break3 = next.find('\n', break2+1);
  if (break3 == std::string_view::npos || break3+1 != next.size()) return false;
  const auto line1 = next.substr(0, break1);
  const auto line2 = next.substr(break1+1, break2-break1-1);
  const auto line3 = next.substr(break2+1, break3-break2-1);
  if (!line1.starts_with("record=") || !line2.starts_with("median=") ||
      !line3.starts_with("samples=")) return false;
  BootBaseline candidate{};
  std::uint64_t count{};
  if (!ParseInteger(line1.substr(7), candidate.latestRecordId) ||
      !ParseInteger(line2.substr(7), candidate.medianMs) ||
      !ParseInteger(line3.substr(8), count) ||
      candidate.latestRecordId == 0 || candidate.medianMs == 0 ||
      candidate.medianMs > 86400000 || count < 3 || count > 20) return false;
  candidate.samples = static_cast<unsigned>(count);
  result = candidate;
  return true;
}

bool CreateSnapshot(const BootHistory& history, BootBaseline& baseline,
                    std::wstring& error) {
  if (history.milliseconds.size() < 3 ||
      history.recordIds.size() != history.milliseconds.size()) {
    error = L"En az üç geçerli Windows açılış olayı gerekli.";
    return false;
  }
  const auto largest = *std::max_element(history.recordIds.begin(),
                                          history.recordIds.end());
  if (largest == 0) {
    error = L"Windows açılış olaylarının RecordID bilgisi bulunamadı.";
    return false;
  }
  const auto stats = CalculateBootStats(history.milliseconds);
  if (stats.medianMs == 0) {
    error = L"Açılış medyanı hesaplanamadı.";
    return false;
  }
  baseline.latestRecordId = largest;
  baseline.medianMs = stats.medianMs;
  baseline.samples = static_cast<unsigned>(history.milliseconds.size());
  return true;
}
}  // namespace

bool SaveBootBaseline(const BootHistory& history, std::wstring& error) {
  BootBaseline baseline;
  if (!CreateSnapshot(history, baseline, error)) return false;
  const auto path = BaselinePath();
  if (path.empty()) {
    error = L"LOCALAPPDATA dizini bulunamadı.";
    return false;
  }
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    error = L"Başlangıç ölçüm klasörü oluşturulamadı.";
    return false;
  }
  const std::string data = std::string(kHeader) +
      "\nrecord=" + std::to_string(baseline.latestRecordId) +
      "\nmedian=" + std::to_string(baseline.medianMs) +
      "\nsamples=" + std::to_string(baseline.samples) + "\n";
  const auto temp = path.wstring() + L"." +
      std::to_wstring(GetCurrentProcessId()) + L".tmp";
  HANDLE handle = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    error = L"Geçici ölçüm dosyası oluşturulamadı.";
    return false;
  }
  DWORD written = 0;
  const bool ok = WriteFile(handle, data.data(),
                             static_cast<DWORD>(data.size()), &written,
                             nullptr) && written == data.size() &&
                  FlushFileBuffers(handle);
  CloseHandle(handle);
  if (!ok) {
    DeleteFileW(temp.c_str());
    error = L"Başlangıç ölçüm dosyası yazılamadı.";
    return false;
  }
  if (!MoveFileExW(temp.c_str(), path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    DeleteFileW(temp.c_str());
    error = L"Başlangıç ölçümü güvenle kaydedilemedi.";
    return false;
  }
  return true;
}

bool LoadBootBaseline(BootBaseline& baseline, std::wstring& error) {
  const auto path = BaselinePath();
  if (path.empty()) {
    error = L"LOCALAPPDATA dizini bulunamadı.";
    return false;
  }
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    error = L"Önce Sistem araçları menüsünden başlangıç ölçümü kaydedin.";
    return false;
  }
  const std::string bytes((std::istreambuf_iterator<char>(stream)),
                           std::istreambuf_iterator<char>());
  if (stream.bad() || !ParseBaseline(bytes, baseline)) {
    error = L"Başlangıç ölçüm dosyası okunamadı veya geçersiz.";
    return false;
  }
  return true;
}

BootComparison CompareBoots(const BootBaseline& baseline,
                            const BootHistory& current) {
  BootComparison result;
  result.beforeMedianMs = baseline.medianMs;
  std::vector<std::uint64_t> fresh;
  const auto count = std::min(current.recordIds.size(),
                               current.milliseconds.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (current.recordIds[i] > baseline.latestRecordId &&
        current.milliseconds[i] > 0 &&
        current.milliseconds[i] <= 86400000ULL)
      fresh.push_back(current.milliseconds[i]);
  }
  result.newSamples = static_cast<unsigned>(fresh.size());
  if (fresh.size() < 3 || baseline.medianMs == 0) return result;
  result.afterMedianMs = CalculateBootStats(fresh).medianMs;
  result.deltaMs = static_cast<std::int64_t>(result.afterMedianMs) -
                   static_cast<std::int64_t>(baseline.medianMs);
  result.enoughData = true;
  return result;
}

bool BootComparisonModelSelfTest() {
  BootHistory source;
  source.milliseconds = {10000, 20000, 14000, 12000};
  source.recordIds = {43, 42, 41, 40};
  BootBaseline baseline;
  std::wstring error;
  if (!CreateSnapshot(source, baseline, error) || baseline.latestRecordId != 43 ||
      baseline.medianMs != 13000 || baseline.samples != 4) return false;
  const std::string serialized = std::string(kHeader) +
       "\nrecord=43\nmedian=13000\nsamples=4\n";
  BootBaseline parsed;
  if (!ParseBaseline(serialized, parsed) ||
      parsed.latestRecordId != 43 || parsed.medianMs != 13000 ||
      ParseBaseline("invalid", parsed)) return false;
  BootHistory newer;
  newer.milliseconds = {9000, 11000, 10000, 10000, 14000};
  newer.recordIds = {47, 46, 45, 44, 43};
  const auto result = CompareBoots(parsed, newer);
  if (!result.enoughData || result.newSamples != 4 ||
      result.afterMedianMs != 10000 || result.deltaMs != -3000) return false;
  newer.recordIds = {43, 42, 41};
  newer.milliseconds = {9000, 11000, 10000};
  return !CompareBoots(parsed, newer).enoughData;
}
} // namespace noya
