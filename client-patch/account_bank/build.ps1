param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$env:PATH='C:/msys64/mingw32/bin;'+$env:PATH
$cc='C:/msys64/mingw32/bin/gcc.exe';$cxx='C:/msys64/mingw32/bin/g++.exe'
$objects=@()
foreach($name in @('buffer','hook','trampoline','hde/hde32')) {
    $object=Join-Path $out ((Split-Path $name -Leaf)+'.o')
    & $cc -O2 -c (Join-Path $PSScriptRoot "vendor/MinHook/src/$name.c") -o $object
    if($LASTEXITCODE){throw "Build failed: $name"};$objects+=$object
}
$ui=Join-Path $PSScriptRoot 'bank_ui.cpp';$transport=Join-Path $PSScriptRoot 'bank_transport.cpp'
$flags=@('-std=c++17','-O2','-Wall','-Wextra','-static')
$libs=@('-lws2_32','-lgdi32','-luser32','-lwinpthread')
& $cxx @flags -shared $ui $transport @objects -o (Join-Path $out 'BankUI.dll') @libs
if($LASTEXITCODE){throw 'Bank DLL build failed'}
& $cxx @flags -DPN_BANK_PREVIEW $ui $transport @objects -o (Join-Path $out 'BankPreview.exe') @libs
if($LASTEXITCODE){throw 'Preview build failed'}
foreach($name in @('bank_ui_test','bank_transport_test','bank_refresh_test')) {
    & $cxx @flags (Join-Path $PSScriptRoot "../../tools/ci/$name.cpp") @objects -o (Join-Path $out "$name.exe") @libs
    if($LASTEXITCODE){throw "Test build failed: $name"}
}
& $cxx @flags (Join-Path $PSScriptRoot '../../tools/ci/bank_loader_test.cpp') -o (Join-Path $out 'bank_loader_test.exe') @libs
if($LASTEXITCODE){throw 'Loader test build failed'}
Write-Output "Built BankUI.dll, preview and native regression suites in $out"
