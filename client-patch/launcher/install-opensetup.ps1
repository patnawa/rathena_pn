[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ClientRoot,
    [string]$Archive
)
$ErrorActionPreference='Stop'
$client=(Resolve-Path -LiteralPath $ClientRoot).Path
foreach($name in @('Ragexe.exe','System/OptionInfo.lub','SystemEN/OptionInfo.lub')){
    if(-not (Test-Path -LiteralPath (Join-Path $client $name) -PathType Leaf)){throw "Choose the complete PN Lua client. Missing: $name"}
}
function Assert-NoLinks([string]$Path){
    $candidate=[IO.Path]::GetFullPath($Path)
    while($candidate){
        if(Test-Path -LiteralPath $candidate){if((Get-Item -LiteralPath $candidate -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked paths are not supported: $candidate"}}
        $candidate=[IO.Path]::GetDirectoryName($candidate)
    }
}
Assert-NoLinks $client
$source='https://nn.ai4rei.net/dev/opensetup/release/2026-07-04opensetup-lua-leech-3.5.0.692.zip'
$expected='588B5A45DB3FC3C370C909BEA19C991996AC13B08A7FCFCA1A483E77801525C3'
$temp=Join-Path ([IO.Path]::GetTempPath()) ('pn-opensetup-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try{
    if(-not $Archive){$Archive=Join-Path $temp 'opensetup.zip';Invoke-WebRequest -Uri $source -OutFile $Archive}
    if((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne $expected){throw 'OpenSetup archive does not match the pinned official stable release.'}
    Expand-Archive -LiteralPath $Archive -DestinationPath (Join-Path $temp 'package')
    $exe=Join-Path $temp 'package/opensetupl.exe'
    if((Get-Item -LiteralPath $exe).VersionInfo.FileVersion -ne '3.5.0.692'){throw 'Unexpected OpenSetup version.'}
    $destination=Join-Path $client 'PNOpenSetup.exe';Assert-NoLinks $destination
    if(Get-Process -Name PNOpenSetup -ErrorAction SilentlyContinue){throw 'Close PN OpenSetup before installing it.'}
    $docs=Join-Path $client 'PN-Tools/OpenSetup';Assert-NoLinks $docs
    New-Item -ItemType Directory -Force -Path $docs | Out-Null
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $temp 'package/doc') -File){
        $target=Join-Path $docs $file.Name;Assert-NoLinks $target;Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    }
    Copy-Item -LiteralPath $exe -Destination $destination -Force
    $ini=Join-Path $client 'PNOpenSetup.ini';Assert-NoLinks $ini
    if(-not (Test-Path -LiteralPath $ini)){
        # This PN client uses savedata Lua and the DirectX 9 renderer.
        # Keep personal resolution, volumes and all Lua settings untouched.
        $config=Get-Content -LiteralPath (Join-Path $temp 'package/opensetup.ini.sample') -Raw
        $config=$config.Replace('UILang=1024','UILang=1033').Replace('PolicyDisableDirectXVersion=0','PolicyDisableDirectXVersion=1')
        [IO.File]::WriteAllText($ini,$config,[Text.Encoding]::UTF8)
    }
    $provenance=@{name='RO OpenSetup';author='Ai4rei/AN';version='3.5.0.692';edition='Lua, no telemetry';website='https://nn.ai4rei.net/dev/opensetup/';source=$source;archive_sha256=$expected.ToLower();binary_sha256=(Get-FileHash -LiteralPath $destination).Hash.ToLower();license='CC BY-NC 4.0';settings='savedata/OptionInfo.lua';renderer='DirectX 9';installed_utc=[DateTime]::UtcNow.ToString('o')}
    $manifest=Join-Path $docs 'PN-INTEGRATION.json';Assert-NoLinks $manifest
    [IO.File]::WriteAllText($manifest,($provenance|ConvertTo-Json),[Text.Encoding]::UTF8)
    Write-Output 'Installed OpenSetup 3.5.0.692 (Lua, no telemetry). Personal game settings and original Setup.exe preserved.'
}finally{
    # The exact owned temporary directory is verified before recursive cleanup.
    $resolved=[IO.Path]::GetFullPath($temp)
    $parent=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')+'\'
    if(-not $resolved.StartsWith($parent,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'pn-opensetup-*'){throw 'Unsafe temporary cleanup path.'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
