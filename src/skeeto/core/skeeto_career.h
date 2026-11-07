#pragma once
#include <Windows.h>

struct CareerStatValue {
	bool valid;
	int value;
};
struct CareerRankValue {
	int source;
	int value;
};
struct CareerArchiveSnapshot {
	CareerStatValue playTimeSec;
	CareerStatValue infectedKills;
	CareerStatValue gamesPlayed;
	CareerStatValue finalesFinished;
	CareerStatValue teamRevived;
	CareerStatValue teamProtected;
	CareerStatValue kitsShared;
	CareerStatValue kitsUsed;
	CareerStatValue pillsShared;
	CareerStatValue pillsUsed;
	CareerStatValue adrenalineShared;
	CareerStatValue adrenalineUsed;
	CareerStatValue defibrillatorsUsed;
	CareerStatValue defibrillated;
	CareerStatValue survivorGames[4];
	CareerStatValue modeGames[5];
	CareerRankValue topWeapons[6];
	int topWeaponCount;
	DWORD refreshedAt;
	int state; // 0 not read, 1 unavailable, 2 partial, 3 ready
};

extern CareerArchiveSnapshot g_careerArchive;

void CareerArchiveRefresh();
void CareerFormatNumber(const CareerStatValue& stat, wchar_t* out, int cap);
void CareerFormatDuration(const CareerStatValue& stat, wchar_t* out, int cap);
void CareerFormatRatio(const CareerStatValue& part, const CareerStatValue& total, wchar_t* out, int cap);
void CareerFormatKillsPerHour(const CareerStatValue& kills, const CareerStatValue& seconds, wchar_t* out, int cap);
void CareerFormatPair(const CareerStatValue& first, const CareerStatValue& second, wchar_t* out, int cap);
const wchar_t* CareerWeaponName(int source);
