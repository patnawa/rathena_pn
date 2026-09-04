[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DataRoot
)

$ErrorActionPreference = 'Stop'
$target = Join-Path $DataRoot 'SystemEN\OngoingQuests.lub'
$patch = Join-Path $PSScriptRoot 'OngoingQuests_Biosphere.lua'
$backup = "$target.bak-before-biosphere"
$begin = '-- BEGIN RATHENA_PN BIOSPHERE QUEST PATCH'
$end = '-- END RATHENA_PN BIOSPHERE QUEST PATCH'
$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
# Treat the existing translation as a byte-preserving single-byte stream. Some
# ROenglishRE releases contain legacy CP949 bytes mixed into this Lua file;
# decoding and re-encoding the whole file as UTF-8 would corrupt those bytes.
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
        throw 'The existing Biosphere patch marker is incomplete; restore the backup before retrying.'
    }
    $endIndex += $end.Length
    $content = ($content.Substring(0, $startIndex) + $content.Substring($endIndex)).TrimEnd()
}

$payload = [System.IO.File]::ReadAllText($patch, $utf8).Trim()
$content = $content.TrimEnd() + "`r`n`r`n" + $payload + "`r`n"
[System.IO.File]::WriteAllText($target, $content, $byteSafe)

Write-Host "Installed Biosphere quest patch: $target"
Write-Host "Original backup: $backup"
