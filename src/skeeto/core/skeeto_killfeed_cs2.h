#pragma once
// CS2-style kill feed helpers ported from L4N reference (icons / UV / comic panel).
// Paint uses Skeeto Surf* APIs. Event/row ownership stays in skeeto_proxy.cpp.

#include <cstring>
#include <cctype>

namespace kfcs2 {

inline int ClampI(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}
inline bool Eq(const char* a, const char* b) {
	if (!a || !b) return a == b;
	while (*a && *b) {
		char ca = *a, cb = *b;
		if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + 32);
		if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
		if (ca != cb) return false;
		++a; ++b;
	}
	return !*a && !*b;
}

struct Icon {
	const char* weapon;
	int sheet, x, y, w, h;
};

static const Icon kIcons[] = {
	{"autoshotgun", 0, 0, 448, 192, 64},
	{"chainsaw", 1, 384, 384, 128, 64},
	{"grenade_launcher", 1, 0, 256, 148, 64},
	{"hunting_rifle", 0, 320, 384, 192, 64},
	{"molotov", 0, 0, 320, 64, 64},
	{"fire", 0, 256, 448, 64, 64},
	{"pipe_bomb", 0, 448, 256, 64, 64},
	{"pistol_magnum", 0, 320, 256, 64, 64},
	{"pistol", 0, 256, 320, 64, 64},
	{"pumpshotgun", 0, 320, 320, 192, 64},
	{"rifle_ak47", 1, 128, 384, 192, 64},
	{"rifle_desert", 1, 224, 128, 160, 64},
	{"rifle_sg552", 2, 0, 0, 192, 64},
	{"rifle", 0, 128, 384, 192, 64},
	{"shotgun_chrome", 1, 192, 192, 192, 64},
	{"shotgun_spas", 1, 256, 64, 128, 64},
	{"smg_mp5", 2, 192, 0, 192, 64},
	{"smg_silenced", 1, 256, 0, 128, 64},
	{"smg", 0, 0, 384, 128, 64},
	{"sniper_awp", 2, 192, 64, 192, 64},
	{"sniper_military", 1, 0, 192, 192, 64},
	{"sniper_scout", 2, 0, 64, 192, 64},
	{"vomitjar", 1, 384, 192, 64, 64},
	{"fireaxe", 1, 0, 320, 128, 64},
	{"frying_pan", 1, 256, 320, 128, 64},
	{"machete", 1, 384, 320, 128, 64},
	{"katana", 1, 128, 448, 128, 64},
	{"crowbar", 1, 256, 448, 128, 64},
	{"cricket_bat", 1, 0, 384, 128, 64},
	{"baseball_bat", 1, 128, 320, 128, 64},
	{"tonfa", 1, 0, 448, 128, 64},
	{"knife", 2, 392, 0, 120, 64},
	{"electric_guitar", 1, 256, 256, 128, 64},
	{"golfclub", 3, 0, 0, 128, 64},
	{"pitchfork", 4, 0, 0, 128, 64},
	{"shovel", 5, 0, 0, 128, 64},
	{"melee", 2, 392, 0, 120, 64},
	{"rifle_m60", 6, 0, 0, 256, 64},
};
static const Icon kDeathIcon = {"", 7, 0, 0, 64, 64};

inline const char* SheetPath(int sheet) {
	static const char* kPaths[] = {
		"l4n_cs2killfeed/killfeed/iconsheet",
		"l4n_cs2killfeed/killfeed/iconsheet2",
		"l4n_cs2killfeed/killfeed/iconsheet3",
		"vgui/hud/icon_golfclub",
		"vgui/hud/icon_pitchfork",
		"vgui/hud/icon_shovel",
		"l4n_cs2killfeed/icons136/m60",
		"l4n_cs2killfeed/icons136/death",
		"l4n_cs2killfeed/icons136/finish",
		"l4n_cs2killfeed/killfeed/katana_full",
	};
	if (sheet < 0 || sheet >= (int)(sizeof(kPaths) / sizeof(kPaths[0])))
		return kPaths[7];
	return kPaths[sheet];
}

