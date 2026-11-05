# Manual Windows 11 test checklist (before any real release)

Automated CI includes Windows Server x64 and Windows 11 Enterprise ARM64
(build 26200) runners. The ARM64 CI validates native compilation, five automated
tests and Win32 GUI creation/teardown. It does **not** establish compatibility
with a Windows 11 **x64** interactive desktop, all Windows 11 releases or
real-world system tray/Explorer/monitor behaviors.

## Requirements

- A disposable Windows 11 test VM or test account and restore point.
- Record OS build, language, DPI, primary and secondary monitor resolutions.
- Enable Windows Defender and Smart App Control. **Do not** disable either.
- Do not sideload the GitHub unsigned EXE or MSIX on a protected production PC.
- Test through an eligible trusted signed build after publisher onboarding.

## Interface and functionality
- [ ] Window opens without a blank layout or a long blocking delay.
- [ ] RAM total/available values are reasonable; no invented boot-time scores.
- [ ] Refresh displays latest event 100 boot duration, or an explanatory error.
- [ ] Sistem araçları > Açılış geçmişi displays available records and
      mean/median/min/max with no claim of proven speedup.
- [ ] Read-only startup inventory scans HKCU/HKLM Run and user/common Startup
      folders without changing registry values or shortcut files.
- [ ] NoyaPLANET tray menu opens separate native calendar; original Windows
      clock, calendar, flyouts and third-party tray icons still work.
- [ ] Ctrl+Alt+S opens Start preview while dashboard is running, if available.
- [ ] Start menu enumerates per-user/common Start Menu links without crashing.
- [ ] Search matches a shortcut in a nested Start Menu folder.
- [ ] Native classic file browser opens separately without modifying Windows Explorer.
- [ ] Its address bar, back/forward/up/home and folder filter work with Unicode
      filenames, empty folders and inaccessible folders.
- [ ] Double-click opens only explicitly selected files/folders; copying a path
      requires explicit click; no file is modified by the browser.
- [ ] Close the browser, restart Explorer, and confirm all Windows shell
      associations, context menus and taskbar operations remain intact.
- [ ] Right-click a specific Start list item: the exact clicked app is chosen;
      Open, Pin, Unpin work; Enter activates the selected item and closes Start.
- [ ] No app launches during indexing or right-click without an explicit choice.
- [ ] Read-only Windows service report shows expected totals with no SCM
      permission to modify services; Windows Security and Update remain active.
- [ ] Pin a listed shortcut to an available tile; click it to launch only that app.
- [ ] Unpin all four tiles, close/reopen app, confirm empty layout persists.
- [ ] Verify a default tile layout appears only when no pin configuration exists.
- [ ] Corrupt the pin config only in a disposable test profile; confirm safe
      error reporting and no arbitrary command execution.
- [ ] A click launches the selected app; no app runs automatically during scan.
- [ ] Taskbar preview lists and activates visible windows without stealing focus
      until a user clicks one; updates time while open.
- [ ] Enabling the optional docked bar requires explicit confirmation.
- [ ] Docked bar reserves an additional strip without covering the actual
      taskbar, taskbar flyouts or other docked appbars.
- [ ] Docked Start opens the preview positioned above the additional bar.
- [ ] Clicking docked window buttons activates the selected app; refresh is
      user-triggered, not a continuously running enumeration timer.
- [ ] Docked Kapat and menu removal release screen space immediately.
- [ ] Restart Explorer: the bar either re-registers or closes safely.
- [ ] Launch and close exclusive/fullscreen apps and check z-order recovery.
- [ ] Move dashboard between primary and secondary monitors before docking;
      check usable desktop space and no overlap at 100% and 150% DPI.
- [ ] Closing the dashboard always unregisters and removes the optional bar.
- [ ] Original taskbar, clock, tray icons, context menus and Explorer remain
      operational throughout; no injected code or shell replacement.
- [ ] Closing the dashboard exits the process and releases any hotkeys.

## Startup management (test with a harmless disposable Run key only)
- [ ] Add a deliberately harmless, uniquely named HKCU Run string in a test
      account; no HKLM or RunOnce writes.
- [ ] One user-approved disable records the REG_SZ/REG_EXPAND_SZ original type
      and bytes in the NoyaPLANET disabled-startup backup key.
- [ ] Original HKCU Run entry disappears; no other Run entry changes.
- [ ] Restore writes identical bytes, then removes only its own backup.
- [ ] Protected SecurityHealth entry cannot be selected for modification.
- [ ] Conflicting/newly recreated entries are never silently overwritten.
- [ ] Crash/restart between disable and restore leaves recovery data intact.
- [ ] Windows's built-in startup settings continue to display correctly.

## Visual effects and rollback
- [ ] Apply requests confirmation and creates first original per-user baseline.
- [ ] Font smoothing remains enabled; requested visual effects are disabled
      where Windows supports the documented API.
- [ ] Second apply never overwrites the original baseline.
- [ ] Restore returns all 15 original setting values where supported.
- [ ] Partial failures display an error and attempt rollback.

## Compatibility
- [x] Automated native Windows 11 ARM64 runner (build 26200): five tests,
      GUI creation/teardown and read-only smoke checks. See workflow
      run 42041127070. No hands-on GUI testing is implied.
- [ ] Windows 11 x64 desktop: test two DPI settings (100%, 150%) and at least
      two monitor arrangements.
- [ ] Test light/dark mode, high contrast, keyboard and screen-reader access.
- [ ] Test Windows Explorer restart and sign-out/sign-in.
- [ ] Benchmark memory and launch latency while idle and during app search.
- [ ] Complete two reboot baselines with diagnostics Event 100 and note hardware.
- [ ] Save a boot baseline with at least three Event 100 records; reboot at
      least three times with controlled conditions and compare only post-baseline
      EventRecordIDs. New sample count below three must not show a speed claim.
- [ ] Test before/after comparisons over repeated reboots; ignore OS uptime.
- [ ] No increase in persistent service footprint or automatic startup entries.
- [ ] Verify MSIX identity, publisher, signing status and Store installation
      once Microsoft has actually signed and approved it.

## Publishing gate
- [ ] Final product branding/assets and accurate Store screenshots.
- [ ] Owner reviews README, privacy policy, support details and screenshots.
- [ ] Partner Center certification has passed for exact package identity.
- [ ] Downloaded MSIX has a valid trusted publisher signature.
- [ ] Only then label a download or Store listing as installable.

Always preserve Windows Defender, Windows Update, existing system tray
application interactions, and rollback instructions. Unsupported Windows
builds must be reported as such rather than silently patched.
