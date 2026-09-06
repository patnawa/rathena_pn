[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DataRoot
)

$ErrorActionPreference = 'Stop'
$target = Join-Path $DataRoot 'SystemEN\OngoingQuests.lub'
$patch = Join-Path $PSScriptRoot 'OngoingQuests_Compatibility.lua'
$backup = "$target.bak-before-quest-compat"
$begin = '-- BEGIN RATHENA_PN QUEST COMPATIBILITY PATCH'
$end = '-- END RATHENA_PN QUEST COMPATIBILITY PATCH'
$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$byteSafe = [System.Text.Encoding]::GetEncoding(28591)

if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
    throw "Client quest file was not found: $target"
}
if (-not (Test-Path -LiteralPath $patch -PathType Leaf)) {
    throw "Patch payload was not found: $patch"
}

$content = [System.IO.File]::ReadAllText($target, $byteSafe)
if ($content -notmatch 'QuestInfoList\s*=\s*\{') {
    throw 'The target is not a plaintext OngoingQuests.lub file.'
}
if (-not (Test-Path -LiteralPath $backup)) {
    Copy-Item -LiteralPath $target -Destination $backup
}

$startIndex = $content.IndexOf($begin, [System.StringComparison]::Ordinal)
if ($startIndex -ge 0) {
    $endIndex = $content.IndexOf($end, $startIndex, [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) {
        throw 'The existing quest compatibility marker is incomplete; restore the backup before retrying.'
    }
    $endIndex += $end.Length
    $content = ($content.Substring(0, $startIndex) + $content.Substring($endIndex)).TrimEnd()
}

$payload = [System.IO.File]::ReadAllText($patch, $utf8).Trim()
$content = $content.TrimEnd() + "`r`n`r`n" + $payload + "`r`n"
[System.IO.File]::WriteAllText($target, $content, $byteSafe)

Write-Host "Installed quest compatibility patch: $target"
Write-Host "Original backup: $backup"
