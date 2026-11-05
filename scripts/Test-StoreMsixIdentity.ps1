# Confirms MSIX manifest matches public Partner Center product identity.
# This does NOT validate signatures, Microsoft Store approval or app safety.
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [string]$IdentityFile = ""
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($IdentityFile)) {
  $IdentityFile = Join-Path $PSScriptRoot "..\packaging\store-identity.json"
}
$config = Get-Content -LiteralPath $IdentityFile -Raw -Encoding UTF8 | ConvertFrom-Json
$package = (Resolve-Path -LiteralPath $Path).Path
Add-Type -AssemblyName System.IO.Compression
$zip = [IO.Compression.ZipFile]::OpenRead($package)
try {
  if ($null -eq $zip.GetEntry("NoyaPLANET11to10.exe")) {
    throw "Native GUI executable is missing in package."
  }
  if ($null -eq $zip.GetEntry("Win11To10.exe")) {
    throw "On-demand performance engine is missing in package."
  }
  $entry = $zip.GetEntry("AppxManifest.xml")
  if ($null -eq $entry) { throw "AppxManifest.xml is missing." }
  $stream = $entry.Open()
  $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::UTF8)
  try {
    [xml]$manifest = $reader.ReadToEnd()
  } finally {
    $reader.Dispose()
    $stream.Dispose()
  }

  $identity = $manifest.SelectSingleNode("//*[local-name()='Identity']")
  $display = $manifest.SelectSingleNode("//*[local-name()='Properties']/*[local-name()='DisplayName']")
  $publisherDisplay = $manifest.SelectSingleNode("//*[local-name()='Properties']/*[local-name()='PublisherDisplayName']")
  $visual = $manifest.SelectSingleNode("//*[local-name()='VisualElements']")
  $app = $manifest.SelectSingleNode("//*[local-name()='Application']")
  if ($null -eq $identity -or $null -eq $display -or $null -eq $publisherDisplay -or $null -eq $visual -or $null -eq $app) {
    throw "MSIX manifest lacks identity or display elements."
  }
  $actual = @{
    identityName = $identity.GetAttribute("Name")
    publisher = $identity.GetAttribute("Publisher")
    version = $identity.GetAttribute("Version")
    displayName = $display.InnerText
    publisherDisplayName = $publisherDisplay.InnerText
    visualDisplayName = $visual.GetAttribute("DisplayName")
    executable = $app.GetAttribute("Executable")
  }
  $expected = @{
    identityName = [string]$config.identityName
    publisher = [string]$config.publisher
    version = [string]$config.version
    displayName = [string]$config.displayName
    publisherDisplayName = [string]$config.publisherDisplayName
    visualDisplayName = [string]$config.displayName
    executable = "NoyaPLANET11to10.exe"
  }
  foreach ($key in $expected.Keys) {
    if ($actual[$key] -cne $expected[$key]) {
      throw ("MSIX mismatch for " + $key + ": expected '" + $expected[$key] + "', actual '" + $actual[$key] + "'.")
    }
  }
  Write-Host "MSIX identity matches NoyaPLANET 11to10 Partner Center identity."
  Write-Warning "Manifest validation passed, but this package is UNSIGNED. Do not install it."
}
finally {
  $zip.Dispose()
}