inline int SheetW(int sheet) {
	return sheet == 6 || sheet == 9 ? 256 : sheet == 7 ? 64 : sheet >= 3 ? 128 : 512;
}
inline int SheetH(int sheet) {
	return sheet >= 3 ? 64 : sheet == 2 ? 128 : 512;
}

inline const Icon* FindIcon(const char* weapon) {
	if (Eq(weapon, "m60")) weapon = "rifle_m60";
	for (const auto& icon : kIcons)
		if (Eq(icon.weapon, weapon)) return &icon;
	return nullptr;
}
inline const Icon* RenderIcon(const char* weapon) {
	const Icon* icon = FindIcon(weapon);
	return icon ? icon : &kDeathIcon;
}
inline bool HasFinish(const Icon* icon) {
	return icon != &kDeathIcon
		&& !Eq(icon->weapon, "fire") && !Eq(icon->weapon, "molotov")
		&& !Eq(icon->weapon, "pipe_bomb") && !Eq(icon->weapon, "vomitjar");
}
inline Icon VisibleIcon(const Icon& icon) {
	return Eq(icon.weapon, "katana") ? Icon{icon.weapon, 9, 0, 0, 128, 48} : icon;
}
inline int IconWidth(const char* weapon, int height, int maxWidth) {
	const Icon* icon = FindIcon(weapon);
	if (!icon) return height < maxWidth ? height : maxWidth;
	Icon visible = VisibleIcon(*icon);
	int width = height * visible.w / visible.h;
	return width < maxWidth ? width : maxWidth;
}

struct UV { float left, top, right, bottom; };
inline UV IconUV(const Icon& icon) {
	int tw = SheetW(icon.sheet), th = SheetH(icon.sheet);
	int xr = ClampI(icon.x + icon.w, 0, tw);
	return {(xr - .5f) / tw, (icon.y + .5f) / th, (icon.x + .5f) / tw, (icon.y + icon.h - .5f) / th};
}
inline Icon FinishedIcon(const Icon* icon) {
	int i = (int)(icon - kIcons);
	Icon tile{icon->weapon, 8, (i % 8) * 128 + 2, (i / 8) * 64 + 2, 124, 60};
	if (Eq(icon->weapon, "katana")) return {icon->weapon, 9, 128, 0, 128, 48};
	return tile;
}
inline UV FinishUV(const Icon& icon) {
	if (icon.sheet == 9) return IconUV(icon);
	return {(icon.x + icon.w - .5f) / 1024.f, (icon.y + .5f) / 512.f,
		(icon.x + .5f) / 1024.f, (icon.y + icon.h - .5f) / 512.f};
}

inline unsigned Mix(unsigned base, unsigned other, int amount) {
	amount = ClampI(amount, 0, 255);
	auto ch = [&](int shift) {
		return ((((base >> shift) & 255) * (255 - amount)
			+ ((other >> shift) & 255) * amount + 127) / 255) << shift;
	};
	return ch(16) | ch(8) | ch(0);
}
inline unsigned TeamColor(int team) {
	return team == 2 ? 0x6f9ce6u : team == 3 ? 0xeabe54u : 0x888888u;
}

inline void FillRgb(unsigned rgb, int a, int* r, int* g, int* b, int* aa) {
	*r = (rgb >> 16) & 255;
	*g = (rgb >> 8) & 255;
	*b = rgb & 255;
	*aa = ClampI(a, 0, 255);
}

// Comic cut-corner panel via SurfFill (x0,y0,x1,y1).
template <class FillFn>
inline void ComicPanel(int x, int y, int w, int h, int cut, unsigned bg, int alpha, FillFn fill) {
	cut = ClampI(cut, 1, h / 4);
	int r, g, b, a;
	FillRgb(bg, alpha, &r, &g, &b, &a);
	for (int i = 0; i < cut; ++i) {
		int inset = cut - i;
		fill(x + inset, y + i, x + w, y + i + 1, r, g, b, a);
	}
	fill(x, y + cut, x + w, y + h - cut, r, g, b, a);
	for (int i = 0; i < cut; ++i) {
		int inset = i + 1;
		fill(x, y + h - cut + i, x + w - inset, y + h - cut + i + 1, r, g, b, a);
	}
}

