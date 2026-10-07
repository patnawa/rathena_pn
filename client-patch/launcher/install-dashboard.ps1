param(
    [Parameter(Mandatory=$true)][string]$Build,
    [Parameter(Mandatory=$true)][string]$ClientRoot,
    [string]$OpenSetupArchive,
    [switch]$SkipOpenSetup
)
$ErrorActionPreference='Stop'
$clientPath=(Resolve-Path -LiteralPath $ClientRoot).Path
$binary=Join-Path $Build 'PNLauncher.exe'
if(-not (Test-Path -LiteralPath $binary -PathType Leaf)){throw 'Build the launcher before installing the dashboard.'}
if(-not (Test-Path -LiteralPath (Join-Path $clientPath 'Start Game.cmd') -PathType Leaf)){throw 'Choose the complete PN-Client folder.'}
if(-not $SkipOpenSetup){
    & (Join-Path $PSScriptRoot 'install-opensetup.ps1') -ClientRoot $clientPath -Archive $OpenSetupArchive
}
$branding=Join-Path $clientPath 'PN-Branding/Midgard'
New-Item -ItemType Directory -Force -Path $branding | Out-Null
foreach($name in @('pn-launcher.ico','ragexe.ico')){Copy-Item -LiteralPath (Join-Path $PSScriptRoot "assets/midgard/$name") -Destination (Join-Path $branding $name) -Force}
$launcher=Join-Path $clientPath 'PNLauncher-20261007.exe'
Copy-Item -LiteralPath $binary -Destination $launcher -Force
$command="@echo off`r`ncd /d `"%~dp0`"`r`nstart `"`" `"%~dp0PNLauncher-20261007.exe`"`r`n"
[IO.File]::WriteAllText((Join-Path $clientPath 'Launch PN Dashboard.cmd'),$command,[Text.Encoding]::ASCII)
$shell=New-Object -ComObject WScript.Shell
try{
    $launcherShortcut=$shell.CreateShortcut((Join-Path $clientPath 'PN Launcher.lnk'))
    $launcherShortcut.TargetPath=$launcher
    $launcherShortcut.WorkingDirectory=$clientPath
    $launcherShortcut.IconLocation=(Join-Path $branding 'pn-launcher.ico')+',0'
    $launcherShortcut.Description='PN Ragnarok dashboard: play, update, repair, settings and release notes.'
    $launcherShortcut.Save()
    $gameShortcut=$shell.CreateShortcut((Join-Path $clientPath 'PN Ragnarok.lnk'))
    $gameShortcut.TargetPath=Join-Path $clientPath 'Start Game.cmd'
    $gameShortcut.WorkingDirectory=$clientPath
    $gameShortcut.IconLocation=(Join-Path $branding 'ragexe.ico')+',0'
    $gameShortcut.Description='Start PN Ragnarok with the client readiness checks.'
    $gameShortcut.Save()
}finally{[void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shell)}
if(-not ('PNBrandingShell' -as [type])){
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class PNBrandingShell {
    [DllImport("shell32.dll",CharSet=CharSet.Unicode)]
    public static extern void SHChangeNotify(uint change,uint flags,string path,IntPtr unused);
}
'@
}
foreach($path in @($launcher,(Join-Path $clientPath 'PN Launcher.lnk'),(Join-Path $clientPath 'PN Ragnarok.lnk'))){
    [PNBrandingShell]::SHChangeNotify(0x2000,0x2005,$path,[IntPtr]::Zero)
}
Write-Output "Installed: $launcher"
Write-Output 'Open PN Launcher.lnk or Launch PN Dashboard.cmd for the new dashboard.'
