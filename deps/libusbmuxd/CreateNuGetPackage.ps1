Param(
  [string]$build
)

# Update the build number
(gc .\libusbmuxd.autoconfig).replace('{build}', $build)|sc .\libusbmuxd.out.autoconfig

# Create the NuGet package
Import-Module "C:\Program Files (x86)\Outercurve Foundation\modules\CoApp"
Write-NuGetPackage .\libusbmuxd.out.autoconfig
