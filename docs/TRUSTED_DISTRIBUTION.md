# Smart App Control and verified distribution

**No installable Win11To10 release is currently available.** The existing
proof-of-concept EXE and older unsigned MSIX builds have no publisher
signature. Windows refusing installation, including error 0x800B010A or
"Publisher: Unknown", is expected.

A SHA-256 checksum only checks file integrity. It does not create a trusted
publisher, sign an application, or override Smart App Control (SAC). Do not
disable SAC, change security policies, import random certificates, or attempt
PowerShell execution-policy tricks to install these development builds.

## Actions taken in repository

1. The regular Windows CI workflow builds and tests only. It no longer
   uploads unsigned EXE or MSIX artifacts for users to install.
2. scripts/Test-TrustedSignature.ps1 verifies a Windows EXE or MSIX with the
   Windows SDK SignTool. It blocks a release that lacks a trusted signature.
   The -ExpectUntrusted switch is for CI NEGATIVE TESTS only. It is not
   an install bypass or release approval.
3. MSIX preparation workflow requires MANUAL dispatch, an explicit
   acknowledgement, and actual Partner Center identity/publisher GitHub
   variables. It uploads a submission-only artifact with clear warnings and
   one-day retention. It is never an installable release.
4. Do not publish a GitHub Release or provide an installer link until the
   release's trusted signature is independently verified and the app passes
   Windows 11 interactive testing.

Older historical workflow runs and their unsigned artifacts may remain on
GitHub until deleted or expired. Do not use those artifacts.

## Path A: Microsoft Store MSIX (recommended; Store signs after approval)

Microsoft Store signs an approved MSIX automatically. You do not need to
purchase a code signing certificate for this publishing route.

Owner action is required (these steps cannot be completed by CI alone):

1. Open https://storedeveloper.microsoft.com/ and sign into a developer
   account; reserve Win11To10 as the Store app name.
2. Obtain the exact package Identity Name and package Publisher from
   Partner Center's app identity details.
3. In GitHub repository Settings > Secrets and variables > Actions >
   Variables, set:
   * WIN11TO10_STORE_IDENTITY = exact Partner Center identity name
   * WIN11TO10_STORE_PUBLISHER = exact Partner Center publisher DN
4. AFTER the CI changes are merged to main, open Actions and manually run
   "Prepare unsigned MSIX for Partner Center (MANUAL ONLY)".
   Enter the exact acknowledgement STORE_SUBMISSION_ONLY_NOT_FOR_INSTALL.
5. Download the artifact ONLY for submitting it to Partner Center, not for
   double-click installation. Replace placeholder artwork and complete
   publishing requirements first. Our v0.1 CLI is only a prototype and
   may not meet Microsoft Store certification or customer experience rules.
6. Submit the MSIX from Partner Center for Store review. If approved,
   install and update the MICROSOFT-SIGNED package through Microsoft Store.
   Store certification is not guaranteed.

A signed Store MSIX does NOT make an unrelated separately downloaded GitHub
EXE or older MSIX trusted.

## Path B: Direct EXE release from GitHub

Requires a valid, publicly trusted code-signing certificate from an eligible
provider, or a supported trusted cloud signing service. Signing and checking
both the Windows binary and installer are prerequisites for distribution.
A self-signed certificate alone does not solve public Smart App Control trust.

After obtaining a certificate, integrate signing with the release pipeline.
Use the release gate before uploading or advertising the binary:

    pwsh -File scripts/Test-TrustedSignature.ps1 -Path Win11To10.exe

It uses SignTool verify /pa /all /v and fails if there is no trusted signer.
A valid signature does not guarantee that antivirus systems will approve
every app or that the app has no bugs.

## Immediate Windows-native workaround (no third-party executable)

For the requested performance visual-effects profile, press Win+R, run
SystemPropertiesPerformance.exe, choose "Adjust for best performance",
and turn on only "Smooth edges of screen fonts". Click Apply.

## Sources

- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options
- https://learn.microsoft.com/en-us/windows/apps/publish/get-started
- https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/upload-app-packages
- https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions
