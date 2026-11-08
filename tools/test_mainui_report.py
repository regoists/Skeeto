"""Compile the production browser renderer; synthetic preview, no game files."""
from pathlib import Path
import tempfile
import shutil
from html.parser import HTMLParser
from test_last_music import compile_run
from test_chapstats_pages import ROOT


class Document(HTMLParser):
    def __init__(self):
        super().__init__()
        self.matches = self.charts = self.scripts = self.images = 0

    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        if tag == 'article' and 'match' in attrs.get('class', '').split():
            self.matches += 1
        if tag == 'svg':
            self.charts += 1
        if tag == 'script':
            self.scripts += 1
            assert 'src' not in attrs
        if tag == 'img':
            src = attrs.get('src', '')
            assert src.startswith('data:image/'), src
            assert 'http://' not in src and 'https://' not in src
            self.images += 1
            return
        assert tag not in ('iframe', 'link'), (tag, attrs)


def main(export_preview=False):
    report_source = (ROOT / 'src/skeeto/core/skeeto_mainui_report.h').read_text(encoding='utf-8-sig')
    local_match = report_source.split('static int MainUiReportLocalMate(', 1)[1].split('static int MainUiReportMvpMate(', 1)[0]
    assert 'b.mates[m].local' in local_match, 'local squad identity must use the captured player identity, not the display name'
    proxy_source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    capture_start = proxy_source.rfind('static void MainUiHubCaptureLive(int result)')
    capture = proxy_source[capture_start:].split('static void ChapStatsScoreHudClear()', 1)[0]
    assert 'if(g_localUserId<=0)return;' in capture, 'battle capture must skip when the local userid is unknown'
    hub = (ROOT / 'src/skeeto/core/skeeto_mainui_hub.h').read_text(encoding='utf-8-sig')
    timing = hub.split(' // Retire timing once on disable;')[1].split(' if(!show)return;')[0]
    timing = timing[timing.index('\n'):]
    source = '''
static bool g_optMainMenuUi=true,g_optMainUiBattleArchive=true,g_optChapStats=true,g_mainUiInGame=true,g_mainUiLoading=false,g_mainUiBattleCaptured=false;
static unsigned long g_mainUiBattleStart=0;
static void Timing(unsigned long now){
''' + timing + '}\n' + r'''
#include <cassert>
#include <cstring>
#include "skeeto_mainui_report.h"
int main(){
 Timing(100);assert(g_mainUiBattleStart==100);
 g_optMainUiBattleArchive=false;g_mainUiBattleCaptured=true;Timing(200);assert(!g_mainUiBattleStart&&!g_mainUiBattleCaptured);
 Timing(300);assert(!g_mainUiBattleStart);g_optMainUiBattleArchive=true;Timing(400);assert(g_mainUiBattleStart==400);
 g_optMainMenuUi=false;Timing(500);assert(!g_mainUiBattleStart);
 MainUiBattleDays daily{};
 FILE* f=fopen("invalid.html","wb");assert(f);
 assert(!MainUiReportWrite(f,nullptr,1,time(nullptr),daily));
 assert(!MainUiReportWrite(f,nullptr,11,time(nullptr),daily));fclose(f);
 f=fopen("empty.html","wb");assert(f);assert(MainUiReportWrite(f,nullptr,0,time(nullptr),daily));fclose(f);
 MainUiBattleRecord rows[10]{};
 for(int i=0;i<10;++i){auto& b=rows[i];b.time=(uint64_t)(time(nullptr)-i*43200);b.seconds=260+i*53;b.result=0; /* only successful chapters are stored */
  strcpy_s(b.player,"regoists");strcpy_s(b.server,"127.0.0.1:27015");strcpy_s(b.version,"1.7.1");
  const char* maps[]={"c5m1_waterfront","c5m2_park","c5m3_cemetery","c5m4_quarter","c5m5_bridge"};strcpy_s(b.map,maps[i%5]);
  if(!i){strcpy_s(b.campaign,u8"教区");strcpy_s(b.chapter,u8"码头");strcpy_s(b.serverName,"Listen");}
  b.pageMask=(i==0?3:(i%2?7:15))|(i>=2?16:0);b.scoreEnabled=i%3!=1;b.score=(80+i*12)*100+34;b.statCount=27;
  b.stats[0]=80+i*26;for(int k=1;k<9;++k){b.stats[k]=(i+k)%11;b.headshots[k]=b.stats[k]/3;}b.headshots[0]=b.stats[0]/4;
  b.stats[9]=b.headshots[0];for(int k=1;k<9;++k)b.stats[9]+=b.headshots[k];
  for(int k=10;k<23;++k){b.stats[k]=(i+k)%4;b.weights[k]=k==12?-20:5;}b.weights[9]=150;
  b.stats[23]=42+i;b.weights[23]=-1;
  b.stats[24]=17+i;b.stats[25]=1200+i;b.stats[26]=300+i;b.weights[25]=1;b.weights[26]=2;
  // Squad: local player + bot + two humans (unsorted on purpose); mate i%4 is the MVP, local MVP when i%4==0.
  const char* names[]={"regoists","Bot Nick","Teammate A","Teammate B"};b.mateCount=4;
  for(int m=0;m<4;++m){auto& s=b.mates[m];strcpy_s(s.name,names[m]);s.userid=10+m;s.bot=m==1;s.mvp=m==i%4;s.score=s.mvp?123456:1234+m;
   s.local=m==0;for(int k=0;k<27;++k)s.stats[k]=m==0?b.stats[k]:(k+m+i)%9;s.stats[0]=m==0?b.stats[0]:40+m;s.stats[23]=m==0?b.stats[23]:5+m;}
  }
  for(const auto& row:rows)MainUiBattleDaysAdd(daily,row,time(nullptr));
  MainUiBattleRecord identity{};strcpy_s(identity.player,"Duplicate");identity.statCount=27;identity.mateCount=2;
  for(int m=0;m<2;++m){auto& s=identity.mates[m];strcpy_s(s.name,"Duplicate");s.userid=20+m;s.local=m==1;s.mvp=m==1;s.stats[0]=30+m;}
  f=fopen("identity.html","wb");assert(f);assert(MainUiReportWrite(f,&identity,1,time(nullptr),daily));fclose(f);
  MainUiCareerReport career{};career.stats.state=3;
 strcpy_s(career.personaName,"regoists");strcpy_s(career.steamId,"76561198000000000");
 // Tiny synthetic 2x2 BMP data-URI (local only; no remote image fetch).
 const char* avatarUri="data:image/bmp;base64,Qk1GAAAAAAAAADYAAAAoAAAAAgAAAAIAAAABABgAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAA////AAAAAAAA/////wAAAAA=";
 career.stats.playTimeSec={true,606240};career.stats.infectedKills={true,187650};career.stats.gamesPlayed={true,925};career.stats.finalesFinished={true,346};
 career.stats.teamRevived={true,245};career.stats.teamProtected={true,1632};career.stats.kitsUsed={true,712};career.stats.kitsShared={true,195};
 career.stats.pillsUsed={true,830};career.stats.pillsShared={true,211};career.stats.adrenalineUsed={true,93};career.stats.adrenalineShared={true,38};
 career.stats.defibrillatorsUsed={true,67};career.stats.defibrillated={true,44};
 for(int i=0;i<4;++i)career.stats.survivorGames[i]={true,210+i*15};for(int i=0;i<5;++i)career.stats.modeGames[i]={true,120+i*7};
 const char* weapons[]={"AK-47","M16","SPAS-12","Magnum","UZI","Katana"};career.stats.topWeaponCount=6;
 for(int i=0;i<6;++i){career.stats.topWeapons[i]={i,42000-i*6200};strcpy_s(career.weaponNames[i],weapons[i]);}
 f=fopen("preview.html","wb");assert(f);assert(MainUiReportWrite(f,rows,10,time(nullptr),daily,&career,avatarUri));fclose(f);
 career.stats.playTimeSec.valid=false;career.stats.infectedKills={true,0};career.stats.state=2;
 strcpy_s(career.weaponNames[0],"<script>weapon</script>");
 f=fopen("partial.html","wb");assert(f);assert(MainUiReportWrite(f,rows,1,time(nullptr),daily,&career));fclose(f);
 strcpy_s(rows[0].player,"<script>alert('x')</script>&\"");rows[0].pageMask=7;rows[0].stats[17]=777777;strcpy_s(rows[0].mates[1].name,"<b onclick=1>&mate</b>");
 f=fopen("escaped.html","wb");assert(f);assert(MainUiReportWrite(f,rows,1,time(nullptr),daily));fclose(f);
 rows[0].statCount=0;rows[0].mateCount=0;
 f=fopen("missing.html","wb");assert(f);assert(MainUiReportWrite(f,rows,1,time(nullptr),daily));fclose(f);
 rows[0].statCount=23;
 f=fopen("legacy.html","wb");assert(f);assert(MainUiReportWrite(f,rows,1,time(nullptr),daily));fclose(f);
 puts("PASS: production HTML renderer: empty/ten records, escaping, disabled scoring and special counters, missing fields, inline charts and responsive cards");
}
'''
    with tempfile.TemporaryDirectory(prefix='skeeto-report-') as temporary:
        folder = Path(temporary)
        compile_run(source, folder)
        for name, expected in [('preview.html', 10), ('empty.html', 0), ('escaped.html', 1)]:
            body = (folder / name).read_text(encoding='utf-8')
            document = Document()
            document.feed(body)
            got = (document.matches, document.charts, document.scripts, document.images)
            if (document.matches, document.charts, document.scripts) != (expected, 2, 1):
                raise AssertionError(f'{name}: got {got}, want matches={expected} charts=2 scripts=1')
            assert body.endswith('</html>')
            if name == 'preview.html':
                assert document.images >= 2, got
        escaped = (folder / 'escaped.html').read_text(encoding='utf-8')
        assert '<script>alert' not in escaped and '&lt;script&gt;' in escaped
        assert '777777' not in escaped and '未记录' in escaped
        assert '未记录' in (folder / 'missing.html').read_text(encoding='utf-8')
        preview = (folder / 'preview.html').read_text(encoding='utf-8')
        assert preview.index('STEAM / L4D2 CAREER') < preview.index('RECENT / SQUAD BATTLES')
        identity = (folder / 'identity.html').read_text(encoding='utf-8')
        assert 'MVP: Duplicate（你 / you）' in identity
        assert '<tr class="mvp me"><td class=name><span class=crown>MVP</span>Duplicate<span class="tag you">你 / You</span>' in identity
        assert '教区' in preview and '码头' in preview and 'c5m1_waterfront' in preview
        assert 'Listen' in preview and '127.0.0.1:27015' in preview
        assert 'SteamID64: 76561198000000000' in preview
        assert 'data:image/bmp;base64,' in preview and preview.count('<img ') >= 2
        assert '187650' in preview and 'AK-47' in preview and '168.4' in preview
        assert '黑枪伤害 / Friendly-fire damage' in preview and '>42<' in preview
        assert '被黑枪伤害 / Friendly-fire damage received' in preview and '>17<' in preview
        assert '每 2 点伤害扣 1 分' in preview and '未参与评分' in preview
        assert '未记录' in (folder / 'legacy.html').read_text(encoding='utf-8')
        # Squad table + gold MVP styling; only successes, filters are All / MVP / non-MVP.
        assert preview.count('<table class=squad>') == 10
        assert preview.count('<span class=crown>MVP</span>') == 10 and preview.count('class="badge gold"') == 10
        assert preview.count('<tr class="mvp') == 10 and preview.count('data-mvp=1>') == 3 and preview.count('data-mvp=0>') == 7
        assert 'regoists<span class="tag you">' in preview and '<span class=tag>BOT</span>' in preview
        assert 'Teammate A' in preview and '1234.56' in preview and '80.34' in preview
        assert 'data-filter=1' in preview and 'data-filter=0' in preview
        assert '未完成' not in preview and 'data-result' not in preview
        assert '失败、团灭和断线不记录' not in preview and 'Evolve Beyond' not in preview
        assert '每 100 点伤害 1 分' in preview and 'Tank 伤害' in preview
        assert '&lt;b onclick=1&gt;&amp;mate&lt;/b&gt;' in escaped and '<b onclick' not in escaped
        missing = (folder / 'missing.html').read_text(encoding='utf-8')
        assert '旧版本' in missing and '<table class=squad>' not in missing
        assert '暂无成功章节记录' in (folder / 'empty.html').read_text(encoding='utf-8')
        partial = (folder / 'partial.html').read_text(encoding='utf-8')
        assert '&lt;script&gt;weapon&lt;/script&gt;' in partial and '<script>weapon' not in partial
        assert 'Steam 仅提供了部分字段' in partial and '<strong>--</strong>' in partial
        assert '当前未能读取 Steam 生涯数据' in (folder / 'empty.html').read_text(encoding='utf-8')
        if export_preview:
            destination = ROOT / 'archive/预览/主菜单报告'
            destination.mkdir(parents=True, exist_ok=True)
            shutil.copy2(folder / 'preview.html', destination / 'preview.html')
            desktop_candidates = [
                Path.home() / 'Desktop' / 'Skeeto_战报预览.html',
            ]
            for desktop in desktop_candidates:
                try:
                    if desktop.parent.is_dir():
                        shutil.copy2(folder / 'preview.html', desktop)
                        print('Desktop preview:', desktop)
                        break
                except OSError as error:
                    print('Desktop preview copy skipped:', error)
            print('Synthetic preview:', destination / 'preview.html')


if __name__ == '__main__':
    import sys
    main(export_preview="--export-preview" in sys.argv)
