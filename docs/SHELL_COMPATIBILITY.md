# Windows 10 shell compatibility boundaries

Target: achieve as much Windows 10 22H2 desktop behavior as possible
on supported Windows 11 builds, with low overhead and reliable rollback.

## Implemented, but not a native shell replacement

- The on-demand Start preview searches installed Start Menu shortcuts and
  launches selected applications. Optional Ctrl+Alt+S works only while the
  native dashboard is running. The Windows key and built-in Start menu remain.
- The taskbar preview enumerates visible windows and opens selected windows,
  displaying a clock. It does not replace or auto-hide Windows' own taskbar.
- The NoyaPLANET tray icon is the application's own notification-area icon.
  It is added with Shell_NotifyIcon, and is removed on exit. Existing icons
  remain owned by Explorer. No programmatic migration of other apps' icons is
  attempted.
- Windows Folder Options opens as a supported system control; Windows 10
  Explorer ribbon and legacy global context menus are not restored.

## Why these limitations matter

Windows 11 Explorer/Start/taskbar internals vary by build. Public Win32 APIs
do not offer a stable, supported switch to wholesale replace the Windows 11
system tray, arbitrary legacy notification flyouts or the built-in taskbar.
Undocumented binary patching/injection would undermine compatibility,
security and our low-overhead goal. We will not represent preview windows
as finished replacements.

Future native shell integration requires explicit per-build validation,
recovery under Explorer restarts, monitoring for changed system interfaces,
test coverage of window accessibility and original app-tray interaction,
and a tested switch back to original Explorer.

## Performance policy

- No polling service, continuous scan or automatic disabling of services.
- Installed Start Menu links are read only when the preview is opened.
- Latest Event 100 boot duration is queried only on demand.
- The tray clock timer is active only while its preview window is open.
- Current-user HKCU Run entries are changed only on confirmation and with
  the original registry value backed up.
- Windows Defender, Windows Update, HKLM Run and Windows core services are
  never modified by this development version.
- Claims about faster startup require before/after Windows 11 measurements
  on the same device, not just a GitHub Actions compilation.

This document describes the implementation's limits; it is not a promise of
full binary compatibility with Windows 10 or of an upcoming installable build.
