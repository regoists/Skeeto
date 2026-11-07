// Friendly-fire event policy shared by the bounded event dispatcher and tests.
// Pure checks only; Event copies fields and Paint owns entity resolution.
#pragma once

namespace skeeto_damage {

enum DamageSourceMode {
	kDamageSourceTeammates = 0,
	kDamageSourceAll = 1
};

enum FriendlyFireDamageTier {
	kFriendlyFireDamageNone = 0,
	kFriendlyFireDamageLight,
	kFriendlyFireDamageHeavy
};

inline bool IsFriendlyFireDamage(int victimEnt, int victimTeam,
	int attackerEnt, int attackerTeam, int damage) {
	return victimEnt > 0 && attackerEnt > 0 && victimEnt != attackerEnt
		&& victimTeam == 2 && attackerTeam == 2 && damage > 0;
}

inline bool IsLocalFriendlyFireDamage(int localEnt, int victimEnt, int victimTeam,
	int attackerEnt, int attackerTeam, int damage) {
	return localEnt > 0 && attackerEnt == localEnt
		&& IsFriendlyFireDamage(victimEnt, victimTeam, attackerEnt, attackerTeam, damage);
}

inline FriendlyFireDamageTier FriendlyFireDamageTierFor(int damage) {
	if (damage <= 0) return kFriendlyFireDamageNone;
	return damage < 15 ? kFriendlyFireDamageLight : kFriendlyFireDamageHeavy;
}

inline int FriendlyFireDamageFontPx(int baseNonCritPx) {
	if (baseNonCritPx <= 0 || baseNonCritPx > 1024) return 0;
	return baseNonCritPx * 2;
}

inline bool DamageSourceHudAccepts(int mode, int localEnt, int localTeam,
	int victimEnt, int /*victimTeam*/, int attackerEnt, int attackerTeam, int damage) {
	if (localEnt <= 0 || victimEnt != localEnt || damage <= 0) return false;
	if (mode == kDamageSourceAll) return true;
	return attackerEnt > 0 && attackerEnt != localEnt && localTeam > 0
		&& attackerTeam == localTeam;
}

// Chapter-stats FF penalty in hundredths of a point.
// mulHundredths is points-per-damage * 100 (default 50 => damage * 0.5).
inline int FriendlyFirePenaltyCents(int totalDamage, int mulHundredths) {
	if (totalDamage <= 0 || mulHundredths <= 0) return 0;
	const long long cents = (long long)totalDamage * (long long)mulHundredths;
	return cents > 2000000000LL ? 2000000000 : (int)cents;
}

} // namespace skeeto_damage
