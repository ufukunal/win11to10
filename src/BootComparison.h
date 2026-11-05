#pragma once
#include "BootHistory.h"
#include <cstdint>
#include <string>
namespace noya {
struct BootBaseline {
  std::uint64_t latestRecordId{};
  std::uint64_t medianMs{};
  unsigned samples{};
};
struct BootComparison {
  unsigned newSamples{};
  std::uint64_t beforeMedianMs{};
  std::uint64_t afterMedianMs{};
  std::int64_t deltaMs{};
  bool enoughData = false; // at least 3 post-baseline boot events
};
// Only after explicit user confirmation. Stores simple local per-user
// baseline metadata and never modifies the Event Log.
bool SaveBootBaseline(const BootHistory& history, std::wstring& error);
bool LoadBootBaseline(BootBaseline& baseline, std::wstring& error);
BootComparison CompareBoots(const BootBaseline& baseline,
                            const BootHistory& current);
bool BootComparisonModelSelfTest();
}  // namespace noya
