<#
.SYNOPSIS
  Fails closed when a binary or MSIX does not have a trusted Authenticode
  signature. This is a build/release safety check, NOT a way around SAC.
.DESCRIPTION
  Uses Windows SDK SignTool with the Authenticode policy (/pa).
  -ExpectUntrusted is for CI's *negative* test of deliberately unsigned builds.
  Never use -ExpectUntrusted to authorize or publish a release.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [switch]$ExpectUntrusted
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
  throw "Cannot verify: file is missing: $Path"
}
$resolved = (Resolve-Path -LiteralPath $Path).Path
if ([IO.Path]::GetExtension($resolved).ToLowerInvariant() -notin @(".exe", ".msix")) {
  throw "Only .exe and .msix signatures can be verified by this script."
}

$sdkRoot = Join-Path ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) "Windows Kits\10\bin"
if (-not (Test-Path -LiteralPath $sdkRoot)) {
  throw "Windows SDK is missing; refusing to treat this file as verified."
}
$signTool = Get-ChildItem -Path (Join-Path $sdkRoot "*\x64\signtool.exe") -File |
  Sort-Object FullName -Descending | Select-Object -First 1
if (-not $signTool) {
  throw "SignTool is missing; refusing to treat this file as verified."
}

& $signTool.FullName verify /pa /all /v $resolved
$verificationCode = $LASTEXITCODE
$hash = (Get-FileHash -LiteralPath $resolved -Algorithm SHA256).Hash
Write-Host "SHA-256: $hash"
Write-Host "SignTool exit status: $verificationCode"

if ($ExpectUntrusted) {
  if ($verificationCode -eq 0) {
    throw "Negative test failed: the build is unexpectedly signed and trusted."
  }
  Write-Host "Expected refusal: unsigned or untrusted file cannot be distributed."
  exit 0
}
if ($verificationCode -ne 0) {
  throw "BLOCKED: No trusted publisher signature. Refusing installation/release."
}
Write-Host "Trusted signature verified. Store certification and app safety are separate checks."
