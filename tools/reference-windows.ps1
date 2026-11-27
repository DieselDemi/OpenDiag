param([Parameter(Mandatory=$true)][string]$ReferenceExe,
      [Parameter(Mandatory=$true)][string]$Probe,
      [Parameter(Mandatory=$true)][string]$OutputDirectory,
      [switch]$Kam,
      [ValidateSet('vid128','trid256','vidtrid384','trid128')][string]$MxConfig)
$ErrorActionPreference = 'Stop'
$probePath = (Resolve-Path $Probe).Path
$exe = (Resolve-Path $ReferenceExe).Path
if ($Kam -and $MxConfig) { throw 'Choose one reference workflow.' }
$pattern = if ($MxConfig) {'exes/MX32_'} else {'exes/PTDIAG_'}
$expected = Get-Content (Join-Path $PSScriptRoot '../reference/SHA256SUMS') | Where-Object { $_ -match $pattern } | ForEach-Object { ($_ -split '\s+')[0] }
if ($expected -notcontains (Get-FileHash -Algorithm SHA256 $exe).Hash) { throw 'Unexpected reference executable for this workflow.' }
if (Test-Path $OutputDirectory) { throw 'Choose a new directory; no existing capture will be overwritten.' }
$work = New-Item -ItemType Directory $OutputDirectory
if ($MxConfig) {
    Copy-Item $exe (Join-Path $work 'MX32.exe')
    Push-Location $work
    try {
        & $probePath (Join-Path $work 'MX32.exe') --mx-config $MxConfig
        if ($LASTEXITCODE -ne 0) { throw 'Original MX32 conversion failed.' }
        Get-FileHash -Algorithm SHA256 'Sample.VBF'
    } finally { Pop-Location }
    return
}
Copy-Item $exe (Join-Path $work 'PTDIAG.exe')
$fixture = if ($Kam) {'../reference/examples/programming/KAM.cfg'} else {'../reference/examples/RPM-VBAT.cfg'}
Copy-Item (Join-Path $PSScriptRoot $fixture) $work
Push-Location $work
try {
    if ($Kam) { & $probePath (Join-Path $work 'PTDIAG.exe') --kam } else { & $probePath (Join-Path $work 'PTDIAG.exe') }
    if ($LASTEXITCODE -ne 0) { throw 'Reference probe did not complete. Inspect the reported dialog.' }
    $rows = @(Get-Content RoundTrip.cfg | Where-Object { $_ -match '^:' })
    $first = if ($Kam) {'^:\s*1010\s+8\s*$'} else {'(?i)rpm\s+1165'}
    $second = if ($Kam) {'^:\s*1048\s+4\s*$'} else {'(?i)vbat\s+1172'}
    if ($rows.Count -ne 2 -or $rows[0] -notmatch $first -or $rows[1] -notmatch $second) { throw 'Unexpected reference output.' }
    Get-FileHash -Algorithm SHA256 'RoundTrip.cfg'
} finally { Pop-Location }
