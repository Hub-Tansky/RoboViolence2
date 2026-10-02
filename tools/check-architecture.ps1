# Fails if ARCHITECTURE.md's file inventory and `git ls-files` disagree.
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$tracked = git ls-files | Sort-Object
$listed = Select-String -Path ARCHITECTURE.md -Pattern '^\| `([^`]+)` \| ' | ForEach-Object { $_.Matches[0].Groups[1].Value } | Sort-Object
$diff = Compare-Object $tracked $listed
if ($diff) {
  $diff | ForEach-Object { if ($_.SideIndicator -eq '<=') { "unlisted: $($_.InputObject)" } else { "stale: $($_.InputObject)" } }
  exit 1
}
Write-Output 'ok: ARCHITECTURE.md matches git ls-files'
