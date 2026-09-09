# ============================================================================
#  PN  /  CLIENT TOOLING
#  install_client_patch.ps1
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/chapter2/install_client_patch.ps1
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

param(
    [Parameter(Mandatory = $true)][string]$ClientDataDirectory,
    [string]$BaseResnametable
)

$targetDirectory = [IO.Path]::GetFullPath($ClientDataDirectory)
if (!(Test-Path -LiteralPath $targetDirectory -PathType Container)) {
    throw "Client data directory not found: $targetDirectory"
}

$target = Join-Path $targetDirectory 'resnametable.txt'
$snippet = Join-Path $PSScriptRoot 'resnametable_chapter2.txt'
if (!(Test-Path -LiteralPath $snippet -PathType Leaf)) {
    throw "Chapter 2 resnametable snippet not found: $snippet"
}

if (!(Test-Path -LiteralPath $target -PathType Leaf)) {
    if (!$BaseResnametable) {
        throw 'No loose resnametable.txt exists. Supply -BaseResnametable with the table extracted from your highest-priority GRF.'
    }
    $base = [IO.Path]::GetFullPath($BaseResnametable)
    if (!(Test-Path -LiteralPath $base -PathType Leaf)) {
        throw "Base resnametable not found: $base"
    }
    [IO.File]::WriteAllBytes($target,[IO.File]::ReadAllBytes($base))
}

$existing = [IO.File]::ReadAllText($target,[Text.Encoding]::Default)
if (!$existing.Contains('kindlbrk.gnd#icas_in.gnd#')) {
    $patch = [IO.File]::ReadAllText($snippet,[Text.Encoding]::UTF8)
    [IO.File]::AppendAllText($target,"`r`n"+$patch,[Text.Encoding]::Default)
}

$clientRoot = Split-Path $targetDirectory -Parent
$systemEn = Join-Path $clientRoot 'SystemEN'
$loader = Join-Path $systemEn 'itemInfo.lua'
$metadataSource = Join-Path $PSScriptRoot 'SystemEN\itemInfo_Chapter2.lua'
$metadataTarget = Join-Path $systemEn 'itemInfo_Chapter2.lua'
if ((Test-Path -LiteralPath $loader) -and (Test-Path -LiteralPath $metadataSource)) {
    [IO.File]::WriteAllBytes($metadataTarget,[IO.File]::ReadAllBytes($metadataSource))
    $loaderText = [IO.File]::ReadAllText($loader,[Text.Encoding]::UTF8)
    if (!$loaderText.Contains('itemInfo_Chapter2.lua')) {
        $loaderText = $loaderText.Replace('"itemInfo_C.lua", -- custom items','"itemInfo_C.lua", -- custom items' + "`r`n`t" + '"itemInfo_Chapter2.lua", -- Chapter 2 Flame Branch')
        $loaderText = $loaderText.Replace('"custom",','"custom",' + "`r`n`t" + '"chapter2",')
        [IO.File]::WriteAllText($loader,$loaderText,[Text.Encoding]::UTF8)
    }
}

$questInstaller = Join-Path (Split-Path $PSScriptRoot -Parent) 'quest_compat\install_client_patch.ps1'
if (Test-Path -LiteralPath $questInstaller -PathType Leaf) {
    # The quest table lives beside the loose data directory under
    # <client>\SystemEN, not under <client>\data\SystemEN.
    & $questInstaller -DataRoot $clientRoot
}

Write-Host "Installed Chapter 2 map aliases, item metadata, and quest text."
