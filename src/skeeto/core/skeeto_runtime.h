#pragma once
#include <Windows.h>
#include <atomic>
#include <cstddef>

void Log(const char* fmt, ...);
extern HMODULE g_self;
extern std::atomic_bool g_run;
void SkeetoGameRoot(char* out, size_t n);
