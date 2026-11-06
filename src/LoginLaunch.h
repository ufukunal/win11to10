#pragma once
#include <string>

namespace noya {
// Only manages NoyaPLANET's own HKCU Run entry. Never changes other apps.
enum class LoginLaunchState { Disabled, Enabled, Conflict, Error };
LoginLaunchState InspectLoginLaunch(std::wstring& error);
bool SetLoginLaunch(bool enable, std::wstring& error);
bool LoginLaunchModelSelfTest();
}  // namespace noya
