#include "ServiceInventory.h"

#include <windows.h>
#include <winsvc.h>

#include <array>
#include <string>
#include <vector>

namespace noya {
namespace {
constexpr unsigned kMaxListed = 12;
constexpr unsigned kMaxServices = 10000;
constexpr DWORD kMaxBuffer = 2 * 1024 * 1024;

void CountService(const ENUM_SERVICE_STATUS_PROCESSW& item,
                  ServiceInventory& out) {
  ++out.total;
  if (item.ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) {
    ++out.running;
    if (out.sampleRunningNames.size() < kMaxListed && item.lpDisplayName)
      out.sampleRunningNames.emplace_back(item.lpDisplayName);
  } else {
    ++out.stopped;
  }
}
}  // namespace

ServiceInventory InspectWindowsServices() {
  ServiceInventory result;
  SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr,
                                     SC_MANAGER_ENUMERATE_SERVICE);
  if (!manager) {
    result.warning = L"Windows servisleri okunamıyor (Win32 " +
                     std::to_wstring(GetLastError()) + L").";
    return result;
  }

  DWORD resume = 0;
  std::vector<BYTE> buffer(256 * 1024);
  for (unsigned page = 0; page < 8 && result.total < kMaxServices; ++page) {
    DWORD required = 0;
    DWORD count = 0;
    const BOOL ok = EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32, SERVICE_STATE_ALL, buffer.data(),
        static_cast<DWORD>(buffer.size()), &required, &count,
        &resume, nullptr);
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    if (ok || error == ERROR_MORE_DATA) {
      const auto* services =
          reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
      for (DWORD i = 0; i < count && result.total < kMaxServices; ++i)
        CountService(services[i], result);
      if (ok) break;
      if (required > kMaxBuffer) {
        result.warning = L"Servis kayıtları üst boyut sınırını aştı.";
        break;
      }
      if (required > buffer.size()) buffer.resize(required);
    } else {
      result.warning = L"Servisler kısmen okunamadı (Win32 " +
                       std::to_wstring(error) + L").";
      break;
    }
  }
  CloseServiceHandle(manager);
  return result;
}

bool ServiceSummarySelfTest() {
  ENUM_SERVICE_STATUS_PROCESSW running{};
  ENUM_SERVICE_STATUS_PROCESSW stopped{};
  running.ServiceStatusProcess.dwCurrentState = SERVICE_RUNNING;
  stopped.ServiceStatusProcess.dwCurrentState = SERVICE_STOPPED;
  ServiceInventory result;
  CountService(running, result);
  CountService(stopped, result);
  return result.total == 2 && result.running == 1 && result.stopped == 1;
}
}  // namespace noya
