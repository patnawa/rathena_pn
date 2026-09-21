#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=${1:-build}
mkdir -p "$out"
compiler=${CXX:-i686-w64-mingw32-g++}
for source in buffer hook trampoline hde/hde32; do
    i686-w64-mingw32-gcc -O2 -c "vendor/MinHook/src/$source.c" -o "$out/$(basename "$source").o"
done
"$compiler" -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++ -shared bank_ui.cpp bank_transport.cpp "$out/buffer.o" "$out/hook.o" "$out/trampoline.o" "$out/hde32.o" -o "$out/BankUI.dll" -lws2_32 -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++ -shared fontscale_forward.cpp FontScale.def -o "$out/FontScale.dll" -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra -DPN_BANK_PREVIEW bank_ui.cpp bank_transport.cpp "$out/buffer.o" "$out/hook.o" "$out/trampoline.o" "$out/hde32.o" -o "$out/BankPreview.exe" -lws2_32 -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra ../../tools/ci/bank_transport_test.cpp "$out/buffer.o" "$out/hook.o" "$out/trampoline.o" "$out/hde32.o" -o "$out/BankTransportTest.exe" -lws2_32 -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra ../../tools/ci/bank_loader_test.cpp -o "$out/BankLoaderTest.exe" -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra ../../tools/ci/bank_ui_test.cpp -o "$out/BankUITest.exe" -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra ../../tools/ci/bank_reentry_test.cpp -o "$out/BankReentryTest.exe" -lgdi32 -luser32 -static -lwinpthread
"$compiler" -std=c++17 -O2 -Wall -Wextra ../../tools/ci/bank_refresh_test.cpp "$out/buffer.o" "$out/hook.o" "$out/trampoline.o" "$out/hde32.o" -o "$out/BankRefreshTest.exe" -lws2_32 -lgdi32 -luser32 -static -lwinpthread
