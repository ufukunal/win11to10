#pragma once
#include <cstddef>
#include <string_view>
#include <vector>

namespace noya {
struct LaunchTarget {
  std::wstring_view title;
  std::wstring_view command;
};
// Curated built-in Windows apps, never arbitrary commands from search input.
const std::vector<LaunchTarget>& BuiltInApps();
std::vector<std::size_t> SearchApps(std::wstring_view query);
bool ModelSelfTest();
}  // namespace noya
