"""Execute production feedback policy/queues with controlled x86 providers.

This validates attribution and bounded dispatch, not client hitboxes or FPS.
SKEETO_FEEDBACK_SOURCE optionally selects a pre-change source for a red run.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

PRELUDE = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "skeeto_style.h"
#include "skeeto_damage_events.h"
static bool g_optSiVisual = false, g_optSiSound = false;
'''

PROVIDERS = r'''
#include <cmath>
static DWORD tickNow = 1000;
#define GetTickCount() tickNow
static bool enabled = true, lmb = false;
static bool g_optPauseSampling = false;
static short FakeKey(int) { return lmb ? (short)0x8000 : 0; }
#define GetAsyncKeyState FakeKey
static void Log(const char*, ...) {}
static bool SkeetoFeaturesOn() { return enabled; }
static std::atomic<bool> g_ready{true}, g_hitDetectArmed{true};
static std::atomic<bool> g_paintInGame{true}, g_paintLoading{false}, g_paintGameUiVisible{false};
static std::atomic<int> g_dbgLocalEnt{1}, g_dbgLifeState{0}, g_dbgLocalTeam{2}, g_dbgObsMode{0};
static std::atomic<DWORD> g_meleeAttackUntil{0}, g_lastLocalGunImpactAt{0};
static bool g_optFf = true, g_optClientInfectedHp = true, g_optClientDmgNum = true;
static bool hitAllowed = true, airWanted = false;
static bool HitModeSiAllowed() { return hitAllowed; }
static bool NeedHealthProxyWork() { return hitAllowed || airWanted; }
static bool WorldNoticeAirborneTrackingWanted() { return airWanted; }
static bool g_optSound = true, g_optIcon = true;
static bool g_menuVisible = false, g_menuParked = false;
static DWORD g_paintTid = 0, g_lastHitFeedbackAt = 0;
static void* g_engine = (void*)1;
static bool EngInGame() { return true; }
static bool EngDrawingLoading() { return false; }
static bool g_optWorldFriendlyFire = false, g_optDamageSourceHud = false, g_optChapStats = false;
static bool g_optWorldFriendlyFireDmgNum = false, g_optFriendlyFireHud = false;
static int g_damageSourceHudMode = 0;
static unsigned ffStatEvents = 0, worldFfEvents = 0, damageSourceEvents = 0;
static unsigned ffTakenStatEvents = 0, friendlyDamageNumberEvents = 0, friendlyFireHudEvents = 0;
static int lastFfStatUid = 0, lastFfStatDamage = 0, lastWorldAttacker = 0;
static int lastFfTakenUid = 0, lastFfTakenDamage = 0, lastFriendlyDamageVictim = 0, lastFriendlyDamageNumber = 0;
static int lastFriendlyHudVictim = 0, lastFriendlyHudDamage = 0;
static int lastDamageSourceAttacker = 0, lastDamageSourceDamage = 0;
static void ChapStatsBumpDamage(int uid, int damage) { ++ffStatEvents; lastFfStatUid=uid; lastFfStatDamage=damage; }
static void ChapStatsBumpTakenDamage(int uid, int damage) { ++ffTakenStatEvents; lastFfTakenUid=uid; lastFfTakenDamage=damage; }
static void WorldNoticeSetFriendlyFire(int attacker) { ++worldFfEvents; lastWorldAttacker=attacker; }
static void ClientUxDmgOnFriendlyFire(int victim, int damage) { ++friendlyDamageNumberEvents; lastFriendlyDamageVictim=victim; lastFriendlyDamageNumber=damage; }
static void FriendlyFireHudOnEvent(int victim, int damage) { ++friendlyFireHudEvents; lastFriendlyHudVictim=victim; lastFriendlyHudDamage=damage; }
static void DamageSourceHudOnEvent(int attacker, int damage) { ++damageSourceEvents; lastDamageSourceAttacker=attacker; lastDamageSourceDamage=damage; }
static void DamageSourceHudClear() {}
static void FriendlyFireHudClear() {}
enum { TW_None, TW_Melee, TW_Saw, TW_Gun };
struct Entity { int health, team; unsigned char incap; float origin[3]; int weapon, flags, zombieClass; };
static Entity pawn{100,2,0,{0,0,0},100}, siPawn{100,3,0,{200,0,0},0}, ffPawn{100,2,0,{200,0,0},0};
static void* slots[65] = {nullptr, &pawn, &siPawn, &ffPawn};
static int g_offHealth = offsetof(Entity,health), g_offTeam = offsetof(Entity,team);
static int g_offOrigin = offsetof(Entity,origin), g_offActiveWeapon = offsetof(Entity,weapon);
static int g_offIsIncapacitated = offsetof(Entity,incap), g_offZombieClass = -1;
static int g_offFlags = offsetof(Entity,flags);
static DWORD g_localAirHitAt[65]{};
static bool hunterLunging = false;
static bool WorldNoticeReadAttackLeap(void*,int zc) { return zc==3 && hunterLunging; }
static const char* weaponClass = "gun";
static unsigned classQueries = 0, siHits = 0, ffHits = 0, bars = 0, damages = 0, airSamples = 0;
static unsigned entityQueries = 0;
static void* EntGet(int idx) { ++entityQueries; return idx > 0 && idx < 65 ? slots[idx] : nullptr; }
static bool EntReadable(void* p) { return p != nullptr; }
static bool PtrCommitted(const void* p) { return p != nullptr; }
static int EntIndexFromHandle(int h) { return h & 0xFFF; }
static const char* EntNetClassName(int) { ++classQueries; return weaponClass; }
static int TeamHudWepKind(const char* s) { return !strcmp(s,"melee") ? TW_Melee : !strcmp(s,"saw") ? TW_Saw : TW_Gun; }
static int EngLocal() { return g_dbgLocalEnt.load(); }
static unsigned eyeSamples=0, viewSamples=0;
static bool eyeOk=true;
static bool LocalEyePos(float* p) { ++eyeSamples; p[0]=p[1]=p[2]=0; return eyeOk; }
static void EngGetViewAngles(float* p) { ++viewSamples; p[0]=p[1]=p[2]=0; }
struct PlayerInfo { int userid; };
static bool EngPlayerInfo(int idx, PlayerInfo* p) { p->userid=idx+100; return true; }
static int EngPlayerForUserID(int userid) { return userid > 100 && userid <= 164 ? userid - 100 : 0; }
static int g_hpLocalSiEnt = 0;
static void* g_hpLocalSiEntity = nullptr;
static unsigned g_hpLocalSiGeneration = 0;
static DWORD g_hpLocalSiUntil = 0;
static constexpr int kSiHpLimit = 1000000;
static std::atomic_int g_hpLifeUid[65]{};
static std::atomic_uint g_hpLifeSpawn[65]{};
static std::atomic_bool g_hpLifeDead[65]{};
// The HP-panel provider owns these lives; mirror its boundary reset while the
// queue and attribution implementations below remain production code.
static void LocalPlayResetHpLives() {
    for(int i=1;i<=64;++i) { g_hpLifeUid[i]=0; g_hpLifeSpawn[i]=0; g_hpLifeDead[i]=false; }
    g_hpLocalSiEnt=0; g_hpLocalSiEntity=nullptr; g_hpLocalSiGeneration=0; g_hpLocalSiUntil=0;
}
static int g_offCheckpointDamageToTank = -1;
static int g_ckptWitchCachedId = 0, g_ckptTankCachedId = 0;
static bool g_elimCkptHaveSample = false;
static void CheckpointDmgReset() {}
static void SiAggroHudClear() {}
static void WitchRageHudClear() {}
static void WitchHpHudClear() {}
static void BossDmgBoardClear() {}
static void IgniteHudClear() {}
static void TeamHudResetRuntimeState() {}
static int g_hitDetectLogLeft = 0, g_feedbackLogLeft = 0;
static void LocalPlayNotifySiHp(void*,int,int,int) { ++bars; }
static void ClientUxDmgOnSi(void*,int,int,int) { if(g_optClientDmgNum) ++damages; }
static void ClientUxDmgConfirmDeath(int) {}
static DWORD g_meleeKillAt = 0;
static int g_meleeKillUid = 0;
static void BloodSplatNoteHit() {}
static void FeedbackSiHit(float,float,float);
static void FeedbackFf(int,float,float,float) { ++ffHits; }
struct TraceResultPod { void* ent; };
static constexpr unsigned kMaskShot = 0x46004003u;
static void* traceHit = &siPawn;
static unsigned traceCalls = 0;
static bool traceOk = true, boundaryInTrace = false;
static float traceEnd[3]{};
static void* traceSkip = nullptr;
static void ClearFeedbackQueuesOnMapBoundary();
static bool EngTraceLine(const float*,const float* end,unsigned mask,void* skip,TraceResultPod* out) {
    ++traceCalls; traceSkip=skip; memcpy(traceEnd,end,sizeof(traceEnd));
    if(boundaryInTrace) { boundaryInTrace=false; ClearFeedbackQueuesOnMapBoundary(); }
    out->ent=traceHit; return traceOk && mask==kMaskShot;
}
void WorldNoticeTrackAirborneEntity(void*,int);
struct RecvProxyDataPod { const void* prop; int valueInt, pad0[2], type, element, objectId; };
using Proxy = void(*)(const void*,void*,void*);
static void OrigProxy(const void* p,void*,void* out) { if(out) *(int*)out=((const RecvProxyDataPod*)p)->valueInt; }
static Proxy g_origHealthProxy = OrigProxy;
static bool g_pendingCiImpactScan = false, g_pendingCommonHit = false;
static DWORD g_pendingCommonHitAt = 0, g_overlayClearAt = 0;
static std::atomic<bool> g_overlayBoundaryClearPending{false}, g_run{true};
static unsigned overlays = 0, sounds = 0, particles = 0;
static bool boundaryInParticle = false, enqueueInParticle = false, reenterInParticle = false;
static int lastOverlayPriority = -1;
static DWORD lastOverlayMs = 0;
static void ClearFeedbackQueuesOnMapBoundary();
static void EnqueueFxJob(const struct FxJob&);
static void PumpGameThreadFeedback();
static bool StartFeedbackOverlay(const DlcFx* fx,DWORD,DWORD) { ++overlays; lastOverlayPriority=fx->priority; lastOverlayMs=fx->overlayMs; return true; }
static void PlayHitSound(const char*) { ++sounds; }
static void FireParticle(const char*,float,float,float,bool);
static void FireFX(const char*,float,float,float) { ++particles; }
static constexpr DWORD kOverlayClearMs = 500;
static void ApplyDlcFx(const DlcFx*,bool,float,float,float,DWORD=kOverlayClearMs,bool=false,bool=true);
static void DrainHpDropQueue();
static void DrainDamageEventQueue(bool allowChapterSettle=false);
static void RunDeferredCiTrace() {}
static void FlushPendingCommonHit() {}
static void PumpFeedbackOverlaySequence(DWORD) {}
static void ClearHitOverlay() { g_overlayClearAt=0; }
static constexpr int kBcHit = 1;
static void CrashMark(int) {}
static DlcFx siStyle{}, ciStyle{};
static unsigned ciResolves = 0;
static bool resolvedMelee = false;
static bool SiChannelOn() { return true; }
static bool CiChannelOn() { return true; }
bool DlcResolve(const char* channel,const char*,bool,bool melee,int,DlcFx* out) {
    resolvedMelee=melee;
    if(!strcmp(channel,"si")) { ++siHits; *out=siStyle; }
    else { ++ciResolves; *out=ciStyle; }
    return out->used;
}
const char* DlcGetSelected(const char*) { return "fixture"; }
static constexpr DWORD kOverlayClearMsHit = 200;
static int g_siStreak = 0, g_feedbackStreakWindowSec = 3;
static DWORD g_siStreakLastKillAt = 0, g_suppressCommonHitUntil = 0;
static bool g_haveImpactPos = false;
static float g_lastImpactPos[3]{};
static int FeedbackStreakMax(const char*) { return 10; }
static const char* eventWeapon = "";
static const char* EvStr(void*,const char*) { return eventWeapon; }
static bool EvBool(void*,const char*) { return false; }
struct TestEvent { int userid, attackerentid, dmg_health; };
static int EvInt(void* ev,const char* key,int def=0) {
    const TestEvent* e=(const TestEvent*)ev;
    if(!e||!key) return def;
    if(!strcmp(key,"userid")) return e->userid;
    if(!strcmp(key,"attackerentid")) return e->attackerentid;
    if(!strcmp(key,"dmg_health")) return e->dmg_health;
    return def;
}
static float EvFloat(void*,const char*) { return 0; }
'''

QUEUE_CASES = r'''
static FxJob Kill(int priority) {
    FxJob j{}; j.kind=1; j.isKill=true; j.epoch=g_feedbackQueueEpoch.load();
    j.fx.used=true; j.fx.priority=priority;
    strcpy(j.fx.particle,"particle"); strcpy(j.fx.sound,"sound"); return j;
}
static void FireParticle(const char*,float,float,float,bool) {
    ++particles;
    if(boundaryInParticle) { boundaryInParticle=false; ClearFeedbackQueuesOnMapBoundary(); }
    if(enqueueInParticle) { enqueueInParticle=false; EnqueueFxJob(Kill(99)); }
    if(reenterInParticle) { reenterInParticle=false; PumpGameThreadFeedback(); }
}
static void Drop(int id,int hp) {
    RecvProxyDataPod p{}; p.objectId=id; p.valueInt=hp;
    HookedHealthProxy(&p,slots[id], &((Entity*)slots[id])->health);
}
static bool ExtraCase(int test) {
    bool ok=true;
    // Attribution cases count dedicated SI resolves. Explicitly select those
    // sources instead of depending on an implicit CI-to-SI fallback.
    g_optSiVisual=g_optSiSound=true;
    if(test==4) {
        EnqueueFxJob(Kill(1)); EnqueueFxJob(Kill(9)); EnqueueFxJob(Kill(2)); PumpGameThreadFeedback();
        ok &= Check(overlays==1 && sounds==1 && particles==3 && lastOverlayPriority==9,"multi-kill selects one media winner and keeps all particles");
    } else if(test==5) {
        FxJob first=Kill(7), last=Kill(7); last.fx.overlayMs=123;
        EnqueueFxJob(first); EnqueueFxJob(last); PumpGameThreadFeedback();
        ok &= Check(overlays==1 && sounds==1 && particles==2 && lastOverlayMs==123,"equal-priority kills select the last job");
    } else if(test==6) {
        FxJob ff=Kill(100); ff.isKill=false; ff.throttle=true;
        EnqueueFxJob(ff); EnqueueFxJob(Kill(1)); PumpGameThreadFeedback();
        ok &= Check(overlays==2 && sounds==2 && particles==2,"heavy FF style must not be treated as a kill");
    } else if(test==7) {
        EnqueueFxJob(Kill(1)); EnqueueFxJob(Kill(2)); boundaryInParticle=true; PumpGameThreadFeedback();
        ok &= Check(particles==1 && overlays==0,"map boundary cancels remaining detached old-map jobs");
    } else if(test==8) {
        EnqueueFxJob(Kill(1)); enqueueInParticle=reenterInParticle=true; PumpGameThreadFeedback();
        ok &= Check(particles==1 && g_fxJobN==1 && !g_inFxPump,"reentrant Paint cannot recycle active buffer; producers retain next batch");
        tickNow+=100; PumpGameThreadFeedback();
        ok &= Check(particles==2 && lastOverlayPriority==99,"next buffer drains on subsequent Paint");
    } else if(test==9) {
        for(int i=0;i<100;++i) EnqueueFxJob(Kill(i)); PumpGameThreadFeedback();
        ok &= Check(particles==24 && sounds==1 && !g_fxJobsPending,"burst dispatch remains bounded by fixed queue capacity");
    } else if(test==10) {
        g_inFxPump=true; DlcFx fx=Kill(1).fx;
        std::thread producer([&]{ ApplyDlcFx(&fx,false,0,0,0); }); producer.join();
        ok &= Check(particles==0 && g_fxJobN==1,"concurrent event must queue, not invoke Source effects");
        g_inFxPump=false; PumpGameThreadFeedback(); ok &= Check(particles==1,"queued concurrent work runs on Paint");
    } else if(test==11) {
        g_hpLocalSiEnt=2; g_hpLocalSiEntity=&siPawn; g_hpLocalSiUntil=1200;
        HpAimCache cache{};
        OnSiHealthDecreased(&siPawn,2,100,90,false,cache);
        ok &= Check(bars==1 && g_hpLocalSiUntil==1200,"locked follow-up does not renew attribution");
        tickNow=1201; siPawn.origin[1]=400;
        OnSiHealthDecreased(&siPawn,2,90,80,false,cache);
        ok &= Check(bars==1,"expired lock requires fresh aim confirmation");
    } else if(test==12) {
        g_lastLocalGunImpactAt=1000; g_dbgLocalTeam=3; Drop(2,90);
        ok &= Check(g_hpDropN==0 && siPawn.health==90,"infected local skips feedback and preserves original recv update");
    } else if(test==13) {
        g_lastLocalGunImpactAt=1000; g_dbgLifeState=1; Drop(2,90);
        g_dbgLifeState=0; g_dbgObsMode=4; siPawn.health=100; Drop(2,90);
        ok &= Check(g_hpDropN==0,"dead and observing local cannot attribute HP drops");
    } else if(test==14) {
        g_lastLocalGunImpactAt=1000; Drop(1,90);
        ok &= Check(g_hpDropN==0 && pawn.health==90,"own health loss is not FF");
    } else if(test==15) {
        g_lastLocalGunImpactAt=1000; Drop(2,90);
        Entity replacement=siPawn; slots[2]=&replacement; DrainHpDropQueue();
        ok &= Check(siHits==0 && bars==0,"different entity in same slot rejects stale HP work");
    } else if(test==16) {
        g_lastLocalGunImpactAt=1000; Drop(2,90); Drop(2,100); DrainHpDropQueue();
        ok &= Check(siHits==0 && bars==0,"health reset at same address invalidates prior-life drop");
    } else if(test==17) {
        g_lastLocalGunImpactAt=1000; Drop(2,90); g_dbgObsMode=4; DrainHpDropQueue();
        ok &= Check(siHits==0 && g_hpDropN==0,"local mode is rechecked after enqueue");
    } else if(test==18) {
        g_lastLocalGunImpactAt=1000; Drop(2,90); g_optClientInfectedHp=g_optClientDmgNum=hitAllowed=false;
        DrainHpDropQueue(); ok &= Check(siHits==0 && bars==0 && damages==0,"closing SI consumers discards already queued SI work");
    } else if(test==19) {
        lmb=true; weaponClass="gun"; Sample(); ok &= Check(g_meleeAttackUntil==0,"gun trigger does not arm melee");
        lmb=false; Sample(); weaponClass="melee"; ++pawn.weapon; Sample();
        lmb=true; Sample(); ok &= Check(g_meleeAttackUntil==1400,"melee edge arms a short window");
        tickNow=1100; Sample(); ok &= Check(g_meleeAttackUntil==1400 && classQueries==2,"holding melee does not renew swing or reclassify stable handle");
    } else if(test==20) {
        lmb=true; weaponClass="saw"; Sample(); tickNow=1500; Sample();
        ok &= Check(g_meleeAttackUntil==1900,"held chainsaw renews its attack window");
        weaponClass="gun"; ++pawn.weapon; Sample(); ok &= Check(g_meleeAttackUntil==0,"switch to gun clears stale melee window");
    } else if(test==21) {
        lmb=true; weaponClass="melee"; g_paintGameUiVisible=true; Sample();
        ok &= Check(g_meleeAttackUntil==0,"menu click cannot arm melee");
        g_meleeAttackUntil=1400; g_dbgObsMode=4; Sample();
        ok &= Check(g_meleeAttackUntil==0,"entering observation clears melee state");
    } else if(test==22) {
        g_lastLocalGunImpactAt=1000; Drop(2,90); ClearFeedbackQueuesOnMapBoundary();
        DrainHpDropQueue(); ok &= Check(siHits==0 && !g_hpDropsPending,"map boundary cancels HP work");
        HpDropJob stale{}; stale.epoch=0; EnqueueHpDrop(stale);
        ok &= Check(g_hpDropN==0,"old producer cannot publish after boundary cancellation");
    } else if(test==23) {
        enabled=false; EnqueueFxJob(Kill(1)); PumpGameThreadFeedback();
        ok &= Check(particles==0 && sounds==0 && !g_fxJobsPending,"global disable drains without effects or repeated pending work");
    } else if(test==24) {
        TestEvent e{103,1,10}; EnqueueDamageEvent(&e); g_optFf=false;
        DrainDamageEventQueue();
        ok &= Check(ffHits==0 && !g_damageEventsPending,"closing FF discards queued event work");
    } else if(test==25) {
        airWanted=true; g_offZombieClass=offsetof(Entity,zombieClass); siPawn.zombieClass=3;
        WorldNoticeTrackAirborneEntity(&siPawn,2);
        ok &= Check(g_localAirHitAt[2]==0,"ordinary Hunter jump is not attack airborne");
        hunterLunging=true; WorldNoticeTrackAirborneEntity(&siPawn,2);
        ok &= Check(g_localAirHitAt[2]==1000,"Hunter lunge records a recent airborne hit");
        siPawn.flags=1; WorldNoticeTrackAirborneEntity(&siPawn,2);
        ok &= Check(g_localAirHitAt[2]==0,"grounded target clears airborne state");
        siPawn.flags=0; siPawn.zombieClass=5; hunterLunging=false;
        WorldNoticeTrackAirborneEntity(&siPawn,2);
        ok &= Check(g_localAirHitAt[2]==1000,"Jockey preserves its ground-flag rule");
    } else if(test==26) {
        hitAllowed=g_optClientInfectedHp=g_optClientDmgNum=g_optFf=false; airWanted=true;
        g_offZombieClass=offsetof(Entity,zombieClass); siPawn.zombieClass=5;
        g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(g_localAirHitAt[2]==1000 && bars==0 && damages==0 && siHits==0,"world tracking uses attributed HP without enabling feedback or bars");
    } else if(test==27) {
        siStyle.used=true; strcpy(siStyle.soundExtra[0],"si_pool"); siStyle.soundExtraCount=1;
        g_optSiVisual=g_optSiSound=true; g_inFxPump=true; FeedbackSiHit(0,0,0);
        ok &= Check(siHits==1 && ciResolves==0 && sounds==1,"pool-only SI hit plays and does not resolve unused CI fallback");
    } else if(test==28) {
        siStyle.used=ciStyle.used=true; strcpy(ciStyle.sound,"fallback");
        g_optSiVisual=g_optSiSound=true; g_inFxPump=true; FeedbackSiHit(0,0,0);
        ok &= Check(siHits==1 && ciResolves==0 && sounds==0,"silent SI selection remains silent without resolving CI audio");
    } else if(test==29) {
        siStyle.used=ciStyle.used=true; strcpy(siStyle.sound,"si"); strcpy(ciStyle.sound,"ci");
        g_optSiSound=true; g_optSiVisual=false; g_inFxPump=true; FeedbackSiHit(0,0,0);
        ok &= Check(ciResolves==1 && sounds==1,"CI visual selection retains its resolve even with SI audio");
    } else if(test==30) {
        siStyle.used=true; strcpy(siStyle.sound,"si"); strcpy(siStyle.particle,"pt");
        g_optSiVisual=g_optSiSound=true;
        FeedbackSiKill((void*)1); FeedbackSiKill((void*)1); PumpGameThreadFeedback();
        ok &= Check(g_siStreak==2 && ciResolves==0 && sounds==1 && particles==2,"actual kill entry keeps streaks while coalescing media and skipping unused CI resolve");
    } else if(test==31) {
        siStyle.used=true; strcpy(siStyle.sound,"si"); g_optSiVisual=g_optSiSound=true;
        weaponClass="melee"; FeedbackSiKill((void*)1);
        ok &= Check(resolvedMelee,"empty weapon field uses the existing active-melee fallback");
        eventWeapon="grenade_launcher"; FeedbackSiKill((void*)1);
        ok &= Check(!resolvedMelee,"named gun/explosive does not borrow active-melee classification");
    } else if(test==51) {
        // The second chosen visual event contains only audio, which is stripped
        // because this configuration explicitly takes audio from unavailable CI.
        g_optSiVisual=true; g_optSiSound=false;
        siStyle.used=true; siStyle.priority=10; strcpy(siStyle.overlay,"si_overlay");
        FeedbackSiKill((void*)1);
        siStyle={}; siStyle.used=true; siStyle.priority=30; strcpy(siStyle.sound,"si_audio");
        FeedbackSiKill((void*)1);
        ok &= Check(g_siStreak==2 && g_fxJobN==1,"empty final SI combination must not enqueue a media contender, while both kills advance the streak");
        PumpGameThreadFeedback();
        ok &= Check(overlays==1 && sounds==0 && lastOverlayPriority==10,"empty high-priority SI combination cannot suppress the valid same-batch overlay");
    } else if(test==52) {
        g_optSiVisual=true; g_optSiSound=false;
        ciStyle.used=true; ciStyle.priority=9; strcpy(ciStyle.sound,"ci_played");
        siStyle.used=true; siStyle.priority=10; strcpy(siStyle.overlay,"si_overlay");
        FeedbackSiKill((void*)1);
        siStyle={}; siStyle.used=true; siStyle.priority=30;
        strcpy(siStyle.soundExtra[0],"si_unused"); siStyle.soundExtraCount=1;
        FeedbackSiKill((void*)1);
        ok &= Check(g_siStreak==2 && g_fxJobN==2,"both nonempty composed kills remain queued and counted");
        PumpGameThreadFeedback();
        ok &= Check(overlays==1 && sounds==1 && lastOverlayPriority==10,"actual low-priority audio cannot borrow discarded visual-event priority to suppress the valid overlay");
    } else if(test==32) {
        g_lastLocalGunImpactAt=1000; g_dbgLifeState=0; g_dbgObsMode=0;
        // Incapacitated still means a living survivor for SI attribution.
        // FF now follows explicit player_hurt_concise events, not HP changes.
        pawn.incap=1; Drop(2,90); DrainHpDropQueue();
        ok &= Check(bars==1,"living incapacitated survivor remains eligible for real pistol hits");
    } else if(test==33) {
        g_lastLocalGunImpactAt=1000; Drop(2,90); tickNow=1501; DrainHpDropQueue();
        ok &= Check(bars==0 && siHits==0,"delayed HP work expires instead of showing stale feedback");
    } else if(test==34) {
        tickNow=0xFFFFFFF0u; g_meleeAttackUntil=0x80u; Drop(2,90);
        ok &= Check(g_hpDropN==1,"melee deadline survives DWORD tick wrap");
        tickNow=0x81u; siPawn.health=100; Drop(2,90);
        ok &= Check(g_hpDropN==1,"wrapped melee window expires normally");
    } else if(test==35) {
        for(int i=0;i<1000;++i) Sample();
        ok &= Check(entityQueries==0 && classQueries==0,"idle attack input does not query entities or weapon classes");
        weaponClass="melee"; lmb=true; Sample(); lmb=false; tickNow=1401; Sample();
        const unsigned before=entityQueries;
        for(int i=0;i<1000;++i) Sample();
        ok &= Check(g_meleeAttackUntil==0 && entityQueries==before,"expired melee window returns to the idle cheap path");
    } else if(test==36) {
        traceHit=&ffPawn; g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(traceCalls==0 && bars==1 && damages==1,"SI cone accepts aligned damage without tracing");
    } else if(test==37) {
        ffPawn.origin[0]=0; ffPawn.origin[1]=200;
        TestEvent e{103,1,10}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(traceCalls==0 && ffHits==1,"direct friendly-damage event does not depend on inferred aim cone");
    } else if(test==38) {
        siPawn.origin[0]=0; siPawn.origin[1]=200;
        g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(traceCalls==0 && bars==0 && damages==0,"SI outside cone rejects both world consumers");
    } else if(test==39) {
        g_hpLocalSiEnt=2; g_hpLocalSiEntity=&siPawn; g_hpLocalSiUntil=1400;
        siPawn.origin[0]=0; siPawn.origin[1]=200;
        g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(traceCalls==0 && bars==0 && damages==0,"locked SI gun follow-up cannot bypass the aim cone");
    } else if(test==40 || test==41) {
        g_lastLocalGunImpactAt=1000; eyeOk=test==40;
        for(int i=0;i<32;++i) Drop(2,99-i);
        DrainHpDropQueue();
        ok &= Check(traceCalls==0 && eyeSamples==1 && viewSamples==(test==40?1u:0u)
            && bars==(test==40?32u:0u),"a full HP batch shares one view sample, including failure results");
    } else if(test==42) {
        siPawn.origin[0]=4000; ffPawn.origin[0]=4000;
        g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        TestEvent e{103,1,10}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(bars==1 && ffHits==1 && traceCalls==0 && eyeSamples==1,"direct FF event is independent of SI aim/range attribution");
    } else if(test==46) {
        g_optChapStats=g_optWorldFriendlyFire=g_optDamageSourceHud=true;
        TestEvent e{101,3,12}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(ffStatEvents==1 && lastFfStatUid==103 && lastFfStatDamage==12
            && worldFfEvents==1 && lastWorldAttacker==3
            && damageSourceEvents==1 && lastDamageSourceAttacker==3 && lastDamageSourceDamage==12,
            "local victim FF event reaches per-attacker stats, world warning, and teammate damage HUD");
    } else if(test==47) {
        g_optDamageSourceHud=true; g_damageSourceHudMode=1;
        TestEvent e{101,0,8}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(damageSourceEvents==1 && lastDamageSourceAttacker==0 && lastDamageSourceDamage==8,
            "all-source damage HUD includes world/environment damage");
    } else if(test==48) {
        g_optDamageSourceHud=true; g_damageSourceHudMode=0;
        TestEvent e{101,2,8}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(damageSourceEvents==0,"teammate-only damage HUD rejects infected attackers");
    } else if(test==49) {
        g_optDamageSourceHud=true; g_damageSourceHudMode=1;
        TestEvent e{101,2,8}; EnqueueDamageEvent(&e); DrainDamageEventQueue();
        ok &= Check(damageSourceEvents==1 && lastDamageSourceAttacker==2,
            "all-source damage HUD includes infected attackers");
    } else if(test==50) {
        g_optChapStats=true;g_optWorldFriendlyFireDmgNum=true;g_optFriendlyFireHud=true;
        TestEvent e{103,1,15};EnqueueDamageEvent(&e);DrainDamageEventQueue();
        ok &= Check(ffStatEvents==1 && lastFfStatUid==101 && lastFfStatDamage==15
            && ffTakenStatEvents==1 && lastFfTakenUid==103 && lastFfTakenDamage==15,
            "friendly-fire ledger credits attacker damage and victim received damage by userid");
        ok &= Check(friendlyDamageNumberEvents==1 && lastFriendlyDamageVictim==3
            && lastFriendlyDamageNumber==15 && friendlyFireHudEvents==1
            && lastFriendlyHudVictim==3 && lastFriendlyHudDamage==15,
            "local attacker event reaches both independent teammate damage displays");
    } else if(test==43) {
        siPawn.origin[0]=100; g_meleeAttackUntil=1400; Drop(2,90); DrainHpDropQueue();
        ok &= Check(traceCalls==0 && bars==1,"melee retains short-range confirmation without any bullet trace");
    } else if(test==44) {
        siPawn.origin[0]=NAN; g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(traceCalls==0 && bars==0,"non-finite target coordinates cannot trigger world feedback");
    } else if(test==45) {
        siPawn.origin[0]=200; siPawn.origin[1]=80;
        g_lastLocalGunImpactAt=1000; Drop(2,90); DrainHpDropQueue();
        ok &= Check(bars==0 && damages==0 && traceCalls==0,"gun cone retains the original 0.93 cosine threshold");
    } else return false;
    return ok;
}
'''

CASES = r'''
static bool Check(bool ok, const char* label) { if (!ok) puts(label); return ok; }
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const int test = atoi(argv[1]); bool ok = true;
    DlcFx si{}, ci{}, out{}; si.used = ci.used = true;
    strcpy(si.particle, "si_visual"); strcpy(ci.particle, "ci_visual");
    strcpy(si.soundExtra[0], "si_sound"); si.soundExtraCount = 1;
    strcpy(ci.soundExtra[0], "ci_sound"); ci.soundExtraCount = 1;
    if (test == 0) {
        g_optSiVisual = true; g_optSiSound = false;
        AssembleSiFx(&si, true, &ci, true, &out);
        ok &= Check(!strcmp(out.particle, "si_visual"), "SI visual selection is retained");
        ok &= Check(!strcmp(out.soundExtra[0], "ci_sound"), "sound pool must follow sound selection");
    } else if (test == 1) {
        g_optSiVisual = false; g_optSiSound = true;
        AssembleSiFx(&si, true, &ci, true, &out);
        ok &= Check(!strcmp(out.particle, "ci_visual"), "CI visual selection is retained");
        ok &= Check(!strcmp(out.soundExtra[0], "si_sound"), "SI sound pool is independent of visual selection");
    } else if (test == 2) {
        strcpy(ci.sound, "ci_primary"); g_optSiSound = true;
        AssembleSiFx(&si, true, &ci, true, &out);
        ok &= Check(!out.sound[0] && out.soundExtraCount == 1, "selected SI random pool must not acquire CI primary sound");
    } else if (test == 3) {
        g_inFxPump = true; bool otherThreadDirect = true;
        std::thread other([&] { otherThreadDirect = g_inFxPump; }); other.join();
        ok &= Check(!otherThreadDirect && g_inFxPump, "a concurrent producer must not borrow Paint's FX execution permission");
    }
#ifdef TEST_QUEUES
    else return ExtraCase(test) ? 0 : 1;
#else
    else return 2;
#endif
    return ok ? 0 : 1;
}
'''


class FeedbackDispatch(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source_path = Path(os.environ.get('SKEETO_FEEDBACK_SOURCE',
                                         ROOT / 'src/skeeto/core/skeeto_proxy.cpp'))
        source = source_path.read_text(encoding='utf-8-sig')
        extracted = re.search(r'^static [^;\n]+\bg_inFxPump\s*=\s*false;', source, re.M).group() + '\n'
        extracted += block(source, r'^static void AssembleSiFx\(') + '\n'
        cls.queues = 'struct HpAimCache {' in source
        if cls.queues:
            hud = (ROOT / 'src/skeeto/core/skeeto_hud.cpp').read_text(encoding='utf-8-sig')
            extracted += block(hud, r'^void WorldNoticeTrackAirborneEntity\(') + '\n'
            style = (ROOT / 'src/skeeto/core/skeeto_style.cpp').read_text(encoding='utf-8-sig')
            extracted += block(style, r'^const char\* DlcPickSound\(') + '\n'
            section = source[source.index('struct HpAimCache {'):source.index('static bool HookHealthProp(')]
            extracted += section
            section = source[source.index('enum { kFxJobMax'):source.index('// Game-thread pump: CI TraceRay')]
            # g_inFxPump was already extracted for the legacy red test.
            section = re.sub(r'^static thread_local bool g_inFxPump = false;\n?', '', section, flags=re.M)
            extracted += section
            for pattern in [r'^static void AngleVectors\([^\n]+\) \{', r'^static void LocalPlayMarkLocalSi\([^\n]+\) \{',
                            r'^static bool HpAimHitConfirm\(',
                            r'^static void OnSiHealthDecreased\([^\n]+\) \{',
                            r'^static void DrainHpDropQueue\(\) \{',
                            r'^static bool DamageEventReadTeam\([^\n]+\) \{',
                            r'^static void DrainDamageEventQueue\(bool allowChapterSettle\) \{',
                            r'^struct FeedbackMeleeSample \{']:
                extracted += block(source, pattern) + (';\n' if 'struct ' in pattern else '\n')
            extracted += 'static FeedbackMeleeSample g_feedbackMeleeSample;\n'
            for pattern in [r'^static void Sample\(\) \{', r'^static void ApplyDlcFx\([^;]+?\) \{',
                            r'^static void PumpGameThreadFeedback\(\) \{']:
                extracted += block(source, pattern) + '\n'
            for name in ['IsMeleeWeaponName', 'WeaponIsMelee', 'EventWeaponIsMelee',
                         'FeedbackAdvanceStreak', 'FeedbackSiHit', 'FeedbackSiKill']:
                extracted += block(source, r'^static (?:void|bool) '+name+r'\([^;]+?\) \{') + '\n'
        cls.temp = tempfile.TemporaryDirectory(prefix='skeeto-feedback-')
        out = Path(cls.temp.name); cpp = out / 'test.cpp'
        cpp.write_text(PRELUDE + (PROVIDERS if cls.queues else '') + extracted
                       + ('#define TEST_QUEUES\nstatic bool Check(bool,const char*);\n' + QUEUE_CASES if cls.queues else '')
                       + CASES, encoding='utf-8')
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                      'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                      '-property', 'installationPath'], text=True).strip()
        cls.exe = out / 'test.exe'; bat = out / 'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                       f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /I"{ROOT / "src/skeeto/core"}" '
                       f'/Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n', encoding='utf-8')
        result = subprocess.run(['cmd', '/c', str(bat)], capture_output=True, text=True, errors='replace')
        if result.returncode: raise RuntimeError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()

    def case(self, n):
        result = subprocess.run([str(self.exe), str(n)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_ci_sound_pool_with_si_visuals(self): self.case(0)
    def test_si_sound_pool_with_ci_visuals(self): self.case(1)
    def test_selected_random_pool_does_not_mix_fallback_primary(self): self.case(2)
    def test_execution_permission_is_thread_local(self): self.case(3)

    def test_dispatch_and_attribution_boundaries(self):
        if not self.queues: self.skipTest('pre-change source lacks the current attribution API')
        for n in range(4, 53):
            with self.subTest(case=n): self.case(n)

    def test_disproven_event_and_boomer_paths_are_removed(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        for event in ['player_hurt', 'infected_hurt', 'weapon_fire', 'weapon_fire_on_empty']:
            self.assertNotIn(f'AddOne("{event}")', source)
            self.assertNotIn(f'!strcmp(n, "{event}")', source)
        for name in ['OnHurt(', 'WitchFxOnHurt(', 'StaggerHudOnPlayerHurt(',
                     'TankRockNoticeOnPlayerHurt(', 'ClientUxOnLocalThrowableFire(',
                     'ClientUxOnWeaponFireEmptyMag(', 'g_throwLockPulse', 'SiHpBarLocalImpactNear(']:
            self.assertNotIn(name, source)
        for name in ['HpGunHitConfirm(', 'HpGunTraceCache', 'g_localGunImpact']:
            self.assertNotIn(name, source)
        self.assertNotIn('OnSurvivorHealthDecreased', source,
                         'the old inferred friendly-fire consumer must stay removed')
        for name in ['HpAimHitConfirm', 'OnSiHealthDecreased', 'DrainHpDropQueue']:
            implementation = block(source, r'^static (?:void|bool) '+name+r'\([^;]*?\) \{')
            self.assertNotIn('EngTrace', implementation)


if __name__ == '__main__': unittest.main()
