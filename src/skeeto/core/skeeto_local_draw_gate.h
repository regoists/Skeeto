#pragma once
// Shared retail client.dll C_TerrorPlayer::DrawModel local-player JZ gate.
// FreeCam flight and first-person body (FPB) both open the same one-byte
// PassThrough; restore only when no owner remains. Paint/game thread only.
#include "skeeto_memread.h"
#include <cstdint>
#include <cstring>

namespace skeeto_local_draw {

enum class Owner : unsigned char { FreeCam = 1, Fpb = 2 };

inline constexpr uint32_t kGateRva = 0x2234d1u;
inline constexpr uint32_t kDispRva = 0x2234e2u;
inline constexpr uint8_t kOrig = 0x13;
inline constexpr uint8_t kPass = 0x00;
inline constexpr uint8_t kGateBytes[] = {
	0x57, 0xe8, 0x99, 0xc1, 0xde, 0xff, 0x83, 0xc4, 0x04,
	0x80, 0xb8, 0xec, 0x25, 0x00, 0x00, 0x00, 0x74, 0x13,
	0x8b, 0x55, 0x0c, 0x52, 0x53, 0x8b, 0xce, 0xe8,
	0x11, 0x0b, 0xe4, 0xff, 0x5b, 0x5f, 0x5e, 0x5d,
	0xc2, 0x08, 0x00, 0x5b, 0x5f, 0x33, 0xc0, 0x5e, 0x5d,
	0xc2, 0x08, 0x00
};
static_assert(kDispRva - kGateRva == 17 && kGateBytes[17] == kOrig, "local DrawModel gate layout");

// Retail x86 client.dll after 2026-11-25 small update (stamp only; SizeOfImage unchanged).
inline constexpr uint32_t kClientStamp = 0x6AFCCCABu;
inline constexpr uint32_t kClientImage = 0x963000u;

using WriteFn = bool (*)(uint8_t* p, const uint8_t* src, int n);
using ExecFn = bool (*)(void* p);

struct State {
	unsigned owners = 0;
	bool rejected = false;
	HMODULE mod = nullptr;
};

inline State& state() {
	static State s;
	return s;
}

inline bool Readable(const void* p, size_t bytes) {
	return skmem::SpanProbeAt(p, bytes, (uintptr_t)_ReturnAddress());
}

inline bool GateMatch(const uint8_t* data, bool allowPass) {
	if (!data) return false;
	for (size_t i = 0; i < sizeof(kGateBytes); ++i) {
		if (i == 17) {
			if (data[i] != kOrig && !(allowPass && data[i] == kPass))
				return false;
		} else if (data[i] != kGateBytes[i]) {
			return false;
		}
	}
	return true;
}

inline bool PeOk(HMODULE mod) {
	if (!mod) return false;
	const auto dos = (const IMAGE_DOS_HEADER*)mod;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 4096)
		return false;
	const auto pe = (const IMAGE_NT_HEADERS32*)((const uint8_t*)mod + dos->e_lfanew);
	return pe->Signature == IMAGE_NT_SIGNATURE
		&& pe->FileHeader.Machine == IMAGE_FILE_MACHINE_I386
		&& pe->FileHeader.TimeDateStamp == kClientStamp
		&& pe->OptionalHeader.SizeOfImage == kClientImage;
}

// write/exec provided by host (AfkWrite / IsExec). paintTid must match caller.
inline bool Acquire(Owner owner, DWORD paintTid, WriteFn write, ExecFn isExec) {
	if (!write || !isExec || GetCurrentThreadId() != paintTid) return false;
	State& s = state();
	const unsigned bit = (unsigned)owner;
	if (s.owners & bit) return true;
	if (s.rejected) return false;

	HMODULE mod = GetModuleHandleA("client.dll");
	if (!mod || !PeOk(mod)) {
		s.rejected = true;
		return false; // caller may log; PE stamp/size must match kClient*
	}
	uint8_t* gate = (uint8_t*)mod + kGateRva;
	uint8_t* disp = (uint8_t*)mod + kDispRva;
	if (!Readable(gate, sizeof(kGateBytes)) || !isExec((void*)disp) || !GateMatch(gate, true)) {
		s.rejected = true;
		return false;
	}
	if (*disp == kPass) {
		// Already pass-through (us or peer). Claim ownership without rewriting.
		s.owners |= bit;
		s.mod = mod;
		return true;
	}
	if (*disp != kOrig) {
		s.rejected = true;
		return false;
	}
	const uint8_t pass = kPass;
	if (!write(disp, &pass, 1) || *disp != kPass) {
		s.rejected = true;
		return false;
	}
	s.owners |= bit;
	s.mod = mod;
	return true;
}

inline void Release(Owner owner, DWORD paintTid, WriteFn write, ExecFn isExec) {
	if (!write || !isExec || GetCurrentThreadId() != paintTid) return;
	State& s = state();
	const unsigned bit = (unsigned)owner;
	if (!(s.owners & bit)) return;
	s.owners &= ~bit;
	if (s.owners) return;

	HMODULE mod = GetModuleHandleA("client.dll");
	if (!mod || mod != s.mod) {
		s.mod = nullptr;
		return;
	}
	uint8_t* gate = (uint8_t*)mod + kGateRva;
	uint8_t* disp = (uint8_t*)mod + kDispRva;
	if (!Readable(gate, sizeof(kGateBytes)) || !isExec((void*)disp)
		|| !GateMatch(gate, true) || *disp != kPass) {
		s.rejected = true;
		s.mod = nullptr;
		return;
	}
	const uint8_t orig = kOrig;
	if (!write(disp, &orig, 1) || *disp != kOrig) {
		s.rejected = true;
		s.mod = nullptr;
		return;
	}
	s.mod = nullptr;
}

inline void ReleaseAll(DWORD paintTid, WriteFn write, ExecFn isExec) {
	State& s = state();
	if (!s.owners && !s.mod) return;
	s.owners = 0;
	if (GetCurrentThreadId() != paintTid || !write || !isExec) {
		s.mod = nullptr;
		return;
	}
	HMODULE mod = GetModuleHandleA("client.dll");
	if (!mod || (s.mod && mod != s.mod)) {
		s.mod = nullptr;
		return;
	}
	uint8_t* gate = (uint8_t*)mod + kGateRva;
	uint8_t* disp = (uint8_t*)mod + kDispRva;
	if (Readable(gate, sizeof(kGateBytes)) && isExec((void*)disp) && GateMatch(gate, true) && *disp == kPass) {
		const uint8_t orig = kOrig;
		write(disp, &orig, 1);
	}
	s.mod = nullptr;
}

inline bool Open() { return state().owners != 0; }
inline bool Rejected() { return state().rejected; }

} // namespace skeeto_local_draw
