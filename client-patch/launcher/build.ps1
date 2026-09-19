param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$sources=@((Join-Path $PSScriptRoot 'Launcher.cs'),(Join-Path $PSScriptRoot 'SelfTest.cs'))
$key=Join-Path $PSScriptRoot 'trusted-public-key.xml'
& $compiler /nologo /target:winexe /platform:anycpu /optimize+ /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Web.Extensions.dll "/resource:$key,trusted-public-key.xml" "/out:$out/PNLauncher.exe" @sources
if($LASTEXITCODE){throw 'Launcher build failed'}
& $compiler /nologo /target:exe /platform:anycpu /optimize+ /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Web.Extensions.dll "/resource:$key,trusted-public-key.xml" "/out:$out/PNLauncherCheck.exe" @sources
if($LASTEXITCODE){throw 'Launcher check build failed'}
& (Join-Path $out 'PNLauncherCheck.exe') --self-test
if($LASTEXITCODE){throw 'Launcher self-tests failed'}
