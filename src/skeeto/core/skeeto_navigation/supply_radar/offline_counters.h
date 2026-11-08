#pragma once
// Radar diagnostic counters. Skeeto keeps them enabled: each is a single
// plain increment on a game-thread-owned static (no atomics, locks or logging),
// and RadarStats() exposes them so cost stays observable in long sessions.
#define L4N_OBSERVE(...) do { __VA_ARGS__; } while(false)
#define L4N_COUNTERS(...) __VA_ARGS__
