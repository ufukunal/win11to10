#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace noya {
struct BootHistory {
  std::vector<std::uint64_t> milliseconds; // newest event first
  std::wstring warning;
};
struct BootStats {
  std::uint64_t newestMs{};
  std::uint64_t minimumMs{};
  std::uint64_t maximumMs{};
  std::uint64_t medianMs{};
  std::uint64_t meanMs{};
};
BootHistory ReadBootHistory(unsigned maxEntries = 8);
BootStats CalculateBootStats(const std::vector<std::uint64_t>& values);
bool BootHistoryModelSelfTest();
}  // namespace noya
