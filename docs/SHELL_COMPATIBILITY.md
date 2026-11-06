# Windows 10 shell compatibility boundaries

Target: achieve as much Windows 10 22H2 desktop behavior as possible
on supported Windows 11 builds, with low overhead and reliable rollback.

## Implemented, but not a native shell replacement

- The Start preview implements its own traditional context menu, Enter-key
  activation and closes upon app launch. This applies only within NoyaPLANET;
  it does **not** alter Windows 11 Explorer's system-wide context menus.
- The Start preview also enumerates Windows AppsFolder for packaged/Store apps.
  Selected targets are re-resolved via the Windows Shell namespace and opened
  only on explicit user interaction. The index is in-memory cached until an
  explicit refresh. Turkish-language search and Enter/Down/Escape are supported.
  The real Windows key and original Start integration remain untouched.
- The on-demand Start preview searches installed Start Menu shortcuts and
  launches selected applications. It supports four editable pinned tiles,
  persisted in a per-user config file. It does not change real Start pins. Optional Ctrl+Alt+S works only while the
  native dashboard is running. The Windows key and built-in Start menu remain.
- The taskbar preview enumerates visible windows and opens selected windows,
  displaying a clock. It does not replace or auto-hide Windows' own taskbar.
- An additional optional docked Win32 **AppBar** can be enabled from the
  dashboard with user confirmation. It reserves a thin area along a monitor's
  bottom edge with SHAppBarMessage (ABM_NEW/QUERYPOS/SETPOS/REMOVE), displays
  Start, selected windows and a clock, and releases the area on close.
  Explorer restarts attempt to re-register; a failure closes it safely.
  This is a documented desktop toolbar, **not** an implementation of the
  real Windows taskbar and not a replacement for the notification area.
  The "Tümü" button opens a native keyboard-navigable menu of visible
  application windows, grouped into submenus of 30 entries (up to 1020).
  The menu reports when its safety limit is reached and preserves the
  selected handle while the menu is open, even if window enumeration refreshes.
  Choosing a menu item activates or restores that window; the existing
  taskbar buttons still toggle minimization for the foreground window.
  This overflow menu is not an application-grouping or thumbnail system.
  Multi-monitor/DPI behavior and fullscreen application behavior need testing.
  The bar never enables itself at Windows startup.
- The optional AppBar clock is an accessible, keyboard-operable button which
  opens the existing app-owned native calendar near that clock, clamped to its
  monitor work area. The NoyaPLANET tray icon can open the same calendar.
  Neither control replaces Windows' original clock or notification flyouts.
- The NoyaPLANET tray icon is the application's own notification-area icon.
  It is added with Shell_NotifyIcon, and is removed on exit. Existing icons
  remain owned by Explorer. No programmatic migration of other apps' icons is
  attempted.
- The separate native classic-style file browser provides folder navigation,
  local filtering and basic file opening with Windows APIs. It does **not**
  replace File Explorer, alter file associations, install shell extensions or
  restore the Windows 10 Explorer ribbon/global context menus.
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
