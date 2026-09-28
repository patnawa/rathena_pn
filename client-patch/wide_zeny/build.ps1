param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$env:PATH='C:/msys64/mingw32/bin;'+$env:PATH
$cc='C:/msys64/mingw32/bin/gcc.exe';$cxx='C:/msys64/mingw32/bin/g++.exe'
$objects=@()
foreach($name in @('buffer','hook','trampoline','hde/hde32')) {
    $object=Join-Path $out ((Split-Path $name -Leaf)+'.o')
    & $cc -O2 -c (Join-Path $PSScriptRoot "../account_bank/vendor/MinHook/src/$name.c") -o $object
    if($LASTEXITCODE){throw "Build failed: $name"};$objects+=$object
}
$ui=Join-Path $PSScriptRoot 'bank_ui.cpp';$transport=Join-Path $PSScriptRoot 'bank_transport.cpp'
$market=@((Join-Path $PSScriptRoot '../wide_market/market_ui.cpp'),(Join-Path $PSScriptRoot '../wide_market/market_transport.cpp'))
$mail=@((Join-Path $PSScriptRoot '../wide_mail/mail_ui.cpp'),(Join-Path $PSScriptRoot '../wide_mail/mail_transport.cpp'))
$flags=@('-std=c++17','-O2','-Wall','-Wextra','-static')
$libs=@('-lws2_32','-lgdi32','-luser32','-lwinpthread','-lcomctl32')
& $cxx @flags -shared $ui $transport @market @mail @objects -o (Join-Path $out 'PNWallet64.dll') @libs
if($LASTEXITCODE){throw 'Wallet DLL build failed'}
& $cxx @flags -DPN_BANK_PREVIEW $ui $transport @market @mail @objects -o (Join-Path $out 'PNWallet64Preview.exe') @libs
if($LASTEXITCODE){throw 'Wallet preview build failed'}
& $cxx @flags -shared (Join-Path $PSScriptRoot 'fontscale_forward.cpp') (Join-Path $PSScriptRoot '../account_bank/FontScale.def') -o (Join-Path $out 'FontScale.dll') -lwinpthread
if($LASTEXITCODE){throw 'Economy loader build failed'}
& $cxx @flags -shared (Join-Path $PSScriptRoot '../turbo/turbo.cpp') -o (Join-Path $out 'PNTurbo.dll') -luser32 -lgdi32 -lwinpthread
if($LASTEXITCODE){throw 'Economy turbo compatibility build failed'}
& $cxx @flags (Join-Path $PSScriptRoot '../../tools/ci/wide_zeny_loader_test.cpp') -o (Join-Path $out 'wide_zeny_loader_test.exe') -luser32 -lgdi32 -lwinpthread
if($LASTEXITCODE){throw 'Economy loader test build failed'}
foreach($name in @('wide_zeny_client_ui_test','wide_zeny_client_transport_test','wide_market_ui_test','wide_market_transport_test','wide_market_numbers_test','wide_mail_ui_test','wide_mail_transport_test')) {
    & $cxx @flags (Join-Path $PSScriptRoot "../../tools/ci/$name.cpp") @objects -o (Join-Path $out "$name.exe") @libs
    if($LASTEXITCODE){throw "Test build failed: $name"}
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'PNWallet64.ini') -Destination $out
Write-Output "Built isolated wallet v3 DLL, preview and client regression suites in $out"
