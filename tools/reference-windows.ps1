param([Parameter(Mandatory=$true)][string]$ReferenceExe,
      [Parameter(Mandatory=$true)][string]$Probe,
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$probePath = (Resolve-Path $Probe).Path
$exe = (Resolve-Path $ReferenceExe).Path
$expected = Get-Content (Join-Path $PSScriptRoot '../reference/SHA256SUMS') | Where-Object { $_ -match 'exes/PTDIAG_' } | ForEach-Object { ($_ -split '\s+')[0] }
if ($expected -notcontains (Get-FileHash -Algorithm SHA256 $exe).Hash) { throw 'Expected one of the five original PTDIAG references.' }
if (Test-Path $OutputDirectory) { throw 'Choose a new directory; no existing capture will be overwritten.' }
$work = New-Item -ItemType Directory $OutputDirectory
Copy-Item $exe (Join-Path $work 'PTDIAG.exe')
Copy-Item (Join-Path $PSScriptRoot '../reference/examples/RPM-VBAT.cfg') $work
Push-Location $work
try {
    & $probePath (Join-Path $work 'PTDIAG.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Reference probe did not complete. Inspect the reported dialog.' }
    $rows = @(Get-Content RoundTrip.cfg | Where-Object { $_ -match '^:' })
    if ($rows.Count -ne 2 -or $rows[0] -notmatch '(?i)rpm\s+1165' -or $rows[1] -notmatch '(?i)vbat\s+1172') { throw 'Unexpected reference output.' }
    Get-FileHash -Algorithm SHA256 'RoundTrip.cfg'
} finally { Pop-Location }
