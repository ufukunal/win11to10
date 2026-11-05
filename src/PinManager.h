#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace noya {
// A small, local list of at most four pinned app launch targets.
// No background worker, registry hook, or Windows Start menu modification.
bool LoadPinnedTargets(std::vector<std::wstring>& targets, bool& hadConfiguration,
                       std::wstring& error);
bool SavePinnedTargets(const std::vector<std::wstring>& targets, std::wstring& error);
bool IsValidPinnedTarget(std::wstring_view target);
bool PinModelSelfTest();
}  // namespace noya
