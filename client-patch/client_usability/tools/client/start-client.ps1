[CmdletBinding()]
param(
    [switch]$CheckOnly,
    [string]$ClientRoot
)

$ErrorActionPreference = 'Stop'
try {
    if (-not $ClientRoot) { $ClientRoot = Join-Path $PSScriptRoot '../..' }
    $gameRoot = (Resolve-Path -LiteralPath $ClientRoot).Path
    $problems = [System.Collections.Generic.List[string]]::new()
    function Require-ClientFile([string]$RelativePath) {
        $candidate = Join-Path $gameRoot $RelativePath
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            $problems.Add("Missing file: $RelativePath")
            return $false
        }
        return $true
    }
    $null = Require-ClientFile 'Ragexe.exe'
    $archives = @{}
    $archiveNames = @{}
    if (Require-ClientFile 'DATA.INI') {
        $inData = $false
        foreach ($rawLine in Get-Content -LiteralPath (Join-Path $gameRoot 'DATA.INI')) {
            $line = $rawLine.Trim()
            if (-not $line -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }
            if ($line -match '^\[(.+)\]$') { $inData = ($Matches[1] -ieq 'Data'); continue }
            if (-not $inData) { continue }
            if ($line -notmatch '^(\d+)\s*=\s*(.+)$') {
                $problems.Add("Invalid DATA.INI entry: $line"); continue
            }
            $priority = [int]$Matches[1]
            $name = $Matches[2].Trim()
            if ($archives.ContainsKey($priority)) { $problems.Add("Duplicate archive priority: $priority") }
            if ($archiveNames.ContainsKey($name)) { $problems.Add("Archive listed twice: $name") }
            $archives[$priority] = $name
            $archiveNames[$name] = $true
            if ($name -match '[/\\:]' -or $name -notmatch '(?i)\.grf$') {
                $problems.Add("Use a GRF filename in DATA.INI: $name"); continue
            }
            if (Require-ClientFile $name) {
                $stream = [System.IO.File]::OpenRead((Join-Path $gameRoot $name))
                try {
                    $header = New-Object byte[] 46
                    $count = $stream.Read($header, 0, 46)
                    $signature = [System.Text.Encoding]::ASCII.GetString($header, 0, 16)
                    $version = [System.BitConverter]::ToUInt32($header, 42)
                    $classic = $signature.StartsWith("Master of Magic`0") -and $version -eq 0x200
                    $modern = $signature.StartsWith("Event Horizon`0") -and $version -eq 0x300
                    if ($count -ne 46 -or -not ($classic -or $modern)) {
                        $problems.Add("Unreadable GRF header: $name")
                    }
                } finally { $stream.Dispose() }
            }
        }
        if ($archives.Count -eq 0) { $problems.Add('DATA.INI has no archives in its [Data] section.') }
        for ($i = 0; $i -lt $archives.Count; $i++) {
            if (-not $archives.ContainsKey($i)) { $problems.Add("Missing archive priority $i in DATA.INI.") }
        }
    }
    if (Require-ClientFile 'SystemEN/itemInfo.lua') {
        $loader = Get-Content -LiteralPath (Join-Path $gameRoot 'SystemEN/itemInfo.lua') -Raw
        # Check the literal imports used by this client's loader without executing Lua.
        foreach ($match in [regex]::Matches($loader, '(?m)^\s*(?:dofile|require)\s*\(\s*["'']([^"'']+)["'']\s*\)')) {
            $import = $match.Groups[1].Value
            if ($import -notmatch '\.(lua|lub)$') { $import += '.lua' }
            $null = Require-ClientFile $import
        }
        $imports = [regex]::Match($loader, '(?s)ImportFiles\s*=\s*\{(.*?)\}')
        foreach ($match in [regex]::Matches($imports.Groups[1].Value, '(?m)^\s*["'']([^"'']+)["'']')) {
            $null = Require-ClientFile ('SystemEN/' + $match.Groups[1].Value)
        }
    }
    if (Test-Path -LiteralPath (Join-Path $gameRoot 'FontScale.ini')) {
        $fontSettings = Get-Content -LiteralPath (Join-Path $gameRoot 'FontScale.ini') -Raw
        $scaleMatch = [regex]::Match($fontSettings, '(?m)^\s*Scale\s*=\s*([^\r\n;]+)')
        $scale = 0.0
        if (-not $scaleMatch.Success -or -not [double]::TryParse($scaleMatch.Groups[1].Value.Trim(), [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$scale) -or [double]::IsNaN($scale) -or $scale -lt 1 -or $scale -gt 2) {
            $problems.Add('FontScale.ini needs a Scale between 1.00 and 2.00.')
        }
        $null = Require-ClientFile 'FontScale.dll'
    }
    if ($problems.Count -gt 0) {
        Write-Host 'The client needs attention before starting:' -ForegroundColor Yellow
        foreach ($problem in $problems) { Write-Host "  - $problem" }
        Write-Host 'Restore the named files from your client backup or install the matching client patch.'
        exit 1
    }
    Write-Host "Client file check passed: $($archives.Count) archives and item-info imports are present." -ForegroundColor Green
    Write-Host 'This checks file presence and GRF headers, not every archive entry or in-game behavior.'
    if (-not $CheckOnly) {
        Start-Process -FilePath (Join-Path $gameRoot 'Ragexe.exe') -WorkingDirectory $gameRoot -WindowStyle Normal | Out-Null
    }
    exit 0
} catch {
    Write-Host ('Client check failed: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
}
