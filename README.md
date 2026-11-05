# Win11To10

A Windows 11 to Windows 10 appearance and performance project.

**Status: v0.1 performance foundation — Windows 10 shell is NOT yet implemented.**

The current native C++ utility measures limited read-only Windows information
and offers an opt-in, reversible visual-effects performance profile.
No background process runs continuously, and no service is installed.

## Already implemented
- On-demand memory load, available RAM, system uptime, Windows build number.
- Read-only startup candidate names from HKCU and HKLM Run registry keys.
- Read-only status for 15 visual-effect settings.
- An opt-in profile disabling listed fades, shadows and animations while
  enabling font smoothing.
- Original current-user visual settings backed up before the first change.
- Restore from the saved original settings, with rollback attempts on failures.
- Windows x64 CI with test and downloadable executable.

**Not yet implemented:** Windows 10 Start menu, taskbar, system tray, Explorer,
startup program controls, background-service management, measured boot time,
GUI or one-click installer. Some visual flags may be ignored by Windows 11.
System uptime is NOT a measure of boot duration.

## Smart App Control protection

The GitHub EXE is **unsigned**. Windows 11 Smart App Control may block it.
Do **not** turn off Windows security to run it. A SHA-256 checksum does not
make an executable trusted. A second GitHub Actions workflow now builds an
**UNSIGNED Microsoft Store MSIX submission candidate** (NOT a trusted installer).
Microsoft Store signing requires a Partner Center submission and approval;
the resulting Store-published package is the intended trusted distribution path.
See [trusted distribution instructions](docs/TRUSTED_DISTRIBUTION.md).

For the visual profile right now without an EXE, open
`SystemPropertiesPerformance.exe` from Win+R, choose best performance, and
re-enable the font-smoothing option before applying.

## Download without Visual Studio
Open the GitHub Actions tab, choose a successful Windows x64 build, and
download the Win11To10-v0.1.0-x64 artifact. Extract its executable.
No local compiler or Visual Studio installation is required.

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
services, startup programs or kernel features. The EXE is not signed; Windows
SmartScreen may warn about unknown publishers.

See docs/ROADMAP.md for planned interface and performance modules.
