# Smart App Control — trusted distribution

The original GitHub EXE is unsigned, so Windows Smart App Control can block it.
Matching SHA-256 proves file identity, not publisher trust. Smart App Control
has no per-application exception. Do not turn off security protection.

## Microsoft Store path (no purchased code signing certificate required)

Microsoft Store signs MSIX packages *after* they pass Store certification.
The GitHub Actions MSIX artifact is UNSIGNED and cannot be treated as a
trusted installer or sideloaded as a signed release.

1. Create/use a Microsoft Partner Center developer account and reserve the app name.
2. Copy the exact package Identity Name and Publisher from Partner Center.
3. In GitHub Settings / Secrets and variables / Actions / Variables, set:
   - WIN11TO10_STORE_IDENTITY: exact Partner Center package identity name
   - WIN11TO10_STORE_PUBLISHER: exact Partner Center publisher distinguished name
4. Re-run the "Store MSIX submission candidate (UNSIGNED)" workflow on the
   configured branch. Inspect the generated package before submission.
5. Replace placeholder icons, finish the real user interface and Windows
   compatibility testing, and submit the MSIX for Microsoft Store review.
6. When approved, install the package *from Microsoft Store*.

Default identities (Win11To10.TestPackage / CN=Win11To10 Test) are for
pipeline testing only. They are not valid for the owner's Store submission.

The project is still a command-line proof of concept. Packaging can build
successfully without passing Store certification. The MSIX itself cannot
remove Smart App Control warnings before Microsoft or another trusted
publisher signs the distributable.

For direct EXE downloads, a trusted code signing certificate and verified
Authenticode signature are needed. A self-signed certificate does not solve
public Smart App Control trust.

Sources:
https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions
https://learn.microsoft.com/windows/apps/publish/faq/get-started-with-the-microsoft-store
https://learn.microsoft.com/windows/msix/desktop/desktop-to-uwp-manual-conversion

## Immediate Windows built-in alternative (no unsigned executable)

Open SystemPropertiesPerformance.exe using Win+R.
Choose Adjust for best performance, then re-enable Smooth edges of screen
fonts. Apply. This provides the requested visual profile using Windows'
native UI with Smart App Control still enabled.
