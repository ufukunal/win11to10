#include "TurkishSearch.h"

#include <windows.h>

#include <climits>
#include <string_view>

namespace noya {
bool ContainsTurkish(std::wstring_view text, std::wstring_view query) {
  if (query.empty()) return true;
  if (text.empty() || text.size() > INT_MAX || query.size() > INT_MAX)
    return false;
  const DWORD flags = FIND_FROMSTART | NORM_IGNORECASE | NORM_LINGUISTIC_CASING;
  SetLastError(ERROR_SUCCESS);
  const int found = FindNLSStringEx(L"tr-TR", flags,
      text.data(), static_cast<int>(text.size()),
      query.data(), static_cast<int>(query.size()),
      nullptr, nullptr, nullptr, 0);
  if (found >= 0) return true;
  if (GetLastError() == ERROR_SUCCESS) return false;
  // If Turkish NLS data cannot be loaded, fall back to ordinal Unicode
  // case-insensitive search rather than failing all search results.
  return FindStringOrdinal(FIND_FROMSTART,
      text.data(), static_cast<int>(text.size()),
      query.data(), static_cast<int>(query.size()), TRUE) >= 0;
}

bool TurkishSearchModelSelfTest() {
  return ContainsTurkish(L"Dosya Gezgini", L"GEZGİNİ") &&
         ContainsTurkish(L"İSTANBUL", L"istanbul") &&
         ContainsTurkish(L"ISPARTA", L"ısparta") &&
         !ContainsTurkish(L"ISPARTA", L"isparta") &&
         ContainsTurkish(L"Ayarlar", L"") &&
         !ContainsTurkish(L"Not Defteri", L"xyz123");
}
} // namespace noya
