#include "BootHistory.h"
#include "BootMetrics.h"

#include <windows.h>
#include <winevt.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace noya {
namespace {
struct SafeEvent {
  EVT_HANDLE value{};
  ~SafeEvent() { if (value) EvtClose(value); }
  SafeEvent() = default;
  SafeEvent(const SafeEvent&) = delete;
  SafeEvent& operator=(const SafeEvent&) = delete;
};

bool RenderDuration(EVT_HANDLE event, std::uint64_t& out) {
  DWORD needed = 0;
  DWORD propertyCount = 0;
  EvtRender(nullptr, event, EvtRenderEventXml, 0, nullptr,
            &needed, &propertyCount);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
      needed < sizeof(wchar_t) || needed > 1024 * 1024) {
    return false;
  }
  std::vector<wchar_t> xml(needed / sizeof(wchar_t) + 1, L'\0');
  if (!EvtRender(nullptr, event, EvtRenderEventXml,
                 needed, xml.data(), &needed, &propertyCount)) {
    return false;
  }
  return ParseBootTimeXml(xml.data(), out);
}
}  // namespace

BootHistory ReadBootHistory(unsigned maxEntries) {
  BootHistory result;
  if (maxEntries == 0 || maxEntries > 20) maxEntries = 8;
  SafeEvent query;
  query.value = EvtQuery(nullptr,
      L"Microsoft-Windows-Diagnostics-Performance/Operational",
      L"Event/System[EventID=100]",
      EvtQueryChannelPath | EvtQueryReverseDirection);
  if (!query.value) {
    result.warning = L"Diagnostics-Performance günlüğüne erişilemiyor (Win32 " +
                     std::to_wstring(GetLastError()) + L").";
    return result;
  }

  for (unsigned attempts = 0; attempts < 60 &&
                             result.milliseconds.size() < maxEntries; ++attempts) {
    SafeEvent event;
    DWORD returned = 0;
    if (!EvtNext(query.value, 1, &event.value, 250, 0, &returned)) {
      if (GetLastError() != ERROR_NO_MORE_ITEMS) {
        result.warning = L"Açılış olayları tamamen okunamadı.";
      }
      break;
    }
    if (!event.value || returned != 1) break;
    std::uint64_t duration{};
    if (RenderDuration(event.value, duration) &&
        duration > 0 && duration <= 86400000ULL) {
      result.milliseconds.push_back(duration);
    }
  }
  if (result.milliseconds.empty() && result.warning.empty()) {
    result.warning = L"Karşılaştırılabilir açılış kaydı bulunamadı.";
  }
  return result;
}

BootStats CalculateBootStats(const std::vector<std::uint64_t>& values) {
  BootStats result{};
  if (values.empty()) return result;
  std::vector<std::uint64_t> sorted(values);
  std::sort(sorted.begin(), sorted.end());
  result.newestMs = values.front();
  result.minimumMs = sorted.front();
  result.maximumMs = sorted.back();
  const auto middle = sorted.size() / 2;
  result.medianMs = sorted.size() % 2 ? sorted[middle]
                   : sorted[middle - 1] / 2 + sorted[middle] / 2 +
                     ((sorted[middle - 1] % 2 + sorted[middle] % 2) / 2);
  // No overflow: event durations <= 86,400,000 ms, list size <= 20.
  const std::uint64_t total = std::accumulate(
      values.begin(), values.end(), std::uint64_t{0});
  result.meanMs = total / values.size();
  return result;
}

bool BootHistoryModelSelfTest() {
  if (CalculateBootStats({}).meanMs != 0) return false;
  const auto even = CalculateBootStats({24000, 10000, 20000, 18000});
  if (even.newestMs != 24000 || even.minimumMs != 10000 ||
      even.maximumMs != 24000 || even.medianMs != 19000 ||
      even.meanMs != 18000) return false;
  const auto odd = CalculateBootStats({5000, 10000, 15000});
  return odd.medianMs == 10000 && odd.meanMs == 10000;
}
}  // namespace noya
