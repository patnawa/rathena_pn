param(
	[Parameter(Mandatory = $true)][string]$ClientRoot
)

$root = [IO.Path]::GetFullPath($ClientRoot)
$systemEn = Join-Path $root 'SystemEN'
$loader = Join-Path $systemEn 'itemInfo.lua'
$metadataSource = Join-Path $PSScriptRoot 'SystemEN\itemInfo_ZeroCell.lua'
$metadataTarget = Join-Path $systemEn 'itemInfo_ZeroCell.lua'

if (!(Test-Path -LiteralPath $loader -PathType Leaf)) {
	throw "SystemEN itemInfo loader not found: $loader"
}

[IO.File]::WriteAllBytes($metadataTarget, [IO.File]::ReadAllBytes($metadataSource))
$loaderText = [IO.File]::ReadAllText($loader, [Text.Encoding]::UTF8)

if (!$loaderText.Contains('itemInfo_ZeroCell.lua')) {
	$loaderText = $loaderText.Replace(
		'"itemInfo_C.lua", -- custom items',
		'"itemInfo_C.lua", -- custom items' + "`r`n`t" + '"itemInfo_ZeroCell.lua", -- Zero Cell'
	)
	$loaderText = $loaderText.Replace(
		'"custom",',
		'"custom",' + "`r`n`t" + '"zerocell",'
	)
	[IO.File]::WriteAllText($loader, $loaderText, [Text.Encoding]::UTF8)
}

Write-Host 'Installed Zero Cell item metadata.'
