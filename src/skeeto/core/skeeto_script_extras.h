// Included once from skeeto_proxy.cpp after trace, fade, particle and entity helpers exist.
// Seven client-side SkeetoScript features. Event threads only set atomics; Paint does the work.
#pragma once

static std::atomic<bool> g_parryPending{false};
static std::atomic<int> g_parryShoveUserId{0};
static DWORD g_parryNextAt = 0;
static int g_parryJockeyId = 0;
static DWORD g_parryJockeyUntil = 0;
static bool g_parrySlowActive = false;
static float g_parrySlowSaved = 1.f;
static DWORD g_parrySlowUntil = 0;
static constexpr DWORD kParryEffectMs = 600; // listen-server slow, wall clock

static void ScriptExtrasStopParrySlow() {
	if (!g_parrySlowActive) return;
	CvarSetFloatValue("host_timescale", g_parrySlowSaved);
	g_parrySlowActive = false;
	g_parrySlowUntil = 0;
}

static void ScriptExtrasStartParrySlow() {
	if (!ListenDetectHost()) return;
	void* var = CvarFind("host_timescale");
	if (!var) {
		static bool s_logged = false;
		if (!s_logged) {
			s_logged = true;
			Log("parry slow: host_timescale missing");
		}
		return;
	}
	if (!g_parrySlowActive) {
		int n = 0;
		float saved = 1.f;
		if (!CvarReadPair(var, &n, &saved) || !(saved > 0.f) || saved > 10.f)
			saved = 1.f;
		g_parrySlowSaved = saved;
	}
	CvarSetFloatValue("host_timescale", 0.10f);
	g_parrySlowActive = true;
	g_parrySlowUntil = GetTickCount() + kParryEffectMs;
}

static void ScriptExtrasPaintParrySlow() {
	if (!g_parrySlowActive) return;
	if (!g_optParry || !SkeetoFeaturesOn() || !EngInGame() || EngDrawingLoading()
		|| (LONG)(GetTickCount() - g_parrySlowUntil) >= 0)
		ScriptExtrasStopParrySlow();
}

static bool ScriptExtrasActive() {
	return g_optParry || g_optLadderStep || g_optJockeyLeap
		|| g_parryPending.load(std::memory_order_relaxed)
		|| g_parrySlowActive;
}

static void ScriptExtrasClearParry() {
	g_parryPending.store(false, std::memory_order_release);
	g_parryShoveUserId.store(0, std::memory_order_release);
	g_parryNextAt = 0;
	ScriptExtrasStopParrySlow();
}

static void ScriptExtrasRequestParry(bool enabled) {
	if (!g_optParry || !enabled || !SkeetoFeaturesOn()) return;
	const DWORD now = GetTickCount();
	if (g_parryNextAt && (LONG)(now - g_parryNextAt) < 0) return;
	g_parryNextAt = now + 300;
	g_parryPending.store(true, std::memory_order_release);
}

static void ScriptExtrasOnLocalKill(const char* vn, bool melee, bool airborne,
	bool chargerLevel, bool hunterOrJockey) {
	if (!vn) return;
	if (chargerLevel) ScriptExtrasRequestParry(g_optParryLevel);
	if (hunterOrJockey && airborne)
		ScriptExtrasRequestParry(melee ? g_optParryMeleeSkeet : g_optParrySkeet);
	if (!strcmp(vn, "Boomer")) ScriptExtrasRequestParry(g_optParryBoomer);
}

static void ScriptExtrasOnTongueCut() {
	ScriptExtrasRequestParry(g_optParryTongue);
}

static void ScriptExtrasOnShove(void* ev) {
	if (!g_optParry || !ev || !IsLocalAttacker(ev)) return;
	const int userid = EvInt(ev, "userid");
	if (userid > 0) g_parryShoveUserId.store(userid, std::memory_order_release);
}

