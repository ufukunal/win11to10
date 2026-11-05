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

## v0.3 — Startup inventory and first boot metrics (implemented in development)
- Reversible, user-confirmed current-user HKCU Run entry control.
- Never touch HKLM Run, RunOnce, protected names, services or scheduled tasks.
- Installed per-user/common Start Menu shortcuts indexed on preview open.
- On-demand Windows Diagnostics-Performance event 100 read (milliseconds).
- Event data may be stale or inaccessible; no unverified speed claims.
- Ctrl+Alt+S hotkey when the GUI is running, without hijacking Windows key.
- Taskbar preview native clock; Windows Folder Options access.
- Open items: real Windows 11 manual tests, advanced startup source coverage,
  robust benchmark comparisons before/after sign-in and reboot.

## v0.4 — Full startup diagnostics and accurate benchmarks
- Boot and sign-in comparisons across several clean restarts.
- Startup folders, Task Scheduler, per-machine Run entries read-only analysis.
- Detailed impact measurement and safe per-item recovery tests.

## v0.5 — Integrated Windows 10 Start menu
- Boot/sign-in tracing rather than estimating boot time from OS uptime.
- Startup folders, task scheduler and enabled-state analysis.
- User-approved reversible changes, individual per-item restoration.

## v0.5 — Integrated Windows 10 Start menu
- Native menu, keyboard navigation, installed app search and pinned apps.
- DPI, accessibility, explorer restart and multimonitor compatibility tests.

## v0.6 — Integrated taskbar and system tray
- Native taskbar controls and real window enumeration.
- Preserve third-party tray interactions; avoid unsupported Explorer injection.
- Strict Windows build compatibility matrix.

## v0.7 — Explorer, Store readiness and signed distribution
- Supported classic Explorer presentation options.
- Separate optional settings UI, closed when not in use.
- Recovery path, tests and installer.

## Safety requirements
- Never disable Windows Defender or Windows Update.
- Never silently modify startup items or services.
- No permanent modification of Windows system binaries.
- Keep a recovery path and disclose when OS ignores requests.
- Do not promise any speedup without before/after measurements.
