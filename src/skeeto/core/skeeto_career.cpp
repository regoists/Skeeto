#include "skeeto_ui_text.h"
#include "skeeto_career.h"
#include "skeeto_runtime.h"

#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

CareerArchiveSnapshot g_careerArchive{};

// =============================================================================
// Career Archive — Steam user stats, menu page only.
// Refresh from explicit menu/page-enter/refresh actions, not the steady Paint loop.
// Cache GetProcAddress. Do not refresh every frame.
// =============================================================================
using SteamUserStatsIfaceFn = void* (__cdecl*)();
using SteamGetStatIntFn = bool (__cdecl*)(void*, const char*, int32_t*);

struct CareerWeaponDef {
	const char* stat;
	UiTextEntry name;
};
static const CareerWeaponDef kCareerWeapons[] = {
	{ "Stat.pistol.Kills.Total", {UiId::Text_7f1448b1, L"手枪"} },
	{ "Stat.pistol_magnum.Kills.Total", {UiId::Text_a129e9f1, L"马格南"} },
	{ "Stat.smg.Kills.Total", {UiId::Count, L"UZI"} },
	{ "Stat.smg_silenced.Kills.Total", {UiId::Text_efbc2f2e, L"消音冲锋枪"} },
	{ "Stat.smg_mp5.Kills.Total", {UiId::Count, L"MP5"} },
	{ "Stat.pumpshotgun.Kills.Total", {UiId::Text_49a41c07, L"木喷"} },
	{ "Stat.shotgun_chrome.Kills.Total", {UiId::Text_826b934e, L"铁喷"} },
	{ "Stat.autoshotgun.Kills.Total", {UiId::Text_1c302d07, L"一代连喷"} },
	{ "Stat.shotgun_spas.Kills.Total", {UiId::Count, L"SPAS-12"} },
	{ "Stat.rifle.Kills.Total", {UiId::Count, L"M16"} },
	{ "Stat.rifle_desert.Kills.Total", {UiId::Count, L"SCAR"} },
	{ "Stat.rifle_ak47.Kills.Total", {UiId::Count, L"AK-47"} },
	{ "Stat.rifle_sg552.Kills.Total", {UiId::Count, L"SG552"} },
	{ "Stat.hunting_rifle.Kills.Total", {UiId::Text_fc54ed83, L"木狙"} },
	{ "Stat.sniper_military.Kills.Total", {UiId::Text_a4c62326, L"军用狙击枪"} },
	{ "Stat.sniper_awp.Kills.Total", {UiId::Count, L"AWP"} },
	{ "Stat.sniper_scout.Kills.Total", {UiId::Count, L"Scout"} },
	{ "Stat.grenade_launcher.Kills.Total", {UiId::Text_1a1c3f0d, L"榴弹发射器"} },
	{ "Stat.rifle_m60.Kills.Total", {UiId::Count, L"M60"} },
	{ "Stat.machinegun.Kills.Total", {UiId::Text_f157dd0f, L"固定机枪"} },
	{ "Stat.baseball_bat.Kills.Total", {UiId::Text_447c643c, L"棒球棍"} },
	{ "Stat.chainsaw.Kills.Total", {UiId::Text_50b93562, L"电锯"} },
	{ "Stat.cricket_bat.Kills.Total", {UiId::Text_f6a21fa3, L"板球拍"} },
	{ "Stat.crowbar.Kills.Total", {UiId::Text_93928d5b, L"撬棍"} },
	{ "Stat.electric_guitar.Kills.Total", {UiId::Text_de783107, L"电吉他"} },
	{ "Stat.fireaxe.Kills.Total", {UiId::Text_22b6cdb4, L"消防斧"} },
	{ "Stat.frying_pan.Kills.Total", {UiId::Text_ccef4908, L"平底锅"} },
	{ "Stat.katana.Kills.Total", {UiId::Text_fafa9a64, L"武士刀"} },
	{ "Stat.machete.Kills.Total", {UiId::Text_13f55e5f, L"砍刀"} },
	{ "Stat.tonfa.Kills.Total", {UiId::Text_7c315bdf, L"警棍"} },
	{ "Stat.golfclub.Kills.Total", {UiId::Text_ef6ad945, L"高尔夫球杆"} },
	{ "Stat.pitchfork.Kills.Total", {UiId::Text_7f823352, L"草叉"} },
	{ "Stat.shovel.Kills.Total", {UiId::Text_b3011a58, L"铁铲"} },
	{ "Stat.knife.Kills.Total", {UiId::Text_fd7c1e8d, L"小刀"} },
	{ "Stat.molotov.Kills.Total", {UiId::Text_d5d47924, L"燃烧瓶"} },
	{ "Stat.pipe_bomb.Kills.Total", {UiId::Text_80cedce8, L"土制炸弹"} },
	{ "Stat.vomitjar.Kills.Total", {UiId::Text_90970a77, L"胆汁瓶"} }
};
static bool CareerReadSteamInt(void* stats, SteamGetStatIntFn getInt,
	const char* key, CareerStatValue* out) {
	if (out) *out = {};
	if (!stats || !getInt || !key || !key[0] || !out) return false;
	int32_t value = 0;
	bool ok = false;
	__try { ok = getInt(stats, key, &value); }
	__except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
	if (!ok || value < 0) return false;
	out->valid = true;
	out->value = value;
	return true;
}