static void ScriptExtrasPaintShove() {
	const int userid = g_parryShoveUserId.exchange(0, std::memory_order_acq_rel);
	if (userid <= 0 || !g_optParry) return;
	const int victim = EngPlayerForUserID(userid);
	void* ent = victim > 0 ? EntGet(victim) : nullptr;
	int zombieClass = 0;
	if (!EntRead(ent, g_offZombieClass, &zombieClass)) return;
	if (zombieClass == 3) {
		int flags = 0;
		if (EntRead(ent, g_offFlags, &flags) && (flags & 1) == 0)
			ScriptExtrasRequestParry(g_optParryHunter);
		return;
	}
	if (zombieClass != 5) return;
	const DWORD now = GetTickCount();
	if (g_parryJockeyId == victim && g_parryJockeyUntil && (LONG)(now - g_parryJockeyUntil) < 0)
		return;
	g_parryJockeyId = victim;
	g_parryJockeyUntil = now + 1700;
	ScriptExtrasRequestParry(g_optParryJockey);
}

static void ScriptExtrasOnWitch(bool localKiller) {
	if (localKiller) ScriptExtrasRequestParry(g_optParryWitch);
}

static void ScriptExtrasOnRock(void* ev) {
	if (!ev) return;
	RefreshLocalUserId();
	if (g_localUserId > 0 && EvInt(ev, "userid") == g_localUserId)
		ScriptExtrasRequestParry(g_optParryRock);
}

static int g_scriptMoveTypeOff = -1;
static ClientClass* g_scriptLeapClass = nullptr;
static int g_scriptLeapOff = -1;
static bool g_scriptResolved = false;
static bool g_scriptLeapLogged = false;
static bool g_scriptMoveLogged = false;
static unsigned char g_scriptJockeyWas[33]{};

static void ScriptExtrasResolve() {
	if (g_scriptResolved) return;
	void* client = GetIface("client.dll", "VClient016");
	if (!client) return;
	using GetClasses = ClientClass*(__thiscall*)(void*);
	auto get = (GetClasses)VGet(client, 7);
	if (!get || !IsExec((void*)get)) return;
	g_scriptResolved = true;
	for (auto* cc = get(client); cc; cc = cc->next) {
		if (!cc->name || !cc->table) continue;
		if (g_scriptMoveTypeOff < 0
			&& (!strcmp(cc->name, "CTerrorPlayer") || !strcmp(cc->name, "CBasePlayer")
				|| !strcmp(cc->name, "CBaseEntity"))) {
			int off = FindOffset(cc->table, "m_MoveType");
			if (off <= 0) off = FindOffset(cc->table, "m_nMoveType");
			if (off <= 0) off = FindOffset(cc->table, "movetype");
			// Client recv prop "movetype" is offset 0; the byte lives beside m_nRenderMode.
			if (off <= 0) {
				const int render = FindOffset(cc->table, "m_nRenderMode");
				if (render > 0 && render < 0x4000) off = render + 1;
			}
			if (off > 0 && off <= 0x4000) g_scriptMoveTypeOff = off;
		}
		if (!g_scriptLeapClass && !strcmp(cc->name, "CLeap")) {
			const int leap = FindOffset(cc->table, "m_isLeaping");
			if (leap >= 0) {
				g_scriptLeapClass = cc;
				g_scriptLeapOff = leap;
			}
		}
	}
}

static void ScriptExtrasPaintParry() {
	if (!g_parryPending.exchange(false, std::memory_order_acq_rel)) return;
	if (!g_optParry) return;
	if (g_optParryWhiteFlash)
		ClientUxScreenFadeTrigger(3, 255, 255, 255, 100,
			static_cast<DWORD>(g_parryFlashDurationTenths * 100));
	const char* sample = ParrySoundSample();
	if (sample && sample[0])
		EngEmitLocalSfxBus(sample, 1.f, SfxEmitBus::ClientUx);
	ScriptExtrasStartParrySlow();
}

