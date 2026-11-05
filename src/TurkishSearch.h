#pragma once
#include <string_view>

namespace noya {
// Linguistic, case-insensitive Turkish UI substring search.
// Not for security identifiers, file paths or persisted registry keys.
bool ContainsTurkish(std::wstring_view text, std::wstring_view query);
bool TurkishSearchModelSelfTest();
} // namespace noya
