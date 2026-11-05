# Win11To10

A Windows 11 to Windows 10 appearance and performance project.

**Store product:** NoyaPLANET 11to10. **Publisher:** NoyaPLANET.

**Status: v0.4 development — native GUI, current-user startup controls, boot history and classic calendar preview.**

The native C++ utility is now paired with a Turkish Win32 graphical dashboard.
The dashboard shows RAM/build details and offers confirmed, reversible effects
settings. The standalone Start preview launches built-in apps and searches
installed user/common Start Menu shortcuts. When the dashboard is open,
Ctrl+Alt+S can open the preview if that global shortcut is available.
Up to four Start menu preview tiles can be individually pinned and unpinned;
their targets are persisted per user and only launched on a click.
The taskbar preview lists and activates real windows on demand, and displays
a native clock (refreshed only while preview is open). While the dashboard
is open, a NoyaPLANET-owned tray icon offers a menu for the dashboard and
previews; it does not replace or enumerate other apps' tray icons.
No resident background service is installed. The system-tools menu can read up to eight
previous Windows boot-duration events (minimum/median/average) and list registered
startup sources across the user and machine Run keys and two Startup folders.
All of these reports are read-only. The program's own tray menu also opens a
native calendar window; it does not replace Windows' taskbar clock.

## Already implemented
- On-demand memory load, available RAM, system uptime, Windows build number.
- Read-only startup candidate names from HKCU and HKLM Run registry keys.
- Read-only status for 15 visual-effect settings.
- An opt-in profile disabling listed fades, shadows and animations while
  enabling font smoothing.
- Original current-user visual settings backed up before the first change.
- Restore from the saved original settings, with rollback attempts on failures.
- A current-user startup manager can disable *one selected* HKCU Run entry after
  confirmation and restore it from its original per-user registry backup.
  It does not modify startup folders, scheduled tasks, other users or services.
- On-demand boot-duration history from Windows Diagnostics-Performance Event
  ID 100: latest, minimum, maximum, mean and median of up to 8 records.
  Logs may be missing or stale; these figures do not prove speed gains.
- Read-only inventory of HKCU/HKLM Run and current/common Startup folders.
  Scheduled Tasks and packaged-app startup sources are not yet indexed.
- Native app-only classic calendar from NoyaPLANET's own system tray menu.
- Persistent, user-editable four-tile Start preview layout saved under
  `%LOCALAPPDATA%\\NoyaPLANET\\11to10\\start-pins-v1.txt`.
  This does not modify Windows' built-in Start menu or pinned items.
- Windows Folder Options opens using supported Windows controls. A classic
  Windows 10 Explorer ribbon is **not** recreated.
- Windows x64 CI with tests; unsigned binaries are not distributed as releases.

**Not yet implemented:** real Windows-key Start-menu replacement, actual
Windows 10 taskbar and complete third-party system tray, the classic Explorer
ribbon/context menus, full startup coverage, dependency-safe service analysis,
repeatable boot-speed comparisons on Windows 11 hardware, Store publication,
or a signed installer. On-demand boot-event reading is available, but does not
prove a performance gain. Some visual flags may be ignored by Windows 11.
System uptime is NOT a measure of boot duration.

## Smart App Control protection

The GitHub EXE is **unsigned**. Windows 11 Smart App Control may block it.
Do **not** turn off Windows security to run it. A SHA-256 checksum does not
make an executable trusted. An opt-in GitHub Actions workflow can build an
**UNSIGNED Microsoft Store MSIX submission candidate** using the real reserved
NoyaPLANET 11to10 identity in `packaging/store-identity.json`. It is NOT a trusted installer.
Microsoft Store signing requires a Partner Center submission and approval;
the resulting Store-published package is the intended trusted distribution path.
See [trusted distribution instructions](docs/TRUSTED_DISTRIBUTION.md).

For the visual profile right now without an EXE, open
`SystemPropertiesPerformance.exe` from Win+R, choose best performance, and
re-enable the font-smoothing option before applying.

## Install status (no Visual Studio required)
There is currently **no publicly installable release**. GitHub CI compiles and
tests the code, but **does not upload unsigned executables as releases**.
Old artifact downloads are unsigned and remain blocked by Smart App Control.
See the trusted distribution guide for Microsoft Store signing steps.
When an approved, signed Store version exists, it can be installed directly
through Microsoft Store without Visual Studio.

## Windows 11 compatibility and validation

Builds run on GitHub-hosted Windows Server, **not** a representative Windows 11
desktop. The Windows 11 manual test plan is in
[docs/WINDOWS11_TEST_PLAN.md](docs/WINDOWS11_TEST_PLAN.md).
Do not claim that a Windows 11 build is supported without testing it.
The startup registry manager edits HKCU only after explicit confirmation;
the real effects of packaging and Windows StartupApproved policy still
require desktop testing.

## Developer binaries
The build outputs `NoyaPLANET11to10.exe` (native Turkish GUI) and
`Win11To10.exe` (on-demand settings helper). Both are packed into the MSIX;
the GUI is the package entry point. Previews do not replace Windows Explorer.

## Commands
Open PowerShell in the EXE folder. Run as a standard user.

    .\Win11To10.exe analyze
    .\Win11To10.exe status
    .\Win11To10.exe apply-performance --confirm
    .\Win11To10.exe restore --confirm

The first two commands only read information.
Only the last two change current-user visual settings and require explicit
confirmation. The backup is stored at:

    %LOCALAPPDATA%\Win11To10\baseline-v1.txt

The initial backup is not overwritten on future applications. Keep it
for recovery. Windows may not honor every requested setting.
Disabling full-window dragging can reduce visual convenience.

## Building on Windows (developers)
GitHub Actions compiles this project using CMake with MSVC on a hosted runner.

    cmake -S . -B build -A x64
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

**Security:** This utility does not disable Defender, Windows Update, security
services, startup programs or kernel features. The prototype is **unsigned**;
Smart App Control may block it. Do not disable security protections to use it.

See docs/ROADMAP.md for planned interface and performance modules.

## Store package version

The development MSIX manifest and native GUI file metadata use version `0.4.0.0`.
The packaging artifact remains unsigned and is only suitable for testing and
Partner Center submission preparation, not local installation.

## Windows 10 compatibility limits

The Start/taskbar and notification-area features are currently safe,
non-invasive previews. Details: [shell compatibility](docs/SHELL_COMPATIBILITY.md).

### New developer commands (v0.4)

Run the graphical dashboard and use **Sistem araçları** to view the
multi-boot history or registered startup sources. Use the NoyaPLANET tray
icon to open the separate native calendar. These are local, user-triggered
features, not system replacements.

## Major remaining work tracked in GitHub

- [Windows 10 shell integration and Explorer fallback](https://github.com/ufukunal/win11to10/issues/2)
- [Measured performance and safe startup coverage](https://github.com/ufukunal/win11to10/issues/3)
- [Real Windows 11 QA, Store approval and trusted signing](https://github.com/ufukunal/win11to10/issues/4)

**A passing GitHub Actions build is not Windows 11 desktop validation or Store approval.**