static void ScriptExtrasPaintLadder() {
	if (!g_optLadderStep) return;
	static DWORD s_next = 0;
	const DWORD now = GetTickCount();
	if (s_next && (LONG)(now - s_next) < 0) return;
	s_next = now + 500;
	ScriptExtrasResolve();
	if (g_scriptMoveTypeOff < 0) {
		if (!g_scriptMoveLogged) {
			g_scriptMoveLogged = true;
			Log("ladder step: m_MoveType not found");
		}
		return;
	}
	const int local = EngLocal();
	void* me = local > 0 ? EntGet(local) : nullptr;
	int team = 0;
	unsigned char life = 1;
	unsigned char moveType = 0;
	float vel[3]{};
	if (!EntRead(me, g_offTeam, &team) || team != 2) return;
	if (!EntRead(me, g_offLifeState, &life) || life != 0) return;
	if (!EntRead(me, g_scriptMoveTypeOff, &moveType) || moveType != 9) return;
	if (!EntReadBlock(me, g_offAbsVelocity, vel, sizeof(vel))) return;
	const float speed2 = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
	if (speed2 <= 25.f) return;
	static const char* kSteps[] = {
		"player/footsteps/survivor/run/ladder1.wav",
		"player/footsteps/survivor/run/ladder2.wav",
		"player/footsteps/survivor/run/ladder3.wav",
		"player/footsteps/survivor/run/ladder4.wav",
	};
	static int s_step = 0;
	const char* sample = kSteps[s_step];
	s_step = (s_step + 1) % 4;
	EngEmitLocalSfxBus(sample, 1.f, SfxEmitBus::ClientUx);
}

static bool ScriptExtrasAbilityLeaping(void* player) {
	if (!g_scriptLeapClass || g_scriptLeapOff < 0 || g_offCustomAbility < 0) return false;
	int handle = 0;
	if (!EntRead(player, g_offCustomAbility, &handle)) return false;
	const int abilityIndex = EntIndexFromHandle(handle);
	if (abilityIndex <= 0 || EntClientClass(abilityIndex) != g_scriptLeapClass) return false;
	unsigned char leaping = 0;
	return EntRead(EntGet(abilityIndex), g_scriptLeapOff, &leaping) && leaping != 0;
}

static void ScriptExtrasPaintJockey() {
	if (!g_optJockeyLeap || !SkeetoFeaturesOn() || !EngInGame() || EngDrawingLoading()) {
		memset(g_scriptJockeyWas, 0, sizeof(g_scriptJockeyWas));
		return;
	}
	static DWORD s_next = 0;
	static DWORD s_sampled = 0;
	const DWORD now = GetTickCount();
	if (s_next && (LONG)(now - s_next) < 0) return;
	s_next = now + 100;
	if (!s_sampled || (LONG)(now - s_sampled) > 500)
		memset(g_scriptJockeyWas, 0, sizeof(g_scriptJockeyWas));
	s_sampled = now;
	ScriptExtrasResolve();
	if (!g_scriptLeapClass) {
		if (!g_scriptLeapLogged) {
			g_scriptLeapLogged = true;
			Log("jockey leap: m_isLeaping not found");
		}
		return;
	}
	static const char* kSamples[] = {
		"player/jockey/voice/idle/jockey_lurk04.wav",
		"player/jockey/voice/idle/jockey_lurk05.wav",
		"player/jockey/voice/idle/jockey_spotprey_02.wav",
		"player/jockey/voice/pain/jockey_pain01.wav",
		"player/jockey/voice/pain/jockey_pain02.wav",
		"player/jockey/voice/pain/jockey_pain03.wav",
		"player/jockey/voice/pain/jockey_pain05.wav",
		"player/jockey/voice/pain/jockey_pain06.wav",
		"player/jockey/voice/pain/jockey_pain08.wav",
		"player/jockey/voice/pain/jockey_painshort02.wav",
		"player/jockey/voice/pain/jockey_painshort03.wav",
		"player/jockey/voice/pain/jockey_painshort04.wav",
	};
	static unsigned s_rng = 0xA341316Cu;
	static int s_last = -1;
	bool played = false;
	for (int i = 1; i <= 32; ++i) {
		void* ent = EntGet(i);
		int zombieClass = 0;
		const bool jockey = EntRead(ent, g_offZombieClass, &zombieClass) && zombieClass == 5;
		const bool leaping = jockey && ScriptExtrasAbilityLeaping(ent);
		if (leaping && !g_scriptJockeyWas[i] && !played) {
			s_rng = s_rng * 1664525u + 1013904223u;
			int pick = (int)((s_rng >> 16) % (sizeof(kSamples) / sizeof(kSamples[0])));
			if (pick == s_last) pick = (pick + 1) % (int)(sizeof(kSamples) / sizeof(kSamples[0]));
			s_last = pick;
			EngEmitLocalSfxBus(kSamples[pick], 0.85f, SfxEmitBus::ClientUx);
			played = true;
		}
		g_scriptJockeyWas[i] = leaping ? 1 : 0;
	}
}

