#include "BootMetrics.h"

#include <windows.h>
#include <winevt.h>

#include <cstdint>
#include <cwchar>
#include <string>
#include <string_view>
#include <vector>

namespace noya {
namespace {
struct EventHandle {
  EVT_HANDLE handle{};
  ~EventHandle() { if (handle) EvtClose(handle); }
  EventHandle() = default;
  EventHandle(const EventHandle&) = delete;
  EventHandle& operator=(const EventHandle&) = delete;
};

bool ParseField(std::wstring_view xml, std::wstring_view name,
                std::uint64_t& duration) {
  for (wchar_t quote : {L'\"', L'\''}) {
    const std::wstring attribute = L"Name=" + std::wstring(1, quote) +
                                   std::wstring(name) + std::wstring(1, quote);
    std::size_t pos = xml.find(attribute);
    if (pos == std::wstring_view::npos) continue;
    const auto tagStart = xml.rfind(L"<Data", pos);
    if (tagStart == std::wstring_view::npos) continue;
    const auto tagEnd = xml.find(L'>', pos);
    if (tagEnd == std::wstring_view::npos) continue;
    if (xml.find(L"</Data>", tagEnd) == std::wstring_view::npos) continue;
    const auto close = xml.find(L"</Data>", tagEnd);
    if (close == std::wstring_view::npos || close <= tagEnd + 1) continue;
    const auto token = xml.substr(tagEnd + 1, close - tagEnd - 1);
    if (token.empty() || token.size() > 20) continue;
    std::uint64_t number = 0;
    for (const wchar_t ch : token) {
      if (ch < L'0' || ch > L'9') {
        number = 0;
        break;
      }
      const auto digit = static_cast<std::uint64_t>(ch - L'0');
      if (number > (86400000ULL - digit) / 10ULL) {
        number = 0;
        break;
      }
      number = number * 10ULL + digit;
    }
    if (number != 0) {
      duration = number;
      return true;
    }
  }
  return false;
}
}  // namespace

bool ParseBootTimeXml(std::wstring_view xml, std::uint64_t& duration) {
  return ParseField(xml, L"BootTime", duration);
}

BootReport ReadLastBootEvent() {
  BootReport report;
  EventHandle query;
  query.handle = EvtQuery(
      nullptr, L"Microsoft-Windows-Diagnostics-Performance/Operational",
      L"Event/System[EventID=100]",
      EvtQueryChannelPath | EvtQueryReverseDirection);
  if (!query.handle) {
    report.diagnostic = L"Olay günlüğü kullanılamıyor (Win32 " +
                        std::to_wstring(GetLastError()) + L").";
    return report;
  }

  EventHandle event;
  DWORD returned = 0;
  if (!EvtNext(query.handle, 1, &event.handle, 1000, 0, &returned) ||
      returned != 1) {
    report.diagnostic = L"Açılış kaydı bulunamadı veya erişilemiyor.";
    return report;
  }

  DWORD required = 0;
  DWORD properties = 0;
  EvtRender(nullptr, event.handle, EvtRenderEventXml, 0, nullptr,
            &required, &properties);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
      required < sizeof(wchar_t) || required > 1024 * 1024) {
    report.diagnostic = L"Olay günlüğü verisi okunamadı.";
    return report;
  }

  std::vector<BYTE> buffer(required);
  if (!EvtRender(nullptr, event.handle, EvtRenderEventXml,
                 static_cast<DWORD>(buffer.size()),
                 buffer.data(), &required, &properties)) {
    report.diagnostic = L"Açılış olayı çözümlenemedi.";
    return report;
  }

  const auto* xml = reinterpret_cast<const wchar_t*>(buffer.data());
  std::uint64_t duration{};
  if (!ParseBootTimeXml(std::wstring_view(xml), duration)) {
    report.diagnostic = L"Olayda BootTime alanı bulunamadı.";
    return report;
  }
  report.available = true;
  report.milliseconds = duration;
  report.diagnostic = L"Son kayıt: Windows Diagnostics-Performance Olay 100";
  return report;
}

bool BootMetricsModelSelfTest() {
  std::uint64_t ms = 0;
  return ParseBootTimeXml(L"<Event><EventData><Data Name=\"BootTime\">12345</Data></EventData></Event>", ms)
         && ms == 12345
         && ParseBootTimeXml(L"<Data Name='BootTime'>52000</Data>", ms) && ms == 52000
         && !ParseBootTimeXml(L"<Data Name='BootTime'>-1</Data>", ms)
         && !ParseBootTimeXml(L"<Data Name='BootTime'>999999999999</Data>", ms)
         && !ParseBootTimeXml(L"<Data Name='Other'>3000</Data>", ms);
}
}  // namespace noya
