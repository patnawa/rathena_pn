param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$compiler='C:/msys64/mingw32/bin/gcc.exe'
$env:PATH='C:/msys64/mingw32/bin;'+$env:PATH
$vendor=Join-Path $PSScriptRoot '../account_bank/vendor/MinHook/src'
$sources=@('buffer.c','hook.c','trampoline.c','hde/hde32.c') | ForEach-Object {Join-Path $vendor $_}
& $compiler -std=c11 -O2 -Wall -Wextra -shared (Join-Path $PSScriptRoot 'native_font.c') @sources (Join-Path $PSScriptRoot 'exports.def') -static-libgcc -static -s -lgdi32 -o (Join-Path $out 'FontScaleOriginal.dll')
if($LASTEXITCODE){throw 'Font build failed'}
& $compiler -std=c11 -O2 -Wall -Wextra (Join-Path $PSScriptRoot 'test.c') -static -s -lgdi32 -luser32 -o (Join-Path $out 'NativeFontTest.exe')
if($LASTEXITCODE){throw 'Test build failed'}
& $compiler -std=c11 -O2 -Wall -Wextra -shared (Join-Path $PSScriptRoot 'test_external.c') -static -s -lgdi32 -o (Join-Path $out 'ExternalFontTest.dll')
if($LASTEXITCODE){throw 'External fixture build failed'}
