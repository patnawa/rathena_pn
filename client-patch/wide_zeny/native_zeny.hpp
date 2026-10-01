// Private in-process native HUD bridge. Never exports session credentials.
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>

// Install only for the independently verified executable and HUD render path.
void native_zeny_install();
// Call with a current, authenticated, durable wallet snapshot. Pending saves
// must retain the last confirmed snapshot instead of promoting staged values.
// valid=false invalidates the cached value; zero/negative wallets never wrap.
void native_zeny_update(LONG generation, std::int64_t wallet, bool valid);
// Logout / character transition: invalidate before rendering another session.
void native_zeny_reset();
