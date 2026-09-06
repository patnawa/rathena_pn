[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$DataRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceFragment = Join-Path $PSScriptRoot 'SystemEN\LuaFiles514\itemInfo_fashion_points.lua'
$systemRoot = Join-Path $DataRoot 'SystemEN'
$loader = Join-Path $systemRoot 'itemInfo.lua'
$targetFragment = Join-Path $systemRoot 'LuaFiles514\itemInfo_fashion_points.lua'
$adapter = Join-Path $systemRoot 'itemInfo_Fashion.lua'
$loaderBackup = "$loader.bak-before-fashion"
$fragmentBackup = "$targetFragment.bak-before-fashion"
$adapterBackup = "$adapter.bak-before-fashion"
$byteSafe = [Text.Encoding]::GetEncoding(28591)
$utf8 = [Text.UTF8Encoding]::new($false)

if (!(Test-Path -LiteralPath $sourceFragment -PathType Leaf)) {
	throw "Fashion item metadata was not found: $sourceFragment"
}
if (!(Test-Path -LiteralPath $loader -PathType Leaf)) {
	throw "ROenglishRE itemInfo loader was not found: $loader"
}

$content = [IO.File]::ReadAllText($loader, $byteSafe)
foreach ($required in @('ImportFiles = {', 'ImportTables = {', 'F_itemInfoMerge')) {
	if (!$content.Contains($required)) {
		throw "The target itemInfo.lua is not a supported ROenglishRE multi-iteminfo loader (missing '$required')."
	}
}

$newline = if ($content.Contains("`r`n")) { "`r`n" } else { "`n" }
$fileLine = "`t`"itemInfo_Fashion.lua`", -- Fashion Points"
$tableLine = "`t`"fashion`","

function Add-ImportLine {
	param(
		[string]$Text,
		[string]$TableName,
		[string]$EntryPattern,
		[string]$Line
	)
	if ([regex]::IsMatch($Text, $EntryPattern, [Text.RegularExpressions.RegexOptions]::Multiline)) {
		return $Text
	}
	$block = [regex]::Match(
		$Text,
		'(?ms)^' + [regex]::Escape($TableName) + '\s*=\s*\{.*?^\}'
	)
	if (!$block.Success) {
		throw "Could not find a simple $TableName table in SystemEN/itemInfo.lua."
	}
	$closingOffset = $block.Value.LastIndexOf('}')
	$insertAt = $block.Index + $closingOffset
	return $Text.Insert($insertAt, $Line + $newline)
}

$content = Add-ImportLine $content 'ImportFiles' '(?m)^\s*"itemInfo_Fashion\.lua"\s*,' $fileLine
$content = Add-ImportLine $content 'ImportTables' '(?m)^\s*"fashion"\s*,' $tableLine

if (!(Test-Path -LiteralPath $loaderBackup)) {
	Copy-Item -LiteralPath $loader -Destination $loaderBackup
}
if ((Test-Path -LiteralPath $targetFragment) -and !(Test-Path -LiteralPath $fragmentBackup)) {
	Copy-Item -LiteralPath $targetFragment -Destination $fragmentBackup
}
if ((Test-Path -LiteralPath $adapter) -and !(Test-Path -LiteralPath $adapterBackup)) {
	Copy-Item -LiteralPath $adapter -Destination $adapterBackup
}

$targetDirectory = Split-Path -Parent $targetFragment
[IO.Directory]::CreateDirectory($targetDirectory) | Out-Null
Copy-Item -LiteralPath $sourceFragment -Destination $targetFragment -Force
[IO.File]::WriteAllText($adapter,
	"-- Fashion Points metadata adapter for the ROenglishRE multi-iteminfo loader.$newline" +
	"tbl_fashion = dofile(`"SystemEN/LuaFiles514/itemInfo_fashion_points.lua`")$newline",
	$utf8)
[IO.File]::WriteAllText($loader, $content, $byteSafe)

$installed = [IO.File]::ReadAllText($loader, $byteSafe)
if (([regex]::Matches($installed, '(?m)^\s*"itemInfo_Fashion\.lua"\s*,')).Count -ne 1 -or
	([regex]::Matches($installed, '(?m)^\s*"fashion"\s*,')).Count -ne 1) {
	throw 'Fashion itemInfo registration verification failed.'
}

Write-Host "Installed Fashion Points item metadata: $targetFragment"
Write-Host "Registered adapter: $adapter"
Write-Host "Original loader backup: $loaderBackup"