static SteamUserStatsIfaceFn s_careerIface = nullptr;
static SteamGetStatIntFn s_careerGetInt = nullptr;
static HMODULE s_careerSteam = nullptr;

void CareerArchiveRefresh() {
	CareerArchiveSnapshot fresh{};
	HMODULE steam = GetModuleHandleA("steam_api.dll");
	if (!steam) {
		s_careerIface = nullptr;
		s_careerGetInt = nullptr;
		s_careerSteam = nullptr;
		fresh.state = 1;
		g_careerArchive = fresh;
		return;
	}
	if (s_careerSteam != steam || !s_careerIface || !s_careerGetInt) {
		s_careerSteam = steam;
		s_careerIface = (SteamUserStatsIfaceFn)GetProcAddress(steam, "SteamAPI_SteamUserStats_v012");
		s_careerGetInt = (SteamGetStatIntFn)GetProcAddress(steam, "SteamAPI_ISteamUserStats_GetStatInt32");
	}
	auto iface = s_careerIface;
	auto getInt = s_careerGetInt;
	void* stats = nullptr;
	if (iface) {
		__try { stats = iface(); }
		__except (EXCEPTION_EXECUTE_HANDLER) { stats = nullptr; }
	}
	if (!stats || !getInt) {
		fresh.state = 1;
		g_careerArchive = fresh;
		return;
	}

	// These are L4D2's real Steam stat identifiers. Keep this list explicit: guessing
	// similarly named fields can silently turn infected-side stats into survivor kills.
	CareerReadSteamInt(stats, getInt, "Stat.TotalPlayTime.Total", &fresh.playTimeSec);
	CareerReadSteamInt(stats, getInt, "Stat.InfectedKilled.Total", &fresh.infectedKills);
	CareerReadSteamInt(stats, getInt, "Stat.GamesPlayed.Total", &fresh.gamesPlayed);
	CareerReadSteamInt(stats, getInt, "Stat.FinaleFinished.Total", &fresh.finalesFinished);
	CareerReadSteamInt(stats, getInt, "Stat.TeamRevived.Total", &fresh.teamRevived);
	CareerReadSteamInt(stats, getInt, "Stat.TeamProtected.Total", &fresh.teamProtected);
	CareerReadSteamInt(stats, getInt, "Stat.KitsShared.Total", &fresh.kitsShared);
	CareerReadSteamInt(stats, getInt, "Stat.KitsUsed.Total", &fresh.kitsUsed);
	CareerReadSteamInt(stats, getInt, "Stat.PillsShared.Total", &fresh.pillsShared);
	CareerReadSteamInt(stats, getInt, "Stat.PillsUsed.Total", &fresh.pillsUsed);
	CareerReadSteamInt(stats, getInt, "Stat.AdrenalineShared.Total", &fresh.adrenalineShared);
	CareerReadSteamInt(stats, getInt, "Stat.AdrenalineUsed.Total", &fresh.adrenalineUsed);
	CareerReadSteamInt(stats, getInt, "Stat.DefibrillatorsUsed.Total", &fresh.defibrillatorsUsed);
	CareerReadSteamInt(stats, getInt, "Stat.Defibrillated.Total", &fresh.defibrillated);

	static const char* kSurvivorStats[4] = {
		"Stat.PlayedNick.Total", "Stat.PlayedCoach.Total",
		"Stat.PlayedEllis.Total", "Stat.PlayedRochelle.Total"
	};
	for (int i = 0; i < 4; ++i)
		CareerReadSteamInt(stats, getInt, kSurvivorStats[i], &fresh.survivorGames[i]);

	static const char* kModeStats[5] = {
		"Stat.RealismGamesPlayed.Total", "Stat.GamesPlayed.Versus",
		"Stat.GamesWon.Versus", "Stat.SurvivalGamesPlayed.Total",
		"Stat.ScavengeGamesPlayed.Total"
	};
	for (int i = 0; i < 5; ++i)
		CareerReadSteamInt(stats, getInt, kModeStats[i], &fresh.modeGames[i]);

	for (int i = 0; i < (int)_countof(kCareerWeapons); ++i) {
		CareerStatValue kills{};
		if (!CareerReadSteamInt(stats, getInt, kCareerWeapons[i].stat, &kills) || kills.value <= 0)
			continue;
		int pos = fresh.topWeaponCount;
		if (pos > 6) pos = 6;
		while (pos > 0 && fresh.topWeapons[pos - 1].value < kills.value)
			--pos;
		if (pos >= 6) continue;
		const int moveEnd = fresh.topWeaponCount < 6 ? fresh.topWeaponCount : 5;
		for (int j = moveEnd; j > pos; --j)
			fresh.topWeapons[j] = fresh.topWeapons[j - 1];
		fresh.topWeapons[pos].source = i;
		fresh.topWeapons[pos].value = kills.value;
		if (fresh.topWeaponCount < 6) ++fresh.topWeaponCount;
	}

	int valid = 0;
	valid += fresh.playTimeSec.valid ? 1 : 0;
	valid += fresh.infectedKills.valid ? 1 : 0;
	valid += fresh.gamesPlayed.valid ? 1 : 0;
	valid += fresh.finalesFinished.valid ? 1 : 0;
	valid += fresh.teamRevived.valid ? 1 : 0;
	valid += fresh.teamProtected.valid ? 1 : 0;
	valid += fresh.kitsShared.valid ? 1 : 0;
	valid += fresh.kitsUsed.valid ? 1 : 0;
	valid += fresh.pillsShared.valid ? 1 : 0;
	valid += fresh.pillsUsed.valid ? 1 : 0;
	valid += fresh.adrenalineShared.valid ? 1 : 0;
	valid += fresh.adrenalineUsed.valid ? 1 : 0;
	valid += fresh.defibrillatorsUsed.valid ? 1 : 0;
	valid += fresh.defibrillated.valid ? 1 : 0;
	for (int i = 0; i < 4; ++i) valid += fresh.survivorGames[i].valid ? 1 : 0;
	for (int i = 0; i < 5; ++i) valid += fresh.modeGames[i].valid ? 1 : 0;
	fresh.refreshedAt = GetTickCount();
	fresh.state = valid <= 0 ? 1 : ((fresh.playTimeSec.valid && fresh.infectedKills.valid
		&& fresh.gamesPlayed.valid && fresh.finalesFinished.valid) ? 3 : 2);
	g_careerArchive = fresh;
	Log("career archive: refresh state=%d fields=%d", fresh.state, valid);
}