inline void NormalizeWeapon(char* out, int outN, const char* input) {
	if (!out || outN < 2) return;
	out[0] = 0;
	if (!input) input = "";
	if (std::strlen(input) >= 7) {
		bool match = true;
		const char* prefix = "weapon_";
		for (int i = 0; i < 7; ++i) {
			char c = input[i];
			if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
			if (c != prefix[i]) { match = false; break; }
		}
		if (match) input += 7;
	}
	int n = 0;
	for (; input[n] && n < outN - 1; ++n) {
		char c = input[n];
		if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
		out[n] = ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') ? c : '_';
	}
	out[n] = 0;
}

struct ComicUV { float l, t, r, b; };
inline int ComicFireFrame(unsigned now, unsigned born) {
	return (int)(((now - born) % 4000u) * 64u / 4000u);
}
inline ComicUV ComicTileUV(bool fire, int frame) {
	int cols = fire ? 8 : 4, rows = fire ? 8 : 2;
	frame = ClampI(frame, 0, cols * rows - 1);
	float w = (float)(cols * 256), h = (float)(rows * 64);
	int x = (frame % cols) * 256, y = (frame / cols) * 64;
	return {(x + 2.5f) / w, (y + 2.5f) / h, (x + 253.5f) / w, (y + 61.5f) / h};
}

struct ChargeUV { float l, t, r, b; };
inline ChargeUV ChargeBreakTileUV(int frame) {
	frame = ClampI(frame, 0, 15);
	int x = (frame % 4) * 256 + 2, y = (frame / 4) * 144 + 2;
	return {(x + .5f) / 1024.f, (y + .5f) / 1024.f, (x + 251.5f) / 1024.f, (y + 139.5f) / 1024.f};
}
inline ChargeUV HunterAirshotTileUV(int frame) {
	frame = ClampI(frame, 0, 15);
	int x = (frame % 4) * 256 + 2, y = (frame / 4) * 256 + 2;
	return {(x + .5f) / 1024.f, (y + .5f) / 1024.f, (x + 251.5f) / 1024.f, (y + 251.5f) / 1024.f};
}
inline int HunterAirshotFrame(unsigned age) {
	if (age < 540u) return (int)(age / 90u);
	if (age < 820u) return 6 + (int)((age - 540u) / 70u);
	return ClampI(10 + (int)((age - 820u) / 90u), 10, 15);
}

enum ComicEffect { ComicNone, ComicSlash, ComicImpact, ComicFire, ComicHeadshot };
inline ComicEffect ComicType(const char* weapon, bool headshot, int attackerTeam) {
	if (headshot) return ComicHeadshot;
	if (Eq(weapon, "inferno") || Eq(weapon, "entityflame") || Eq(weapon, "env_fire")
		|| Eq(weapon, "fire") || Eq(weapon, "molotov"))
		return ComicFire;
	const char* blades[] = {"katana", "machete", "knife", "fireaxe", "chainsaw"};
	for (const char* w : blades)
		if (Eq(weapon, w)) return ComicSlash;
	const char* impacts[] = {
		"baseball_bat", "cricket_bat", "crowbar", "electric_guitar", "frying_pan",
		"golfclub", "pitchfork", "shovel", "tonfa"
	};
	for (const char* w : impacts)
		if (Eq(weapon, w)) return ComicImpact;
	if (Eq(weapon, "melee") && attackerTeam == 2) return ComicImpact;
	return ComicNone;
}
inline unsigned ComicAccent(ComicEffect kind) {
	return kind == ComicSlash ? 0x31e3e0u
		: kind == ComicImpact ? 0xa5cbe0u
		: kind == ComicFire ? 0xf2a15bu : 0xf08187u;
}
inline const wchar_t* ComicWord(ComicEffect kind) {
	return kind == ComicSlash ? L"SLASH!"
		: kind == ComicImpact ? L"BONK!"
		: kind == ComicFire ? L"BURN" : L"POP!";
}

} // namespace kfcs2
