#pragma once
#include <Windows.h>
#include <cstdint>
constexpr uint32_t SKT_BOOT_MAGIC = 0x31544B53; // SKT1
constexpr uint32_t SKT_BOOT_ABI = 1;
enum SktBootstrapState : LONG {
    SKT_BOOT_DUPLICATE = 0, SKT_BOOT_LOADING = 1, SKT_BOOT_WORKER = 2,
    SKT_BOOT_READY = 3, SKT_BOOT_HIDDEN = 4, SKT_BOOT_FAILED = 5
};
struct SktBootstrapIdentity {
    uint32_t size, magic, abi, processId;
    HMODULE module;
    mutable volatile LONG state;
};
using SktBootstrapQueryFn = const SktBootstrapIdentity* (__cdecl*)();
