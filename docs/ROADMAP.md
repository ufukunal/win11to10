# Win11To10 development roadmap

The long-term objective is a responsive Windows 10-like desktop on Windows 11,
with optional visual effects disabled and font smoothing preserved.

## v0.1 — Performance foundation (completed in CI)
- Native C++20 command-line utility, no resident background process.
- On-demand memory, OS uptime and Windows build report.
- Startup candidates from HKCU/HKLM Run registry keys, read-only.
- Current visual effects status and explicitly confirmed performance profile.
- A write-once per-user baseline, validated restore and rollback attempts.
- Windows x64 build, read-only smoke tests. Unsigned builds are not released.

## v0.2 — Native interface previews (implemented in development branch)
- Turkish Win32 control panel; on-demand system information.
- Confirmed performance profile and original-settings recovery.
- Standalone Windows 10-inspired Start menu with app search and fixed launchers.
- Standalone taskbar with on-demand enumeration and activation of real windows.
- Store MSIX packs the dashboard and CLI helper; package identity validated in CI.
- GUI, Start-menu model, packaging and signing-gate automated checks.
- These windows are prototypes, not Explorer or shell replacements.

## v0.3 — Measured startup performance
- Boot/sign-in tracing rather than estimating boot time from OS uptime.
- Startup folders, task scheduler and enabled-state analysis.
- User-approved reversible changes, individual per-item restoration.

## v0.4 — Windows 10 integrated Start menu
- Native menu, keyboard navigation, search and pinned apps.
- DPI, accessibility, explorer restart and multimonitor compatibility tests.

## v0.5 — Integrated taskbar and system tray
- Native taskbar controls and real window enumeration.
- Preserve third-party tray interactions; avoid unsupported Explorer injection.
- Strict Windows build compatibility matrix.

## v0.6 — Explorer, Store readiness and signed distribution
- Supported classic Explorer presentation options.
- Separate optional settings UI, closed when not in use.
- Recovery path, tests and installer.

## Safety requirements
- Never disable Windows Defender or Windows Update.
- Never silently modify startup items or services.
- No permanent modification of Windows system binaries.
- Keep a recovery path and disclose when OS ignores requests.
- Do not promise any speedup without before/after measurements.
