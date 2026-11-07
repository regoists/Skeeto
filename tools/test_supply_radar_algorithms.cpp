// Offline checks for the BSP entity-lump supply metadata (SupplyRadar 1.3.6 parser): the incremental
// parser must publish exactly what the one-shot parser publishes, at any step size, and fail closed.
#include <cstdio>
#include <string>
#include "../src/skeeto/core/skeeto_navigation/performance.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/radar_abi.h"
namespace supplyradar {
#include "../src/skeeto/core/skeeto_navigation/supply_radar/radar_math.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/items.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/budget_clock.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/map_supply.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/map_supply_parser.h"
}
using namespace supplyradar;
static int failures=0,checks=0;
static void Check(bool ok,const char* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}}
static const unsigned char* Bytes(const std::string& s){return reinterpret_cast<const unsigned char*>(s.data());}
static std::string Entity(const char* classname,const char* origin,const std::string& extra=""){
    return std::string("{\"classname\" \"")+classname+"\" \"origin\" \""+origin+"\" "+extra+"}\n";
}
static map_supply::Parser parser;
// Runs the incremental parser with a tiny action budget and checks it equals the one-shot result.
static bool Increment(map_supply::Map& map,const std::string& data,unsigned actions=7){
    static map_supply::Map oneShot;
    bool expected=oneShot.Parse(Bytes(data),unsigned(data.size()));
    bool began=parser.Begin(map,Bytes(data),unsigned(data.size()));
    unsigned calls=0;
    while(parser.Active()&&++calls<400000)parser.Step(100000,actions);
    Check(!parser.Active(),"incremental parser terminates");
    Check(map.loaded==expected&&(began||!expected),"incremental result matches one-shot result");
    if(expected&&map.loaded)Check(map.supplies==oneShot.supplies&&map.parents==oneShot.parents&&map.entities==oneShot.entities&&map.buttonDisplays==oneShot.buttonDisplays,"incremental counts match one-shot counts");
    return map.loaded;
}
static void SupplyFlags(){
    static map_supply::Map map;
    std::string data=Entity("weapon_rifle_spawn","10 0 0")+Entity("weapon_rifle_spawn","20 0 0","\"spawnflags\" \"8\"");
    Check(Increment(map,data),"finite and infinite supplies parse");
    Check(map.Find({12,{10,0,0},nullptr})==map_supply::Finite&&map.Find({12,{20,0,0},nullptr})==map_supply::Unlimited,"spawnflags 8 marks an unlimited supply");
    Check(Increment(map,data,1)&&Increment(map,data,64),"different action budgets complete");
    data+=Entity("weapon_rifle_spawn","20 0 0");
    Check(Increment(map,data)&&map.Find({12,{20,0,0},nullptr})==map_supply::Unknown,"conflicting finite and infinite records stay unknown");
}
static void Parents(){
    static map_supply::Map map;
    std::string door=Entity("func_door","100 0 0","\"targetname\" \"door\" \"model\" \"*1\"");
    std::string supply=Entity("weapon_rifle_spawn","110 0 0","\"parentname\" \"door\"");
    Check(Increment(map,supply+door),"parented supply parses");
    Check(map.Find({12,{10,0,0},"*1"})==map_supply::Finite,"unique unrotated parent supplies use local coordinates");
    Check(Increment(map,supply+door+door)&&map.Find({12,{10,0,0},"*1"})==map_supply::Unknown,"duplicate parent names leave the supply unknown");
    std::string rotated=Entity("func_door","100 0 0","\"targetname\" \"door\" \"model\" \"*1\" \"angles\" \"0 90 0\"");
    Check(Increment(map,supply+rotated)&&map.Find({12,{10,0,0},"*1"})==map_supply::Unknown,"rotated parent is conservatively unknown");
}
static void Laser(){
    static map_supply::Map map;
    Check(Increment(map,Entity("upgrade_laser_sight","0 0 0","\"spawnflags\" \"8\""))&&map.FixedLaser({0,0,8})&&!map.FixedLaser({2,0,0}),"fixed laser uses a tight position match");
    Check(Increment(map,Entity("upgrade_spawn","0 0 0","\"laser_sight\" \"1\" \"upgradepack_incendiary\" \"1\" \"upgradepack_explosive\" \"0\""))&&!map.FixedLaser({0,0,0}),"mixed upgrade pack is not a fixed laser");
}
static void MalformedAndLimits(){
    static map_supply::Map map;
    const std::string malformed[]={"","{\"classname\" \"worldspawn\"","{\"origin\" \"1 2\"}","{\"classname\" \"worldspawn\" \"classname\" \"func_door\"}","{\"spawnflags\" \"4294967296\"}"};
    for(const auto& data:malformed)Check(!Increment(map,data)&&!map.loaded,"malformed BSP fails closed without partial hints");
    std::string token="{\"classname\" \"worldspawn\" \"script\" \""+std::string(9000,'x')+"\"}";
    Increment(map,token); // a long unknown value is skipped in chunks; equivalence with the one-shot parser is checked inside
    std::string pairs="{";for(int i=0;i<129;++i)pairs+="\"unknown\" \"value\" ";pairs+="}";
    Increment(map,pairs); // many unknown pairs: incremental and one-shot results must agree
    std::string many;for(int i=0;i<513;++i)many+=Entity("weapon_rifle_spawn","0 0 0");
    Check(!Increment(map,many),"supply capacity rejects excessive records");
    std::string tail("{}");tail.push_back('\0');tail.push_back('x');
    Check(!Increment(map,tail),"data after the terminating NUL is rejected");
    Check(parser.Begin(map,Bytes(token),unsigned(token.size())),"partial parse begins");
    unsigned at=parser.reader.at;parser.Step(100000,64);
    Check(parser.reader.at<=at+64||!parser.Active(),"one Step never reads far beyond its action budget");
    parser.Cancel();map.Clear();Check(!parser.Active()&&!map.loaded&&!map.supplies,"cancel leaves an empty map");
}
static void Buttons(){
    static map_supply::Map map;
    std::string display=Entity("prop_dynamic","0 0 0","\"targetname\" \"reward\" \"model\" \"models/w_models/weapons/w_m60.mdl\"");
    Check(Increment(map,display)&&map.DisplayLabel({0,{0,0,0},nullptr})==0,"a display prop alone creates no supply hint");
}
int main(){SupplyFlags();Parents();Laser();MalformedAndLimits();Buttons();std::printf("%d algorithm checks, %d failures\n",checks,failures);return failures?1:0;}
