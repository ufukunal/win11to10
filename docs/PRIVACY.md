# NoyaPLANET 11to10 — Privacy and local data notice

Development version; last reviewed 2026-11-05.

## Data handled locally
NoyaPLANET 11to10 currently reads the following information **locally**
on the Windows device when the relevant screen is opened or refreshed:
- Windows build, system uptime, total and available physical RAM.
- Current user visual-effects preferences.
- Current user's HKCU Run startup names and commands and the application's
  own backups, if the user explicitly disables a startup entry.
- Current user and common Start Menu shortcuts to populate the preview.
- Up to four user-selected pinned Start preview targets, saved locally at
  `%LOCALAPPDATA%\\NoyaPLANET\\11to10\\start-pins-v1.txt`.
- At the user's request, recent Windows boot-duration events (up to 8)
  and names of startup entries in HKCU/HKLM Run and Startup folders.
- Visible window titles for the taskbar preview and optional docked appbar.
- When selected, running/stopped service state and a short list of display
  names read through Windows' Service Control Manager; no service edits.
- The last Windows Diagnostics-Performance Event 100 boot duration, if the
  Windows Event Log is accessible.

## User-controlled changes
- The selected visual effects may be changed after explicit confirmation.
  Their original state is stored under
  %LOCALAPPDATA%\Win11To10\baseline-v1.txt.
- An explicitly selected HKCU Run entry may be copied to
  HKCU\Software\NoyaPLANET\11to10\DisabledStartup and removed from Run.
  The reverse action restores the saved value. This does not affect HKLM,
  other accounts or system services.

Pin/unpin actions update only the per-user local pin configuration file.
This does not edit Windows' own Start layout. Removing the app does not
necessarily delete the pin configuration.

## Network access and telemetry
The application code in this development branch does **not** contain a
telemetry uploader, advertisement SDK, account login, analytics backend or
cloud synchronization. It does not transmit the locally read information
to NoyaPLANET. Opening Windows Settings or other installed applications can
invoke those applications' own, separately governed network services.

Microsoft Store, GitHub and Windows themselves may collect information
under their own policies when their services are used; this project cannot
control those independent activities.

## Deletion and recovery
Uninstalling the app does not automatically erase saved original settings
or the per-user disabled-startup backups: doing so could destroy recovery
data. Restore settings first, then remove only data you no longer require.

Before public publication, validate the packaged app and third-party
dependencies against this notice and add a functioning support/contact link.
This document is a development privacy notice, not a promise of a finalized
Microsoft Store publication.