static void ScriptExtrasPaint() {
	if (!SkeetoFeaturesOn() || !g_engine || !EngInGame() || EngDrawingLoading()) {
		g_parryPending.store(false, std::memory_order_release);
		g_parryShoveUserId.store(0, std::memory_order_release);
		ScriptExtrasStopParrySlow();
		memset(g_scriptJockeyWas, 0, sizeof(g_scriptJockeyWas));
		return;
	}
	ScriptExtrasPaintShove();
	ScriptExtrasPaintParry();
	ScriptExtrasPaintParrySlow();
	if (g_optPauseSampling) return;
	ScriptExtrasPaintLadder();
	ScriptExtrasPaintJockey();
}

static int ScriptExtrasRoomBand(const float* eye, void* skip) {
	static int s_cellX = 0, s_cellY = 0, s_cellZ = 0, s_band = 1;
	static DWORD s_at = 0;
	const int cellX = (int)(eye[0] / 256.f);
	const int cellY = (int)(eye[1] / 256.f);
	const int cellZ = (int)(eye[2] / 256.f);
	const DWORD now = GetTickCount();
	if (s_at && cellX == s_cellX && cellY == s_cellY && cellZ == s_cellZ
		&& now - s_at < 2000u)
		return s_band;
	static const float kDir[9][3] = {
		{0.f, 0.f, 1.f}, {1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f},
		{0.57735f, 0.57735f, 0.57735f}, {-0.57735f, 0.57735f, 0.57735f},
		{0.57735f, -0.57735f, 0.57735f}, {-0.57735f, -0.57735f, 0.57735f},
	};
	float dist[9]{};
	for (int i = 0; i < 9; ++i) {
		float end[3] = {
			eye[0] + kDir[i][0] * 2000.f,
			eye[1] + kDir[i][1] * 2000.f,
			eye[2] + kDir[i][2] * 2000.f,
		};
		TraceResultPod hit{};
		dist[i] = 2000.f;
		if (EngTraceLine(eye, end, 0x0200400Bu, skip, &hit))
			dist[i] = hit.fraction * 2000.f;
	}
	for (int i = 1; i < 9; ++i) {
		const float v = dist[i];
		int j = i;
		while (j > 0 && dist[j - 1] > v) {
			dist[j] = dist[j - 1];
			--j;
		}
		dist[j] = v;
	}
	const float avg = (dist[2] + dist[3] + dist[4] + dist[5] + dist[6]) / 5.f;
	s_band = avg < 100.f ? 0 : avg < 300.f ? 1 : avg < 500.f ? 2 : 3;
	s_cellX = cellX; s_cellY = cellY; s_cellZ = cellZ; s_at = now;
	return s_band;
}

static void ScriptExtrasOnLocalShot() {
	if (!g_optGunReverb || g_optClientDspLock || g_optPauseSampling) return;
	if (!SkeetoFeaturesOn() || !EngInGame() || EngDrawingLoading()) return;
	float eye[3]{};
	if (!LocalEyePos(eye)) return;
	const int local = EngLocal();
	void* me = local > 0 ? EntGet(local) : nullptr;
	static const char* kRooms[] = {
		"skeeto/reverb/room1.wav",
		"skeeto/reverb/room2.wav",
		"skeeto/reverb/room3.wav",
		"skeeto/reverb/room4.wav",
	};
	const int band = ScriptExtrasRoomBand(eye, me);
	EngEmitLocalSfxBus(kRooms[band], 0.85f, SfxEmitBus::ClientUx);
}
