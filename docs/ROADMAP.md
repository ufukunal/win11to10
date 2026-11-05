# Win11To10 development roadmap

The long-term objective is a responsive Windows 10-like desktop on Windows 11,
with optional visual effects disabled and font smoothing preserved.

## v0.1 — Performance foundation (current)
- Native C++20 command-line utility, no resident background process.
- On-demand memory, OS uptime and Windows build report.
- Startup candidates from HKCU/HKLM Run registry keys, read-only.
- Current visual effects status and explicitly confirmed performance profile.
- A write-once per-user baseline, validated restore and rollback attempts.
- Windows x64 build, read-only smoke test and downloadable EXE on GitHub Actions.

## v0.2 — Measured startup performance
- Boot/sign-in tracing rather than estimating boot time from OS uptime.
- Startup folders, task scheduler and enabled-state analysis.
- User-approved reversible changes, individual per-item restoration.

## v0.3 — Windows 10 Start menu
- Native menu, keyboard navigation, search and pinned apps.
- DPI, accessibility, explorer restart and multimonitor compatibility tests.

## v0.4 — Taskbar and system tray
- Native taskbar controls and real window enumeration.
- Preserve third-party tray interactions; avoid unsupported Explorer injection.
- Strict Windows build compatibility matrix.

## v0.5 — Explorer, settings and packaging
- Supported classic Explorer presentation options.
- Separate optional settings UI, closed when not in use.
- Recovery path, tests and installer.

## Safety requirements
- Never disable Windows Defender or Windows Update.
- Never silently modify startup items or services.
- No permanent modification of Windows system binaries.
- Keep a recovery path and disclose when OS ignores requests.
- Do not promise any speedup without before/after measurements.
