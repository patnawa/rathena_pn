param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$env:PATH='C:/msys64/mingw32/bin;'+$env:PATH
$cc='C:/msys64/mingw32/bin/gcc.exe';$cxx='C:/msys64/mingw32/bin/g++.exe'
$bank=Join-Path $PSScriptRoot '../account_bank'
$objects=@()
foreach($name in @('buffer','hook','trampoline','hde/hde32')){
    $object=Join-Path $out ((Split-Path $name -Leaf)+'.o')
    & $cc -O2 -c (Join-Path $bank "vendor/MinHook/src/$name.c") -o $object
    if($LASTEXITCODE){throw 'MinHook build failed'};$objects+=$object
}
& $cxx -std=c++17 -O2 -s -Wall -Wextra -static -shared (Join-Path $bank 'bank_ui.cpp') (Join-Path $bank 'bank_transport.cpp') @objects -o (Join-Path $out 'BankUI.dll') -lws2_32 -lgdi32 -luser32 -lwinpthread
if($LASTEXITCODE){throw 'Bank compatibility build failed'}
& $cxx -std=c++17 -O2 -s -Wall -Wextra -static -shared (Join-Path $bank 'fontscale_forward.cpp') (Join-Path $bank 'FontScale.def') -o (Join-Path $out 'FontScale.dll') -lwinpthread
if($LASTEXITCODE){throw 'Loader build failed'}
& $cxx -std=c++17 -O2 -s -Wall -Wextra -static -shared (Join-Path $PSScriptRoot 'turbo.cpp') -o (Join-Path $out 'PNTurbo.dll') -luser32 -lgdi32 -lwinpthread
if($LASTEXITCODE){throw 'Turbo build failed'}
& $cxx -std=c++17 -O2 -Wall -Wextra -static (Join-Path $PSScriptRoot 'core_test.cpp') -o (Join-Path $out 'TurboCoreTest.exe')
if($LASTEXITCODE){throw 'Scheduler test build failed'}
& (Join-Path $out 'TurboCoreTest.exe');if($LASTEXITCODE){throw 'Scheduler tests failed'}
& $cxx -std=c++17 -O2 -Wall -Wextra -static (Join-Path $PSScriptRoot 'native_test.cpp') -o (Join-Path $out 'TurboNativeTest.exe') -luser32 -lgdi32 -lwinpthread
if($LASTEXITCODE){throw 'Native adapter test build failed'}
& (Join-Path $out 'TurboNativeTest.exe');if($LASTEXITCODE){throw 'Native adapter tests failed'}
foreach($name in @('bank_transport_test','bank_ui_test','bank_refresh_test')){
    & $cxx -std=c++17 -O2 -static (Join-Path $PSScriptRoot "../../tools/ci/$name.cpp") @objects -o (Join-Path $out "$name.exe") -lws2_32 -lgdi32 -luser32 -lwinpthread
    if($LASTEXITCODE){throw "Build failed: $name"}
    Push-Location $out
    try{& (Join-Path $out "$name.exe");if($LASTEXITCODE){throw "Test failed: $name"}}finally{Pop-Location}
}
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
& $compiler /nologo /target:winexe /optimize+ /r:System.Windows.Forms.dll /r:System.Drawing.dll "/out:$out/PNTurboConfig.exe" (Join-Path $PSScriptRoot 'TurboConfig.cs')
if($LASTEXITCODE){throw 'Setup build failed'}
& $compiler /nologo /target:exe /optimize+ /r:System.Windows.Forms.dll /r:System.Drawing.dll "/out:$out/TurboConfigTest.exe" (Join-Path $PSScriptRoot 'TurboConfig.cs')
if($LASTEXITCODE){throw 'Setup test build failed'}
& (Join-Path $out 'TurboConfigTest.exe') --self-test;if($LASTEXITCODE){throw 'Setup tests failed'}
