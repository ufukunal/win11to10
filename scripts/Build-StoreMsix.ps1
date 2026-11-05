# Builds an UNSIGNED MSIX for Microsoft Store submission only.
# Microsoft signs approved Store packages; this is NOT safe to sideload.
param(
  [Parameter(Mandatory)][string]$ExePath,
  [Parameter(Mandatory)][string]$AuxiliaryExecutable,
  [string]$IdentityFile = "",
  [string]$OutputDirectory = "dist"
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($IdentityFile)) {
  $IdentityFile = Join-Path $PSScriptRoot "..\packaging\store-identity.json"
}
if (-not (Test-Path -LiteralPath $IdentityFile -PathType Leaf)) {
  throw "Store identity file not found: $IdentityFile"
}
$config = Get-Content -LiteralPath $IdentityFile -Raw -Encoding UTF8 | ConvertFrom-Json
$IdentityName = [string]$config.identityName
$Publisher = [string]$config.publisher
$DisplayName = [string]$config.displayName
$PublisherDisplayName = [string]$config.publisherDisplayName
$Version = [string]$config.version
if ($IdentityName -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{2,49}$') { throw "Invalid Store identity name." }
if ($Publisher -notmatch '^CN=[0-9A-Fa-f]{8}(-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}$') { throw "Invalid Store publisher GUID." }
if ([string]::IsNullOrWhiteSpace($DisplayName) -or $DisplayName.Length -gt 100) { throw "Invalid display name." }
if ([string]::IsNullOrWhiteSpace($PublisherDisplayName) -or $PublisherDisplayName.Length -gt 100) { throw "Invalid publisher display name." }
if ($Version -notmatch '^\d{1,5}(\.\d{1,5}){3}$') { throw "Invalid Store version." }
foreach ($part in $Version.Split('.')) {
  if ([int]$part -gt 65535) { throw "Store version component too large." }
}
$exe = (Resolve-Path -LiteralPath $ExePath).Path
$auxiliaryExe = (Resolve-Path -LiteralPath $AuxiliaryExecutable).Path
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$outDir = (Resolve-Path -LiteralPath $OutputDirectory).Path
$stage = Join-Path ([IO.Path]::GetTempPath()) ("Win11To10-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path (Join-Path $stage "Assets") -Force | Out-Null

# Procedural NoyaPLANET icon: an original ringed-planet mark.
# Replaces the old generic four-square placeholder, no external font/assets.
function Write-NoyaPlanetIcon([int]$size, [string]$path) {
  $image = [Drawing.Bitmap]::new($size, $size)
  $graphic = [Drawing.Graphics]::FromImage($image)
  $planet = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(99, 212, 236))
  $shine = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(246, 251, 255))
  $ring = [Drawing.Pen]::new(
    [Drawing.Color]::FromArgb(246, 251, 255),
    [single]([Math]::Max(2, $size * 0.060))
  )
  try {
    $graphic.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphic.Clear([Drawing.Color]::FromArgb(20, 32, 56))
    $graphic.TranslateTransform([single]($size / 2), [single]($size / 2))
    $graphic.RotateTransform(-24)
    $graphic.DrawEllipse($ring, [int](-$size * 0.39), [int](-$size * 0.18),
                         [int]($size * 0.78), [int]($size * 0.36))
    $graphic.ResetTransform()
    $graphic.FillEllipse($planet, [int]($size * 0.28), [int]($size * 0.28),
                        [int]($size * 0.44), [int]($size * 0.44))
    $graphic.FillEllipse($shine, [int]($size * 0.36), [int]($size * 0.34),
                        [int]([Math]::Max(2, $size * 0.13)),
                        [int]([Math]::Max(2, $size * 0.13)))
    $graphic.FillEllipse($shine, [int]($size * 0.78), [int]($size * 0.15),
                        [int]([Math]::Max(2, $size * 0.055)),
                        [int]([Math]::Max(2, $size * 0.055)))
    $image.Save($path, [Drawing.Imaging.ImageFormat]::Png)
  }
  finally {
    $ring.Dispose()
    $shine.Dispose()
    $planet.Dispose()
    $graphic.Dispose()
    $image.Dispose()
  }
}

try {
  Add-Type -AssemblyName System.Drawing
  Copy-Item -LiteralPath $exe -Destination (Join-Path $stage "NoyaPLANET11to10.exe")
  Copy-Item -LiteralPath $auxiliaryExe -Destination (Join-Path $stage "Win11To10.exe")
  Write-NoyaPlanetIcon 44 (Join-Path $stage "Assets\Square44.png")
  Write-NoyaPlanetIcon 150 (Join-Path $stage "Assets\Square150.png")
  Write-NoyaPlanetIcon 50 (Join-Path $stage "Assets\StoreLogo.png")

  $identity = [Security.SecurityElement]::Escape($IdentityName)
  $publisherDn = [Security.SecurityElement]::Escape($Publisher)
  $displayXml = [Security.SecurityElement]::Escape($DisplayName)
  $publisherDisplayXml = [Security.SecurityElement]::Escape($PublisherDisplayName)
  $versionXml = [Security.SecurityElement]::Escape($Version)
  $manifest = @"
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
 xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
 xmlns:uap10="http://schemas.microsoft.com/appx/manifest/uap/windows10/10"
 xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
 IgnorableNamespaces="uap uap10 rescap">
  <Identity Name="$identity" Publisher="$publisherDn" Version="$versionXml" ProcessorArchitecture="x64"/>
  <Properties>
    <DisplayName>$displayXml</DisplayName>
    <PublisherDisplayName>$publisherDisplayXml</PublisherDisplayName>
    <Logo>Assets\StoreLogo.png</Logo>
  </Properties>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.22000.0" MaxVersionTested="10.0.26100.0"/>
  </Dependencies>
  <Resources><Resource Language="tr-TR"/><Resource Language="en-US"/></Resources>
  <Applications>
    <Application Id="App" Executable="NoyaPLANET11to10.exe"
      uap10:RuntimeBehavior="packagedClassicApp" uap10:TrustLevel="mediumIL">
      <uap:VisualElements DisplayName="$displayXml" Description="Windows 10 appearance and performance panel"
        BackgroundColor="#182B4A" Square150x150Logo="Assets\Square150.png"
        Square44x44Logo="Assets\Square44.png"/>
    </Application>
  </Applications>
  <Capabilities><rescap:Capability Name="runFullTrust"/></Capabilities>
</Package>
"@
  [IO.File]::WriteAllText((Join-Path $stage "AppxManifest.xml"), $manifest, [Text.UTF8Encoding]::new($false))
  $sdkBin = Join-Path ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) "Windows Kits\10\bin"
  $makeAppx = Get-ChildItem -Path (Join-Path $sdkBin "*\x64\makeappx.exe") -File |
    Sort-Object FullName -Descending | Select-Object -First 1
  if (-not $makeAppx) { throw "Windows SDK MakeAppx.exe not found." }
  $package = Join-Path $outDir "Win11To10-Store-UNSIGNED-x64.msix"
  & $makeAppx.FullName pack /d $stage /p $package /o
  if ($LASTEXITCODE -ne 0) { throw "MakeAppx failed, exit code $LASTEXITCODE" }
  $sha = (Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash.ToLowerInvariant()
  [IO.File]::WriteAllText("$package.sha256",
    ($sha + " *Win11To10-Store-UNSIGNED-x64.msix" + [Environment]::NewLine),
    [Text.Encoding]::ASCII)
  Write-Host "Built unsigned Store submission candidate: $package"
  Write-Warning "Do NOT sideload. Microsoft Store signing and approval are required."
} finally {
  if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
