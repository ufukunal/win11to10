#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace noya {
struct StartupEntry {
  std::wstring name;
  std::wstring command;
  bool disabled = false;
  bool protectedEntry = false;
};

// Only HKCU Run and backups created by this application are enumerated.
// Windows Settings remains the primary management tool for other sources.
std::vector<StartupEntry> ListCurrentUserStartup(std::wstring& error);

// Makes one selected HKCU Run entry reversibly inactive. Before any removal,
// its original value type and data are copied into an app-owned HKCU key.
// Returns false on conflicts or failure; never touches HKLM or services.
bool ChangeCurrentUserStartup(std::wstring_view name, bool disable,
                              std::wstring& error);

// Blocks common Windows protection-related entries. Never assumes an
// unrecognized entry is expendable: the user must opt in individually.
bool StartupNameProtected(std::wstring_view name);
bool StartupModelSelfTest();
}  // namespace noya
