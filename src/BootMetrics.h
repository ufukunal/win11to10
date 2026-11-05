#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace noya {
struct BootReport {
  bool available = false;
  std::uint64_t milliseconds = 0;
  std::wstring diagnostic;
};

// On demand: reads the last Diagnostics-Performance event 100, if available.
// Never guesses a boot time from OS uptime; no event-log settings are changed.
BootReport ReadLastBootEvent();

// Exposed for pure unit testing.
bool ParseBootTimeXml(std::wstring_view xml, std::uint64_t& duration);
bool BootMetricsModelSelfTest();
}  // namespace noya
