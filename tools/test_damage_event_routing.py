"""Exercise the production, allocation-free damage event routing rules."""
from pathlib import Path
import tempfile
import unittest

from test_last_music import compile_run
from test_chapstats_pages import ROOT, block


SOURCE = r'''
#include <cassert>
#include "skeeto_damage_events.h"
int main() {
    using namespace skeeto_damage;
    assert(IsFriendlyFireDamage(2, 2, 1, 2, 11));
    assert(!IsFriendlyFireDamage(1, 2, 1, 2, 11)); // self damage
    assert(!IsFriendlyFireDamage(2, 3, 1, 2, 11)); // infected victim
    assert(!IsFriendlyFireDamage(2, 2, 3, 3, 11)); // enemy attacker
    assert(!IsFriendlyFireDamage(2, 2, 1, 2, 0));  // zero health damage

    assert(DamageSourceHudAccepts(kDamageSourceTeammates, 1, 2, 1, 2, 2, 2, 8));
    assert(!DamageSourceHudAccepts(kDamageSourceTeammates, 1, 2, 1, 2, 1, 2, 8)); // self
    assert(!DamageSourceHudAccepts(kDamageSourceTeammates, 1, 2, 1, 2, 3, 3, 8)); // enemy
    assert(DamageSourceHudAccepts(kDamageSourceAll, 1, 2, 1, 2, 3, 3, 8));
    assert(DamageSourceHudAccepts(kDamageSourceAll, 1, 2, 1, 2, 0, 0, 8)); // world/environment
    assert(!DamageSourceHudAccepts(kDamageSourceAll, 1, 2, 2, 2, 3, 3, 8)); // another victim
    assert(!DamageSourceHudAccepts(kDamageSourceAll, 1, 2, 1, 2, 3, 3, 0));

    assert(FriendlyFirePenaltyCents(0, 50) == 0);
    assert(FriendlyFirePenaltyCents(1, 50) == 50);   // 0.5 point
    assert(FriendlyFirePenaltyCents(2, 50) == 100);  // 1.0
    assert(FriendlyFirePenaltyCents(3, 50) == 150);  // 1.5
    assert(FriendlyFirePenaltyCents(5, 50) == 250);  // 2.5
    assert(FriendlyFirePenaltyCents(4, 25) == 100);  // 0.25 * 4
    assert(FriendlyFirePenaltyCents(5, 0) == 0);

    assert(IsLocalFriendlyFireDamage(1, 2, 2, 1, 2, 14));
    assert(!IsLocalFriendlyFireDamage(1, 1, 2, 2, 2, 14)); // local player was victim
    assert(!IsLocalFriendlyFireDamage(1, 2, 3, 1, 2, 14)); // infected victim
    assert(FriendlyFireDamageTierFor(14) == kFriendlyFireDamageLight);
    assert(FriendlyFireDamageTierFor(15) == kFriendlyFireDamageHeavy);
    assert(FriendlyFireDamageTierFor(40) == kFriendlyFireDamageHeavy);
    assert(FriendlyFireDamageTierFor(0) == kFriendlyFireDamageNone);
    assert(FriendlyFireDamageFontPx(24) == 48);
    assert(FriendlyFireDamageFontPx(0) == 0);
}
'''


class DamageEventRouting(unittest.TestCase):
    def test_direct_event_filters_and_fixed_ff_penalty(self):
        with tempfile.TemporaryDirectory(prefix="skeeto-damage-routing-") as temporary:
            compile_run(SOURCE, Path(temporary))

    def test_tank_target_absolute_position_uses_full_screen_range(self):
        hud = (ROOT / "src/skeeto/core/skeeto_hud.cpp").read_text(encoding="utf-8-sig")
        helper = block(hud, r"^static void TankTargetHudClampOffsets\(")
        source = r'''
#include <cassert>
''' + helper + r'''
int main() {
    int x=1200,y=900;
    TankTargetHudClampOffsets(true,&x,&y);
    assert(x==1200 && y==900);
    x=7000;y=4100;TankTargetHudClampOffsets(true,&x,&y);
    assert(x==7000 && y==4100);
    x=-30;y=9999;TankTargetHudClampOffsets(true,&x,&y);
    assert(x==0 && y==4320);
    x=1200;y=900;TankTargetHudClampOffsets(false,&x,&y);
    assert(x==600 && y==600);
}
'''
        with tempfile.TemporaryDirectory(prefix="skeeto-tank-hud-position-") as temporary:
            compile_run(source, Path(temporary))
        self.assertGreater(hud.find("TankTargetHudClampOffsets(g_tankTargetHudAbs"), -1,
                            "Tank target Paint should clamp using its current placement mode")

    def test_world_warning_and_source_hud_use_shared_concise_event(self):
        proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        self.assertGreater(proxy.find("DrainDamageEventQueue"), -1,
                            "the concise event should feed one shared dispatch")
        self.assertGreater(proxy.find("WorldNoticeSetFriendlyFire"), -1,
                            "the restored world warning should use resolved event damage")
        self.assertGreater(proxy.find("DamageSourceHudOnEvent"), -1,
                            "the damage-source HUD should use the shared event path")
        self.assertEqual(proxy.find('!strcmp(n, "player_hurt")'), -1,
                         "the unreliable local event must remain removed")

    def test_friendly_fire_hud_coalesces_only_short_bursts(self):
        hud = (ROOT / "src/skeeto/core/skeeto_hud.cpp").read_text(encoding="utf-8-sig")
        source = r'''
#include <Windows.h>
#include <cassert>
#include <cstring>
static DWORD g_now=1000;
#define GetTickCount() g_now
static bool g_optFriendlyFireHud=true;
struct PlayerInfo { char name[64]; };
static bool EngPlayerInfo(int ent,PlayerInfo* pi){
    strcpy_s(pi->name, ent==2 ? "Coach" : "Nick");return true;
}
enum { kFriendlyFireHudMaxRows=4 };
struct FriendlyFireHudRow { int victimEnt;int damage;DWORD until;DWORD mergeUntil;wchar_t name[32]; };
static FriendlyFireHudRow g_friendlyFireHudRows[kFriendlyFireHudMaxRows]{};
''' + block(hud, r"^void FriendlyFireHudOnEvent\(") + r'''
int main(){
    FriendlyFireHudOnEvent(2,12);assert(g_friendlyFireHudRows[0].damage==12);
    g_now+=200;FriendlyFireHudOnEvent(2,8);assert(g_friendlyFireHudRows[0].damage==20);
    g_now+=500;FriendlyFireHudOnEvent(2,5);assert(g_friendlyFireHudRows[0].damage==5);
    FriendlyFireHudOnEvent(3,15);assert(g_friendlyFireHudRows[1].victimEnt==3);
    assert(g_friendlyFireHudRows[1].damage==15);
}
'''
        with tempfile.TemporaryDirectory(prefix="skeeto-friendly-hud-burst-") as temporary:
            compile_run(source, Path(temporary))


if __name__ == "__main__":
    unittest.main()
