# Windows copy of tools/check-architecture.sh (ADR 0061): every module directory has a row under
# ARCHITECTURE.md "## Modules", and every row names an existing directory.
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$needed = git ls-files | ForEach-Object {
  $p = $_ -split '/'
  if ($p.Count -gt 2 -and $p[0] -in 'engine', 'packaging', 'content') { "$($p[0])/$($p[1])" }
  elseif ($p.Count -gt 2 -and (($p[0] -eq 'game' -and $p[1] -eq 'src') -or ($p[0] -eq 'masterserver' -and $p[1] -eq 'src'))) { "$($p[0])/$($p[1])" }
  elseif ($p.Count -gt 1 -and $p[0] -in 'tests', 'tools') { $p[0] }
} | Sort-Object -Unique
$inModules = $false
$rows = Get-Content ARCHITECTURE.md | ForEach-Object {
  if ($_ -match '^## ') { $inModules = ($_ -eq '## Modules') }
  elseif ($inModules -and $_ -match '^\| `([^`]+)/` \| ') { $Matches[1] }
} | Sort-Object -Unique
$problems = @()
$problems += $needed | Where-Object { $_ -notin $rows } | ForEach-Object { "no row in ARCHITECTURE.md `"## Modules`": $_" }
$problems += $rows | Where-Object { -not (Test-Path $_ -PathType Container) } | ForEach-Object { "row names a missing directory: $_/" }
if (-not $rows) { $problems += 'ARCHITECTURE.md has no "## Modules" rows' }
if ($problems) { $problems; exit 1 }
Write-Output 'ok: ARCHITECTURE.md has a row for every module directory'