void CareerFormatNumber(const CareerStatValue& stat, wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	if (!stat.valid) {
		wcsncpy(out, L"--", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	wchar_t raw[32]{};
	swprintf_s(raw, 32, L"%d", stat.value);
	const int n = (int)wcslen(raw);
	int commas = n > 0 ? (n - 1) / 3 : 0;
	int total = n + commas;
	if (total >= cap) total = cap - 1;
	out[total] = 0;
	int src = n - 1, dst = total - 1, group = 0;
	while (src >= 0 && dst >= 0) {
		out[dst--] = raw[src--];
		if (++group == 3 && src >= 0 && dst >= 0) {
			out[dst--] = L',';
			group = 0;
		}
	}
}

void CareerFormatDuration(const CareerStatValue& stat, wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	if (!stat.valid) {
		wcsncpy(out, L"--", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	const int seconds = stat.value;
	const int hours = seconds / 3600;
	const int minutes = (seconds / 60) % 60;
	const int remain = seconds % 60;
	swprintf_s(out, cap, L"%dh %02dm %02ds", hours, minutes, remain);
}

void CareerFormatRatio(const CareerStatValue& part, const CareerStatValue& total,
	wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	if (!part.valid || !total.valid || total.value <= 0) {
		wcsncpy(out, L"--", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	double percent = (double)part.value * 100.0 / (double)total.value;
	if (percent < 0.0) percent = 0.0;
	if (percent > 999.9) percent = 999.9;
	swprintf_s(out, cap, L"%.1f%%", percent);
}

void CareerFormatKillsPerHour(const CareerStatValue& kills,
	const CareerStatValue& seconds, wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	if (!kills.valid || !seconds.valid || seconds.value <= 0) {
		wcsncpy(out, L"--", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	double rate = (double)kills.value * 3600.0 / (double)seconds.value;
	if (rate < 0.0) rate = 0.0;
	if (rate > 999999.9) rate = 999999.9;
	swprintf_s(out, cap, L"%.1f / h", rate);
}

void CareerFormatPair(const CareerStatValue& first, const CareerStatValue& second,
	wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	wchar_t a[32]{}, b[32]{};
	CareerFormatNumber(first, a, 32);
	CareerFormatNumber(second, b, 32);
	swprintf_s(out, cap, L"%s / %s", a, b);
}


const wchar_t* CareerWeaponName(int source) {
	if (source < 0 || source >= (int)_countof(kCareerWeapons))
		return L"?";
	return UiText(kCareerWeapons[source].name.id, kCareerWeapons[source].name.chinese);
}
