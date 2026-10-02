# Activates the repo hooks and checks required tools.
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
git config core.hooksPath .githooks
$missing = $false
foreach ($t in 'gitleaks', 'cmake', 'ninja', 'python') {
  if (-not (Get-Command $t -ErrorAction SilentlyContinue)) { Write-Error "missing: $t" -ErrorAction Continue; $missing = $true }
}
if (-not $env:VCPKG_ROOT) { Write-Warning 'VCPKG_ROOT is not set (the presets use it)' }
if ($missing) { exit 1 }
Write-Output 'hooks active (.githooks); tools found'
