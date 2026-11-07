// Browser report renderer. Existing service worker calls only on explicit export.
// Ten detail records and seven independent daily totals; inline CSS/SVG only.
#pragma once
#include <cstdio>
#include <ctime>
#include <cstdint>
#include "skeeto_mainui_services.h"
#include "skeeto_mainui_battle_days.h"

static void MainUiReportEscape(FILE* f,const char* s) {
 for(;s&&*s;++s)switch(*s){case '&':fputs("&amp;",f);break;case '<':fputs("&lt;",f);break;case '>':fputs("&gt;",f);break;case '"':fputs("&quot;",f);break;case '\'':fputs("&#39;",f);break;default:fputc((unsigned char)*s,f);}
}
static const char* const kMainUiReportNames[]={
 u8"普感 / Common", "Smoker", "Boomer", "Hunter", "Spitter", "Jockey", "Charger", "Tank", "Witch",
 u8"爆头击杀 / Headshot kills",u8"被控 / Pinned",u8"倒地 / Incapacitations",u8"死亡 / Deaths",
 u8"扶起队友 / Revives",u8"电击救援 / Defibrillations",u8"治疗队友 / Teammate heals",u8"解控救援 / Pin rescues",
 u8"近战特感 / Melee SI",u8"空爆 / Skeet",u8"近战空爆 / Melee skeet",u8"近战秒女巫 / Melee crown",
 u8"切舌 / Tongue cuts",u8"牛刀 / Charger levels",u8"黑枪伤害 / Friendly-fire damage",
 u8"被黑枪伤害 / Friendly-fire damage received",
 u8"Tank 伤害 / Tank damage",u8"Witch 伤害 / Witch damage"
};
static int64_t MainUiReportStat(const MainUiBattleRecord& b,int k) {
 return k>=0&&k<b.statCount&&k<27&&b.stats[k]>0?b.stats[k]:0;
}
static void MainUiReportMetric(FILE* f,const char* label,int64_t value) {
 fprintf(f,"<div class=metric><span>%s</span><strong>%lld</strong></div>",label,(long long)value);
}
static void MainUiReportCareerValue(FILE* f,const CareerStatValue& stat,bool hours=false) {
 if(!stat.valid||stat.value<0)fputs("--",f);
 else if(hours)fprintf(f,"%.1f <small>h</small>",stat.value/3600.);
 else fprintf(f,"%d",stat.value);
}
static void MainUiReportCareerRow(FILE* f,const char* label,const CareerStatValue& stat) {
 fprintf(f,"<div class=stat><span>%s</span><b>",label);MainUiReportCareerValue(f,stat);fputs("</b></div>",f);
}
static void MainUiReportAvatar(FILE* f,const char* avatarUri,char fallback) {
 if(avatarUri&&avatarUri[0]=='d'&&strstr(avatarUri,"data:image/")==avatarUri){
  fputs("<div class=avatar><img alt=\"\" src=\"",f);MainUiReportEscape(f,avatarUri);fputs("\"></div>",f);return;}
 fprintf(f,"<div class=avatar aria-hidden=true>%c</div>",fallback?fallback:'S');
}
static void MainUiReportIdentity(FILE* f,const MainUiCareerReport* report,const char* avatarUri,const char* eyebrow,const char* titleFallback,const char* subtitle) {
 fputs("<div class=profile>",f);MainUiReportAvatar(f,avatarUri,'S');fputs("<div class=identity><div class=eyebrow>",f);fputs(eyebrow,f);fputs("</div><h1>",f);
 if(report&&report->personaName[0])MainUiReportEscape(f,report->personaName);
 else MainUiReportEscape(f,titleFallback&&titleFallback[0]?titleFallback:u8"小队战报 / Squad report");
 fputs("</h1>",f);
 if(report&&report->steamId[0]){fputs(u8"<p class=steamid>SteamID64: ",f);MainUiReportEscape(f,report->steamId);fputs("</p>",f);}
 fputs("<p class=muted>",f);fputs(subtitle,f);fputs("</p></div></div>",f);
}
static void MainUiReportCareerSection(FILE* f,const MainUiCareerReport* report,const char* avatarUri) {
 const MainUiCareerReport unavailable{};const auto& c=report?*report:unavailable;const auto& s=c.stats;
 fputs(u8"<section class=\"panel career\">",f);
 MainUiReportIdentity(f,report,avatarUri,u8"STEAM / L4D2 CAREER",u8"Steam 生涯总览",u8"个人累计游戏统计 / Lifetime game statistics");
 fputs(u8"<div class=kpis>",f);
 const CareerStatValue* overview[]={&s.playTimeSec,&s.infectedKills,&s.gamesPlayed,&s.finalesFinished};
 const char* labels[]={u8"游戏统计时长 / Game play time",u8"感染者击杀 / Infected kills",u8"游戏场次 / Games played",u8"终章完成 / Finales finished"};
 for(int i=0;i<4;++i){fprintf(f,"<div class=metric><span>%s</span><strong>",labels[i]);MainUiReportCareerValue(f,*overview[i],i==0);fputs("</strong></div>",f);}
 if(s.state<2)fputs(u8"<p class=scope>当前未能读取 Steam 生涯数据；下方近期战报仍可查看。</p>",f);
 else if(s.state==2)fputs(u8"<p class=scope>Steam 仅提供了部分字段。可用数据已展示。</p>",f);
 fputs(u8"</section><div class=columns><div><section class=panel><h2>团队与物资 / Team & supplies</h2><div class=stats>",f);
 MainUiReportCareerRow(f,u8"扶起队友 / Revives",s.teamRevived);MainUiReportCareerRow(f,u8"保护队友 / Teammates protected",s.teamProtected);
 MainUiReportCareerRow(f,u8"分享医疗包 / Kits shared",s.kitsShared);MainUiReportCareerRow(f,u8"使用医疗包 / Kits used",s.kitsUsed);
 MainUiReportCareerRow(f,u8"分享止痛药 / Pills shared",s.pillsShared);MainUiReportCareerRow(f,u8"使用止痛药 / Pills used",s.pillsUsed);
 MainUiReportCareerRow(f,u8"分享肾上腺素 / Adrenaline shared",s.adrenalineShared);MainUiReportCareerRow(f,u8"使用肾上腺素 / Adrenaline used",s.adrenalineUsed);
 MainUiReportCareerRow(f,u8"使用电击器 / Defibrillators used",s.defibrillatorsUsed);MainUiReportCareerRow(f,u8"被电击救援 / Times defibrillated",s.defibrillated);
 fputs(u8"</div></section><section class=panel><h2>角色与模式 / Characters & modes</h2><div class=stats>",f);
 const char* survivor[]={"Nick","Coach","Ellis","Rochelle"};for(int i=0;i<4;++i)MainUiReportCareerRow(f,survivor[i],s.survivorGames[i]);
 const char* modes[]={u8"写实场次 / Realism",u8"对抗场次 / Versus",u8"对抗胜场 / Versus wins",u8"生存场次 / Survival",u8"清道夫场次 / Scavenge"};
 for(int i=0;i<5;++i)MainUiReportCareerRow(f,modes[i],s.modeGames[i]);
 fputs(u8"</div></section></div><section class=panel><h2>常用武器 / Top weapons</h2><div class=scroll><table><thead><tr><th>武器 / Weapon</th><th>累计击杀 / Kills</th><th>六项内占比 / Top-six share</th></tr></thead><tbody>",f);
 int n=s.topWeaponCount;if(n<0)n=0;if(n>6)n=6;int64_t sum=0;for(int i=0;i<n;++i)if(s.topWeapons[i].value>0)sum+=s.topWeapons[i].value;
 for(int i=0;i<n;++i){const int value=s.topWeapons[i].value>0?s.topWeapons[i].value:0;const double share=sum?100.*value/sum:0;
  fputs("<tr><td>",f);MainUiReportEscape(f,c.weaponNames[i][0]?c.weaponNames[i]:u8"武器名称不可用");
  fprintf(f,"</td><td>%d</td><td><div class=meter><i style=\"width:%.1f%%\"></i></div><span class=minor>%.1f%%</span></td></tr>",value,share,share);}
 if(!n)fputs(u8"<tr><td colspan=3>暂无可用武器统计 / No weapon statistics available</td></tr>",f);
 fputs(u8"</tbody></table></div></section></div>",f);
}
static void MainUiReportChart(FILE* f,const char* title,const char* color,const int64_t* values,char dates[7][16]) {
 int64_t maximum=1;for(int i=0;i<7;++i)if(values[i]>maximum)maximum=values[i];
 fprintf(f,"<section class=panel><h2>%s</h2><svg role=img aria-label=\"%s\" viewBox=\"0 0 540 210\">",title,title);
 for(int y=30;y<=150;y+=40)fprintf(f,"<path class=gridline d=\"M36 %dH508\"/>",y);
 fprintf(f,"<polyline fill=none stroke=\"%s\" stroke-width=3 points=\"",color);
 for(int i=0;i<7;++i)fprintf(f,"%d,%.1f ",40+i*76,150.-110.*(double)values[i]/(double)maximum);
 fputs("\"/>",f);
 for(int i=0;i<7;++i){double y=150.-110.*(double)values[i]/(double)maximum;
  fprintf(f,"<circle cx=%d cy=%.1f r=4 fill=\"%s\"/><text x=%d y=%.1f text-anchor=middle>%lld</text><text class=axis x=%d y=185 text-anchor=middle>%s</text>",40+i*76,y,color,40+i*76,y-12,(long long)values[i],40+i*76,dates[i]);}
 fputs("</svg></section>",f);
}
static void MainUiReportScore(FILE* f,int cents) { // chapter-stats hundredths -> "12" / "12.5" / "12.25"
 const long long m=cents<0?-(long long)cents:(long long)cents;const int frac=(int)(m%100);
 if(!frac)fprintf(f,"%s%lld",cents<0?"-":"",m/100);
 else if(frac%10==0)fprintf(f,"%s%lld.%d",cents<0?"-":"",m/100,frac/10);
 else fprintf(f,"%s%lld.%02d",cents<0?"-":"",m/100,frac);
}
static int64_t MainUiReportMateStat(const MainUiBattleMate& m,int statCount,int k) {
 return k>=0&&k<statCount&&k<27&&m.stats[k]>0?m.stats[k]:0;
}
static int64_t MainUiReportMateSi(const MainUiBattleMate& m,int statCount) {
 int64_t si=0;for(int k=1;k<=6;++k)si+=MainUiReportMateStat(m,statCount,k);return si;
}
// The capture marks the local survivor directly; old personal-only records have no squad rows.
static int MainUiReportLocalMate(const MainUiBattleRecord& b) {
 const int n=b.mateCount<kMainUiBattleMates?b.mateCount:kMainUiBattleMates;
 for(int m=0;m<n;++m)if(b.mates[m].local&&!b.mates[m].bot)return m;
 return -1;
}
static int MainUiReportMvpMate(const MainUiBattleRecord& b) {
 const int n=b.mateCount<kMainUiBattleMates?b.mateCount:kMainUiBattleMates;
 for(int m=0;m<n;++m)if(b.mates[m].mvp)return m;
 return -1;
}
static void MainUiReportSquad(FILE* f,const MainUiBattleRecord& b,int me) {
 const int n=b.mateCount<kMainUiBattleMates?b.mateCount:kMainUiBattleMates;
 if(n<=0){fputs(u8"<p class=muted>这条记录来自旧版本，只保存了个人数据，没有小队明细。<br>Legacy record: personal data only.</p>",f);return;}
 fputs(u8"<div class=scroll><table class=squad><thead><tr><th>队员 / Survivor</th>",f);
 if(b.scoreEnabled)fputs(u8"<th>评分 / Score</th>",f);
 fputs(u8"<th>普感 / CI</th><th>特感 / SI</th><th>Tank</th><th>Witch</th><th>爆头 / HS</th><th>倒地 / Incap</th><th>死亡 / Deaths</th><th>黑枪 / FF dmg</th></tr></thead><tbody>",f);
 for(int m=0;m<n;++m){const auto& s=b.mates[m];
  fprintf(f,"<tr class=\"%s%s\"><td class=name>",s.mvp?"mvp":"",m==me?(s.mvp?" me":"me"):"");
  if(s.mvp)fputs("<span class=crown>MVP</span>",f);
  MainUiReportEscape(f,s.name[0]?s.name:"?");
  if(s.bot)fputs("<span class=tag>BOT</span>",f);
  if(m==me)fputs(u8"<span class=\"tag you\">你 / You</span>",f);
  fputs("</td>",f);
  if(b.scoreEnabled){fputs("<td>",f);MainUiReportScore(f,s.score);fputs("</td>",f);}
  fprintf(f,"<td>%lld</td><td>%lld</td><td>%lld</td><td>%lld</td><td>%lld</td><td>%lld</td><td>%lld</td>",
   (long long)MainUiReportMateStat(s,b.statCount,0),(long long)MainUiReportMateSi(s,b.statCount),(long long)MainUiReportMateStat(s,b.statCount,7),(long long)MainUiReportMateStat(s,b.statCount,8),
   (long long)MainUiReportMateStat(s,b.statCount,9),(long long)MainUiReportMateStat(s,b.statCount,11),(long long)MainUiReportMateStat(s,b.statCount,12));
  if(b.statCount>23)fprintf(f,"<td>%lld</td>",(long long)MainUiReportMateStat(s,b.statCount,23));else fputs("<td>--</td>",f);
  fputs("</tr>",f);}
 fputs("</tbody></table></div>",f);
}
static bool MainUiReportWrite(FILE* f,const MainUiBattleRecord* records,int count,time_t generated,const MainUiBattleDays& daily,const MainUiCareerReport* career=nullptr,const char* avatarUri=nullptr) {
 if(!f||count<0||count>10||(!records&&count))return false;
 int64_t totals[27]{},heads[9]{},seconds=0;int special=0,mvpMine=0;
 for(int i=0;i<count;++i){const auto& b=records[i];seconds+=b.seconds>0?b.seconds:0;special+=(b.pageMask&8)!=0;
  const int me=MainUiReportLocalMate(b);if(me>=0&&b.mates[me].mvp)++mvpMine;
  for(int k=0;k<27;++k)if(k<17||(k<23&&(b.pageMask&8))||((k==23||k==24)&&(b.pageMask&4))||((k==25||k==26)&&(b.pageMask&16)))totals[k]+=MainUiReportStat(b,k);
  for(int k=0;k<9;++k)if(b.headshots[k]>0)heads[k]+=b.headshots[k];}
 int64_t si=0,kills=0;for(int k=0;k<9;++k){kills+=totals[k];if(k>=1&&k<=6)si+=totals[k];}
 fputs(u8R"HTML(<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Skeeto | 小队战报</title><style>
:root{color-scheme:dark;--bg:#1c202a;--panel:#262b35;--edge:#383e49;--text:#e6eaf0;--muted:#a0aabd;--blue:#7db6e4;--gold:#e8c765}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:14px/1.6 "Segoe UI","Microsoft YaHei",sans-serif}main{max-width:1240px;margin:30px auto;padding:0 22px}h1,h2,h3,p{margin:0}h1{font-size:24px;letter-spacing:.3px}h2{font-size:17px;font-weight:600;color:var(--blue);margin-bottom:15px}h3{font-size:17px}small,.muted{color:var(--muted)}.panel{background:var(--panel);border:1px solid var(--edge);border-radius:10px;padding:22px;margin-bottom:18px}.profile{display:flex;gap:20px;align-items:center}.avatar{width:76px;height:76px;flex-shrink:0;border:1px solid #647d99;border-radius:8px;display:grid;place-items:center;background:linear-gradient(140deg,#34495f,#222c3a);font-size:30px;color:var(--blue);font-weight:700;overflow:hidden;padding:0}.avatar img{width:100%;height:100%;object-fit:cover;display:block}.eyebrow{font-size:11px;letter-spacing:2px;color:var(--blue);margin-bottom:5px}.identity{min-width:0;overflow-wrap:anywhere}.steamid{margin:4px 0 0;font-size:12px;color:#9eb4cc;font-family:Consolas,"Segoe UI",monospace;letter-spacing:.3px}.scope{margin-top:16px;border-top:1px solid var(--edge);padding-top:14px;color:var(--muted)}.career .kpis{grid-template-columns:repeat(4,1fr)}.kpis{display:grid;grid-template-columns:repeat(5,1fr);gap:15px;margin-top:22px}.metric span{display:block;color:var(--muted);font-size:12px}.metric strong{display:block;font-size:27px;font-weight:600;line-height:1.5}.metric.goldm strong{color:var(--gold)}.columns{display:grid;grid-template-columns:1fr 1.3fr;gap:18px;align-items:start}.pair{display:grid;grid-template-columns:1fr 1fr;gap:18px}.stats{display:grid;grid-template-columns:1fr 1fr;gap:13px 24px}.stat{display:flex;justify-content:space-between;gap:12px;border-bottom:1px solid #343b47;padding-bottom:7px}.stat span{color:var(--muted)}.stat b{font-weight:500;white-space:nowrap}.scroll{overflow-x:auto}table{border-collapse:collapse;width:100%;font-size:13px}th,td{padding:10px 9px;text-align:right;border-bottom:1px solid #383e49}th{color:var(--muted);font-weight:500;background:#323843}th:first-child,td:first-child{text-align:left}.meter{height:8px;background:#3b424d;border-radius:3px;overflow:hidden;min-width:76px}.meter i{display:block;height:100%;background:#5298d6}.minor{font-size:11px;color:var(--muted)}svg{display:block;width:100%;height:auto;margin-top:5px}svg text{font:12px "Segoe UI",sans-serif;fill:var(--text)}svg .axis{fill:var(--muted)}.gridline{stroke:#414957;stroke-width:1}.history{display:grid;grid-template-columns:1fr;gap:16px}.match{border:1px solid var(--edge);background:#242d39;border-radius:9px;overflow:hidden;min-width:0}.match.mine{border-color:#b8964a;box-shadow:0 0 0 1px rgba(232,199,101,.3),0 8px 24px rgba(0,0,0,.28)}.match-header{padding:18px;background:linear-gradient(115deg,#364a61,#273442 70%);position:relative}.match.mine .match-header{background:linear-gradient(115deg,#5e4f2a,#2b3541 78%)}.badge{float:right;max-width:55%;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;border-radius:4px;padding:3px 10px;font-size:12px;background:#394354;color:#d5dfec}.badge.gold{background:linear-gradient(135deg,#f3d779,#c79a2b);color:#2b2108;font-weight:700;box-shadow:0 0 14px rgba(232,199,101,.35)}.meta{margin-top:8px;color:#b8c9da;font-size:12px;overflow-wrap:anywhere}.match-body{padding:16px}.match .kpis{margin:0 0 14px;grid-template-columns:repeat(5,1fr);gap:10px}.match strong{font-size:21px}table.squad th,table.squad td{padding:8px}tr.mvp td{background:rgba(232,199,101,.13);color:#ffe69a;font-weight:600}tr.mvp td:first-child{box-shadow:inset 3px 0 0 var(--gold)}tr.me td:first-child{box-shadow:inset 3px 0 0 var(--blue)}tr.mvp.me td:first-child{box-shadow:inset 3px 0 0 var(--gold)}.crown{display:inline-block;margin-right:7px;padding:0 6px;border-radius:3px;background:var(--gold);color:#2b2108;font-size:11px;font-weight:800;letter-spacing:.5px}.tag{margin-left:6px;padding:0 5px;border:1px solid #4a5260;border-radius:3px;font-size:10px;font-weight:400;color:var(--muted)}.tag.you{border-color:#6a97c2;color:var(--blue)}.name{overflow-wrap:anywhere}.toolbar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:18px;align-items:center}.toolbar h2{margin:0 auto 0 0}button{font:inherit;color:#ccd7e6;background:#303c4c;border:1px solid #435166;border-radius:5px;padding:5px 12px;cursor:pointer}button[aria-pressed=true]{background:#456482;border-color:#729bc4;color:white}button.gold[aria-pressed=true]{background:#8a722b;border-color:#e8c765;color:#ffe69a}details{margin-top:17px;border-top:1px solid var(--edge);padding-top:10px}summary{cursor:pointer;color:var(--blue)}details .scroll{margin-top:12px}.empty{padding:40px;text-align:center;color:var(--muted)}footer{text-align:center;color:var(--muted);font-size:12px;padding:10px 0 25px}.notice{border-left:3px solid var(--blue);font-size:13px}.match[hidden]{display:none}@media(max-width:850px){.columns,.pair{grid-template-columns:1fr}.kpis{grid-template-columns:repeat(3,1fr)}main{padding:0 14px;margin-top:16px}}@media(max-width:560px){.career .kpis{grid-template-columns:repeat(2,1fr)}.stats{grid-template-columns:1fr}.panel{padding:17px}.profile{gap:12px}.avatar{width:58px;height:58px;font-size:24px}.kpis,.match .kpis{grid-template-columns:repeat(2,1fr)}h1{font-size:20px}.badge{float:none;display:inline-block;margin-bottom:6px;max-width:100%}}@media print{body{background:white;color:black}.panel,.match{background:white;color:black;break-inside:avoid}button{display:none}.muted,small,.stat span,.metric span{color:#555}details{display:block}}
</style></head><body><main>)HTML",f);
 MainUiReportCareerSection(f,career,avatarUri);
 fputs(u8"<section class=panel>",f);
 MainUiReportIdentity(f,career,avatarUri,u8"RECENT / SQUAD BATTLES",
  count&&records&&records[0].player[0]?records[0].player:u8"小队战报 / Squad report",
  u8"最近成功通关的章节，含全小队统计 / Successful chapters with full squad stats");
 fputs(u8"<div class=kpis>",f);
 MainUiReportMetric(f,u8"成功章节 / Cleared",count);
 fprintf(f,u8"<div class=\"metric goldm\"><span>我的 MVP / My MVPs</span><strong>%d</strong></div>",mvpMine);
 MainUiReportMetric(f,u8"普感击杀 / Common",totals[0]);MainUiReportMetric(f,u8"特感击杀 / SI",si);
 fprintf(f,u8"<div class=metric><span>记录时长 / Play time</span><strong>%.1f <small>h</small></strong></div></div></section><div class=columns><div><section class=panel><h2>个人汇总 / Personal totals</h2><div class=stats>",seconds/3600.);
 for(int k=0;k<17;++k)fprintf(f,"<div class=stat><span>%s</span><b>%lld</b></div>",kMainUiReportNames[k],(long long)totals[k]);
 for(int k=23;k<27;++k)fprintf(f,"<div class=stat><span>%s</span><b>%lld</b></div>",kMainUiReportNames[k],(long long)totals[k]);
 fputs(u8"</div><details><summary>技巧统计 / Special actions</summary><div class=stats style=\"margin-top:12px\">",f);
 for(int k=17;k<23;++k){fprintf(f,"<div class=stat><span>%s</span><b>",kMainUiReportNames[k]);if(special)fprintf(f,"%lld",(long long)totals[k]);else fputs("--",f);fputs("</b></div>",f);}
 fputs(u8"</div></details></section></div><section class=panel><h2>击杀分类 / Infected breakdown</h2><div class=scroll><table><thead><tr><th>类型 / Type</th><th>击杀 / Kills</th><th>爆头 / HS</th><th>爆头占比 / HS %</th><th>击杀占比 / Share</th></tr></thead><tbody>",f);
 for(int k=0;k<9;++k){double hs=totals[k]?100.*heads[k]/totals[k]:0,share=kills?100.*totals[k]/kills:0;if(hs>100)hs=100;
  fprintf(f,"<tr><td>%s</td><td>%lld</td><td>%lld</td><td>",kMainUiReportNames[k],(long long)totals[k],(long long)heads[k]);if(totals[k])fprintf(f,"%.1f%%",hs);else fputs("--",f);
  fprintf(f,"</td><td><div class=meter><i style=\"width:%.1f%%\"></i></div><span class=minor>%.1f%%</span></td></tr>",share,share);}
 fputs(u8"</tbody></table></div></section></div>",f);
 int64_t ciDays[7]{},siDays[7]{};char dates[7][16]{};int keys[7]{};
 if(MainUiBattleWindow(generated,keys))for(int d=0;d<7;++d){
  snprintf(dates[d],sizeof(dates[d]),"%02d-%02d",keys[d]/100%100,keys[d]%100);
  for(const auto& day:daily.rows)if(day.date==keys[d]){ciDays[d]=day.common;siDays[d]=day.special;break;}
 }
 fputs("<div class=pair>",f);MainUiReportChart(f,u8"近期普感击杀 / Common kills","#65a8ed",ciDays,dates);MainUiReportChart(f,u8"近期特感击杀 / SI kills","#efb466",siDays,dates);
 fputs(u8"</div><section class=panel><div class=toolbar><h2>成功章节 / Cleared chapters</h2><button type=button data-filter=all aria-pressed=true>全部 / All</button><button type=button class=gold data-filter=1 aria-pressed=false>我是 MVP</button><button type=button data-filter=0 aria-pressed=false>非 MVP</button></div><div class=history>",f);
 for(int i=0;i<count;++i){const auto& b=records[i];const int me=MainUiReportLocalMate(b),top=MainUiReportMvpMate(b);const bool mine=me>=0&&b.mates[me].mvp;
  fprintf(f,"<article class=\"match%s\" data-mvp=%d><div class=match-header>",mine?" mine":"",mine?1:0);
  if(top>=0){fputs(u8"<span class=\"badge gold\">MVP: ",f);MainUiReportEscape(f,b.mates[top].name);fputs(mine?u8"（你 / you）":"",f);fputs("</span>",f);}
  const char* title=b.campaign[0]?b.campaign:b.chapter[0]?b.chapter:b.map[0]?b.map:u8"地图未知";
  fputs("<h3>",f);MainUiReportEscape(f,title);fputs("</h3><div class=meta>",f);
  if(b.chapter[0]&&strcmp(b.chapter,title)){MainUiReportEscape(f,b.chapter);fputs("<br>",f);}
  if(b.map[0]&&strcmp(b.map,title)&&(!b.chapter[0]||strcmp(b.map,b.chapter))){MainUiReportEscape(f,b.map);fputs("<br>",f);}
  fputs(u8"服务器 / Server: ",f);
  if(b.serverName[0]){MainUiReportEscape(f,b.serverName);if(b.server[0]&&_stricmp(b.server,b.serverName)){fputs("<br>",f);MainUiReportEscape(f,b.server);}}
  else MainUiReportEscape(f,b.server[0]?b.server:u8"未记录地址");
  tm when{};time_t stamp=(time_t)b.time;char date[48]{};if(!localtime_s(&when,&stamp))strftime(date,sizeof(date),"%Y-%m-%d %H:%M",&when);
  fprintf(f,u8"<br>时间 / Time: %s | 回合时长 / Duration: %d:%02d<br>玩家 / Player: ",date,b.seconds/60,b.seconds%60);MainUiReportEscape(f,b.player);fputs(u8" | Skeeto ",f);MainUiReportEscape(f,b.version);
  int64_t localSi=0;for(int k=1;k<=6;++k)localSi+=MainUiReportStat(b,k);
  fputs("</div></div><div class=match-body><div class=kpis>",f);MainUiReportMetric(f,u8"普感 / Common",MainUiReportStat(b,0));MainUiReportMetric(f,u8"特感 / SI",localSi);MainUiReportMetric(f,"Tank",MainUiReportStat(b,7));MainUiReportMetric(f,"Witch",MainUiReportStat(b,8));
  if(b.scoreEnabled){fputs(u8"<div class=metric><span>评分 / Score</span><strong>",f);MainUiReportScore(f,b.score);fputs("</strong></div>",f);}else fputs(u8"<div class=metric><span>评分 / Score</span><strong>--</strong></div>",f);
  fputs("</div>",f);MainUiReportSquad(f,b,me);
  fputs(u8"<details><summary>我的完整统计与当时评分 / My full stats & recorded score</summary><p class=muted>",f);
  if(b.scoreEnabled)fputs(u8"评分使用记录时的权重与评分页面开关，不与其他配置的评分作排名比较。",f);else fputs(u8"记录时评分关闭，MVP 按击杀数判定。",f);
  fprintf(f,u8"<br>评分页面：击杀 %s / 特感 %s / 支援 %s / 技巧 %s / 伤害 %s</p><div class=scroll><table><thead><tr><th>项目 / Metric</th><th>次数 / Count / Damage</th><th>当时权重 / Weight</th></tr></thead><tbody>",b.pageMask&1?u8"开":u8"关",b.pageMask&2?u8"开":u8"关",b.pageMask&4?u8"开":u8"关",b.pageMask&8?u8"开":u8"关",b.pageMask&16?u8"开":u8"关");
  for(int k=0;k<27;++k){fprintf(f,"<tr><td>%s</td><td>",kMainUiReportNames[k]);
   const bool notRecorded=k>=b.statCount||(k>=17&&k<23&&!(b.pageMask&8));
   if(notRecorded)fputs(u8"未记录",f);else fprintf(f,"%lld",(long long)MainUiReportStat(b,k));
   fputs("</td><td>",f);
   if((k==23||k==24)&&(b.scoreEnabled&&!notRecorded)){
    if(k==23&&(b.pageMask&4)){
     if(b.weights[k]<0)fputs(u8"每 2 点伤害扣 1 分",f); // pre-ff_dmg_mul archive
     else{fputs(u8"每点伤害扣 ",f);MainUiReportScore(f,b.weights[k]);fputs(u8" 分",f);}
    }else fputs(u8"未参与评分",f);}
   else if((k==25||k==26)&&(b.scoreEnabled&&!notRecorded)){if(b.pageMask&16)fprintf(f,u8"每 100 点伤害 %d 分",b.weights[k]);else fputs(u8"未参与评分",f);}
   else if(b.scoreEnabled&&k<b.statCount&&k<23)fprintf(f,k==9?"%d%%":"%d",b.weights[k]);else fputs("--",f);fputs("</td></tr>",f);}
  fputs("</tbody></table></div></details></div></article>",f);
 }
 if(!count)fputs(u8"<p class=empty>暂无成功章节记录。开启主菜单的记录战报与章节结算统计，成功通关一章后再查看；失败和断线不会记录。<br>No cleared chapters yet. Enable battle recording and chapter statistics.</p>",f);
 fputs(u8"</div><p class=empty id=filter-empty hidden>这个分类下暂无记录。</p></section><footer>Skeeto / Squad battle report</footer></main><script>const buttons=document.querySelectorAll('[data-filter]');buttons.forEach(b=>b.addEventListener('click',()=>{let shown=0;buttons.forEach(x=>x.setAttribute('aria-pressed',x===b?'true':'false'));document.querySelectorAll('.match').forEach(x=>{x.hidden=b.dataset.filter!=='all'&&x.dataset.mvp!==b.dataset.filter;if(!x.hidden)shown++});document.getElementById('filter-empty').hidden=shown>0}));</script></body></html>",f);
 return !ferror(f);
}
