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
   acknowledgement, and the reserved Partner Center identity stored in the repository.
   It uploads a submission-only artifact with clear warnings and
   one-day retention. It is never an installable release.
4. Do not publish a GitHub Release or provide an installer link until the
   release's trusted signature is independently verified and the app passes
   Windows 11 interactive testing.

Older historical workflow runs and their unsigned artifacts may remain on
GitHub until deleted or expired. Do not use those artifacts.

## Path A: NoyaPLANET 11to10 on Microsoft Store

The owner registered this public package identity in Partner Center:

- Package/Identity/Name: NoyaPLANET.NoyaPLANET11to10
- Package/Identity/Publisher: CN=B8F538D8-3B22-4726-BFFE-72791FFA184C
- Package/Properties/PublisherDisplayName: NoyaPLANET
- Application display name: NoyaPLANET 11to10

The identical values are committed in packaging/store-identity.json and
used automatically when generating the MSIX manifest. CI opens the resulting
MSIX file and checks the embedded manifest field-by-field. These identifiers
are NOT credentials, passwords or signing private keys; GitHub variables
are no longer necessary for these fields.

Next steps for the owner:
1. Complete any remaining Microsoft Partner Center account verification.
2. Finish a Store-quality interface, metadata, screenshots and branding.
   The v0.2 native dashboard and Start/taskbar previews are not yet a complete
   product; interactive Windows 11 validation is still required.
3. After code review and merging to main, manually run the
   "Prepare unsigned MSIX for Partner Center (MANUAL ONLY)" GitHub workflow.
4. Enter the required acknowledgement: STORE_SUBMISSION_ONLY_NOT_FOR_INSTALL.
5. Upload the resulting UNSIGNED MSIX to Partner Center for certification,
   never attempt local installation of that unsigned artifact.
6. If Microsoft approves and publishes the app, install it from Store,
   which will provide a Microsoft-trusted package signature.

Store approval is not automatic. A correct package identity does not by
itself resolve Windows Smart App Control or error 0x800B010A.

## Path B: Direct EXE release from GitHub

Requires a valid, publicly trusted code-signing certificate from an eligible
provider, or a supported trusted cloud signing service. Signing and checking
both the Windows binary and installer are prerequisites for distribution.
A self-signed certificate alone does not solve public Smart App Control trust.

After obtaining a certificate, integrate signing with the release pipeline.
Use the release gate before uploading or advertising the binary:

    pwsh -File scripts/Test-TrustedSignature.ps1 -Path NoyaPLANET11to10.exe
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
