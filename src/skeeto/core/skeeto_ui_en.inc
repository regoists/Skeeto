// Immutable English UI catalog. Format arguments must match the Chinese call site.
SKEETO_UI(MainUiSetupTitle, L"Custom main menu UI  v%s")
SKEETO_UI(MainUiEnabled, L"[1] Custom main menu %s")
SKEETO_UI(MainUiAccentSetup, L"[2] Accent color [%d / 6]")
SKEETO_UI(MainUiShowHome, L"[3] Return to custom main menu (game main menu only)")
SKEETO_UI(MainUiNativeSession, L"[4] Use stock menu for this visit")
SKEETO_UI(MainUiSetupHint, L"Experimental: mouse, arrows / Tab, Enter. Shortcuts are customizable in Main menu settings. Quick lobby opens the native campaign selection.")
SKEETO_UI(MainUiOverlayHint, L"Stock dialogs and console take priority. Shift+Tab suspends this UI; return to Skeeto after closing Steam Overlay. Disabled: no UI takeover.")
SKEETO_UI(MainUiServers, L"SERVER BROWSER")
SKEETO_UI(MainUiBackground, L"BACKGROUND")
SKEETO_UI(MainUiPlaybackPause, L"Pause")
SKEETO_UI(MainUiPlaybackResume, L"Resume")
SKEETO_UI(MainUiPlaybackProgress, L"MP4 progress")
SKEETO_UI(MainUiPlaybackUnavailable, L"MP4 unavailable")
SKEETO_UI(KbSecBgPause, L"Pause / resume the MP4 main-menu background")
SKEETO_UI(LastMusicEntry, L"[4] Auto-play Rainie Love when only one survivor remains [Settings]")
SKEETO_UI(TpsAimEntry, L"[5] Third Person Aim Fix [Settings]")
SKEETO_UI(TpsAimTitle, L"Third-person aim correction  v%s")
SKEETO_UI(TpsAimToggle, L"[1] Third-person aim correction %s")
SKEETO_UI(TpsAimWarning, L"    This feature uses core techniques highly similar to silent aim. It changes shot direction rather than the crosshair to enable a centered third-person crosshair. Experimental entertainment feature: even with optional AAC enabled, prefer the crosshair correction in Crosshair settings over this shot-direction correction.")
SKEETO_UI(TpsAimAacToggle, L"[2] Anti-Anti-Cheat lightweight countermeasures %s")
SKEETO_UI(TpsAimStatus, L"    Status: %s")
SKEETO_UI(TpsAimHint, L"    In the native shoulder camera, aim at the point under the screen center. First-person input stays unchanged.")
SKEETO_UI(TpsAimScope, L"    All held items share the same aim-direction correction. Normal spread, recoil and projectile motion remain. Nearby cover can still block the action.")
SKEETO_UI(TpsAimAacHint, L"    AAC defaults to OFF. When enabled, it eases parallax changes and pauses briefly after ray loss. Aim may leave the center during transitions; server acceptance is not guaranteed.")
SKEETO_UI(TpsAimMarkerHint, L"    Independent of Third-person crosshair. Reuses the static or texture style, or a small cross if neither is selected. With the third-person marker enabled, it shows the corrected shot ray.")
SKEETO_UI(TpsAimCommandHint, L"    Console: skeeto_tps_aim 0 to disable / 1 to enable / status to report; no argument reports status.")
SKEETO_UI(TpsAimOff, L"Disabled")
SKEETO_UI(TpsAimUnsupported, L"Unsupported game build; input unchanged")
SKEETO_UI(TpsAimConflict, L"Input interface changed; correction stopped")
SKEETO_UI(TpsAimFirstPerson, L"Waiting for the third-person shoulder camera")
SKEETO_UI(TpsAimPaused, L"Current player or scene is not eligible")
SKEETO_UI(TpsAimActive, L"Correcting aim direction")
SKEETO_UI(TpsAimGuarding, L"AAC protection: aim direction is transitioning")
SKEETO_UI(TpsAimRecovering, L"AAC protection: briefly using native input")
SKEETO_UI(TpsAimNoRay, L"Center ray unavailable; native input")
SKEETO_UI(TpsAimWaiting, L"Waiting for game and input interfaces")
SKEETO_UI(LastMusicTitle, L"Last-survivor Rainie Love music  v%s")
SKEETO_UI(LastMusicEnabled, L"[1] Auto-play Rainie Love %s")
SKEETO_UI(LastMusicMode, L"[2] Trigger [%s]")
SKEETO_UI(LastMusicVolume, L"[3] Independent volume [%d%%] - / +")
SKEETO_UI(LastMusicStep, L"[4] Volume step [%d%%] (1 / 5 / 10 / 20)")
SKEETO_UI(LastMusicReload, L"[5] Rescan the fixed track")
SKEETO_UI(LastMusicPathHint, L"Track: sound\\skeeto\\the last man\\Rainie_Love.mp3. GAME / VPK supported.")
SKEETO_UI(LastMusicBehaviorHint, L"Alive includes incapacitated and hanging survivors; standing excludes both. Self modes require the remaining survivor to be you. Teams with only one survivor are excluded. Checks once per second, plays once per new trigger. Disable auto-stop to finish the track. Volume is independent of the category mixer.")
SKEETO_UI(LastMusicMissing, L"Track unavailable. Install the file and rescan.")
SKEETO_UI(LastMusicReady, L"Track ready.")
SKEETO_UI(LastMusicPreparing, L"Preparing track.")
SKEETO_UI(LastMusicWipe, L"After team wipe")
SKEETO_UI(LastMusicOneAlive, L"Only one alive")
SKEETO_UI(LastMusicOneStanding, L"Only one standing")
SKEETO_UI(LastMusicSelfAlive, L"Only you alive")
SKEETO_UI(LastMusicSelfStanding, L"Only you standing")
SKEETO_UI(LastMusicMixerVolume, L"[5] Rainie Love music volume [%d%%] - / +")
SKEETO_UI(LastMusicMixerHint, L"Shares volume and step with Rainie Love settings (default 5%). Adjusting volume does not enable playback.")
SKEETO_UI(LastMusicAutoStop, L"[6] Stop when the condition no longer holds %s")
SKEETO_UI(LastMusicFooter, L"Number keys select; 0 returns")
SKEETO_UI(MainUiQuit, L"EXIT GAME")
SKEETO_UI(MainUiWelcome, L"Welcome to Skeeto")
SKEETO_UI(MainUiWelcomeSub, L"Make L4D2 yours.")
SKEETO_UI(MainUiPersonalize, L"HOMEPAGE SETTINGS")
SKEETO_UI(MainUiCancel, L"CANCEL")
SKEETO_UI(MainUiMap0, L"Dead Center")
SKEETO_UI(MainUiMap1, L"Dark Carnival")
SKEETO_UI(MainUiMap2, L"Swamp Fever")
SKEETO_UI(MainUiMap3, L"Hard Rain")
SKEETO_UI(MainUiMap4, L"The Parish")
SKEETO_UI(MainUiMap5, L"The Passing")
SKEETO_UI(MainUiMap6, L"The Sacrifice")
SKEETO_UI(MainUiMap7, L"No Mercy")
SKEETO_UI(MainUiMap8, L"Crash Course")
SKEETO_UI(MainUiMap9, L"Death Toll")
SKEETO_UI(MainUiMap10, L"Dead Air")
SKEETO_UI(MainUiMap11, L"Blood Harvest")
SKEETO_UI(MainUiMap12, L"Cold Stream")
SKEETO_UI(MainUiMap13, L"The Last Stand")
SKEETO_UI(SafeMusicTitle, L"Safe-room completion music  v%s")
SKEETO_UI(SafeMusicEntry, L"[8] Safe-room completion music [Settings]")
SKEETO_UI(SafeMusicCurrent, L"Selected: %.120s")
SKEETO_UI(SafeMusicFirstTrack, L"First track in the list")
SKEETO_UI(SafeMusicEnabled, L"[1] Custom completion music %s")
SKEETO_UI(SafeMusicMode, L"[2] Playback mode [%s]")
SKEETO_UI(SafeMusicRandom, L"Random")
SKEETO_UI(SafeMusicFixed, L"Fixed")
SKEETO_UI(SafeMusicListEntry, L"[3] Music list (%d tracks) [Settings]")
SKEETO_UI(SafeMusicVolume, L"[4] Independent volume [%d%%] - / + to adjust")
SKEETO_UI(SafeMusicStep, L"[5] Volume step [%d%%] (1 / 5 / 10 / 20)")
SKEETO_UI(SafeMusicReload, L"[6] Rescan music folder")
SKEETO_UI(SafeMusicPreview, L"[7] Preview current mode")
SKEETO_UI(SafeMusicStop, L"[8] Stop playback")
SKEETO_UI(SafeMusicFolderHint, L"Reads WAV/MP3 in sound/skeeto/safe/ from loose files and mounted VPKs. Fixed or random playback; random uses the whole list. Preview works when disabled.")
SKEETO_UI(SafeMusicBehaviorHint, L"Independent volume, unaffected by game music volume or the category master switch. Replaces safe-room completion music only; keeps stock music until ready.")
SKEETO_UI(SafeMusicScanFailed, L"Folder unavailable. Add files, then rescan.")
SKEETO_UI(SafeMusicEmpty, L"No playable music files.")
SKEETO_UI(SafeMusicTruncated, L"The list shows up to 128 tracks.")
SKEETO_UI(SafeMusicTrackRow, L"[%d] %.130s %s")
SKEETO_UI(SafeMusicSelectedMark, L"[Selected]")
SKEETO_UI(SafeMusicPager, L"Page %d / %d; Left / Right to turn pages, number to select")
SKEETO_UI(SafeMusicMixerVolume, L"[4] Safe-room music volume [%d%%] - / + to adjust")
SKEETO_UI(SafeMusicMixerHint, L"Shares its independent volume with the safe-room music page and uses that page's step (5% by default). Changes volume without enabling music.")
SKEETO_UI(LanguageAuto, L"Auto")
SKEETO_UI(LanguageChinese, L"Chinese")
SKEETO_UI(LanguageEnglish, L"English")
SKEETO_UI(LanguageFollow, L"Follow UI")
SKEETO_UI(MenuEnterSettingsFmt, L"[%d] %s [Settings]")
SKEETO_UI(LanguageMenuLabel, L"[1] Language [%s]")
SKEETO_UI(ColorWhite, L"White")
SKEETO_UI(ColorGreen, L"Green")
SKEETO_UI(ColorCyan, L"Cyan")
SKEETO_UI(ColorRed, L"Red")
SKEETO_UI(ColorYellow, L"Yellow")
SKEETO_UI(ColorOrange, L"Orange")
SKEETO_UI(ColorPink, L"Pink")
SKEETO_UI(ColorBlue, L"Blue")
SKEETO_UI(ColorLime, L"Lime")
SKEETO_UI(ColorMint, L"Mint")
SKEETO_UI(NoSelection, L"None selected")
SKEETO_UI(NoBackground, L"No backgrounds found")
SKEETO_UI(StyleOff, L"Off")
SKEETO_UI(FeedbackStyleWaiting, L"Waiting for assets")
SKEETO_UI(FeedbackStyleMissing, L"Assets not found")
SKEETO_UI(FeedbackWorkshopSection, L"-- Workshop assets --")
SKEETO_UI(FeedbackWorkshopRecovery, L"[7] Recover late Workshop assets %s")
SKEETO_UI(FeedbackWorkshopRecoveryHint, L"    OFF by default. Try this if Workshop assets are sometimes missing; reads them after addon loading completes. If the game defers mounting, return to its main menu to recover.")
SKEETO_UI(FeedbackPackLocalWarning, L"For reliability, install the Skeeto preset VPK directly in left4dead2\\addons\\ instead of relying only on its Workshop copy in addons\\workshop\\. Connecting too quickly may leave assets undiscovered.")
// L"[7] 空中加速 【400】"
SKEETO_UI(Text_00b15960, L"[7] Air acceleration [400]")
// L"[3] 强制拉满渲染距离 %s"
SKEETO_UI(Text_00e614ec, L"[3] Force maximum render distance %s")
// L"[2] 样式 【%s】"
SKEETO_UI(Text_011e1a66, L"[2] Style [%s]")
// L"血腥收获"
SKEETO_UI(Text_012171a0, L"Blood Harvest")
// L"    队友被 Boomer 胆汁命中后显示，感染解除时立即消失"
SKEETO_UI(Text_01330eae, L"    Appears when a teammate is hit by Boomer bile; disappears as soon as bile wears off")
// L"控制台 bind xxx skeeto_spotlight 可以把此功能绑定到自定义按键。"
SKEETO_UI(Text_013c78ba, L"Use bind xxx skeeto_spotlight in the console to assign a custom key.")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_01433a34, L"    Status: game cache interface unavailable; AV and entities remain independent")
// L"换弹 / 开火后摇环  v%s"
SKEETO_UI(Text_015d6812, L"Reload / fire recovery ring  v%s")
// L"暴风骤雨"
SKEETO_UI(Text_01e9e325, L"Hard Rain")
// L"圆环+十字"
SKEETO_UI(Text_021696c7, L"Ring + Cross")
// L"文件为空或读取失败。"
SKEETO_UI(Text_022dd99e, L"File is empty or could not be read.")
// L"[4] 整体 【%d%%】  `-`/`=`  字号 【%d】  Shift/Ctrl+`-`/`=`"
SKEETO_UI(Text_026b97c8, L"[4] Scale [%d%%]  `-`/`=`  Font [%d]  Shift/Ctrl+`-`/`=`")
// L"不含：聊天、投票、Admin 菜单、原版血条/武器、SKT 自绘 HUD"
SKEETO_UI(Text_02a480fa, L"Excludes chat, votes, Admin menus, stock health/weapons and SKT custom HUDs")
// L"[6] 文字颜色 【%s】"
SKEETO_UI(Text_02c5ec29, L"[7] Text color [%s]")
// L"圆环+点"
SKEETO_UI(Text_02ead771, L"Ring + Dot")
// L"[3] 自定义 HUD"
SKEETO_UI(Text_03a08d5f, L"[3] Custom HUD")
// L"    客户端视觉效果；本地服和联机服务器均可用，换图后会自动重设"
SKEETO_UI(Text_04349a9b, L"    Client-side visual effect; works locally and online, and is reapplied after map changes.")
// L"经典卡片"
SKEETO_UI(Text_045dc8f8, L"Classic Cards")
// L"[0] 关闭    [/Esc 关闭"
SKEETO_UI(Text_0485820e, L"[0] Close    [/Esc Close")
// L"当前文件预览（原样，最多 %d 行）："
SKEETO_UI(Text_04e2a1bf, L"Current file preview (unchanged, up to %d lines):")
// L"应用失败：名字不能为空或换行，且不能超过 23 个字符。"
SKEETO_UI(Text_05083e40, L"Apply failed: name must be 1-23 characters with no line breaks.")
// L"被 Jockey 骑乘"
SKEETO_UI(Text_052ac4b4, L"Ridden by Jockey")
// L"[7] 空中加速 【原版 10】"
SKEETO_UI(Text_05366b44, L"[7] Air acceleration [Vanilla 10]")
// L"危急生命状态生还者"
SKEETO_UI(Text_0548bbb1, L"Critically injured survivors")
// L"[5] 闲置接管后恢复武器槽 %s"
SKEETO_UI(Text_05ec9bec, L"[4] Restore weapon slot after idle %s")
// L"消音冲锋"
SKEETO_UI(Text_05f424aa, L"Silenced SMG")
// L"死亡中心"
SKEETO_UI(Text_0650cbc4, L"Dead Center")
// L"    受到自己伤害后显示，死亡立即消失"
SKEETO_UI(Text_0652ed96, L"    Appears after you deal damage; disappears on death")
// L"    开启选项后，会自动触发一次 smoker_alert_01.wav 作为音效检查。"
// L"    关闭后须关闭菜单，该项才会从画面消失"
SKEETO_UI(Text_071c202e, L"    Close the menu after disabling this item to hide it")
// L"青色"
SKEETO_UI(Text_0746d7b9, L"Cyan")
// L"闲置：%.19s"
SKEETO_UI(Text_07846cc7, L"AFK:%.19s")
// L"    持久化存储：跨地图和游戏重启保留，仅由 [7] 手动清零"
SKEETO_UI(Text_0894695c, L"    Persistent: keep across maps and restarts; reset manually with [7]")
// L"击杀统计 HUD  v%s"
SKEETO_UI(Text_08a19485, L"Kill statistics HUD  v%s")
// L"开启"
SKEETO_UI(Text_08c78382, L"Enable")
// L"    按 [4] 开关；按 - / + 切换客户端 / 服务器 / 双端"
SKEETO_UI(Text_0949d1b9, L"    Press [4] to toggle; - / + selects Client / Server / Both.")
// L"玩家3"
SKEETO_UI(Text_097c5863, L"Player3")
// L"[2] %s描边柔化范围 【%d%%】"
SKEETO_UI(Text_097ea996, L"[2] %sGlow softness range [%d%%]")
// L"    空爆 / 断舌 / Crown 等"
SKEETO_UI(Text_09e31d65, L"    Skeet / tongue cut / Crown, etc.")
// L"    把 cfg 放到 left4dead2/cfg/，文件名必须以 skeeto_ 开头（如 skeeto_preset1.cfg）。格式与原版 cfg 相同。"
SKEETO_UI(Text_0aae4096, L"    Place standard cfg files in left4dead2/cfg/ with names starting with skeeto_ (e.g. skeeto_preset1.cfg).")
// L"贴图准星  v%s"
SKEETO_UI(Text_0ab934ba, L"Texture crosshair  v%s")
// L"右上"
SKEETO_UI(Text_0ab9940d, L"Top Right")
// L"[5] 整体 【%d%%】  `-`/`=`"
SKEETO_UI(Text_0b10195e, L"[5] Scale [%d%%]  `-`/`=`")
// L"    分别设置八名角色的原版队友描边颜色，并可启用常驻显示"
SKEETO_UI(Text_0c2d6649, L"    Set vanilla teammate glow colors for all eight survivors and enable always-on glow.")
// L"考虑游戏平衡，写实模式下会限制此功能。"
SKEETO_UI(Text_0cc27bc1, L"Restricted in Realism for game balance.")
// L"[5] 背景颜色 【%s】"
SKEETO_UI(Text_0cd679d5, L"[6] Background color [%s]")
// L"现场凭证条：地面危险生成后开始倒计时"
SKEETO_UI(Text_0ce9617f, L"On-site duration bars count down once ground hazards appear")
// L"[5] 其他选项"
// L"[7] 粗细 【%d】"
SKEETO_UI(Text_0d357998, L"[7] Thickness [%d]")
// L"[2] 静态准星 %s"
SKEETO_UI(Text_0d802337, L"[2] Static crosshair [Open settings] %s")
// L"[1] %s描边亮度 【%d%%】"
SKEETO_UI(Text_0dbfc110, L"[1] %sGlow brightness [%d%%]")
// L"在场特感数量 HUD  v%s"
SKEETO_UI(Text_0dfdd98a, L"Active special infected count HUD  v%s")
// L"原版 alert"
SKEETO_UI(Text_0e079f78, L"Original Alert")
// L"一"
SKEETO_UI(Text_0eeab7b8, L"Mon")
// L"[2] Nextbot刷新率 【0.066 省CPU】"
SKEETO_UI(Text_0f5c7dcd, L"[2] NextBot update interval [0.066 Saves CPU]")
// L"    从上到下：投掷物、医疗包/电击器、止痛药/肾上腺素"
SKEETO_UI(Text_0fdbe35f, L"    Top to bottom: throwable, medkit/defibrillator, pills/adrenaline")
// L"    吃止痛药 / 打肾上腺素时闪一次绿屏"
SKEETO_UI(Text_1039ff3e, L"    Flashes green once when taking pain pills or adrenaline.")
// L"玩家2"
SKEETO_UI(Text_10676922, L"Player2")
// L"严格贴合散布"
SKEETO_UI(Text_109694c3, L"Exact Spread")
// L"[1] 启用过关自动执行 %s"
SKEETO_UI(Text_10b4b817, L"[1] Auto-run after chapter changes %s")
// L"[4] 字号 +2（循环）"
SKEETO_UI(Text_10c14e0c, L"[4] Font size +2 (cycle)")
// L"当前渲染后端：D3D9 持久双纹理（GPU路径）"
// L"【开】"
SKEETO_UI(Text_10ffed01, L"[ON]")
// L"自定义 HUD (%d/%d)  v%s"
SKEETO_UI(Text_11a743ec, L"Custom HUD (%d/%d)  v%s")
// L"关闭所有 Skeeto 自定义 HUD"
SKEETO_UI(Text_11db39f1, L"Hide all Skeeto custom HUDs")
// L"Tank 击杀提示"
SKEETO_UI(Text_11de06e3, L"Tank kill indicator")
// L"推击冷却 / 疲劳"
SKEETO_UI(Text_120fd9e5, L"Shove cooldown / fatigue")
// L"队友投掷物提示 HUD  v%s"
SKEETO_UI(Text_12441b4f, L"Teammate throwable alert HUD  v%s")
// L"    对应原版 dsp_off 1：在上一项的基础上把整体音效处理也一起关掉，效果更彻底；需先开启 [1]。"
SKEETO_UI(Text_12a07633, L"    Uses dsp_off 1 to disable overall sound processing in addition to the option above; requires [1].")
// L"    只改这页屏幕血条；头顶血条、被控提示不受影响"
SKEETO_UI(Text_12b846d8, L"    Only affects these screen health bars; overhead bars and pin alerts are unchanged")
// L"[6] 聚光灯夜视 【进入设置】 %s"
SKEETO_UI(Text_12c753c0, L"[5] Spotlight night vision [Settings] %s")
// L"Contain（完整显示）"
SKEETO_UI(Text_12ccf86c, L"Contain (Fit All)")
// L"[4] 特感名字 【%s】"
SKEETO_UI(Text_12d5572b, L"[4] SI / Director label language [%s]")
// L"竞技条"
SKEETO_UI(Text_13974bf8, L"Competitive Bar")
// L"右下"
SKEETO_UI(Text_13a2a54c, L"Bottom Right")
// L"Nick 尼克"
SKEETO_UI(Text_13a3f87f, L"Nick")
// L"砍刀"
SKEETO_UI(Text_13f55e5f, L"Machete")
// L"尸潮来袭"
SKEETO_UI(Text_1466adfe, L"Horde Incoming")
// L"墨绿"
SKEETO_UI(Text_1479f777, L"Dark Green")
// L"[2] 显示样式 【%s】"
SKEETO_UI(Text_14f381a3, L"[2] Display style [%s]")
// L"    显示本机当前时间，可调位置"
SKEETO_UI(Text_1528cbc8, L"    Current local time; movable")
// L"[1] 显示推击 HUD %s"
SKEETO_UI(Text_156ccf36, L"[1] Show shove HUD %s")
// L"强制移除地图狗屎滤镜  v%s"
SKEETO_UI(Text_15768c46, L"Force map color filters off  v%s")
// L"无法用系统默认程序打开该文件。"
SKEETO_UI(Text_1590173d, L"Could not open this file with the default application.")
// L"[3] 整体 【%d%%】  `-`/`=`  字号 【%d】  Shift/Ctrl+`-`/`=`"
SKEETO_UI(Text_164c26c2, L"[3] Scale [%d%%]  `-`/`=`  Font [%d]  Shift/Ctrl+`-`/`=`")
// L"    方向键挪环心相对准星的偏移（按住连移，Shift 大步）"
SKEETO_UI(Text_16ad7ace, L"    Arrow keys move the ring relative to the crosshair (hold to repeat, Shift for larger steps)")
// L"[3] 总画面开关 %s"
SKEETO_UI(Text_17738d49, L"[3] All feedback visuals %s")
// L"[4] 整体大小 【%d%%】  `-`/`=`"
SKEETO_UI(Text_179430d2, L"[4] Overall size [%d%%]  `-`/`=`")
// L"[3] 雷达 / 速度表 / 计时器 %s"
SKEETO_UI(Text_17b4c466, L"[3] Radar / speedometer / timer %s")
// L"本地听服调试功能；专服无效。"
SKEETO_UI(Text_1841999f, L"Listen-server debug features; no effect on dedicated servers.")
// L"Nick 使用场次"
SKEETO_UI(Text_1843e21d, L"Games as Nick")
// L"三方图"
SKEETO_UI(Text_18d473a8, L"Custom Map")
// L"章节重置、团灭不清零"
SKEETO_UI(Text_18dd6f3d, L"Reset on Chapter / Keep on Wipe")
// L"被胆汁影响的生还者"
SKEETO_UI(Text_192eeff5, L"Biled survivors")
// L"橙色"
SKEETO_UI(Text_1935d3fa, L"Orange")
// L"    阶段名与原版导演 HUD 相同；转阶段弹出或常驻，可在子页切换"
SKEETO_UI(Text_19647cf2, L"    Stock Director phase names; choose phase-change popups or always visible on the subpage")
// L"[4] 连杀音效 【%s】"
SKEETO_UI(Text_19937804, L"[4] Streak sound [%s]")
// L"[6] %s蓝色 B 【%d】"
SKEETO_UI(Text_1a05a889, L"[6] %sBlue B [%d]")
// L"榴弹发射器"
SKEETO_UI(Text_1a1c3f0d, L"Grenade Launcher")
// L"[1] 显示关卡名 HUD %s"
SKEETO_UI(Text_1aa7c28f, L"[1] Show map name HUD %s")
// L"—— SURVIVORS  二代角色记录 ——"
SKEETO_UI(Text_1afc5ad9, L"-- SURVIVORS: L4D2 CHARACTERS --")
// L"自己开的本地服可用（含大厅本地联机）。进别人的服会锁定。"
SKEETO_UI(Text_1b3af06b, L"Available on servers you host locally, including local lobby games. Locked on other servers.")
// L"一代连喷"
SKEETO_UI(Text_1c302d07, L"Auto Shotgun")
// L"Louis 路易斯"
SKEETO_UI(Text_1c33624d, L"Louis")
// L"导演压力 HUD  v%s"
SKEETO_UI(Text_1cd2b4f4, L"Director intensity HUD  v%s")
// L"额外的药包投掷物 HUD"
SKEETO_UI(Text_1cea098d, L"Extra healing / throwable HUD")
// L"    按块藏原版：队友框、武器弹药、雷达/速度/计时"
SKEETO_UI(Text_1cf2ae72, L"    Hide stock sections: teammate panels, weapons/ammo, radar/speed/timer")
// L"全向光照会增加渲染负担；当前优化为只照亮世界，不再逐个动态照亮角色模型。"
SKEETO_UI(Text_1d0457d9, L"Omnidirectional lighting adds rendering load; currently lights only the world, not individual character models.")
// L"    仅 Director/Panic Event；不包含胆汁吸引等所有尸潮"
SKEETO_UI(Text_1d6fd5c2, L"    Director/Panic events only; excludes other hordes such as bile-attracted infected")
// L"暂无可用的武器击杀记录"
SKEETO_UI(Text_1e183ad5, L"No weapon kill records available")
// L"章节不重置"
SKEETO_UI(Text_1e490c45, L"Keep Across Chapters")
// L"    只对特感/Tank，在模型外侧显示"
SKEETO_UI(Text_1ebb416d, L"    SI/Tank only, displayed beside their models")
// L"被 Smoker 吊在舌头上"
SKEETO_UI(Text_1ebd0525, L"Hanging from Smoker Tongue")
// L"[6] 去掉闲置延迟 %s"
SKEETO_UI(Text_1ecb10ad, L"[6] Remove idle delay %s")
// L"[9] 冷白预设"
SKEETO_UI(Text_1ed0f986, L"[9] Cool white preset")
// L"[7] 特感素材包 【%s】"
SKEETO_UI(Text_1f0cd1ec, L"[7] Special infected pack [%s]")
// L"    SERVER / CLIENT 对应原版 net_graph 的实际收包与发包频率"
SKEETO_UI(Text_1f1a80bd, L"    SERVER / CLIENT are actual receive/send packet rates, as in stock net_graph")
// L"惊扰女巫提示 HUD  v%s"
SKEETO_UI(Text_1f21b694, L"Witch startled alert HUD  v%s")
// L"关卡名 / 章节进度"
SKEETO_UI(Text_1f530bec, L"Map name / chapter progress")
// L"右键砖"
SKEETO_UI(Text_1f610c18, L"RMB Rock")
// L"完成终局"
SKEETO_UI(Text_1feef661, L"Finales completed")
// L"    关掉后须关闭菜单，该项才会从画面消失"
SKEETO_UI(Text_200846ae, L"    Close the menu after disabling this item to hide it")
// L"坦克 / 尸潮出现提示  v%s"
SKEETO_UI(Text_20479293, L"Tank / horde spawn alerts  v%s")
// L"队友黑白状态提示"
SKEETO_UI(Text_20ebe881, L"Teammate black-and-white indicator")
// L"[6] 禁止生成贴花 %s"
SKEETO_UI(Text_2122c59c, L"[6] Prevent decal creation %s")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_21274482, L"    Game cache usage, remaining virtual address space and current client entity count")
// L"    内置引擎层，用来判断散布"
SKEETO_UI(Text_21329731, L"    Drawn by the engine to indicate weapon spread.")
// L"因为无法照搬原有逻辑，skt从结果反推客户端实现，效果上可能无法完全还原。"
SKEETO_UI(Text_2171be0a, L"The original logic cannot be copied directly; SKT recreates the effects client-side, so results may differ.")
// L"    队友进入黑白时，屏幕左侧提示约 5 秒。默认关。"
SKEETO_UI(Text_21743745, L"    Left-side alert for about 5 seconds when a teammate turns black-and-white. Off by default.")
// L"[2] 整体 【%d%%】  `-`/`=`  字号 【%d】  Shift+`-`/`=`"
SKEETO_UI(Text_21a3e2e2, L"[2] Scale [%d%%]  `-`/`=`  Font [%d]  Shift+`-`/`=`")
// L"[3] 闲置接管后恢复视角 %s"
SKEETO_UI(Text_21c5a885, L"[3] Restore view direction after idle %s")
// L"橙"
SKEETO_UI(Text_22071f6c, L"Orange")
// L"[4] 环位置 【%s】  下→上→左→右"
SKEETO_UI(Text_2283d23c, L"[4] Ring position [%s]  Bottom -> Top -> Left -> Right")
// L"近战"
SKEETO_UI(Text_2283d3cc, L"Melee")
// L"进入地图后已暂停（上次主菜单：D3D9 GPU路径）"
// L"消防斧"
SKEETO_UI(Text_22b6cdb4, L"Fire Axe")
// L"[6] 全部%s"
SKEETO_UI(Text_22ef69e5, L"[6] %s all")
// L"推击冷却 / 疲劳  v%s"
SKEETO_UI(Text_234be0da, L"Shove cooldown / fatigue  v%s")
// L"[5] 连杀规则 【默认循环】"
SKEETO_UI(Text_2354993e, L"[5] Streak rules [Default cycle]")
// L"五"
SKEETO_UI(Text_23c0b483, L"Fri")
// L"肾上腺素持续  v%s"
SKEETO_UI(Text_23f114dd, L"Adrenaline duration  v%s")
// L"    监听 witch_harasser_set，显示玩家名或人机角色名约 4.5 秒；不用 userid，不扫描实体"
SKEETO_UI(Text_2409db0a, L"    Listens for witch_harasser_set; player or bot character name for about 4.5 seconds; no userid or entity scans")
// L"←/→ 翻页（方向键）"
SKEETO_UI(Text_240b755c, L"Left/Right arrows: change page")
// L"—— ARSENAL RECORDS  武器生涯 ——"
SKEETO_UI(Text_24752419, L"-- ARSENAL RECORDS --")
// L"[2] 显示时长 【%d ms】"
SKEETO_UI(Text_24c6e28a, L"[2] Display duration [%d ms]")
// L"清道夫对局"
SKEETO_UI(Text_254062f7, L"Scavenge matches")
// L"完整数字"
SKEETO_UI(Text_25627ddc, L"Full Numbers")
// L"四"
SKEETO_UI(Text_256be3fe, L"Thu")
// L"    队友血量持续在 39 及以下约 10 秒后提示约 5 秒。默认关。"
SKEETO_UI(Text_257675b8, L"    A 5-second alert after a teammate stays at 39 HP or below for about 10 seconds. Off by default.")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_25df8613, L"[1] Memory HUD %s")
// L"幸存者 惊扰了 Witch"
SKEETO_UI(Text_265aa3d6, L"A survivor startled the Witch")
// L"    与工坊现有所有准星mod都兼容，也可自行DIY新增"
SKEETO_UI(Text_266fd4b0, L"    Compatible with existing Workshop crosshair mods; custom additions are also supported.")
// L"本地服体验优化 (%d/%d)  v%s"
SKEETO_UI(Text_26917579, L"Local server enhancements (%d/%d)  v%s")
// L"三类可同时开。用数字进子页，开菜单时不会切枪"
// L"各项独立于常态角色描边；关闭接管后立即恢复启用前的数值。"
SKEETO_UI(Text_27c0c340, L"Independent of normal character glow; disabling overrides immediately restores previous values.")
// L"    即sv_airaccelerate，影响空中转向速度"
SKEETO_UI(Text_280c5e2a, L"    sv_airaccelerate: affects airborne turning speed.")
// L"换人物未开启（本地服页先打开开关）"
SKEETO_UI(Text_2823cc4e, L"Character switching is off (enable it on the local server page).")
// L"惊扰女巫"
SKEETO_UI(Text_284d8661, L"Witch Startled")
// L"砸中按扔者灭；不扫石头实体。"
SKEETO_UI(Text_28bf42c7, L"Clears the thrower's alert on impact; no rock entity scans.")
// L"    队友道具栏从有投掷物变为空时显示玩家 ID 与类型约 5 秒；不依赖 weapon_fire，不扫描实体"
SKEETO_UI(Text_29b98d07, L"    Shows a confirmed teammate projectile and its thrower for about 5 seconds; item transfers do not trigger it; no weapon_fire or entity scans")
// L"被控"
SKEETO_UI(Text_2a093420, L"Pinned")
// L"[4] Ellis 艾利斯"
SKEETO_UI(Text_2a213154, L"[4] Ellis")
// L"    黄色余量从顶部向下收缩，避免遮挡特感模型和血条。"
SKEETO_UI(Text_2a598591, L"    Yellow fill shrinks downward from the top, avoiding the SI model and health bar.")
// L"生还者回合"
SKEETO_UI(Text_2a71013b, L"Survivor rounds")
// L"    队友击杀 Witch 时，在击杀者头顶显示约 5 秒金色奖杯"
SKEETO_UI(Text_2bac349b, L"    A gold trophy above a teammate who kills a Witch, for about 5 seconds")
// L"    不改:完全不碰 tick/速率,可与本地插件共存.档位 不改/30/60/100/128"
SKEETO_UI(Text_2baeb012, L"    Unchanged: leaves tick/rates untouched for local plugin compatibility. Options: Unchanged/30/60/100/128")
// L"显示近战轨迹线"
SKEETO_UI(Text_2c0e5c0b, L"Show melee trace lines")
// L"    主功能关闭时，本页的其它设置不会产生任何影响。"
SKEETO_UI(Text_2c23d766, L"    Other settings on this page have no effect while the main feature is disabled.")
// L"[4] 强制静态放大手电筒、关闭枪火与后座 %s"
SKEETO_UI(Text_2c2cf271, L"[4] Force wide static flashlight; disable muzzle flash/recoil %s")
// L"直接使用原版颜色 CVar；不改描边距离、透视规则或实体状态。"
SKEETO_UI(Text_2c679ba6, L"Uses stock color CVars; leaves glow distance, visibility rules and entity state unchanged.")
// L"简洁版导演 HUD"
SKEETO_UI(Text_2c861b8b, L"Compact Director HUD")
// L"切换准星  v%s"
SKEETO_UI(Text_2c98cb14, L"Crosshairs  v%s")
// L"队友血条"
SKEETO_UI(Text_2cdb9a5f, L"Teammate health bars")
// L"    只藏画面，数字键切枪 / 用道具不受影响"
SKEETO_UI(Text_2cf9f641, L"    Visuals only; number-key weapon switching and item use still work")
// L"    客户端无法改变服务器内人机的行为，这里只是把手电的开关与亮度对齐到你闲置前的样子。"
SKEETO_UI(Text_2d2b9054, L"    The client cannot change server bot behavior; this only matches flashlight state and brightness to your pre-idle settings.")
// L"肾上腺素  使用 / 分享"
SKEETO_UI(Text_2d71d977, L"Adrenaline  Used / Shared")
// L"    两项均关闭时不采样、不绘制；开启后每 0.5 秒检查一次 32 个玩家槽位。"
SKEETO_UI(Text_2d77327e, L"    No sampling or drawing when both are off; otherwise checks 32 player slots every 0.5 seconds.")
// L"更新暂存不完整：启动前更新脚本或核心文件不可用。\n" 			L"Update staging is incomplete; Skeeto will retry."
SKEETO_UI(Text_2da3309e, L"Update staging is incomplete: startup updater or core files unavailable.\nSkeeto will retry.")
// L"精简"
SKEETO_UI(Text_2da3cd40, L"Compact")
// L"Coach 教练"
SKEETO_UI(Text_2db167d6, L"Coach")
// L"    反馈音量 【%d%%】  按 - / = 调节"
SKEETO_UI(Text_2e13bda0, L"    Feedback volume [%d%%]  Adjust with - / =")
// L"[2] 连杀判定时间 【无】"
SKEETO_UI(Text_2e314c4b, L"[2] Streak window [None]")
// L"[3] 关闭特感布偶/尸体 %s"
SKEETO_UI(Text_2e52ac0c, L"[2] Disable special infected ragdolls/bodies %s")
// L"Tank 举石预警 HUD"
SKEETO_UI(Text_2e869c8b, L"Tank rock windup alert HUD")
// L"靠左"
SKEETO_UI(Text_2e96fb4d, L"Left")
// L"    先把准星对着实体，再按下此选项"
SKEETO_UI(Text_2f046268, L"    Aim at an entity first, then select this option.")
// L"    相对 autoexec.cfg：启动只跑一次；这里每个新章节再 exec 当前那一套，局内可切换/绑定 skt_cfg_next。"
SKEETO_UI(Text_2f36c54d, L"    autoexec.cfg runs once at startup; this runs the selected preset each chapter. Switch in-game or bind skt_cfg_next.")
// L"    开启后只显示血条，不显示名字和血量数字"
SKEETO_UI(Text_2fed61ea, L"    Show only the bar, without names or health numbers")
// L"[1] 刷新 Steam 生涯数据"
SKEETO_UI(Text_303c716f, L"[1] Refresh Steam career data")
// L"[1] 输出 debug 日志 skeeto.log %s"
SKEETO_UI(Text_305b68ab, L"[1] Write debug log to skeeto.log %s")
SKEETO_UI(DebugLogLaunchHint, L"    -skeeto_debug forces logging for this launch. Remove the launch option and restart to use this switch.")
// L"军用狙击"
SKEETO_UI(Text_30aed739, L"Military Sniper")
// L"[1] 显示惊扰女巫提示 HUD %s"
SKEETO_UI(Text_30d643ce, L"[1] Show Witch startled alert HUD %s")
// L"关闭空间混响与回声等环境音效果；关闭选项后恢复启用前的值。"
SKEETO_UI(Text_30e4c247, L"Disables reverb, echo, and similar effects; disabling restores prior values.")
// L"止痛药"
SKEETO_UI(Text_31585ad1, L"Pain Pills")
// L"有趣的调试 Cvar (%d/%d)  v%s"
SKEETO_UI(Text_31ce7cfe, L"Fun debug Cvars (%d/%d)  v%s")
// L"    举石起手就闪，并标出 E砖 / 右键砖 / R砖；砸中或打碎提前灭；默认关"
SKEETO_UI(Text_3282f26d, L"    Flashes at rock windup; labels E / right-click / R throws; clears early on impact or destruction; off by default")
// L"[2] 统计模式 【%s】"
SKEETO_UI(Text_32ae4c6e, L"[2] Counter mode [%s]")
// L"    开启时隐藏原版 net_graph；关闭后恢复开启前的档位"
SKEETO_UI(Text_333620ea, L"    Hides stock net_graph while enabled; restores its previous level when disabled")
// L"    选 0 请先开上面的「允许 0 lerp」"
SKEETO_UI(Text_33397679, L"    Enable 'Allow 0 lerp' above before selecting 0.")
// L"胆汁"
SKEETO_UI(Text_3341288f, L"Bile Jar")
// L"    例：Ellis（2）被 Hunter 扑倒，并显示攻击者"
SKEETO_UI(Text_3343b3ca, L"    Example: Ellis (2) pinned by Hunter; also shows the attacker")
// L"靠右"
SKEETO_UI(Text_335ba971, L"Right")
// L"回合时间"
SKEETO_UI(Text_33bb0588, L"Round Time")
// L"使用电击器"
SKEETO_UI(Text_3459630b, L"Defibrillators used")
// L"Tank 举石预警 HUD  v%s"
SKEETO_UI(Text_3494d809, L"Tank rock windup alert HUD  v%s")
// L"死亡"
SKEETO_UI(Text_34b1486b, L"Dead")
// L"纯黑"
SKEETO_UI(Text_359bc4a7, L"Black")
// L"    不改变三类准星的开关状态；控制台指令：skeeto_xhair_toggle"
SKEETO_UI(Text_35c8efd7, L"    Preserves each crosshair type's enabled state; console command: skeeto_xhair_toggle")
// L"[1] 显示导演压力 HUD %s"
SKEETO_UI(Text_363934ad, L"[1] Show Director intensity HUD %s")
// L"命中反馈 (1/2)  v%s"
SKEETO_UI(Text_36c7273e, L"Hit feedback (1/2)  v%s")
// L"现版本采用全向动态光源实现，动态光源的性能不好，只建议高配电脑使用此选项。"
SKEETO_UI(Text_37077535, L"Uses an omnidirectional dynamic light. Dynamic lighting is costly; recommended only for powerful PCs.")
// L"[8] 数字显示 【%s】"
SKEETO_UI(Text_37102588, L"[8] Number format [%s]")
// L"有惩罚或冷却时才显示。钻石格=疲劳进度，灯芯=下次可推"
SKEETO_UI(Text_3729920e, L"Visible during penalties or cooldowns. Diamonds: fatigue; wick: next shove")
// L"[7] 空中加速 【1000】"
SKEETO_UI(Text_37fa1954, L"[7] Air acceleration [1000]")
// L"[1] 显示世界时间 %s"
SKEETO_UI(Text_38900053, L"[1] Show real-world clock %s")
// L"    （画面与音效可分开选，互不影响）"
SKEETO_UI(Text_38d6b1f0, L"    (Visuals and sounds can be selected independently)")
// L"%s 惊扰了 Witch"
SKEETO_UI(Text_391e1347, L"%s startled the Witch")
// L"三方图  第%d/%d关"
SKEETO_UI(Text_3a60e4ed, L"Custom Map  Chapter %d/%d")
// L"[2] 显示在场 Smoker 数量 %s"
SKEETO_UI(Text_3ad1c6c1, L"[2] Show active Smoker count %s")
// L"[3] 复位位置（准星下方）"
SKEETO_UI(Text_3b22a7a9, L"[3] Reset position (below crosshair)")
// L"点"
SKEETO_UI(Text_3b27f5aa, L"Dot")
// L"自己按麦、队友按麦都会显示。原版同一套开麦状态，喇叭 + 玩家 ID，没有底框。"
SKEETO_UI(Text_3b5bcc7a, L"Shows your and teammates' stock voice activity: speaker icon and player ID, without a background.")
// L"方法二：使用控制台指令"
SKEETO_UI(Text_3b8101ce, L"Method 2: Use the console command")
// L"    只影响枪械开火声，不影响游戏音量、音乐音量和其它声音；按 [6] 恢复100%"
// L"队友血条  v%s"
SKEETO_UI(Text_3bd8812d, L"Teammate health bars  v%s")
// L"[1] 关闭空间混响与回声环境音 %s"
SKEETO_UI(Text_3c038245, L"[1] Disable environmental reverb and echo %s")
// L"    随上方功能生效；按 - / + 调节，按 [5] 恢复 0.25。"
SKEETO_UI(Text_3c12a683, L"    Requires the option above; press - / + to adjust, [5] to restore 0.25.")
// L"圆环"
SKEETO_UI(Text_3c9a35e5, L"Ring")
// L"强制 Cvar 修改  v%s"
SKEETO_UI(Text_3cdb5225, L"Force Cvar overrides  v%s")
// L"    原版左下角自己和队友的血条框"
SKEETO_UI(Text_3d0700bc, L"    Stock bottom-left health panels for yourself and teammates")
// L"未选择"
SKEETO_UI(Text_3d33b46e, L"Not selected")
// L"%d连杀"
SKEETO_UI(Text_3d63bccd, L"%d Kill Streak")
// L"你已订阅 Skeeto 工坊更新（%hs → %hs），是否安装新版本？\n\n" 			L"You have a Skeeto Workshop update (%hs → %hs). Install now?\n\n" 			L"双轨：会更新 skeeto.dll / necola.dll（若存在），并同步根目录说明与更新日志。\n" 			L"Both tracks: updates DLLs and root readme/changelog files when present."
// L"    局内改完之后要加载一次才会生效"
SKEETO_UI(Text_3e171a69, L"    Changes made in-game require a map load to take effect.")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_3e5b1316, L"    Status: reads game interfaces when the HUD or automatic cleanup is enabled")
// L"完整"
SKEETO_UI(Text_3e616b8c, L"Full")
// L"Ellis 惊扰了 Witch"
SKEETO_UI(Text_3eb17d28, L"Ellis startled the Witch")
// L"已切换人物：%s"
SKEETO_UI(Text_3f0ac02e, L"Character switched: %s")
// L"终局救援"
SKEETO_UI(Text_3f855867, L"Finale Rescue")
// L"[1] 显示在场特感总数 %s"
SKEETO_UI(Text_3fc5a8a7, L"[1] Show active SI total %s")
// L"显示呕吐判定调试（极其容易掉帧）"
SKEETO_UI(Text_3ff9ef59, L"Show vomit hit-detection debug (severe FPS impact)")
// L"[0] 关闭"
SKEETO_UI(Text_402e6a02, L"[0] Close")
// L"方框"
SKEETO_UI(Text_4073da1a, L"Square")
// L"在场特感数量 HUD"
SKEETO_UI(Text_40a4abe2, L"Active special infected count HUD")
// L"[4] 击中反馈(hit)单独开关 %s"
SKEETO_UI(Text_40ba58f1, L"[4] Hit feedback toggle %s")
// L"[8] Louis 路易斯"
SKEETO_UI(Text_4147146c, L"[8] Louis")
// L"猎枪"
SKEETO_UI(Text_414e5dc6, L"Hunting Rifle")
// L"全队"
SKEETO_UI(Text_418069d1, L"Whole team")
// L"无声特感补声设置  v%s"
SKEETO_UI(Text_41862396, L"Silent special infected sound cues  v%s")
// L"隐藏 EMS 脚本 HUD  v%s"
SKEETO_UI(Text_4208449e, L"Hide EMS script HUD  v%s")
// L"T字"
SKEETO_UI(Text_42512f45, L"T")
// L"    普感击杀/爆头/Hit 用哪一套"
SKEETO_UI(Text_42889ed1, L"    Assets for common infected kills/headshots/hits")
// L"    局内：%s"
SKEETO_UI(Text_42d1e19a, L"    In game: %s")
// L"—— 3. 选素材包 ——"
SKEETO_UI(Text_42da727e, L"-- 3. Choose asset packs --")
// L"[5] Bill 比尔"
SKEETO_UI(Text_42e8ed46, L"[5] Bill")
// L"[1] 禁用所有色彩校正 %s"
SKEETO_UI(Text_4300f822, L"[1] Disable all color correction %s")
// L"    喷吐落地与 Spitter 死亡时生成的酸液都会计时"
SKEETO_UI(Text_43628aae, L"    Times acid from both landed spit and Spitter deaths")
// L"[8] 暖白预设"
SKEETO_UI(Text_4372f4b3, L"[8] Warm white preset")
// L"    SKT的此功能不依赖-lv启动项也可以使用。"
// L"世界HUD (%d/%d)  v%s"
SKEETO_UI(Text_4426d51a, L"World HUD (%d/%d)  v%s")
// L"棒球棍"
SKEETO_UI(Text_447c643c, L"Baseball Bat")
// L"    关闭地图雨雪粒子；关闭选项后恢复启用前的值。"
SKEETO_UI(Text_44a5b7d7, L"    Disables map rain and snow particles; restores prior values when disabled.")
// L"当前渲染后端：Surface兼容回退"
SKEETO_UI(Text_45070a49, L"Renderer: Source Surface")
// L"[2] 显示模式 【%s】"
SKEETO_UI(Text_450a3afe, L"[2] Display mode [%s]")
// L"此页选项在联机服务器内也会生效。"
SKEETO_UI(Text_4517a787, L"Options on this page also work on online servers.")
// L"—— 布局 ——"
SKEETO_UI(Text_45346acf, L"-- Layout --")
// L"Steam 生涯数据暂不可用"
SKEETO_UI(Text_454beed5, L"Steam career data is currently unavailable")
// L"1 转阶段后 5 秒"
SKEETO_UI(Text_4563d3db, L"1 For 5 seconds after phase change")
// L"    恢复原版：skeeto_bot_name_%hs reset"
SKEETO_UI(Text_459035be, L"    Restore default: skeeto_bot_name_%hs reset")
// L"砍断 Smoker 舌头"
SKEETO_UI(Text_45ba8f75, L"Cut Smoker tongue")
// L"只读访问 Steam 已缓存的 L4D2 统计，不修改、不上传数据"
SKEETO_UI(Text_45cf3212, L"Reads cached Steam L4D2 stats only; no data is modified or uploaded")
// L"玩家4"
SKEETO_UI(Text_463dcea4, L"Player4")
// L"仅在队友真正投掷燃烧瓶、胆汁瓶或土质炸弹后显示玩家名字和投掷物类型，持续约 5 秒。"
SKEETO_UI(Text_46438e18, L"Shows player name and throwable type for about 5 seconds after a teammate throws a Molotov, bile jar or pipe bomb.")
// L"← 上一页（←键）"
SKEETO_UI(Text_465c2dbc, L"<- Previous page (Left arrow)")
// L"[4] 背景透明度 【%d%%】  也可按 - / = 调节"
SKEETO_UI(Text_4664d620, L"[5] Background opacity [%d%%]  Also adjust with - / =")
// L"地面燃烧"
SKEETO_UI(Text_46733245, L"Ground Fire")
// L"坦克出现"
SKEETO_UI(Text_467dd523, L"Tank Spawned")
// L"高爆弹 / 燃烧弹 / 激光 HUD"
SKEETO_UI(Text_469fd89c, L"Explosive / incendiary ammo / laser HUD")
// L"    射击后延迟播放弹壳落地声（需 sound/sis/* 素材；对应工坊 Shell Impact Sounds）"
SKEETO_UI(Text_46a57202, L"    Plays delayed casing impact sounds after firing (requires sound/sis/* assets; Workshop: Shell Impact Sounds).")
// L"绿"
SKEETO_UI(Text_46c78c33, L"Green")
// L"[8] 字号 【%d】"
SKEETO_UI(Text_4777ba0b, L"[9] Font size [%d]")
// L"[1] 显示队友血条 %s"
SKEETO_UI(Text_4792beed, L"[1] Show teammate health bars %s")
// L"    只切换前三项；进度条/提示条请单独开"
SKEETO_UI(Text_484284f8, L"    Toggles only the first three; enable progress/hint bars separately")
// L"被 Charger 殴打"
SKEETO_UI(Text_484aed0e, L"Pummeled by Charger")
// L"[1] 特感出生提示 HUD %s"
SKEETO_UI(Text_488f3fb2, L"[1] SI spawn alert HUD %s")
// L"    标题字号 【%d】 Shift/Ctrl+`-`/`=`  数字字号 【%d】 Ctrl+`-`/`=`"
SKEETO_UI(Text_48b5fe2e, L"    Title font [%d] Shift/Ctrl+`-`/`=`  Number font [%d] Ctrl+`-`/`=`")
// L"Steam 生涯统计未提供可验证的普感与特感分类击杀数据。"
SKEETO_UI(Text_48bfa6ad, L"Steam career stats do not provide verifiable separate kill counts for common and special infected.")
// L"[5] 字号 【%d】  Shift/Ctrl+`-`/`=`"
SKEETO_UI(Text_48cbefdc, L"[5] Font [%d]  Shift/Ctrl+`-`/`=`")
// L"[1] 显示计时器 %s"
SKEETO_UI(Text_4901d872, L"[1] Show timer %s")
// L"[6] Config Override 过关之后自动执行CFG文件 【进入设置】 %s"
SKEETO_UI(Text_49462b02, L"[7] Config Override: run CFG after chapter changes [Settings] %s")
// L"【仅特感】"
SKEETO_UI(Text_499b751d, L"[Special infected only]")
// L"木喷"
SKEETO_UI(Text_49a41c07, L"Pump Shotgun")
// L"    不改变各世界 HUD 的开关状态；控制台指令：skeeto_world_hud_toggle"
SKEETO_UI(Text_49a6b371, L"    Keeps individual world HUD settings; console: skeeto_world_hud_toggle")
// L"    可单独开：主弹药 HUD 关掉后，副枪弹匣仍可显示在准星旁"
SKEETO_UI(Text_49fb3a6e, L"    Shown only while holding a sidearm; works independently of the primary ammo switch.")
// L"显示Tank石头飞行轨迹线"
SKEETO_UI(Text_4a10cb8f, L"Show Tank rock trajectory")
// L"%s即将抵达观战席"
SKEETO_UI(Text_4a2f9073, L"%s is heading to spectator mode")
// L"行动受限"
SKEETO_UI(Text_5b7e4c19, L"Movement Slowed")
// L"只覆盖生还者 BOT 的角色名称；真人占用同一角色时仍显示真人自己的玩家名。"
SKEETO_UI(Text_4a822f28, L"Only overrides survivor BOT names; human players using the same character keep their own player names.")
// L"左上"
SKEETO_UI(Text_4a90093f, L"Top Left")
// L"[8] 队友血条最多 【%d】"
SKEETO_UI(Text_4ab6896a, L"[8] Maximum teammate bars [%d]")
// L"【全部关闭】"
SKEETO_UI(Text_4ab87984, L"[All off]")
// L"近处物品"
SKEETO_UI(Text_4adf48f8, L"Nearby items")
// L"[3] 整体大小 【%d%%】  `-`/`=`"
SKEETO_UI(Text_4b2413bb, L"[3] Overall size [%d%%]  `-`/`=`")
// L"    目标 %d - 当前约 %d - 过关后生效"
SKEETO_UI(Text_4bd0450f, L"    Target %d - Current ~%d - Applies after chapter change")
// L"被控信息"
SKEETO_UI(Text_4c1705c9, L"Pinned survivor info")
// L"高爆弹 / 燃烧弹 / 激光 HUD  v%s"
SKEETO_UI(Text_4cf125a1, L"Explosive / incendiary ammo / laser HUD  v%s")
// L"服务器"
SKEETO_UI(Text_4d061b47, L"Server")
// L"[1] 显示击杀列表 HUD %s"
SKEETO_UI(Text_4d19730f, L"[1] Show kill feed HUD %s")
// L"关闭所有 Skeeto 世界 HUD"
SKEETO_UI(Text_4d197a17, L"Hide all Skeeto world HUDs")
// L"救援车已到"
SKEETO_UI(Text_4d42322d, L"Rescue Arrived")
// L"    用于临时测试枪械实际散布，关闭后恢复原值"
SKEETO_UI(Text_4dc365f8, L"    Temporarily tests actual weapon spread; disabling restores prior values.")
// L"未在本地主机，所有项目已锁定。"
SKEETO_UI(Text_4edadd9f, L"Not hosting locally; all items are locked.")
// L"白"
SKEETO_UI(Text_4f387c3f, L"White")
// L"    藏 itempickup 里那一套（拾取提示会一起藏）"
SKEETO_UI(Text_4f6358ab, L"    Hides the itempickup group, including pickup notifications")
// L"显示舌头判定调试"
SKEETO_UI(Text_4f6f31c8, L"Show tongue hit-detection debug")
// L"复用原版描边，只固定队友描边透明度；不创建模型轮廓、不写实体。"
SKEETO_UI(Text_4fa0bd38, L"Uses stock glow and fixes only teammate glow opacity; creates no model outlines and writes no entities.")
// L"击杀 Tank（Tank Slayer）"
SKEETO_UI(Text_502222e0, L"Kill Tank (Tank Slayer)")
// L"关键保护提示HUD"
SKEETO_UI(Text_506eec02, L"Critical rescue alert HUD")
// L"被特感控制"
SKEETO_UI(Text_50a638e0, L"Pinned by Special Infected")
// L"电锯"
SKEETO_UI(Text_50b93562, L"Chainsaw")
// L"[2] 闲置后恢复手电开关状态 %s"
SKEETO_UI(Text_50c7d3b1, L"[2] Restore flashlight state after idle %s")
// L"两次特感击杀在设定窗口内视为连杀，第 3 杀起显示约 2 秒"
SKEETO_UI(Text_50dac82e, L"SI kills within the selected window form a streak; shows for about 2 seconds from kill 3")
// L"特感默认"
SKEETO_UI(Text_51154652, L"SI Default")
// L"Ellis 使用场次"
SKEETO_UI(Text_515280f1, L"Games as Ellis")
// L"高爆包"
SKEETO_UI(Text_5177aadc, L"Explosive Ammo")
// L"—— 4. 特感反馈怎么播 ——"
SKEETO_UI(Text_51a55f5c, L"-- 4. Special infected feedback playback --")
// L"[9] 五项颜色全部恢复启用前原值"
SKEETO_UI(Text_524074ac, L"[9] Restore all five colors to previous values")
// L"[7] 空中加速 【100】"
SKEETO_UI(Text_528976c7, L"[7] Air acceleration [100]")
// L"危"
SKEETO_UI(Text_52abf014, L"!")
// L"显示特感可生成区域"
SKEETO_UI(Text_534e0aac, L"Show special infected spawn areas")
// L"关键保护 / 解控"
SKEETO_UI(Text_538a25d3, L"Critical rescue / freeing")
// L"左下"
SKEETO_UI(Text_538b387e, L"Bottom Left")
// L"    关闭空间混响与回声等环境音效果，让脚步和特感的声音更容易分辨。"
// L"急救包  使用 / 分享"
SKEETO_UI(Text_53c28fef, L"First aid kits  Used / Shared")
// L"    有人按麦时在准星附近显示喇叭和真人玩家 ID，无底框；自己按麦也会显示。默认关。"
SKEETO_UI(Text_53e3626c, L"    Speaker icon and human player ID near the crosshair while speaking, including yourself. No background; off by default.")
// L"状态 【本地听服主机 - 可改】"
SKEETO_UI(Text_5454e9d0, L"Status [Local host - editable]")
// L"[1] 整体大小 【%d%%】"
SKEETO_UI(Text_545dbefe, L"[2] Overall size [%d%%]")
// L"被 Hunter 扑倒"
SKEETO_UI(Text_5571395a, L"Pounced by Hunter")
// L"三方图  第%d关"
SKEETO_UI(Text_55c6fb10, L"Custom Map  Chapter %d")
// L"    燃烧瓶和胆汁瓶须同时开启「投掷物落点预览」；酸液直接监听落地事件"
SKEETO_UI(Text_567ea77b, L"    Molotov and bile require Throwable landing preview; acid uses landing events directly")
// L"[1] 从剪贴板应用新名字"
SKEETO_UI(Text_56aad20a, L"[1] Apply new name from clipboard")
// L"教区"
SKEETO_UI(Text_56f58412, L"The Parish")
// L"    推击惩罚钻石格 + 冷却灯芯；还可推几下到疲劳"
SKEETO_UI(Text_57732c5b, L"    Shove penalty diamonds and cooldown indicator; shows shoves left before fatigue")
// L"队友"
SKEETO_UI(Text_5778ab6e, L"Teammate")
// L"找不到 Skeeto 更新日志。\n请重新运行安装程序，或将 Skeeto_更新日志.txt 放到游戏根目录。"
SKEETO_UI(Text_57decb82, L"Skeeto changelog not found.\nRun the installer again, or put Skeeto_更新日志.txt in the game root.")
// L"[1] 显示开麦提示 HUD %s"
SKEETO_UI(Text_580cf7b3, L"[1] Show voice activity HUD %s")
// L"[1] 关闭所有 Skeeto 准星 %s"
SKEETO_UI(Text_58228105, L"[1] Hide all Skeeto crosshairs %s")
// L"    关=关闭特感命中/击杀反馈，不跟普感"
SKEETO_UI(Text_584a2c77, L"    Off: no SI hit/kill feedback; no CI fallback")
// L"    开启后自动播放一次音效检查。"
SKEETO_UI(Text_58535746, L"    Enabling this plays a sound check once.")
// L"    紫=换弹进度（闲置换弹也能检测） 青=开火后摇/切枪前摇（喷推/狙推也能检测）。只读取当前武器。"
SKEETO_UI(Text_58bd6aaf, L"    Purple: reload (including idle reload); cyan: fire recovery/equip delay (including shotgun/sniper shoves). Current weapon only.")
// L"[4] 透明度 【%d%%】"
SKEETO_UI(Text_5931784e, L"[4] Opacity [%d%%]")
// L"    强制使用全亮渲染；部分服务器或地图可能限制该效果。"
SKEETO_UI(Text_594951d9, L"    Forces fullbright rendering; some servers or maps may restrict it.")
// L"Coach 使用场次"
SKEETO_UI(Text_5a93760f, L"Games as Coach")
// L"[3] 颜色 【%s】"
SKEETO_UI(Text_5a9428d1, L"[3] Color [%s]")
// L"[4] 整体大小 【%d%%】"
SKEETO_UI(Text_5b90cefa, L"[4] Overall size [%d%%]")
// L"统计当前已经生成且存活的特感玩家；未出生的灵魂特感不计入。Tank 计入特感总数。"
SKEETO_UI(Text_5bcb188a, L"Counts spawned, living SI players; excludes unspawned ghosts. Tanks count toward the SI total.")
// L"[2] 胆汁瓶地面倒计时 %s"
SKEETO_UI(Text_5c2a7d50, L"[2] Bile ground countdown %s")
// L"等待救援车"
SKEETO_UI(Text_5c760429, L"Awaiting Rescue")
// L"[1] 开启原版导演 HUD %s"
SKEETO_UI(Text_5cd1df2e, L"[1] Enable vanilla Director HUD %s")
// L"特感预警 HUD  v%s"
SKEETO_UI(Text_5d3e771d, L"Special infected alert HUD  v%s")
// L"[7] 标题颜色 【%s】"
SKEETO_UI(Text_5d6bc90f, L"[8] Title color [%s]")
// L"显示地面危险区域碰撞箱（火焰/胆汁等）"
SKEETO_UI(Text_5d989f75, L"Show ground hazard collision boxes (fire/acid, etc.)")
// L"    方向键全屏挪位置（按住连移，Shift 大步）"
SKEETO_UI(Text_5e126387, L"    Arrow keys move anywhere (hold to repeat, Shift for larger steps)")
// L"[5] 连杀规则 【%d秒窗口，最高档保持】"
SKEETO_UI(Text_5e274d23, L"[5] Streak rules [%d-second window, hold highest tier]")
// L"救起队友"
SKEETO_UI(Text_5e3fab16, L"Teammates revived")
// L"[7] 强制 Cvar 修改 【进入设置】"
SKEETO_UI(Text_5e4f927c, L"[6] Force Cvar overrides [Settings]")
// L"在子页关掉某项后，须关闭菜单该项才会从画面消失"
SKEETO_UI(Text_5e73194c, L"After disabling an item on a subpage, close the menu to hide it")
// L"被 Smoker 拉走"
SKEETO_UI(Text_5e74f3c0, L"Dragged by Smoker")
// L"[2] 重置静态准星参数（仅当前样式）"
SKEETO_UI(Text_5e98c394, L"[2] Reset static crosshair (current style only)")
// L"Bill 比尔"
SKEETO_UI(Text_5ee06449, L"Bill")
// L"玩家5"
SKEETO_UI(Text_5f26ffe5, L"Player5")
// L"原版队友血条会缓存名字：局内修改后需重新进入或加载一次地图才会更新；Skeeto HUD 会立即更新。"
SKEETO_UI(Text_5f638137, L"Vanilla teammate health bars cache names: rejoin or reload the map to update them. Skeeto HUD updates immediately.")
// L"此页内容并非屏幕叠加层，是在游戏HUD层绘制的，相比于原版HUD，会有更多的自由性和信息"
SKEETO_UI(Text_5fcdbe1a, L"Drawn in the game HUD layer, with more flexibility and information than the stock HUD")
// L"[1] 显示肾上腺素 HUD %s"
SKEETO_UI(Text_60820404, L"[1] Show adrenaline HUD %s")
// L"精确调色：skeeto_spotlight_color R G B（例如 235 240 230）"
SKEETO_UI(Text_60bed6fb, L"Exact color: skeeto_spotlight_color R G B (e.g. 235 240 230)")
// L"[7] 清零当前计数"
SKEETO_UI(Text_60bfbf12, L"[7] Reset current counters")
// L"    不改变闲置期间人机的行为；只在你重新接管时切一次。"
SKEETO_UI(Text_612528b5, L"    Does not change bot behavior while idle; switches only once when you take control again.")
// L"[1] 主菜单背景引擎 %s"
SKEETO_UI(Text_61961eac, L"[1] Main menu background engine %s")
// L"    precache 全套模型，开启后可能会影响帧率，默认关"
// L"工坊包尚未完整下载或校验未通过，Skeeto 会继续重试。\nThe Workshop package is incomplete or invalid; Skeeto will retry."
SKEETO_UI(Text_624c3e99, L"The Workshop package is incomplete or invalid.\nSkeeto will retry.")
// L"    循环：仅特感(默认) → 特感+普感 → 全关"
SKEETO_UI(Text_6264c52a, L"    Cycle: SI only -> SI + common -> all off (default)")
// L"刀杀冲锋 Charger（Level）"
SKEETO_UI(Text_6290192b, L"Melee charging Charger (Level)")
// L"    手持燃烧瓶/土制炸弹/胆汁时显示预计落点与轨迹"
SKEETO_UI(Text_62a83e9f, L"    Previews Molotov, pipe bomb and bile jar trajectories and landing points. By default, visible only while holding left mouse.")
// L"    本地击杀计数，可调位置和显示项"
SKEETO_UI(Text_62af49d9, L"    Local kill counts with adjustable position and visible items")
// L"—— TEAM LEGACY  团队足迹 ——"
SKEETO_UI(Text_62d9c505, L"-- TEAM LEGACY --")
// L"右"
SKEETO_UI(Text_631946f8, L"Right")
// L"紫"
SKEETO_UI(Text_63351c5b, L"Purple")
// L"    接管后把手持武器恢复成闲置前的槽位：闲置前用主武器就切回主武器，用副武器就切回副武器。"
SKEETO_UI(Text_63e8d6a2, L"    Restores your pre-idle weapon slot on takeover: primary if you held a primary, secondary if you held a secondary.")
// L"[2] 近距离再次补发提示音 %s"
SKEETO_UI(Text_6439f545, L"[2] Play another cue at close range %s")
// L"    持久化计数会越攒越大；用缩写可让面板宽度不再随位数变化"
SKEETO_UI(Text_647967c8, L"    Persistent totals keep growing; abbreviations keep the panel width stable")
// L"    特感/Tank 死亡后不产生任何渐隐的布偶效果，杀怪体验更清爽。Skeeto 的此功能不依赖 -lv 启动项。"
SKEETO_UI(Text_64defcc3, L"    No fading ragdolls after SI/Tank death — cleaner kills. Works without the -lv launch option.")
// L"土制"
SKEETO_UI(Text_64eedd37, L"Pipe Bomb")
// L"有人被特感控住才显示。例：Ellis（2）被 Hunter 扑倒"
SKEETO_UI(Text_64ef1a27, L"Appears only while someone is pinned. Example: Ellis (2) pinned by Hunter")
// L"    六种特感扁平图标的出生/近身提示；依赖“大幅降低无声特感发生的概率”，可单独调整位置与大小。"
SKEETO_UI(Text_650fc20b, L"    Spawn/proximity icons for six SI types; requires Reduce silent special infected; position and size adjustable.")
// L"应用失败：剪贴板里没有 Unicode 文本。"
SKEETO_UI(Text_65749871, L"Apply failed: clipboard has no Unicode text.")
// L"[3] 补发音效来源 【%s】"
SKEETO_UI(Text_6592ddcd, L"[3] Extra sound source [%s]")
// L"自己"
SKEETO_UI(Text_659f4d33, L"You")
// L"[1] 显示高爆弹 / 燃烧弹 / 激光 HUD %s"
SKEETO_UI(Text_65bd37b1, L"[1] Show explosive / incendiary / laser HUD %s")
// L"刀爆空中 Hunter / Jockey"
SKEETO_UI(Text_664cfd96, L"Melee skeet Hunter / Jockey")
// L"    纯原版 HUD 也能藏；若仍看得见，进图后关菜单等几秒再试"
SKEETO_UI(Text_66542e2f, L"    Also hides unmodified stock HUDs; if still visible, close the menu in a map and wait a few seconds")
// L"速度表  v%s"
SKEETO_UI(Text_665ff202, L"Speedometer  v%s")
// L"[2] 正在移动 【自己】"
SKEETO_UI(Text_668d8680, L"[2] Moving [Self]")
// L"斧头"
SKEETO_UI(Text_66adca8d, L"Axe")
// L"    仅修改产生的尸体上限，不改变普感与特感尸体渐隐的视觉效果。"
SKEETO_UI(Text_66afe91b, L"    Common/Witch deaths leave no fading ragdolls; uses zombie_death for exact handling, mutually exclusive with dissolve. No -lv required.")
// L"    「无」表示不按时间中断连杀；HUD 与音效均默认关闭"
SKEETO_UI(Text_66b1aad3, L"    None means streaks never time out; HUD and sounds are off by default")
// L"牺牲"
SKEETO_UI(Text_66ec98fa, L"The Sacrifice")
// L"击杀统计 HUD"
SKEETO_UI(Text_6710b403, L"Kill counter HUD")
// L"左对齐"
SKEETO_UI(Text_67756783, L"Left")
// L"对抗胜场"
SKEETO_UI(Text_6778b36e, L"Versus wins")
// L"击杀列表 HUD  v%s"
SKEETO_UI(Text_685bc58c, L"Kill feed HUD  v%s")
// L"[3] 显示方式：%s"
SKEETO_UI(Text_6864bee3, L"[3] Display mode: %s")
// L"显示推击与特感爪击判定线"
SKEETO_UI(Text_68edbde5, L"Show shove and infected claw traces")
// L"伤害数字"
SKEETO_UI(Text_691e55bd, L"Damage numbers")
// L"[%d] %s 【进入设置】"
SKEETO_UI(Text_6930b354, L"[%d] %s [Settings]")
// L"    进入子菜单分别管理地图偏色滤镜和地图实体色彩校正。"
SKEETO_UI(Text_6935c8f1, L"    Open settings to manage global color filters and map color-correction entities separately.")
// L"其他选项  v%s"
SKEETO_UI(Text_69680df0, L"Help and diagnostics  v%s")
// L"[3] 上下位置 【%s】"
SKEETO_UI(Text_69a9d1c1, L"[4] Vertical position [%s]")
// L"服务器默认"
SKEETO_UI(Text_69c11909, L"Server Default")
// L"修改人机名字  v%s"
SKEETO_UI(Text_69facb04, L"Rename bots  v%s")
// L"队友低血量提示HUD"
SKEETO_UI(Text_6a1dc3d0, L"Teammate low-health alert HUD")
// L"[7] 自然白预设"
SKEETO_UI(Text_6a7d6f80, L"[7] Natural white preset")
// L"对抗对局"
SKEETO_UI(Text_6adcbc5d, L"Versus matches")
// L"[3] 复位位置（准星右侧）"
SKEETO_UI(Text_6b2896d5, L"[3] Reset position (right of crosshair)")
// L"寂静时分"
SKEETO_UI(Text_6bc66717, L"Dead Air")
// L"[6] 枪械散布调试 %s"
SKEETO_UI(Text_6bcb8876, L"[6] Weapon spread debug %s")
// L"[4] 优化听声辨位 【进入设置】 %s"
// L"当前渲染后端：游戏原生BIK视频材质"
SKEETO_UI(Text_6cd37d39, L"Renderer: native BIK video material")
// L"玩家7"
SKEETO_UI(Text_6d109d67, L"Player7")
// L"倒三角沙漏倒计时"
SKEETO_UI(Text_6d3f2498, L"Inverted-triangle hourglass")
// L"世界时间  v%s"
SKEETO_UI(Text_6d47e392, L"Real-world clock  v%s")
// L"    常驻显示远端队友当前可用血量，不显示最大值；写实模式禁用"
SKEETO_UI(Text_6dbf7f91, L"    Always shows remote teammates' current usable health, without maximum; disabled in Realism")
// L"    血条里的弹药还在。这是一套无底框的弹匣/备弹数字，方便挪到准星旁，省得盯屏幕边缘。"
SKEETO_UI(Text_6f3cb134, L"    Borderless clip/reserve numbers near the crosshair; health-bar ammo remains available.")
// L"跟随原版或其它 Mod"
SKEETO_UI(Text_6f7a0baa, L"Use game or other mod default")
// L"    原版特感的 alert 提示音。按 [3] 切换来源。"
SKEETO_UI(Text_6f8bd946, L"    Original SI alert sounds. Press [3] to change source.")
// L"进入地图后已暂停（上次主菜单：游戏原生BIK）"
SKEETO_UI(Text_6fe4b24d, L"Paused in-game (last main menu renderer: native BIK)")
// L"队友头顶血条"
SKEETO_UI(Text_708a91b2, L"Overhead teammate health bars")
// L"携带物"
SKEETO_UI(Text_709c923b, L"Carry Item")
// L"[2] 特感近身提示 HUD %s"
SKEETO_UI(Text_70c4b307, L"[2] SI proximity alert HUD %s")
// L"    主武器带高爆弹、燃烧弹或激光瞄准器时显示对应图标；特殊弹药打完即消失，激光会一直留着。无底框、无文字。默认关。"
SKEETO_UI(Text_70f12f4b, L"    Primary weapon upgrade icons; special ammo disappears when spent, laser stays. No text or background; off by default.")
// L"关卡名 / 章节进度  v%s"
SKEETO_UI(Text_720e5a20, L"Map name / chapter progress  v%s")
// L"[1] 显示速度表 %s"
SKEETO_UI(Text_724762d7, L"[1] Show speedometer %s")
// L"—— GAME MODES  模式记录 ——"
SKEETO_UI(Text_7303d195, L"-- GAME MODES --")
// L"特感硬直 / 踉跄提示  v%s"
SKEETO_UI(Text_731ef2af, L"SI stun / stagger indicator  v%s")
// L"毫不留情"
SKEETO_UI(Text_7357c8a0, L"No Mercy")
// L"秒"
SKEETO_UI(Text_73d18cc8, L"s")
// L"    先复制写好的名字，再回到此页按 [1]。只在按键时读取一次剪贴板。"
SKEETO_UI(Text_73e71ea5, L"    Copy the name, return here, and press [1]. The clipboard is read only once when pressed.")
// L"[1] 显示特感连杀 HUD %s"
SKEETO_UI(Text_7401dcb2, L"[1] Show SI streak HUD %s")
// L"名字最多 23 个字符，支持中文、英文和空格。未设置时跟随游戏或其它 Mod。"
SKEETO_UI(Text_740206ff, L"Names support Chinese, English, and spaces, up to 23 characters. Unset names follow the game or other mods.")
// L"[9] 战斗与视觉辅助"
// L"玩家6"
SKEETO_UI(Text_740bac26, L"Player6")
// L"[3] 重新扫描配置（发现 %d 套）"
SKEETO_UI(Text_74286f2a, L"[3] Rescan configs (%d found)")
// L"2 常驻显示"
SKEETO_UI(Text_743ad478, L"2 Always visible")
// L"    一些调试选项打开后可能掉帧。"
SKEETO_UI(Text_74807c51, L"    Some debug options may reduce frame rate.")
// L"    弹匣打空或空仓扣扳机时播放 dryfire 音效（需 sound/emsc/* 素材）"
SKEETO_UI(Text_74c4147f, L"    Plays a dry-fire sound when the magazine empties or firing on empty (requires sound/emsc/* assets).")
// L"    队友进入黑白状态（二倒被救起）时，在其血条旁常驻显示裂心心电图标记，恢复后消失"
SKEETO_UI(Text_74f7643d, L"    Black-and-white teammates (revived after two incaps) get a broken-heart ECG beside their health bar until recovered")
// L"控制台精确调色示例：skeeto_glow_bill 111 175 69；输入 default 恢复服务器默认。"
SKEETO_UI(Text_74feaabf, L"Exact console color example: skeeto_glow_bill 111 175 69; use default to restore server defaults.")
// L"[6] 单独开关"
SKEETO_UI(Text_755ff728, L"[6] Individual toggles [Settings]")
// L"    当前 dsp_off=%.3f%s"
SKEETO_UI(Text_7571b721, L"    Current dsp_off=%.3f%s")
// L"[2] 显示尸潮出现提示 %s"
SKEETO_UI(Text_75c000da, L"[2] Show horde spawn alerts %s")
// L"Cover（填满裁剪）"
SKEETO_UI(Text_75d26a4f, L"Cover (Crop to Fill)")
// L"简洁版导演 HUD  v%s"
SKEETO_UI(Text_762c3409, L"Compact Director HUD  v%s")
// L"    自己+最多8名队友，也可只显示自己。栏位按玩家槽摆放，头像仍跟本局角色。"
SKEETO_UI(Text_76ad9ed6, L"    Yourself and up to 8 teammates, or self only. Panels follow player slots; portraits follow characters.")
// L"瞄准误差+散布（偏大）"
SKEETO_UI(Text_76bd9035, L"Aim Error + Spread (Larger)")
// L"R砖"
SKEETO_UI(Text_76dbb07c, L"R Rock")
// L"刺骨寒溪"
SKEETO_UI(Text_771ae2e6, L"Cold Stream")
// L"已保存：%s。总开关关闭，当前不会替换。"
SKEETO_UI(Text_7756922c, L"Saved: %s. Master switch is off; replacement is inactive.")
// L"[2] 材质 【%s】"
SKEETO_UI(Text_77999777, L"[2] Material [%s]")
// L"    匹配粒子清单相对路径particles/skeeto_*.txt，不依赖传统particle_manifest.txt加载粒子，解决粒子平台冲突问题。"
SKEETO_UI(Text_7828a916, L"    Loads particles from particles/skeeto_*.txt lists without traditional particle_manifest.txt, avoiding platform conflicts.")
// L"[2] 仅禁用地图内的色彩校正实体 %s"
SKEETO_UI(Text_784cf076, L"[2] Disable map color-correction entities only %s")
// L"Config Override 过关之后自动执行CFG文件  v%s"
SKEETO_UI(Text_78a4000b, L"Config Override: run CFG after chapter changes  v%s")
// L"【特感+普感】"
SKEETO_UI(Text_78a5ae16, L"[Special + common infected]")
// L"    全局选项：以 1080p 为 100%，按屏幕高度统一缩放所有屏幕 HUD；保留各 HUD 自己的大小设置"
SKEETO_UI(Text_78af987e, L"    Global: 1080p is 100%; scale all screen HUDs by screen height, preserving individual sizes")
// L"    控制台开关：skeeto_character_change 1/0"
// L"    燃烧瓶、胆汁瓶与 Spitter 酸液的地面剩余时间凭证条"
SKEETO_UI(Text_794d7096, L"    Remaining ground duration bars for Molotov fire, bile and Spitter acid")
// L"生还者血量、倒地、被控等；栏位按玩家槽（自己+最多8名队友），不跟角色模型走"
SKEETO_UI(Text_79610678, L"Health, incap and pin status; panels follow player slots (you + up to 8 teammates), not character models")
// L"    用原版每种特感的出场音乐（sound/music/bacteria）替代 alert。文件本身是单声道，方向感来自 [2] 的近距离补声；需要先开启游戏音乐音量。"
SKEETO_UI(Text_7968000d, L"    Uses each SI's original spawn music (sound/music/bacteria). Mono audio; [2] adds nearby directional sound. Enable game music volume.")
// L"    竖排显示自己的投掷物、医疗包/电击器、止痛药/肾上腺素，方便挪到准星附近"
SKEETO_UI(Text_79fa01c3, L"    Vertical throwable, medkit/defibrillator and pills/adrenaline slots near the crosshair")
// L"额外的主武器子弹数HUD"
SKEETO_UI(Text_7a00fd89, L"Extra primary ammo HUD")
// L"被胆汁影响的感染者"
SKEETO_UI(Text_7a6f89c0, L"Biled infected")
// L"    会根据游戏分辨率自适应大小"
SKEETO_UI(Text_7b43ffd4, L"    Automatically scales to game resolution.")
// L"    自己近战切断舌头时，在 Smoker 头顶显示约 2 秒绿色断舌徽"
SKEETO_UI(Text_7b4b0f7f, L"    When you cut a tongue with melee, a green severed-tongue emblem above the Smoker for about 2 seconds")
// L"    队友救起自己或解除自己被控时提示约 5 秒。默认关。"
SKEETO_UI(Text_7b551eb1, L"    A 5-second alert when a teammate revives or frees you. Off by default.")
// L"[6] 枪械开火音量 【%d%%】"
// L"警棍"
SKEETO_UI(Text_7c315bdf, L"Tonfa")
// L"[6] 复位全部位置"
SKEETO_UI(Text_7c8afdbd, L"[6] Reset all positions")
// L"肾上腺素持续"
SKEETO_UI(Text_7caa2a18, L"Adrenaline duration")
// L"[7] 强制移除地图狗屎滤镜 【进入设置】"
SKEETO_UI(Text_7d38ffd0, L"[7] Force map color filters off [Settings]")
// L"[1] 显示特感血条 %s"
SKEETO_UI(Text_7d584006, L"[1] Show SI health bars %s")
// L"    开：准星旁只画弹匣；关：弹匣 + 后备弹药。原版弹药数字不受影响"
SKEETO_UI(Text_7dc4cd49, L"    On: clip only near crosshair; off: clip + reserve. Stock ammo numbers are unchanged")
// L"[0] 返回    [/Esc 关闭"
SKEETO_UI(Text_7ddb0a2b, L"[0] Back    [/Esc Close")
// L"[3] %s光圈范围 【%d units】"
SKEETO_UI(Text_7df2f610, L"[3] %sLight radius [%d units]")
// L"被控信息  v%s"
SKEETO_UI(Text_7df5a6ec, L"Pinned survivor info  v%s")
// L"[4] 立即执行当前预设"
SKEETO_UI(Text_7e83357a, L"[4] Apply current preset now")
// L"[1] 透明度 【%d%%】"
SKEETO_UI(Text_7ebc9656, L"[1] Opacity [%d%%]")
// L"左"
SKEETO_UI(Text_7ed414c4, L"Left")
// L"日"
SKEETO_UI(Text_7f01ae09, L"Sun")
// L"惊扰女巫提示 HUD"
SKEETO_UI(Text_7f020204, L"Witch startled alert HUD")
// L"[2] 吃药绿屏提示，即Healing Green Fade Effect %s"
SKEETO_UI(Text_7f0aa63f, L"[2] Healing Green Fade Effect %s")
// L"手枪"
SKEETO_UI(Text_7f1448b1, L"Pistol")
// L"[5] 世界 HUD"
SKEETO_UI(Text_7f185ee6, L"[5] World HUD")
// L"特感连杀 HUD  v%s"
SKEETO_UI(Text_7f599218, L"Special infected streak HUD  v%s")
// L"    自己击杀 Tank 后，在死亡位置显示约 2.6 秒重装猎杀徽章"
SKEETO_UI(Text_7f5c6152, L"    When you kill a Tank, a heavy hunter badge at the death position for about 2.6 seconds")
// L"草叉"
SKEETO_UI(Text_7f823352, L"Pitchfork")
// L"仅自己"
SKEETO_UI(Text_7fccbe58, L"Self only")
// L"[3] 复位位置（左上）"
SKEETO_UI(Text_7fee52ab, L"[3] Reset position (top left)")
// L"未找到 cfg/skeeto_*.cfg。请放入例如 cfg/skeeto_preset1.cfg 后按 [3] 重扫。"
SKEETO_UI(Text_80095b9a, L"No cfg/skeeto_*.cfg found. Add a file such as cfg/skeeto_preset1.cfg, then press [3] to rescan.")
// L"倒地救援"
SKEETO_UI(Text_8048d5e0, L"Revive")
// L"    对应原版 dsp_volume 0。原版在切换环境（例如从室内走到室外）时会把它改回去，本选项会立即改回并保持关闭。"
SKEETO_UI(Text_807e1111, L"    Uses dsp_volume 0. The game resets it when environments change (e.g. indoors to outdoors); this immediately reapplies it to keep effects off.")
// L"土制炸弹"
SKEETO_UI(Text_80cedce8, L"Pipe Bomb")
// L"    开枪后在控制台观察 spread / avg / max 输出"
SKEETO_UI(Text_813fbba1, L"    Check spread / avg / max output in the console after firing.")
// L"远处物品"
SKEETO_UI(Text_814b05ff, L"Distant items")
// L"三角"
SKEETO_UI(Text_8197cb27, L"Triangle")
// L"描边设置 (%d/2)"
SKEETO_UI(Text_825c906a, L"Glow settings (%d/2)")
// L"铁喷"
SKEETO_UI(Text_826b934e, L"Chrome Shotgun")
// L"击杀 HUD 单独开关  v%s"
SKEETO_UI(Text_8274a558, L"Kill HUD individual toggles  v%s")
// L"若显示 --，表示 Steam 当前未返回该项数据"
SKEETO_UI(Text_82a6bd73, L"-- means Steam has not returned data for this entry")
// L"    闲置（暂离或超时）后由电脑玩家接手。接管回来时，把你自己的手电恢复成闲置前的开关状态。"
SKEETO_UI(Text_83a401d1, L"    A bot takes over when you go idle or time out. Taking control restores your flashlight's pre-idle on/off state.")
// L"[1] 显示踉跄提示 %s"
SKEETO_UI(Text_83cd1191, L"[1] Show stagger indicator %s")
// L"[%d] %s 背景 【%s】"
SKEETO_UI(Text_8429ffc1, L"[%d] %s background [%s]")
// L"持久化存储"
SKEETO_UI(Text_84853399, L"Persistent Storage")
// L"更新已暂存，新版更新日志已打开。\n请完全退出游戏，退出后会自动完成 DLL 替换；若未成功，可运行游戏根目录的 start_skeeto.bat。\n\n" 	L"Update staged and will apply after the game fully exits. start_skeeto.bat remains a fallback."
// L"    游戏自带的导演压力显示；与自定义导演 HUD 相互独立。"
SKEETO_UI(Text_8593a627, L"    The game's built-in Director intensity display; independent of the custom Director HUD.")
// L"[1] Nick 尼克"
SKEETO_UI(Text_85d71413, L"[1] Nick")
// L"[3] 显示副武器弹匣 %s"
SKEETO_UI(Text_860392cc, L"[3] Show secondary clip %s")
// L"[4] 语言 【%s】"
SKEETO_UI(Text_8787808f, L"[4] Language [%s]")
// L"选人物  v%s"
// L"[2] 总音效开关 %s"
SKEETO_UI(Text_883bd372, L"[2] All feedback sounds %s")
// L"[2] 恢复该角色默认名字"
SKEETO_UI(Text_8845d13c, L"[2] Restore this character's default name")
// L"玩家 #%d 投掷了 %s"
SKEETO_UI(Text_88a4f288, L"Player #%d threw %s")
// L"    站立时血量首次降到 39% 闪一次；倒地后余量 ≤72 也会闪（仅倒地，回血后可再触发）"
SKEETO_UI(Text_88e5060f, L"    Flashes once when standing health first drops to 39%; also at <=72 health while incapacitated (can retrigger after healing).")
// L"    从回合开始计时，可调位置"
SKEETO_UI(Text_8936f5bc, L"    Time since round start; movable")
// L"[4] 进度条（治疗/救援/使用） %s"
SKEETO_UI(Text_8941259e, L"[4] Progress bars (heal/revive/use) %s")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_897126ba, L"    DC=cache used/limit  AV=free virtual address space  ENTS=current client entities")
// L"换弹或较长开火间隔时出现在准星旁。紫环=换弹，青环=下次开火"
SKEETO_UI(Text_8983866a, L"Near the crosshair during reloads or long firing intervals. Purple: reload; cyan: next shot")
// L"近战击杀 Witch（Melee Crown）"
SKEETO_UI(Text_89906160, L"Melee kill Witch (Melee Crown)")
// L"地面危险持续时间"
SKEETO_UI(Text_89a367f4, L"Ground hazard duration")
// L"[2] %s亮度 【%d%%】"
SKEETO_UI(Text_89e8fe9b, L"[2] %sBrightness [%d%%]")
// L"隐藏 EMS 脚本 HUD"
SKEETO_UI(Text_8a088766, L"Hide EMS script HUD")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_8a849dbe, L"    Monitors independently of the memory HUD. Clears unlocked cache below 300MB free virtual memory or when datacache reaches 90 percent of its current limit; may hitch, at most once every 10 minutes. Manual command: skeeto_clear_datacache.")
// L"实战盯屏幕边缘看弹药成本高，把弹匣/备弹数字摆到准星旁"
SKEETO_UI(Text_8a9973fd, L"Move clip/reserve numbers near the crosshair to avoid looking at screen edges during combat")
// L"[2] 正在移动 【%s】"
SKEETO_UI(Text_8b29df43, L"[2] Moving [%s]")
// L"已恢复：该角色现在跟随原版或其它 Mod 的名字。"
SKEETO_UI(Text_8b985de7, L"Restored: this character now uses the original or mod-provided name.")
// L"队友投掷物提示 HUD"
SKEETO_UI(Text_8bcd254b, L"Teammate throwable alert HUD")
// L"状态 【可换】"
SKEETO_UI(Text_8cc35b35, L"Status [Ready to switch]")
// L"显示准星所指实体碰撞箱"
SKEETO_UI(Text_8d17363c, L"Show targeted entity collision box")
// L"    这一栏局内暂时没人，仍可先摆位置"
SKEETO_UI(Text_8d6518bf, L"    This slot is currently empty; you can still position it")
// L"自定义 #%02X%02X%02X"
SKEETO_UI(Text_8d6db2d8, L"Custom #%02X%02X%02X")
// L"[2] 颜色 【%s】"
SKEETO_UI(Text_8d76a889, L"[2] Color [%s]")
// L"[4] 大小 【%d】"
SKEETO_UI(Text_8e0efa9b, L"[4] Size [%d]")
// L"[6] 重新扫描背景资源"
SKEETO_UI(Text_8e769f9d, L"[6] Rescan backgrounds")
// L"[2] 当前背景：%s"
SKEETO_UI(Text_8ebecea8, L"[2] Current background: %s")
// L"直接监听 witch_harasser_set，只提示首次惊扰。显示玩家名或人机角色名，不用 userid。"
SKEETO_UI(Text_8ec7c228, L"Listens for witch_harasser_set; first startle only. Shows player or bot character name, without userid.")
// L"关闭时过关不会自动 exec；按 [2] 切换仍会立刻执行当前套。"
SKEETO_UI(Text_8ee82761, L"When disabled, chapter changes do not auto-exec. Switching with [2] still applies the selected preset immediately.")
// L"蓝"
SKEETO_UI(Text_8f48cd1d, L"Blue")
// L"    特感出生时以独立声道无条件补发一次提示音。"
SKEETO_UI(Text_8f78b21a, L"    Always plays an extra spawn cue for special infected on a separate sound channel.")
// L"[1] 强制绑定 Skeeto 菜单到 [ %s"
SKEETO_UI(Text_8fc84c54, L"[1] Force Skeeto menu binding to [ %s")
// L"当前背景加载失败，已自动保留原版主菜单背景。"
SKEETO_UI(Text_8fca4a0c, L"Background failed to load; the original main menu background has been retained.")
// L"[1] 显示网络参数 HUD %s"
SKEETO_UI(Text_8fe6b4be, L"[1] Show network stats HUD %s")
SKEETO_UI(NetGraphHudHint, L"    FPS, Ping, Loss, Choke, Lerp; full mode also shows IN/OUT, SV (server frame rate), and VAR (jitter in msec).")
SKEETO_UI(NetGraphHudDetailToggle, L"[2] Detail mode [%s]")
SKEETO_UI(NetGraphHudDetailCompact, L"Compact")
SKEETO_UI(NetGraphHudDetailFull, L"Full")
SKEETO_UI(NetGraphHudDetailHint, L"    Compact: FPS / Ping / Lerp, packet rates, Loss / Choke. Full: also IN/OUT throughput; SV is server frame rate, VAR is frame-time jitter (msec).")
// L"关掉的项不画在 HUD 上，计数仍累计。"
SKEETO_UI(Text_8feedcb0, L"Hidden items still accumulate counts.")
// L"[1] 显示击杀 HUD %s"
SKEETO_UI(Text_9014f626, L"[1] Show kill HUD %s")
// L"只读当前地图名，不扫实体、不读任务文件"
SKEETO_UI(Text_902b3db1, L"Reads only the current map name; no entity scans or mission-file reads")
// L"推击疲劳"
SKEETO_UI(Text_90346e5e, L"Shove Fatigue")
// L"—— 2. 总开关 ——"
SKEETO_UI(Text_904721b9, L"-- 2. Master switches --")
// L"网络参数 HUD  v%s"
SKEETO_UI(Text_9085cf99, L"Network stats HUD  v%s")
// L"胆汁瓶"
SKEETO_UI(Text_90970a77, L"Bile Jar")
// L"[9] 选人物…"
// L"    开启时首次启动会自动执行 bind [ skeeto_menu；关闭后不再占用 [ 键。仍可使用 skeeto_menu 指令或自行 bind。"
SKEETO_UI(Text_90cb0481, L"    When enabled, runs bind [ skeeto_menu on initial startup. Disabling releases the [ key. You can still use skeeto_menu or bind it yourself.")
// L"[3] 铺满屏幕 %s"
SKEETO_UI(Text_90ff8d18, L"[3] Fill screen %s")
// L"    复用 Melee Skeet 判定，在死亡位置显示约 2 秒冰蓝刀徽"
SKEETO_UI(Text_911c7619, L"    Uses Melee Skeet detection; icy-blue blade emblem at the death position for about 2 seconds")
// L"    精简/完整是快捷预设；单独开关可只留某几项"
SKEETO_UI(Text_918d713b, L"    Compact/Full are presets; use individual toggles to choose items")
// L"[3] 复位位置（上中）"
SKEETO_UI(Text_92ad7249, L"[3] Reset position (top center)")
// L"写实战役"
SKEETO_UI(Text_92c3fc1d, L"Realism campaigns")
// L"[3] 复位位置（准星左下方）"
SKEETO_UI(Text_93260568, L"[3] Reset position (below-left of crosshair)")
// L"撬棍"
SKEETO_UI(Text_93928d5b, L"Crowbar")
// L"换人物失败：SetModel不可用"
SKEETO_UI(Text_93ccfd7d, L"Switch failed: SetModel unavailable.")
// L"冲锋枪"
SKEETO_UI(Text_93f3cca9, L"SMG")
// L"为避免展示推测结果，本档案不提供该项。"
SKEETO_UI(Text_9410a994, L"This archive omits that breakdown to avoid displaying estimates.")
// L"[3] 动态准星 %s"
SKEETO_UI(Text_94fa4530, L"[3] Dynamic crosshair [Open settings] %s")
// L"[1] 大幅降低无声特感发生的概率 【进入设置】 %s"
SKEETO_UI(Text_9506889d, L"[8] Reduce silent special infected [Settings] %s")
// L"[1] Skeeto粒子平台 %s"
SKEETO_UI(Text_952b51df, L"[1] Skeeto particle platform %s")
// L"酸液残留"
SKEETO_UI(Text_95651b7a, L"Acid Pool")
// L"    例如：地图内不同区域或者不同剧情阶段的滤镜控制，针对具体的地图内实体。"
SKEETO_UI(Text_9582d285, L"    Targets map entities that control filters for specific areas or story stages.")
// L"[6] 复位位置（上中）"
SKEETO_UI(Text_95cf7b20, L"[6] Reset position (top center)")
// L"Francis 弗朗西斯"
SKEETO_UI(Text_960267f2, L"Francis")
// L"铲子"
SKEETO_UI(Text_9611e90c, L"Shovel")
// L"双端都显示"
SKEETO_UI(Text_966ad0c5, L"Both")
// L"[4] 复位位置（上中）"
SKEETO_UI(Text_96a4811a, L"[4] Reset position (top center)")
// L"救援者  Nick"
SKEETO_UI(Text_96b37180, L"Reviver  Nick")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_96d1fdb9, L"    Status: game cache interface ready")
// L"此页将一些依赖nut脚本（Vscript）的功能转化为服务器内可用的客户端功能（Skeetoscript），侧重改善视听，不会实现敏感功能。"
SKEETO_UI(Text_96e9439f, L"Skeetoscript adapts selected nut (VScript) features into client features usable on servers, improving audiovisual effects and controls.")
// L"[5] 复位位置（上中）"
SKEETO_UI(Text_97117c07, L"[5] Reset position (top center)")
// L"当前设置：%s"
SKEETO_UI(Text_974fb9b3, L"Current setting: %s")
// L"%s / 坦克事件 %d+"
SKEETO_UI(Text_97534a94, L"%s / Tank Events %d+")
// L"    倒三角沙漏随剩余时间减少填充，中央显示整数秒；最后 1 秒变红。"
SKEETO_UI(Text_975aa751, L"    Triangular hourglass drains with remaining time; whole seconds in the center, red for the final second.")
// L"Ellis 艾利斯"
SKEETO_UI(Text_979c5712, L"Ellis")
// L"[3] 整体 【%d%%】  `-`/`=`  字号 【%d】  Shift+`-`/`=`"
SKEETO_UI(Text_97b80292, L"[3] Scale [%d%%]  `-`/`=`  Font [%d]  Shift+`-`/`=`")
// L"局内计时器"
SKEETO_UI(Text_97ba5d63, L"Round timer")
// L"Witch 击杀提示"
SKEETO_UI(Text_97dbf93d, L"Witch kill indicator")
// L"倒地救援  v%s"
SKEETO_UI(Text_97dfa3cf, L"Revive progress  v%s")
// L"状态 【未在本地主机 - 已锁定】"
SKEETO_UI(Text_97e432ea, L"Status [Not local host - locked]")
// L"—— CAREER OVERVIEW  生涯概览 ——"
SKEETO_UI(Text_98334c06, L"-- CAREER OVERVIEW --")
// L"[2] 切换准星"
SKEETO_UI(Text_9882479e, L"[2] Crosshairs")
// L"控制台：skeeto_character 人物英文名；支持单字母缩写。"
SKEETO_UI(Text_98ae4020, L"Console: skeeto_character <English name>; single-letter abbreviations supported.")
// L"上次主菜单：%d.%d FPS  播放速度：%d%%  缓冲：%d/%d"
SKEETO_UI(Text_99a073a7, L"Last main menu: %d.%d FPS  Playback speed: %d%%  Buffer: %d/%d")
// L"[4] 显示生还与特感移动路线 【关】"
SKEETO_UI(Text_99a3a9e6, L"[4] Show survivor/infected movement paths [OFF]")
// L"救援者  %s"
SKEETO_UI(Text_99afa0ec, L"Reviver  %s")
// L"    进入人物列表"
// L"    打了肾上腺素才出现：安瓿液面 + 剩余秒数"
SKEETO_UI(Text_99e44812, L"    Appears during adrenaline: ampoule level and seconds remaining")
// L"[1] 显示终局救援阶段 HUD %s"
SKEETO_UI(Text_9a5fc7f1, L"[1] Show finale rescue phase HUD %s")
// L"[1] 一些有趣的调试 Cvar 强制修改"
// L"换人物失败：模型未预缓存（关功能重进图后再开）"
SKEETO_UI(Text_9a80a557, L"Switch failed: model not precached (disable, reload map, then enable).")
// L"黄"
SKEETO_UI(Text_9aaa1438, L"Yellow")
// L"[1] 显示救援 HUD %s"
SKEETO_UI(Text_9aae33fe, L"[1] Show revive HUD %s")
// L"使用方法：进入角色页面从剪贴板应用，或直接使用页面内显示的控制台指令。"
SKEETO_UI(Text_9b00c02b, L"Open a character page to apply a name from the clipboard, or use the console command shown there.")
// L"[2] 整体 【%d%%】  `-`/`=`  字号 【%d】  Shift/Ctrl+`-`/`=`"
SKEETO_UI(Text_9b0dc29a, L"[2] Scale [%d%%]  `-`/`=`  Font [%d]  Shift/Ctrl+`-`/`=`")
// L"终局救援阶段 HUD"
SKEETO_UI(Text_9b0e6e51, L"Finale rescue phase HUD")
// L"[7] Francis 弗朗西斯"
SKEETO_UI(Text_9b0f765a, L"[7] Francis")
// L"    原版屏幕下方进度条；默认关，不跟 SKT 倒地救援联动"
SKEETO_UI(Text_9b135a4f, L"    Stock bottom-screen progress; off by default, independent of SKT revive progress")
// L"挂边"
SKEETO_UI(Text_9bb4cb32, L"Hanging")
// L"[1] 启用修改人机名字 %s"
SKEETO_UI(Text_9c1650df, L"[1] Enable custom bot names %s")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_9c4a19c1, L"Reads game cache, Windows free virtual memory and client entity count once per second")
// L"盾牌"
SKEETO_UI(Text_9cb786d7, L"Riot Shield")
// L"[4] 样式 【%s】"
SKEETO_UI(Text_9cc21175, L"[4] Style [%s]")
// L"燃烧包"
SKEETO_UI(Text_9ccc30d2, L"Incendiary Ammo")
// L"    左右移动时视角轻微倾斜（sv_rollangle；对应工坊 Black Mesa View Tilt）"
SKEETO_UI(Text_9d21937e, L"    Slight view tilt while strafing (sv_rollangle / sv_rollspeed; Workshop: Black Mesa View Tilt).")
// L"[1] 聚光灯夜视 %s"
SKEETO_UI(Text_9d962b16, L"[1] Spotlight night vision %s")
// L"    skeeto_bot_name_%hs \"新名字\""
SKEETO_UI(Text_9dde028a, L"    skeeto_bot_name_%hs \"New name\"")
// L"额外的药包投掷物 HUD  v%s"
SKEETO_UI(Text_9df75a0a, L"Extra healing / throwable HUD  v%s")
// L"换弹 / 开火后摇环"
SKEETO_UI(Text_9f478cd3, L"Reload / fire recovery ring")
// L"显示电锯调试信息"
SKEETO_UI(Text_9fb8893c, L"Show chainsaw debug info")
// L"[3] 两项恢复 100% 原值"
SKEETO_UI(Text_9ff5f540, L"[3] Restore both to original 100%")
// L"[4] %s红色 R 【%d】"
SKEETO_UI(Text_9ffd8f3d, L"[4] %sRed R [%d]")
// L"[2] 武器图标和弹药 %s"
SKEETO_UI(Text_a00ff4f2, L"[2] Weapon icons and ammo %s")
// L"显示高爆弹药爆炸范围"
SKEETO_UI(Text_a0275a62, L"Show explosive ammo blast radius")
// L"    战斗与视觉辅助里的「原版导演HUD」是另一套，互不影响"
SKEETO_UI(Text_a0a1923a, L"    Independent of Stock Director HUD under Combat and audiovisual enhancements")
// L"    特感出生补声与可选的近距离补声设置。"
SKEETO_UI(Text_a0cf0fea, L"    Extra spawn cues and optional close-range cues for special infected.")
// L"最近一秒最低缓冲：%d帧  缓冲耗尽：%d次"
SKEETO_UI(Text_a0e1521b, L"Lowest buffer in last second: %d frames  Buffer underruns: %d")
// L"按数字选择项目，再用 - / + 调节；原值表示完全不接管该项。"
SKEETO_UI(Text_a101ed16, L"Select an item by number, then use - / +; Original means no override.")
// L"马格南"
SKEETO_UI(Text_a129e9f1, L"Magnum")
// L"[7] 显示 【%s】"
SKEETO_UI(Text_a199ccdd, L"[7] Display [%s]")
// L"    移除闲置条件，并静音导演报错字符串"
SKEETO_UI(Text_a1d63246, L"    Removes idle restrictions and suppresses Director error messages.")
// L"二"
SKEETO_UI(Text_a1db2cda, L"Tue")
// L"血量持续在 39 及以下约 10 秒才提示，刚掉血不弹。倒地和自己不提示。"
SKEETO_UI(Text_a2995727, L"Alerts after about 10 seconds at 39 HP or below, not immediately after damage. Excludes yourself and incapacitated teammates.")
// L"方法一：从剪贴板应用"
SKEETO_UI(Text_a2bf9333, L"Method 1: Apply from clipboard")
// L"    仅依据队友道具栏的持有到空缺变化；不依赖 weapon_fire。"
SKEETO_UI(Text_a3a9da41, L"    Uses the actual projectile and its networked thrower; item transfers do not trigger it.")
// L"E砖"
SKEETO_UI(Text_a41bfbee, L"E Rock")
// L"靠上"
SKEETO_UI(Text_a447b0bb, L"Top")
// L"[9] 常驻显示队友描边 %s"
SKEETO_UI(Text_a44ae3aa, L"[9] Always show teammate glow %s")
// L"[5] 复位位置"
SKEETO_UI(Text_a45cebb1, L"[5] Reset position")
// L"[8] 关闭枪械曳光弹 %s"
SKEETO_UI(Text_a48a3e64, L"[7] Disable weapon tracers %s")
// L"    现版本采用全向动态光源实现，动态光源的性能不好，只建议高配电脑使用此选项。"
SKEETO_UI(Text_a495bd8e, L"    This version uses an omnidirectional dynamic light, which is costly; recommended only for powerful PCs.")
// L"军用狙击枪"
SKEETO_UI(Text_a4c62326, L"Military Sniper")
// L"倒地"
SKEETO_UI(Text_a51f774c, L"Down")
// L"玩家?"
SKEETO_UI(Text_a5c9176f, L"Player?")
// L"速度表"
SKEETO_UI(Text_a62f70b1, L"Speedometer")
// L"[5] 弹壳落地音效，即Shell Impact Sounds %s"
SKEETO_UI(Text_a6c06d8b, L"[5] Shell Impact Sounds %s")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_a6da95fb, L"    Status: game cache and entities are read in a map; disabled HUD preview uses examples")
// L"    （先在 [6] 选特感素材包，画面和音效这两项才生效）"
SKEETO_UI(Text_a74e9b45, L"    (Select an SI pack in [6]; the picture and sound rows need it)")
// L"跟随文字"
SKEETO_UI(Text_a76ba54a, L"Match Text")
// L"[5] 单人也可闲置 %s"
SKEETO_UI(Text_a77d8b9b, L"[5] Allow going idle when alone %s")
// L"[9] 文字描边 %s"
SKEETO_UI(Text_a7bced81, L"[2] Text outline %s")
// L"    若闲置前你没有开手电，闲置期间会把手电亮度压到 0，让人机的手电也看不见，接管后立刻还原。"
SKEETO_UI(Text_a7d8f567, L"    If your flashlight was off before idle, its brightness is held at 0 during idle to hide the bot's light, then restored on takeover.")
// L"[2] 简洁模式 %s"
SKEETO_UI(Text_a7ea8762, L"[2] Compact mode %s")
// L"隐藏原版 HUD  v%s"
SKEETO_UI(Text_a80afaac, L"Hide stock HUD  v%s")
// L"人字"
SKEETO_UI(Text_a8540134, L"Chevron")
// L"方框+点"
SKEETO_UI(Text_a85e33af, L"Square + Dot")
// L"【使用当前普感素材】"
SKEETO_UI(Text_a8730bf5, L"[Use current common infected assets]")
// L"战役生还率"
SKEETO_UI(Text_a8994e5a, L"Campaign survival rate")
// L"状态 【功能未开】"
SKEETO_UI(Text_a902878d, L"Status [Feature disabled]")
// L"启用前原值"
SKEETO_UI(Text_a90d2811, L"Original Value")
// L"    同时移除地图偏色滤镜、地图实体色彩校正、二倒黑白等。"
SKEETO_UI(Text_a9b7c73d, L"    Removes map color filters, color-correction entities, black-and-white effects after two incapacitations, and more.")
// L"聊天指令会作为普通聊天消息发出；功能关闭或不在本地服时不会执行。"
SKEETO_UI(Text_aa439ead, L"Chat commands are sent as normal messages; they do nothing when disabled or not hosting locally.")
// L"跟踪自己刚推击或命中的一个特感，按实际踉跄动画显示在模型侧边。"
SKEETO_UI(Text_aa843d42, L"Tracks one SI you just shoved or hit; displays beside its model during the actual stagger animation.")
// L"[2] 半径 【%d】  也可按 - / = 调节"
SKEETO_UI(Text_aad393ab, L"[2] Radius [%d]  Also adjustable with - / =")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_aadd0ea4, L"N/A")
// L"举石起手出凭证条；多 Tank 各一条。低抛为 E砖，单手快丢为 右键砖，双手慢丢为 R砖。"
SKEETO_UI(Text_ab389b32, L"One bar per Tank at windup. Underhand: E; fast one-handed: right-click; slow two-handed: R.")
// L"已读取 Steam 生涯档案"
SKEETO_UI(Text_ac72d30b, L"Steam career archive loaded")
// L"静态准星 (%d/%d)  v%s"
SKEETO_UI(Text_ac7558ba, L"Static crosshair (%d/%d)  v%s")
// L"浅灰"
SKEETO_UI(Text_ac7748f5, L"Light Gray")
// L"    强制关闭地图迷雾；过关不重置，且不会使水面贴图消失。"
SKEETO_UI(Text_acccc3f4, L"    Disables map fog across chapter changes without removing water textures.")
// L"[1] 显示简洁版导演 HUD %s"
SKEETO_UI(Text_acce3606, L"[1] Show compact Director HUD %s")
// L"进入地图后已暂停（上次主菜单：Surface兼容回退）"
SKEETO_UI(Text_ad097a84, L"Paused in-game (last main menu renderer: Source Surface)")
// L"终局救援阶段 HUD  v%s"
SKEETO_UI(Text_ad2a7118, L"Finale rescue phase HUD  v%s")
// L"本机本地时间，不是游戏内时间"
SKEETO_UI(Text_adffff69, L"Your computer's local time, not game time")
// L"感染者击杀"
SKEETO_UI(Text_ae0c8bc8, L"Infected killed")
// L"直接监听 tank_spawn / create_panic_event，不轮询实体"
SKEETO_UI(Text_ae84a3c5, L"Listens directly for tank_spawn / create_panic_event; no entity polling")
// L"显示玩家噪声等级"
SKEETO_UI(Text_ae9c33a3, L"Show player noise levels")
// L"[1] 静态准星 %s"
SKEETO_UI(Text_aed6a59e, L"[1] Static crosshair %s")
// L"黑色狂欢节"
SKEETO_UI(Text_aeebb913, L"Dark Carnival")
// L"[2] 强制去除地图迷雾 %s"
SKEETO_UI(Text_af837010, L"[2] Force map fog off %s")
// L"[1] 显示被控 HUD %s"
SKEETO_UI(Text_afae46e6, L"[1] Show pinned survivor HUD %s")
// L"HUD 分辨率自适应"
SKEETO_UI(Text_aff809a6, L"Scale HUDs to resolution")
// L"    菜单保存默认值；启动项 -skeeto_particle_platform 1/0 仅覆盖本次启动"
SKEETO_UI(Text_b05fe064, L"    Menu saves the default; launch option -skeeto_particle_platform 1/0 overrides only this session")
// L"还可推 %d"
SKEETO_UI(Text_b0719d80, L"Shoves Left: %d")
// L"[1] 大幅降低无声特感发生的概率 %s"
SKEETO_UI(Text_b08abdc8, L"[1] Reduce silent special infected %s")
// L"关闭"
SKEETO_UI(Text_b09cdc0e, L"Disable")
// L"主菜单背景关闭时不会初始化、扫描资源或创建解码线程。"
SKEETO_UI(Text_b0a12810, L"When disabled, the background engine does not initialize, scan assets, or create decoding threads.")
// L"特感"
SKEETO_UI(Text_b1024cf3, L"SI")
// L"    不含自己；1~8，按玩家槽顺序截断多余队友"
SKEETO_UI(Text_b1455e19, L"    Excludes yourself; 1~8, truncating extra teammates in player-slot order")
// L"更新暂存目录正在被占用，Skeeto 会继续重试。\nThe staging directory is busy; Skeeto will retry."
// L"(未发现)"
SKEETO_UI(Text_b22bcb72, L"(Not found)")
// L"按数字选择角色，再用 - / + 实时切换；服务器默认表示完全不覆盖。"
SKEETO_UI(Text_b2d70d11, L"Select a character by number, then use - / + to cycle live; Server default means no override.")
// L"铁铲"
SKEETO_UI(Text_b3011a58, L"Shovel")
// L"特感硬直 / 踉跄提示"
SKEETO_UI(Text_b3384f73, L"SI stun / stagger indicator")
// L"[6] 普感素材包 【%s】"
SKEETO_UI(Text_b392dd20, L"[6] Common infected pack [%s]")
// L"    坦克或 Director/Panic 尸潮事件出现时弹出约 2 秒，可单独开关并共用位置"
SKEETO_UI(Text_b3a5e68e, L"    Tank or Director/Panic horde events show for about 2 seconds; separate toggles, shared position")
// L"[5] %s绿色 G 【%d】"
SKEETO_UI(Text_b3e4bae4, L"[5] %sGreen G [%d]")
// L"[6] 间隙 【%d】"
SKEETO_UI(Text_b46e7957, L"[6] Gap [%d]")
// L"MP4按时间戳后台解码并播放音轨；BIK交给游戏原生播放器。进入地图后画面与声音全部暂停。"
SKEETO_UI(Text_b4839be7, L"MP4 uses timestamp-based background decoding with audio; BIK uses the native player. Video and audio pause in-game.")
// L"深灰"
SKEETO_UI(Text_b51ef7b8, L"Dark Gray")
// L"    官图战役名 + 第几/几关；三方图也能显示第几/几关（能识别关卡号时）"
SKEETO_UI(Text_b5dc54f2, L"    Campaign and chapter progress above, chapter name below; missing names use the original display")
// L"已读取部分数据"
SKEETO_UI(Text_b626cd8f, L"Partial data loaded")
// L"连喷"
SKEETO_UI(Text_b6c25de4, L"Auto Shotgun")
// L"（未生效）"
SKEETO_UI(Text_b6f11fd9, L"(Not active)")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_b7484dd9, L"Memory HUD")
// L"最多同时 5 条，默认右对齐。只记玩家、特感、女巫，不刷普通感染者。开启后关掉原版红色击杀记录，关掉后还原。"
SKEETO_UI(Text_b762874f, L"Up to 5 entries, right-aligned by default. Players, SI and Witches only. Hides stock red kill feed while enabled; restores it when disabled.")
// L"队友惊扰女巫警示图标"
SKEETO_UI(Text_b7ee4fe2, L"Teammate Witch startle warning icon")
// L"→ 下一页（→键）"
SKEETO_UI(Text_b825dd88, L"-> Next page (Right arrow)")
// L"[5] 第三人称瞄点修正 %s"
SKEETO_UI(TextXhairTpRow, L"[5] Third-person crosshair [Settings] %s")
// L"红"
SKEETO_UI(Text_b8698a18, L"Red")
// L"应用失败：剪贴板正被其它程序占用，请稍后重试。"
SKEETO_UI(Text_b8a3b333, L"Apply failed: clipboard is busy. Try again later.")
// L"    压力柱按玩家槽；阶段名与原版导演 HUD 相同（BUILD_UP / SUSTAIN_PEAK / PEAK_FADE / RELAX）"
SKEETO_UI(Text_b917fe6b, L"    Intensity bars follow player slots; stock phases: BUILD_UP / SUSTAIN_PEAK / PEAK_FADE / RELAX")
// L"[2] 一并关闭整体音效处理 %s"
SKEETO_UI(Text_b93f7f8f, L"[2] Also disable all DSP effects %s")
// L"有人倒地正在被救才显示，和队友血条里那条互不影响"
SKEETO_UI(Text_b9874f05, L"Visible while an incapacitated player is being revived; independent of teammate health-bar progress")
// L"速度"
SKEETO_UI(Text_ba77560c, L"Speed")
// L"六"
SKEETO_UI(Text_bacffcf1, L"Sat")
// L"[9] 本地服体验优化"
// L"青"
SKEETO_UI(Text_baf29bbd, L"Cyan")
// L"[2] 菜单样式"
// L"[2] 友伤反馈 %s"
SKEETO_UI(Text_bb642853, L"[2] Friendly-fire feedback %s")
// L"资源默认"
SKEETO_UI(Text_bb6be715, L"Asset Default")
// L"特感连杀 HUD"
SKEETO_UI(Text_bbaf86d0, L"Special infected streak HUD")
// L"    自己近战击杀 Witch 时，在死亡位置显示约 2 秒冰蓝王冠"
SKEETO_UI(Text_bbbfa274, L"    When you melee-kill a Witch, an icy-blue crown at the death position for about 2 seconds")
// L"[1] 命中反馈"
SKEETO_UI(Text_bbdb9fb3, L"[1] Hit feedback")
// L"按块藏，互不影响。关某一项只把那一块还原"
SKEETO_UI(Text_bc725cc0, L"Hide sections independently. Disabling one option restores only that section")
// L"[5] 描边颜色与渲染 【进入设置】"
SKEETO_UI(Text_bc8f7842, L"[4] Glow colors and rendering [Settings]")
// L"四角"
SKEETO_UI(Text_bcf873e9, L"Corners")
// L"[1] 显示坦克出现提示 %s"
SKEETO_UI(Text_bd20609b, L"[1] Show Tank spawn alerts %s")
// L"[9] 描边 【%s%d】"
SKEETO_UI(Text_bd3375ef, L"[9] Outline [%s%d]")
// L"[2] Nextbot刷新率 【不改】"
SKEETO_UI(Text_bd38c99a, L"[2] NextBot update interval [Unchanged]")
// L"你自己"
SKEETO_UI(Text_bd40abe0, L"You")
// L"    外置叠加层瞄准准星"
SKEETO_UI(Text_bd47adde, L"    External overlay aiming crosshair")
// L"靠下"
SKEETO_UI(Text_bd5c81fa, L"Bottom")
// L"默认"
SKEETO_UI(Text_bd99cc81, L"Default")
// L"    自己非近战击杀 Witch；一枪秒杀会显示独立 Perfect 样式"
SKEETO_UI(Text_bdf01aec, L"    Your non-melee Witch kills; one-shot kills use a separate Perfect style")
// L"当前画面更新：%d.%d FPS  播放速度：%d%%  缓冲：%d/%d"
SKEETO_UI(Text_be263f08, L"Refresh: %d.%d FPS  Playback speed: %d%%  Buffer: %d/%d")
// L"[2] 整体 【%d%%】  `-`/`=`"
SKEETO_UI(Text_be32b066, L"[2] Scale [%d%%]  `-`/`=`")
// L"特感血条"
SKEETO_UI(Text_be3a68e7, L"SI health bars")
// L"大数字"
SKEETO_UI(Text_be3acf81, L"Large Numbers")
// L"    队友救起自己或解除自己被控时，在施救者头顶显示约 2 秒盾牌十字"
SKEETO_UI(Text_be4836aa, L"    A shield-and-cross above the teammate who revives or frees you, for about 2 seconds")
// L"[4] MP4音量：%d%%"
SKEETO_UI(Text_be5385d7, L"[4] MP4 volume: %d%%")
// L"医疗包"
SKEETO_UI(Text_be7e1d54, L"First Aid Kit")
// L"← 上一页（←键）   → 下一页（→键）"
SKEETO_UI(Text_bf89c110, L"Left: previous page   Right: next page")
// L"地面胆汁"
SKEETO_UI(Text_bfc75a75, L"Ground Bile")
// L"粉"
SKEETO_UI(Text_c00a26c9, L"Pink")
// L"居中"
SKEETO_UI(Text_c06d0d93, L"Center")
// L"队友黑白状态提示HUD"
SKEETO_UI(Text_c150da48, L"Teammate black-and-white alert HUD")
// L"队友进入黑白后显示约 5 秒，不是常驻。多名队友同时触发时往下排。"
SKEETO_UI(Text_c16ff500, L"Shows for about 5 seconds after a teammate turns black-and-white. Simultaneous alerts stack downward.")
// L"T字+点"
SKEETO_UI(Text_c183ba6c, L"T + Dot")
// L"    仅真实掉血触发"
SKEETO_UI(Text_c1ac693f, L"    Triggers only on actual health loss")
// L"十字"
SKEETO_UI(Text_c1adb402, L"Cross")
// L"地面危险持续时间  v%s"
SKEETO_UI(Text_c1b19094, L"Ground hazard duration  v%s")
// L"[4] 贴图准星 %s"
SKEETO_UI(Text_c1cf0221, L"[4] Texture crosshair [Open settings] %s")
// L"[5] 对齐 【%s】"
SKEETO_UI(Text_c1ea765b, L"[5] Alignment [%s]")
// L"    隐藏全部图标与粒子"
SKEETO_UI(Text_c252955c, L"    Hides all icons and particles")
// L"上中"
SKEETO_UI(Text_c345c03e, L"Top Center")
// L"枪空爆 Hunter / Jockey（Skeet）"
SKEETO_UI(Text_c35c7800, L"Shoot airborne Hunter / Jockey (Skeet)")
// L"队友胆汁感染标记"
SKEETO_UI(Text_c3c9f09e, L"Teammate bile mark")
// L"[2] 左右位置 【%s】"
SKEETO_UI(Text_c41e4dcc, L"[3] Horizontal position [%s]")
// L"    在分辨率自适应之后再缩放"
SKEETO_UI(Text_c435498d, L"    Applies additional scaling after resolution adjustment.")
// L"角色色"
SKEETO_UI(Text_c4522936, L"Character Colors")
// L"战术格"
SKEETO_UI(Text_c487d4ab, L"Tactical Grid")
// L"    特感进入约 300 码范围时再补发一次，并带方向感；不需要此效果可单独关闭。"
SKEETO_UI(Text_c49176e4, L"    Plays another directional cue within about 300 units; disable this separately if unwanted.")
// L"不改"
SKEETO_UI(Text_c495bc74, L"Unchanged")
// L"主武器带高爆弹、燃烧弹或激光瞄准器时显示图标。特殊弹药打完消失，激光会一直留着。无底框、无文字。"
SKEETO_UI(Text_c4a7ca12, L"Primary weapon upgrade icons. Special ammo disappears when spent; laser stays. No text or background.")
// L"    当前约 %d. 128 很吃 CPU"
SKEETO_UI(Text_c4dfc6ff, L"    Current ~%d. 128 is CPU-intensive.")
// L"被 Charger 带走"
SKEETO_UI(Text_c50ace51, L"Carried by Charger")
// L"    防止顶点上限弹窗闪退。"
SKEETO_UI(Text_c57ab6e4, L"    Helps prevent vertex-limit error popups and crashes.")
// L"等待数据"
SKEETO_UI(Text_c57f260f, L"WAIT")
// L"[3] 打开 Skeeto 更新日志"
// L"Rochelle 使用场次"
SKEETO_UI(Text_c5fefc7f, L"Games as Rochelle")
// L"    在玩家周围生成柔和动态光源，照亮附近黑暗区域"
SKEETO_UI(Text_c6035f8f, L"    Creates a soft dynamic light around the player to illuminate nearby dark areas.")
// L"右对齐"
SKEETO_UI(Text_c6c73a06, L"Right")
// L"[4] Lerp 【不改】"
SKEETO_UI(Text_c7859162, L"[4] Lerp [Unchanged]")
// L"[2] 当前预设：%s"
SKEETO_UI(Text_c7bd84a8, L"[2] Current preset: %s")
// L"[5] 手电筒亮度调节 【%.2f】"
SKEETO_UI(Text_c81711cd, L"[5] Flashlight brightness [%.2f]")
// L"自定义"
SKEETO_UI(Text_c84b66c6, L"Custom")
// L"[5] 提示条 %s"
SKEETO_UI(Text_c90536b7, L"[5] Hint bars %s")
// L"    接管后将第一人称视角恢复到进入闲置前的朝向；不改变闲置期间人机行为。"
SKEETO_UI(Text_c9651930, L"    Restores your first-person view direction on takeover; does not change bot behavior while idle.")
// L"    队友首次惊扰女巫时，在其头顶显示约 5 秒三角爪痕警示；自己惊扰不显示"
SKEETO_UI(Text_c96e2a1e, L"    First teammate Witch startle: overhead triangular claw warning for about 5 seconds; excludes your own startles")
// L"切角框"
SKEETO_UI(Text_c9730865, L"Clipped Corners")
// L"总游戏时长"
SKEETO_UI(Text_c97d1a8a, L"Total playtime")
// L"%s  第%d/%d关"
SKEETO_UI(Text_c995d810, L"%s  Chapter %d/%d")
// L"[5] 长度 【%d】"
SKEETO_UI(Text_c99fa85b, L"[5] Length [%d]")
// L"[1] 显示队友投掷物提示 HUD %s"
SKEETO_UI(Text_ca41d479, L"[1] Show teammate throwable alert HUD %s")
// L"此页选项在联机服务器内也会生效"
SKEETO_UI(Text_caa31066, L"Options on this page also work on online servers")
// L"    修复闲置响应延迟，按即闲置"
SKEETO_UI(Text_cab808c2, L"    Makes going idle respond immediately.")
// L"    三方图：显示地图名；文件名带 m2/m3 时标出第几关"
SKEETO_UI(Text_caf17fe4, L"    Campaign name above, chapter name below; missing names use the original display")
// L"开麦提示 HUD  v%s"
SKEETO_UI(Text_cafe7614, L"Voice activity HUD  v%s")
// L"    超时从第一杀重计；无连杀档位的素材包不受影响"
SKEETO_UI(Text_cb0c7ca7, L"    Timeout restarts at kill 1; packs without streak tiers are unaffected")
// L"不会藏插件数字菜单（Admin Menu / 投票等）"
SKEETO_UI(Text_cb8bb06b, L"Does not hide numbered plugin menus (Admin Menu, votes, etc.)")
// L"[1] 显示额外弹药 HUD %s"
SKEETO_UI(Text_cbbaeb33, L"[1] Show extra ammo HUD %s")
// L"本机水平速度，不是垂直速度"
SKEETO_UI(Text_cbbcf735, L"Your horizontal speed, excluding vertical motion")
// L"    仅在子菜单中启用的项目写入对应客户端 Cvar；默认全部关闭。"
SKEETO_UI(Text_cbc7ff8c, L"    Only enabled submenu items write their client cvars; all are off by default.")
// L"坦克 / 尸潮出现提示"
SKEETO_UI(Text_cc5a1b0a, L"Tank / horde spawn alerts")
// L"[4] 低弹药射击提示音，即Low Ammo Sound Cue %s"
SKEETO_UI(Text_cc74dbf1, L"[4] Low Ammo Sound Cue %s")
// L"    请先打开上面的「换人物功能」"
// L"平底锅"
SKEETO_UI(Text_ccef4908, L"Frying Pan")
// L"网络参数 HUD"
SKEETO_UI(Text_cd5441a1, L"Network stats HUD")
// L"聚光灯夜视"
SKEETO_UI(Text_cd705db6, L"Spotlight night vision")
// L"    换弹时按武器类型延迟播放弹匣落地声（需 weapons/spent_mags/* 素材；对应工坊 Clank!）"
SKEETO_UI(Text_cdf4601a, L"    Plays delayed magazine impacts by weapon type on reload (requires weapons/spent_mags/* assets; Workshop: Clank!).")
// L"    引擎散布圈"
SKEETO_UI(Text_ce05d245, L"    Engine-drawn spread circle")
// L"    开启此项才关闭原版默认显示的第一人称枪械曳光；关闭选项时恢复原值。"
SKEETO_UI(Text_ce0ce477, L"    Disables vanilla first-person weapon tracers when enabled; restores the original value when disabled.")
// L"参与战役"
SKEETO_UI(Text_ce1ee4ed, L"Campaigns played")
// L"坠机险途"
SKEETO_UI(Text_ce6e5cf8, L"Crash Course")
// L"    方向键挪这一栏（按住连移，Shift 大步）"
SKEETO_UI(Text_ce81b9c5, L"    Arrow keys move this panel (hold to repeat, Shift for larger steps)")
// L"    想使用此选项请不要开启服务器内的人机增强选项，尤其点名汽水服。人机增强的 ConVar 会有大概率导致此功能失效。此问题只能由服主自己修复。"
SKEETO_UI(Text_ceb1cc64, L"    Disable server bot enhancements when using this, especially on Qishui servers. Their ConVars often break this feature; only the server owner can fix that.")
// L"[3] 关闭普感布偶/尸体 %s"
SKEETO_UI(Text_cf0f9d87, L"[3] Disable common infected ragdolls/bodies %s")
// L"    分别显示当前已生成且存活的特感总数与 Smoker 数量；每 0.5 秒检查一次玩家槽位"
SKEETO_UI(Text_cf296a7e, L"    Spawned, living special infected total and Smoker count; checks player slots every 0.5 seconds")
// L"[2] 只显示当前弹匣 %s"
SKEETO_UI(Text_cf4ec8a1, L"[2] Current clip only %s")
// L"    同时写入阴影渲染距离上限，缓解部分场景阴影范围异常。"
SKEETO_UI(Text_cfc13436, L"    Also sets the maximum shadow render distance to reduce incorrect shadow coverage in some scenes.")
// L"[1] 低血量红屏提示，即Low Health Visual Cue %s"
SKEETO_UI(Text_cff3a6c6, L"[1] Low Health Visual Cue %s")
// L"    声音来源"
// L"平均每小时击杀"
SKEETO_UI(Text_d0b9efbe, L"Average kills per hour")
// L"死亡丧钟"
SKEETO_UI(Text_d0d4e433, L"Death Toll")
// L"    默认 8 秒内连续击杀特感；第 3 杀起弹出约 2 秒并显示当前连杀数"
SKEETO_UI(Text_d143b572, L"    Default: SI kills within 8 seconds; from the third kill, show the streak count for about 2 seconds")
// L"战斗与视觉辅助 (%d/%d)  v%s"
// L"深蓝"
SKEETO_UI(Text_d2f5ca1f, L"Dark Blue")
// L"    仅在终局事件开始后显示；记录每个 Tank 节点，并在救援车辆到达事件时结束阶段"
SKEETO_UI(Text_d3135d70, L"    Appears after the finale starts; records Tank stages and ends on the rescue-arrival event")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_d31d1047, L"Memory HUD  v%s")
// L"找不到文件。"
SKEETO_UI(Text_d3369181, L"File not found.")
// L"Stretch（拉伸）"
SKEETO_UI(Text_d3515d48, L"Stretch")
// L"队友互动进度条"
SKEETO_UI(Text_d3c0f75f, L"Teammate interaction progress")
// L"    只调命中反馈音效，不影响游戏音量/音乐音量"
SKEETO_UI(Text_d3e32b3f, L"    Affects only hit feedback sounds, not game/music volume")
// L"土质炸弹"
SKEETO_UI(Text_d4044787, L"Pipe Bomb")
// L"幸存者"
SKEETO_UI(Text_d455cf05, L"Survivor")
// L"短暂时刻"
SKEETO_UI(Text_d484ccce, L"The Passing")
// L"【关】"
SKEETO_UI(Text_d50b7932, L"[OFF]")
// L"[5] 显示 【%s】"
SKEETO_UI(Text_d52dca2c, L"[5] Display [%s]")
// L"[9] 特感音效 %s"
SKEETO_UI(Text_d55e1d44, L"[9] SI sounds %s")
// L"%s 投掷了 %s"
SKEETO_UI(Text_d5993eeb, L"%s threw %s")
// L"[6] 换弹弹匣落地音，即Clank! %s"
SKEETO_UI(Text_d5a4a145, L"[6] Magazine drop sounds: Clank! %s")
// L"燃烧瓶"
SKEETO_UI(Text_d5d47924, L"Molotov")
// L"击杀列表 HUD"
SKEETO_UI(Text_d627b7c0, L"Kill feed HUD")
// L"支持中文、英文和空格，最多 23 个字符。控制台输入含空格时必须使用双引号。"
SKEETO_UI(Text_d6446b86, L"Supports Chinese, English, and spaces, up to 23 characters. Enclose names containing spaces in double quotes in the console.")
// L"[9] 强制地图全亮 %s"
SKEETO_UI(Text_d6b53c68, L"[9] Force fullbright %s")
// L"    大数字 / 角色色 / 切角框"
SKEETO_UI(Text_d6e1ecb1, L"    Large numbers / Character colors / Chamfered frame")
// L"[3] 样式 【%s】"
SKEETO_UI(Text_d6fc9a3e, L"[3] Style [%s]")
// L"[1] 隐藏 EMS 脚本 HUD %s"
SKEETO_UI(Text_d7673bf6, L"[1] Hide EMS script HUD %s")
// L"客户端"
SKEETO_UI(Text_d7878eec, L"Client")
// L"纯白"
SKEETO_UI(Text_d7bc4ab4, L"White")
// L"按玩家槽画压力柱。阶段名读取与原版导演 HUD 相同的导演节奏，不用压力值猜测"
SKEETO_UI(Text_d7d6edab, L"Intensity bars by player slot. Reads actual stock Director pacing phases, not guesses from intensity")
// L"    经典 / 竞技条 / 极简 / 战术格"
SKEETO_UI(Text_d7f71cb9, L"    Classic / Competitive / Minimal / Tactical grid")
// L"    强制拉满可视距离。"
SKEETO_UI(Text_d7fd2bcf, L"    Forces maximum view distance.")
// L"动态准星  v%s"
SKEETO_UI(Text_d83f10f4, L"Dynamic crosshair  v%s")
// L"[3] 允许 0 lerp %s"
SKEETO_UI(Text_d84f14b6, L"[3] Allow 0 lerp %s")
// L"[1] 队友框 / 血条 %s"
SKEETO_UI(Text_d85bfc98, L"[1] Teammate panels / health bars %s")
// L"排名依据 Steam 记录的累计击杀数，包含枪械、近战和投掷物"
SKEETO_UI(Text_d87a4767, L"Ranked by Steam lifetime kills, including firearms, melee weapons, and throwables")
// L"极简"
SKEETO_UI(Text_d8cb2cee, L"Minimal")
// L"肾上腺素"
SKEETO_UI(Text_d8df1ca7, L"Adrenaline")
// L"仅写入客户端 Cvar；关闭后恢复启用前的值。过关时会重新应用已开启项目。"
SKEETO_UI(Text_d906d9dd, L"Writes client cvars only; disabling restores prior values. Enabled overrides are reapplied on chapter changes.")
// L"[1] 修改人机名字 【进入设置】"
SKEETO_UI(Text_d9904fa8, L"[1] Rename bots [Settings]")
// L"章节重置、团灭清零"
SKEETO_UI(Text_d9ac0ea8, L"Reset on Chapter / Team Wipe")
// L"电击器"
SKEETO_UI(Text_d9fa0654, L"Defibrillator")
// L"用法：把原版格式 cfg 放到 left4dead2/cfg/，文件名必须以 skeeto_ 开头，例如 skeeto_preset1.cfg、skeeto_fog.cfg。只认松散文件。"
SKEETO_UI(Text_da2ac72c, L"Place standard cfg files in left4dead2/cfg/ with names starting with skeeto_, e.g. skeeto_preset1.cfg or skeeto_fog.cfg. Loose files only.")
// L"暖白"
SKEETO_UI(Text_da569ec1, L"Warm White")
// L"改版服用 EMS 脚本 HUD 强制画的白字（HudScriptedMode）"
SKEETO_UI(Text_da71bd51, L"White text forced by server EMS scripts (HudScriptedMode)")
// L"开麦提示 HUD"
SKEETO_UI(Text_da7bd7d8, L"Voice activity HUD")
// L"    默认关闭。关闭时不创建、不写入该文件，对游戏无影响"
SKEETO_UI(Text_da823711, L"    Off by default. When disabled, no file is created or written; gameplay is unaffected.")
// L"  ... 另有 %d 行未列出"
SKEETO_UI(Text_da83dde3, L"  ... %d more lines not shown")
// L"    可自行DIY新增，JSON文件可配置所有路径与规则"
SKEETO_UI(Text_dab3f710, L"    Add your own effects; all paths and rules are configurable in JSON")
// L"[2] 连杀判定时间 【%d 秒】"
SKEETO_UI(Text_daec7718, L"[2] Streak window [%d seconds]")
// L"保护队友"
SKEETO_UI(Text_db0ab67b, L"Teammates protected")
// L"导演压力 HUD"
SKEETO_UI(Text_db47d48d, L"Director intensity HUD")
// L"%s暂时取消了你的观战资格"
SKEETO_UI(Text_dc090d42, L"%s postponed your trip to spectator mode")
// L"%s：Pills Here?"
SKEETO_UI(Text_6f2c8b47, L"%s: Pills Here?")
// L"关键保护"
SKEETO_UI(Text_8e4fa2c6, L"Saved You")
// L"黑白预警"
SKEETO_UI(Text_c7d91a53, L"Last Life")
// L"    普通/爆头 Skeet 均计入，在死亡位置显示约 2 秒红色爆点"
SKEETO_UI(Text_dc1d5738, L"    Includes normal/headshot Skeets; red impact emblem at the death position for about 2 seconds")
// L"酒红"
SKEETO_UI(Text_dc4f0e81, L"Burgundy")
// L"特感血条  v%s"
SKEETO_UI(Text_dc574edb, L"Special infected health bars  v%s")
// L"[8] 特感画面 %s"
SKEETO_UI(Text_dcafede6, L"[8] SI visuals %s")
// L"    VTF贴图，兼容工坊 / DIY"
SKEETO_UI(Text_dcd1eede, L"    VTF texture; supports Workshop and custom assets")
// L"    临时隐藏SKT的所有自定义HUD，不改变各 HUD 的开关状态，不影响隐藏原版 HUD；控制台指令：skeeto_hud_toggle"
SKEETO_UI(Text_dd7fbe6f, L"    Temporarily hides all SKT custom HUDs without changing their settings or stock HUD hiding; console: skeeto_hud_toggle")
// L"    仅藏操作键位提示/公告/队友危险/离开区域；不含游戏提示（惊扰女巫等），那种请在游戏设置关"
SKEETO_UI(Text_ddaf15e1, L"    Hides key prompts/announcements/teammate danger/leave-area hints; game hints such as Witch startles use game settings")
// L"命中反馈 (2/2)  v%s"
SKEETO_UI(Text_ddf09c3d, L"Hit feedback (2/2)  v%s")
// L"[1] 显示 Tank 举石预警 %s"
SKEETO_UI(Text_de549a32, L"[1] Show Tank rock windup alerts %s")
// L"[2] 整体大小 【%d%%】  `-`/`=`"
SKEETO_UI(Text_de54c72e, L"[2] Overall size [%d%%]  `-`/`=`")
// L"[3] 复位位置（右上）"
SKEETO_UI(Text_de5c0f2e, L"[3] Reset position (top right)")
// L"电吉他"
SKEETO_UI(Text_de783107, L"Electric Guitar")
// L"浅黄"
SKEETO_UI(Text_de7eac77, L"Light Yellow")
// L"[3] 字号 +2（循环）"
SKEETO_UI(Text_de91b2a4, L"[3] Font size +2 (cycle)")
// L"    队友击杀 Tank 时，在击杀者头顶显示约 5 秒金色奖杯"
SKEETO_UI(Text_ded4bbd3, L"    A gold trophy above a teammate who kills a Tank, for about 5 seconds")
// L"三"
SKEETO_UI(Text_df280cf1, L"Wed")
// L"胆汁危险"
SKEETO_UI(Text_df334bd4, L"Bile Danger")
// L"    医疗包、救援及通用长按 E 期间，在操作者头顶显示精确进度"
SKEETO_UI(Text_df8ac56d, L"    Precise overhead progress during healing, reviving and general hold-E interactions")
// L"[1] 动态准星 %s"
SKEETO_UI(Text_e04e43c1, L"[1] Dynamic crosshair %s")
// L"    官图：战役中文名 + 第几/共几关 + 地图代码"
SKEETO_UI(Text_e0b0fd6f, L"    Official maps: Chinese campaign name + chapter/total + map code")
// L"从回合开始计时，不是本机时钟"
SKEETO_UI(Text_e17f576e, L"Time since round start, not the system clock")
// L"[1] 投掷物落点预览 %s"
SKEETO_UI(Text_e1b9d99e, L"[1] Throwable landing preview %s")
SKEETO_UI(ThrowLandEntry, L"Throwable landing preview")
SKEETO_UI(ThrowLandTitle, L"Throwable trajectory and landing  v%s")
SKEETO_UI(ThrowLandStyleRow, L"[2] Trajectory style [%s]")
SKEETO_UI(ThrowLandRefreshRow, L"[3] Prediction refresh rate [%s]")
SKEETO_UI(ThrowLandShowModeRow, L"[4] Display mode [%s]")
SKEETO_UI(ThrowLandShowOnAttack, L"While holding left mouse")
SKEETO_UI(ThrowLandShowWhileHeld, L"While holding a throwable")
SKEETO_UI(ThrowLandRefreshDefault, L"Default")
SKEETO_UI(ThrowLandRefreshHint, L"    Default: 20 Hz while moving, about 7 Hz at steady aim. Selected rates apply to both, limited by game FPS; higher rates are smoother and increase collision calculation cost.")
SKEETO_UI(NecolaStartupEntry, L"[6] Necola startup integration")
SKEETO_UI(NecolaStartupTitle, L"Necola startup integration")
SKEETO_UI(NecolaStartupToggle, L"[1] Automatically load Necola at startup %s")
SKEETO_UI(NecolaStartupPurpose, L"Enabled by default. If native Necola is installed, Skeeto can load it when the game starts normally, allowing Necola to work when launching through Steam or other normal methods, without configuring launch arguments in kpatch.ini or using a separate Necola launcher.")
SKEETO_UI(NecolaStartupRequirement, L"Skeeto does not include Necola. Only native Necola cores in the game root are recognized; a core already loaded by another method is reused without loading another copy. Necola keeps its own settings and menu.")
SKEETO_UI(NecolaStartupRestart, L"Changes take effect next time you start the game. Disabling only prevents Skeeto from actively loading Necola; it neither unloads running Necola nor blocks other launchers from loading it.")
SKEETO_UI(NecolaStartupLaunch, L"Launch option -skeeto_necola 0/1 overrides only this startup and does not change the saved switch.")
SKEETO_UI(NecolaStartupOrigin, L"Given the shared history of Skeeto and Necola, this integration is deliberately retained to make things more convenient for Necola users.")
SKEETO_UI(ThrowLandStyleClassic, L"Original line")
SKEETO_UI(ThrowLandStyleBeam, L"Light beam")
SKEETO_UI(ThrowLandStyleHelix, L"Helix orbit")
SKEETO_UI(ThrowLandStyleTunnel, L"Holographic tunnel")
SKEETO_UI(ThrowLandClassicHint, L"    The original thin line and landing marker, with minimal visual detail.")
SKEETO_UI(ThrowLandBeamHint, L"    An ice-blue halo surrounds a bright white core to form a continuous beam.")
SKEETO_UI(ThrowLandHelixHint, L"    Cyan and purple strands spiral around the predicted centerline.")
SKEETO_UI(ThrowLandTunnelHint, L"    Green cross-section rings and rails form a tunnel; the centerline shows the actual trajectory.")
// L"Tank 行为预测"
SKEETO_UI(TankPredictEntry, L"Tank attack prediction")
// L"    跟随 Tank 画出拳计时、朝向、攻击范围和看向目标；估算值，不是命中判定"
SKEETO_UI(TankPredictEntryDesc, L"    Follows Tank with punch timer, facing, attack range, and look-at target; estimates, not hit checks.")
// L"Tank 行为预测  v%s"
SKEETO_UI(TankPredictTitle, L"Tank attack prediction  v%s")
// L"跟随 Tank 的世界空间提示。出拳计时是估算，不是命中或安全窗口。Mod 可能误判。"
SKEETO_UI(TankPredictPageHint, L"World-space hints that follow Tank. Punch timer is an estimate, not a hit or safe-window check. Mods may misidentify.")
// L"[1] Tank 行为预测 %s"
SKEETO_UI(TankPredictMaster, L"[1] Tank attack prediction %s")
// L"    打开后由下面四项控制显示。出拳计时按预备时间与出拳间隔之和估算，不是命中或安全窗口；Mod 可能误判；投石时停出拳计时，举石预警屏幕条仍独立；朝向箭头是出拳朝向，不是假仇恨；目标连线使用现有看向目标，与屏幕仇恨面板互补。"
SKEETO_UI(TankPredictMasterHint, L"    After enabling, the four items below choose what is shown. Punch timer estimates wind-up plus swing interval, not a hit or safe window; Mods may misidentify; rock throw stops the punch timer while the rock-notice screen bar stays independent; facing arrow is punch facing, not fake aggro; attack range is the live swing disk; target line uses the existing look-at target and complements the screen aggro HUD.")
// L"[2] 出拳计时 %s"
SKEETO_UI(TankPredictTimer, L"[2] Punch timer %s")
// L"[3] 朝向箭头 %s"
SKEETO_UI(TankPredictFacing, L"[3] Facing arrow %s")
// L"[4] 攻击范围 %s"
SKEETO_UI(TankPredictRange, L"[4] Attack range %s")
// L"[5] 目标连线 %s"
SKEETO_UI(TankPredictTarget, L"[5] Target line %s")
// L"    藏改版服 EMS 白字（HudScriptedMode）；不含聊天/投票/原版/SKT"
SKEETO_UI(Text_e1c5a0b1, L"    Hide server EMS text (HudScriptedMode); excludes chat/votes/stock/SKT")
// L"榴弹"
SKEETO_UI(Text_e20cacf7, L"Grenade Launcher")
// L"行动受限中  %.1fs"
SKEETO_UI(Text_e27703b2, L"Movement Limited  %.1fs")
// L"[6] Zoey 佐伊"
SKEETO_UI(Text_e2904f2d, L"[6] Zoey")
// L"需在生还者队。一代/二代图都会预缓存全套模型。"
SKEETO_UI(Text_e2b43d12, L"You must be on the survivor team. All models are precached on both L4D1 and L4D2 maps.")
// L"尚未读取"
SKEETO_UI(Text_e34615bc, L"Not loaded yet")
// L"[1] 显示道具 HUD %s"
SKEETO_UI(Text_e3f21fd6, L"[1] Show item HUD %s")
// L"已保存并启用：%s"
SKEETO_UI(Text_e4cd9534, L"Saved and enabled: %s")
// L"万/亿缩写"
SKEETO_UI(Text_e4dadb6f, L"Chinese Units (10K/100M)")
// L"菜单样式 (%d/%d)  v%s"
SKEETO_UI(Text_e519913f, L"Menu appearance (%d/%d)  v%s")
// L"    无色块底框，血条里原来的弹药数字仍保留"
SKEETO_UI(Text_e55d709e, L"    Shows held-gun ammo only; team-bar numbers remain.")
// L"[2] Nextbot刷新率 【0.024 更猛】"
SKEETO_UI(Text_e56dafb9, L"[2] NextBot update interval [0.024 More responsive]")
// L"[8] 换人物功能 %s"
// L"[4] 自动清理 datacache %s"
SKEETO_UI(Text_e6066de5, L"[5] Automatically clear datacache %s")
// L"K/M 缩写"
SKEETO_UI(Text_e61f7bba, L"K/M Abbreviations")
// L"[1] 显示后摇环 %s"
SKEETO_UI(Text_e740f374, L"[1] Show recovery ring %s")
// L"止痛药  使用 / 分享"
SKEETO_UI(Text_e7605f38, L"Pain pills  Used / Shared")
// L"[5] 过关前撤销旧预设写入 %s"
SKEETO_UI(Text_e7f0cc90, L"[5] Restore old preset values before chapter changes %s")
// L"    建议把原版十字线透明度调到 0（控制台 cl_crosshair_alpha 0）"
SKEETO_UI(Text_e866349b, L"    Set vanilla crosshair opacity to 0 (console: cl_crosshair_alpha 0).")
// L"[2] Nextbot刷新率 【0.1】"
SKEETO_UI(Text_e8698489, L"[2] NextBot update interval [0.1]")
// L"—— 1. 粒子平台 ——"
SKEETO_UI(Text_e8edbc11, L"-- 1. Particle platform --")
// L"救援"
SKEETO_UI(Text_e8f5f3ea, L"Rescue")
// L"[3] 空仓射击提示音，即Empty Magazine Sound Cue %s"
SKEETO_UI(Text_e900b7d0, L"[3] Empty Magazine Sound Cue %s")
// L"枪杀 Witch（Crown / Perfect）"
SKEETO_UI(Text_e995489f, L"Shoot Witch (Crown / Perfect)")
// L"无法完成更新暂存，Skeeto 会继续重试。\nCould not finalize staging; Skeeto will retry."
// L"名字会保存到设置；关闭总开关仅停止替换，不会删除已保存名字。"
SKEETO_UI(Text_e9d93fbe, L"Names are saved in settings; turning off the master switch stops replacement without deleting saved names.")
// L"关 "
SKEETO_UI(Text_e9ec4fe7, L"Off ")
// L"队友救起自己或解除自己被控时显示约 5 秒。"
SKEETO_UI(Text_ea164045, L"Shows for about 5 seconds when a teammate revives or frees you.")
// L"—— 样式 ——"
SKEETO_UI(Text_ea3973b9, L"-- Style --")
// Memory HUD / independent datacache cleanup.
SKEETO_UI(Text_ea5d7dd8, L"    Reads game and Windows data; retains the current font, position and style")
// L"[5] 复位位置（准星右下方）"
SKEETO_UI(Text_ea6053db, L"[5] Reset position (below-right of crosshair)")
// L"玩家8"
SKEETO_UI(Text_ea8881a8, L"Player8")
// L"换人物失败：需要生还者"
SKEETO_UI(Text_eacb30f9, L"Switch failed: you must be a survivor.")
// L"    图标+粒子来源；缺失不借另一套"
SKEETO_UI(Text_eacd0f8a, L"    Icon and particle source; no automatic substitution")
// L"原版出场音乐"
SKEETO_UI(Text_eb05cb72, L"Original Spawn Music")
// L"【使用特感专用素材】"
SKEETO_UI(Text_ece2df2b, L"[Use SI-specific assets]")
// L"    完整/精简预设仍在上一页 [5]"
SKEETO_UI(Text_ed168d08, L"    Full/Compact presets are on the previous page [5]")
// L"下"
SKEETO_UI(Text_ed1e6e73, L"Bottom")
// L"控制台总开关：skeeto_bot_names 1 或 0。关闭后由游戏或其它 Mod 提供名字。"
SKEETO_UI(Text_ed20fcec, L"Console master switch: skeeto_bot_names 1 or 0. When off, names come from the game or other mods.")
// L"隐藏原版 HUD"
SKEETO_UI(Text_ed2b25df, L"Hide stock HUD")
// L"    不依赖HUD的res文件，只需要把vmt/vtf放到 materials/skeeto/xhair/（松散或 VPK 均可）。同时开静态准星时，静态会画在贴图上面。"
SKEETO_UI(Text_eda9d43f, L"    No HUD res files needed: place vmt/vtf files in materials/skeeto/xhair/ (loose or VPK). An enabled static crosshair draws on top.")
// L"SKT预设 #%02X%02X%02X"
SKEETO_UI(Text_ee61609c, L"SKT Preset #%02X%02X%02X")
// L"    主武器弹匣余量低于 20% 时，每次射击播放机械点击声（需 sound/lasc/click 素材）"
SKEETO_UI(Text_ee6756d8, L"    Plays a mechanical click on each shot with less than 20% remaining in the primary magazine (requires sound/lasc/click assets).")
// L"[3] Coach 教练"
SKEETO_UI(Text_ee831b2a, L"[3] Coach")
// L"相对 autoexec.cfg：启动只跑一次；这里每个新章节都会再 exec 当前选中的那一套，局内可切换，也可 bind skt_cfg_next / skt_cfg_prev / skt_cfg_apply。"
SKEETO_UI(Text_eec98e86, L"autoexec.cfg runs once at startup; this runs the selected preset each chapter. Switch in-game or bind skt_cfg_next / skt_cfg_prev / skt_cfg_apply.")
// L"[3] 范围 【%s】"
SKEETO_UI(Text_eed6ce6f, L"[3] Range [%s]")
// L"[8] 强制关闭天气系统 %s"
SKEETO_UI(Text_eeed8351, L"[8] Force weather off %s")
// L"高尔夫球杆"
SKEETO_UI(Text_ef6ad945, L"Golf Club")
// L"—— 特殊反馈 ——"
SKEETO_UI(HitFeedbackHitSection, L"-- Hit feedback --")
SKEETO_UI(HitFeedbackLimitSection, L"-- Catalog limits --")
SKEETO_UI(HitFeedbackFriendlySection, L"-- Friendly fire --")
// L"    章节不重置：回大厅清零"
SKEETO_UI(Text_efb06600, L"    Keep across chapters: reset on returning to lobby")
// L"消音冲锋枪"
SKEETO_UI(Text_efbc2f2e, L"Silenced SMG")
// L"[8] 视角倾斜，即Black Mesa View Tilt %s"
SKEETO_UI(Text_efbf530a, L"[9] Black Mesa View Tilt [Settings] %s")
// L"Zoey 佐伊"
SKEETO_UI(Text_efe5ac5b, L"Zoey")
// L"    外置叠加层绘制，用来瞄准，可与其他模块同时开"
SKEETO_UI(Text_f04d3d3e, L"    Drawn in an external overlay for aiming; works alongside other modules.")
// L"必须先开启“大幅降低无声特感发生的概率”；本 HUD 才会工作。可调整整体位置和大小。"
SKEETO_UI(Text_f06d5041, L"Requires Reduce silent special infected. Overall position and size are adjustable.")
// L"%u月%u日  周%s"
SKEETO_UI(Text_f0e98150, L"%u/%u  %s")
// L"[3] 同步播放连杀音效 %s"
SKEETO_UI(Text_f11ffb66, L"[3] Play streak sounds %s")
// L"固定机枪"
SKEETO_UI(Text_f157dd0f, L"Mounted Gun")
// L"世界时间"
SKEETO_UI(Text_f169afc8, L"Real-world clock")
// L"    不改 / 0 / 24 / 40 / 120 / 100"
SKEETO_UI(Text_f18be0b6, L"    Unchanged / 0 / 24 / 40 / 120 / 100")
// L"[1] Skeeto粒子平台 本次%s  默认%s"
SKEETO_UI(Text_f1a46dad, L"[1] Skeeto particle platform: this session %s  default %s")
// L"以下项目可分别启用，关闭后恢复启用前的设置。过关时会重新应用。"
SKEETO_UI(Text_f1a7ad42, L"These options can be enabled separately. Disabling restores prior settings; enabled settings are reapplied on chapter changes.")
// L"菱形"
SKEETO_UI(Text_f284ad89, L"Diamond")
// L"人物提示、伤害数字与投掷物轨迹，各功能独立开关。"
SKEETO_UI(Text_f2ee01ed, L"Character indicators, damage numbers and throwable trajectories, with separate switches.")
// L"[5] 默认关：能跨章节留下的 cvar 会继续留下。开启后，过关执行前会先把本功能写过的旧预设项恢复。"
SKEETO_UI(Text_f302976d, L"[5] Off by default: cvars that persist across chapters remain unchanged. When enabled, values written by the old preset are restored before applying the next chapter's preset.")
// L"[8] 队友血条最多 【--】"
SKEETO_UI(Text_f306313f, L"[8] Maximum teammate bars [--]")
// L"[4] 显示生还与特感移动路线 【%s】"
SKEETO_UI(Text_f3232cb3, L"[4] Show survivor/infected movement paths [%s]")
// L"叉子"
SKEETO_UI(Text_f354c80f, L"Pitchfork")
// L"双环"
SKEETO_UI(Text_f3685d71, L"Double Ring")
// L"修改 %s 的人机名字  v%s"
SKEETO_UI(Text_f386ef15, L"Rename %s bot  v%s")
// L"玩家9"
SKEETO_UI(Text_f393b0e9, L"Player9")
// L"[5] 每次启动随机选择：%s"
SKEETO_UI(Text_f3ea1048, L"[5] Random background on startup: %s")
// L"指令：skeeto_spotlight_brightness 20~200；skeeto_spotlight_radius 250~1200"
SKEETO_UI(Text_f3f26485, L"Commands: skeeto_spotlight_brightness 20~200; skeeto_spotlight_radius 250~1200")
// L"上"
SKEETO_UI(Text_f4055f32, L"Top")
// L"临死一搏"
SKEETO_UI(Text_f420c849, L"The Last Stand")
// L"当前渲染后端：等待首个视频帧"
SKEETO_UI(Text_f43ef3de, L"Renderer: waiting for the first video frame")
// L"黄色竖向进度条"
SKEETO_UI(Text_f4446c93, L"Yellow vertical progress bar")
// L"[1] 特殊击杀反馈 %s"
SKEETO_UI(Text_f471ee0e, L"[1] Special kill feedback %s")
// L"竖排显示自己的3/4/5号槽位，方便摆到准星附近观察"
SKEETO_UI(Text_f4b82c72, L"Vertical slots 3/4/5 for easy viewing near the crosshair")
// L"优化听声辨位  v%s"
SKEETO_UI(Text_f4ca14f3, L"Improve directional audio  v%s")
// L"金黄"
SKEETO_UI(Text_f4e44b7a, L"Gold")
// L"    方向键挪位置；音效目录 sound\\skeeto\\streak\\*.mp3"
SKEETO_UI(Text_f52ce0f2, L"    Arrow keys move; sound folder sound\\skeeto\\streak\\*.mp3")
// L"仅本地服生效；关闭后恢复启用前的值。"
SKEETO_UI(Text_f58e87a6, L"Local servers only; disabling restores prior values.")
SKEETO_UI(ViewRollSetupTitle, L"View tilt settings  v%s")
SKEETO_UI(ViewRollSetupToggle, L"[1] View tilt %s")
SKEETO_UI(ViewRollSetupAngle, L"[2] %sTilt angle [%d degrees]")
SKEETO_UI(ViewRollAngleHint, L"    Maximum view tilt while strafing. Default 4 degrees (0-12). Press - / + after selecting.")
SKEETO_UI(ViewRollSetupSpeed, L"[3] %sStrafe threshold [%d]")
SKEETO_UI(ViewRollSpeedHint, L"    Engine uses sideways move speed for full tilt: lower values tilt more easily, higher values stay steadier. Not animation speed. Default 200. Press - / + after selecting.")
SKEETO_UI(ViewRollSetupFocusHint, L"    Press 2 / 3 to select a value, then - / +.")
// L"[8] 中心点 【%s%d】"
SKEETO_UI(Text_f653934a, L"[8] Center dot [%s%d]")
// L"终局阶段计时与已发生的坦克事件，平时不显示；不预测下一波"
SKEETO_UI(Text_f66a5bc2, L"Finale phase timer and Tank events so far; hidden otherwise, does not predict waves")
// L"额外的主武器子弹数HUD  v%s"
SKEETO_UI(Text_f6a1e46d, L"Extra primary ammo HUD  v%s")
// L"板球拍"
SKEETO_UI(Text_f6a21fa3, L"Cricket Bat")
// L"球杆"
SKEETO_UI(Text_f6bd8e60, L"Golf Club")
// L"不在局内"
SKEETO_UI(Text_f6c72650, L"Not In Game")
// L"球棒"
SKEETO_UI(Text_f6e691af, L"Bat")
// L"%s / 坦克事件 %d"
SKEETO_UI(Text_f725d4ab, L"%s / Tank Events %d")
// L"吉他"
SKEETO_UI(Text_f7996760, L"Guitar")
// L"简洁"
SKEETO_UI(Text_f7b079c5, L"Compact")
// L"[3] 复位位置"
SKEETO_UI(Text_f7cd8255, L"[3] Reset position")
// L"[4] 大小 【全屏时无效】"
SKEETO_UI(Text_f80f9008, L"[4] Size [Unavailable when filling screen]")
// L"[1] 燃烧瓶地面倒计时 %s"
SKEETO_UI(Text_f84ea730, L"[1] Molotov ground countdown %s")
// L"效果生效时才显示。安瓿液面跟剩余时间走"
SKEETO_UI(Text_f8a671d1, L"Visible while active; ampoule level follows remaining time")
// L"    本地水平速度，可调位置"
SKEETO_UI(Text_f9087b76, L"    Local horizontal speed; movable")
// L"    右上角击杀列表：只显示玩家、特感和女巫，不含普通感染者。开启后会关掉原版红色击杀记录。默认关。"
SKEETO_UI(Text_f929c287, L"    Top-right kill feed for players, SI and Witches, excluding common infected. Hides stock red kill feed; off by default.")
// L"换人物失败：找不到服务器实体"
SKEETO_UI(Text_f934ffe3, L"Switch failed: server entity not found.")
// L"[1] 显示%s %s"
SKEETO_UI(Text_f9625b9e, L"[1] Show %s %s")
// L"    复用特殊击杀 Level 判定，在死亡位置显示约 2 秒橙色刀箭"
SKEETO_UI(Text_fa517d34, L"    Uses Level special-kill detection; orange blade-arrow at the death position for about 2 seconds")
// L"选择参数后用 - / + 实时调节；所有数值均有安全上下限。"
SKEETO_UI(Text_fa7a650b, L"Select a parameter, then use - / + to adjust live; all values have safe limits.")
// L"局内计时器  v%s"
SKEETO_UI(Text_fab7d471, L"Round timer  v%s")
// L"十字+点"
SKEETO_UI(Text_fabef1d2, L"Cross + Dot")
// L"[3] Spitter 酸液倒计时 %s"
SKEETO_UI(Text_fada26cd, L"[3] Spitter acid countdown %s")
// L"    进入子菜单，可选择黄色竖条或倒三角沙漏倒计时"
SKEETO_UI(Text_fae72e36, L"    Open settings to choose a yellow bar or triangular hourglass countdown")
// L"武士刀"
SKEETO_UI(Text_fafa9a64, L"Katana")
// L"Rochelle 罗谢尔"
SKEETO_UI(Text_fb8ee8a6, L"Rochelle")
// L"沼泽激战"
SKEETO_UI(Text_fbc9b38d, L"Swamp Fever")
// L"    当前 dsp_volume=%.3f%s"
SKEETO_UI(Text_fbf07a08, L"    Current dsp_volume=%.3f%s")
// L"阶段名与原版导演 HUD 相同（BUILD_UP / SUSTAIN_PEAK / PEAK_FADE / RELAX），读不到则不显示。"
SKEETO_UI(Text_fc21c446, L"Stock phases (BUILD_UP / SUSTAIN_PEAK / PEAK_FADE / RELAX); hidden if unavailable.")
// L"木狙"
SKEETO_UI(Text_fc54ed83, L"Hunting Rifle")
// L"    全屏模式可能不会生效，建议采用无边框窗口游玩"
SKEETO_UI(Text_fcaa5e5d, L"    May not work in fullscreen; borderless windowed mode is recommended.")
// L"小刀"
SKEETO_UI(Text_fd7c1e8d, L"Knife")
// L"特感预警 HUD"
SKEETO_UI(Text_fd83caec, L"Special infected alert HUD")
// L"正在救援"
SKEETO_UI(Text_fda6dd7a, L"Reviving")
// L"    自己的血条位置固定，不随本局角色变化"
SKEETO_UI(Text_fe21a36d, L"    Your health bar stays in place regardless of your character")
// L"下中"
SKEETO_UI(Text_fe25e98e, L"Bottom Center")
// L"浅青"
SKEETO_UI(Text_fe2623f2, L"Light Cyan")
// L"[2] Rochelle 黑妹"
SKEETO_UI(Text_fe94564c, L"[2] Rochelle")
// L"被电击器救活"
SKEETO_UI(Text_fedfea10, L"Revived by defibrillator")
// L"    独立进度条，显示谁在救、救到百分之几"
SKEETO_UI(Text_fee7e889, L"    Separate progress bar showing the rescuer and completion percentage")
// L"横线"
SKEETO_UI(Text_ff08f7db, L"Horizontal Line")
// L"[1] 贴图准星 %s"
SKEETO_UI(Text_ff458f9b, L"[1] Texture crosshair %s")
// L"聊天框：!csm 打开本页；!n !r !c !e !b !z !f !l 直接换对应人物。"
SKEETO_UI(Text_ff493cdf, L"Chat: !csm opens this page; !n !r !c !e !b !z !f !l switches directly to the corresponding character.")
// L"受到自己伤害后显示，死亡立即消失"
SKEETO_UI(Text_ff5c57cc, L"Appears after you deal damage; disappears on death")
// L"    开火音量【%d%%】 按 - / + 调节"
// L"[3] 样式 【%s】"
SKEETO_UI(SiWarningStyleLabel, L"[3] Style [%s]")
// L"文字"
SKEETO_UI(SiWarningStyleText, L"Text")
// L"透明图标"
SKEETO_UI(SiWarningStyleIcon, L"Transparent Icons")
SKEETO_UI(TextHoldSemiFire, L"[7] Hold-to-fire for semi-auto weapons, i.e. Automatic Guns %s")
SKEETO_UI(TextHoldSemiSetupPage, L"Hold-to-fire settings  v%s")
SKEETO_UI(TextHoldSemiSetupToggle, L"[1] Hold-to-fire for semi-auto weapons %s")
SKEETO_UI(TextHoldSemiSetupHint, L"    Only guns that fire once per press; hold actions such as using a medkit or fuelling a generator, and full-auto weapons, are completely unaffected.")
SKEETO_UI(TextHoldSemiRateSub, L"[2] Edge cadence step [%d-%d ms]")
SKEETO_UI(TextHoldSemiRateHint, L"    Press 2 to cycle steps.")
SKEETO_UI(TextShotgunReload, L"[8] Shotgun Auto Reload %s")
SKEETO_UI(TextShotgunReloadPage, L"Shotgun Auto Reload  v%s")
SKEETO_UI(TextShotgunReloadHint, L"    With a shotgun in hand, synthesizes a reload key edge every 200-300 ms at random intervals; the key is ignored while the magazine is full")
SKEETO_UI(TextShotgunReloadPump, L"[1] Auto reload for pump shotguns %s")
SKEETO_UI(TextShotgunReloadPumpHint, L"    pumpshotgun / shotgun_chrome")
SKEETO_UI(TextShotgunReloadAuto, L"[2] Auto reload for auto shotguns %s")
SKEETO_UI(TextShotgunReloadAutoHint, L"    autoshotgun / shotgun_spas")
SKEETO_UI(TextXhairTpHint, L"    Press 5 for its sub-page: switch, style, colour and size; independent of the dynamic crosshair above")
SKEETO_UI(TextXhairTpPageHint, L"    Third person uses a lightweight ray-traced aim marker; first person keeps the spread ring; independent of the dynamic crosshair")
SKEETO_UI(TextXhairTpPage, L"Third-person crosshair  v%s")
SKEETO_UI(TextXhairTpToggle, L"[1] Third-person crosshair %s")
SKEETO_UI(TextXhairTpStyleSub, L"[2] Style [%s]")
SKEETO_UI(TextXhairTpColorSub, L"[3] Colour [%s]")
SKEETO_UI(TextXhairTpSizeSub, L"[4] Size [%d%%]")
SKEETO_UI(TextXhairTpStyleHint, L"    Styles and proportions follow the static crosshair: cross / cross+dot / dot / four corners")
SKEETO_UI(TextHoldSemiWarn1, L"    Note: some servers run a modified LAC plugin or stricter detection of their own (for example xlk, 58).")
SKEETO_UI(TextHoldSemiWarn2, L"    Using this feature is not recommended on those servers; if you still want it, pick a safer cadence step (later steps are more conservative).")
SKEETO_UI(TextHoldSemiAacToggle, L"[3] Anti-Anti-Cheat lightweight countermeasures %s")
SKEETO_UI(TextHoldSemiAacHint, L"    AAC defaults to OFF. When enabled, presses last at least 150ms and releases at least 48ms, limiting fast steps. The range above shows the effective press duration; a full click also includes the release time.")
SKEETO_UI(TextHoldSemiWarn3, L"    Every step retains random jitter. AAC only reduces synthetic click frequency and short pulses; it cannot guarantee no detection or bans. Follow server rules even with AAC enabled.")
SKEETO_UI(HoldSemiCommandHint, L"    Console: skeeto_hold_semiauto 0 to disable / 1 to enable / status to report; no argument reports status.")
SKEETO_UI(TextHoldSemiStatus, L"    Status: %s")
SKEETO_UI(TextInteractHud, L"[1] Interact progress HUD %s")
SKEETO_UI(TextInteractHudHint, L"    Shows a progress bar and the seconds left while you revive, heal or refuel; nothing is drawn otherwise")
SKEETO_UI(TextInteractHudLabel, L"Interact progress  %.1fs")
SKEETO_UI(TextInteractHudName, L"Interact progress HUD")
SKEETO_UI(TextInteractHudDesc, L"    Progress and seconds left while you revive, heal or refuel; position, size and layout are adjustable. Off by default.")
SKEETO_UI(TextInteractHudPage, L"Interact progress  v%s")
SKEETO_UI(TextInteractHudScale, L"[2] Overall size [%d%%]  `-`/`=`")
SKEETO_UI(SoundVolumeTitle, L"Skeeto Audio Mixer (%d/%d)  v%s")
SKEETO_UI(SoundVolumeMaster, L"[1] Category volumes (master) %s")
SKEETO_UI(SoundVolumeMasterHint, L"    Controls the category sliders. Off leaves those sounds unchanged. Does not affect hearing-DSP, hit feedback, background or music volume.")
SKEETO_UI(SoundVolumeStep, L"[2] Volume step [%d%%]")
SKEETO_UI(SoundVolumeStepHint, L"    Press the number to cycle 1% / 5% / 10% / 20%. Default 1%. Shared by category sliders, Main UI volumes, and hit-feedback volume in this module.")
SKEETO_UI(SoundVolumeDspEntry, L"[1] Clearer hearing [settings] %s")
SKEETO_UI(SoundVolumeDspEntryHint, L"    Turns off spatial reverb and echo so footsteps and specials are easier to place.")
SKEETO_UI(SoundVolumeHitSfx, L"[2] Hit-feedback volume [%d%%]")
SKEETO_UI(SoundVolumeHitSfxHint, L"    Same control as the hit-feedback page. Use - / +. Only hit cues; ignores the category master.")
SKEETO_UI(SoundVolumeBackground, L"[3] MP4 background volume [%d%%]")
SKEETO_UI(SoundVolumeBackgroundHint, L"    Same control and ConVar as Background Engine. Use - / +; no decoder or background resources are started here.")
SKEETO_UI(SoundVolumeHubHint, L"Right arrow continues the remaining volume rows, including hearing, hit feedback, background and music.")
SKEETO_UI(SoundVolumeEngineCapNote, L"Engine limit: cannot go above 100%. Raise overall game volume and lower other categories to make one category stand out.")
SKEETO_UI(SoundVolumeGunfire, L"[3] Gunfire [%d%%]")
SKEETO_UI(SoundVolumeGunfireHint, L"    Firearm shots, grenade-launcher fire and mounted guns. Not explosions.")
SKEETO_UI(SoundVolumeHandling, L"[4] Weapon handling [%d%%]")
SKEETO_UI(SoundVolumeHandlingHint, L"    Reloads, bolts, dry fires and weapon switches. Not shots.")
SKEETO_UI(SoundVolumeMelee, L"[5] Melee [%d%%]")
SKEETO_UI(SoundVolumeMeleeHint, L"    Swings and hits from blades, axes and bats, plus shove hits. Not the chainsaw.")
SKEETO_UI(SoundVolumeChainsaw, L"[6] Chainsaw [%d%%]")
SKEETO_UI(SoundVolumeChainsawHint, L"    Chainsaw start, idle and hits.")
SKEETO_UI(SoundVolumeFootstep, L"[7] Footsteps [%d%%]")
SKEETO_UI(SoundVolumeFootstepHint, L"    Player footsteps. Not voices or infected.")
SKEETO_UI(SoundVolumeSurvivor, L"[8] Survivor voices [%d%%]")
SKEETO_UI(SoundVolumeSurvivorHint, L"    Survivor speech. Not swings or footsteps.")
SKEETO_UI(SoundVolumeSpecial, L"[9] Special infected [%d%%]")
SKEETO_UI(SoundVolumeSpecialHint, L"    Hunter, Smoker, Boomer, Charger, Jockey and Spitter. Not Tank or Witch.")
SKEETO_UI(SoundVolumeCommon, L"[1] Common infected [%d%%]")
SKEETO_UI(SoundVolumeCommonHint, L"    Common infected calls and actions. Not specials, Tank or Witch.")
SKEETO_UI(SoundVolumeTank, L"[2] Tank [%d%%]")
SKEETO_UI(SoundVolumeTankHint, L"    Tank roars and attacks. Tank footsteps use the footstep-volume slider.")
SKEETO_UI(SoundVolumeWitch, L"[3] Witch [%d%%]")
SKEETO_UI(SoundVolumeWitchHint, L"    Witch cries, startles and attacks.")
SKEETO_UI(SoundVolumeExplosion, L"[4] Explosions [%d%%]")
SKEETO_UI(SoundVolumeExplosionHint, L"    Grenade and pipe blasts, propane and similar. Not gunfire.")
SKEETO_UI(SoundVolumeThrowable, L"[5] Throwables [%d%%]")
SKEETO_UI(SoundVolumeThrowableHint, L"    Molotov, pipe bomb and bile throw, fuse and burn loops.")
SKEETO_UI(SoundVolumeAmbient, L"[6] Ambient / world [%d%%]")
SKEETO_UI(SoundVolumeAmbientHint, L"    Ambient, doors and physics. Not music.")
SKEETO_UI(SoundVolumeUi, L"[7] UI / menus [%d%%]")
SKEETO_UI(SoundVolumeUiHint, L"    Menu and UI clicks. Also affects Skeeto's own cue sounds.")
SKEETO_UI(SoundVolumeVehicle, L"[8] Vehicles / rescue [%d%%]")
SKEETO_UI(SoundVolumeVehicleHint, L"    Helicopters, propellers, boats and other rescue vehicles. Not music tracks.")
SKEETO_UI(SoundVolumeItem, L"[9] Items [%d%%]")
SKEETO_UI(SoundVolumeItemHint, L"    Pills, shots, ammo boxes and similar pickups or uses. Flashlight clicks can still be muted by idle flashlight.")
SKEETO_UI(SoundVolumeScriptCue, L"[1] Ammo cues [%d%%]")
SKEETO_UI(SoundVolumeScriptCueHint, L"    Shared by dry fire, low ammo and magazine drops. Not shots, shell impacts or hit feedback.")
SKEETO_UI(SoundVolumeHint, L"Select a number, then - / + from 0% to 100%. Press that number again to return to 100%. Turn on the category master first.")
SKEETO_UI(SoundVolumeModHint, L"Workshop sounds that replace the same path change with it. A new directory does not.")
SKEETO_UI(ElimTankModeRow, L"[9] Tank count mode [%s]")
SKEETO_UI(ElimTankModeOwn, L"Own kills only")
SKEETO_UI(ElimTankModeAll, L"Any Tank death")
SKEETO_UI(DissolveEntry, L"[1] Infected corpse particle dissolve, Dissolve Infected 【Open settings】 %s")
SKEETO_UI(DissolveHintClient, L"    Client-only. Works on remote servers. Toggle SI/Tank and Common/Witch separately.")
SKEETO_UI(DissolveHintMats, L"    Purple/black checkers mean dissolve materials are missing — update the Skeeto preset materials pack.")
SKEETO_UI(DissolveHintMutex, L"    Corpse dissolve conflicts with removing SI/common ragdolls under Combat and audiovisual enhancements.")
SKEETO_UI(DissolveHintServerClear, L"    To use this, leave the server's common \"remove corpses on death\" option off. The two conflict.")
SKEETO_UI(DissolveHintBackend, L"    This game build could not open the client dissolve interface. The switch stays, but nothing is spawned.")
SKEETO_UI(DissolvePageNext, L"    Next page: infected corpse particle dissolve.")
SKEETO_UI(DissolveSetupTitle, L"Corpse particle dissolve  v%s")
SKEETO_UI(DissolveSetupToggleSi, L"[1] Special / Tank particle dissolve %s")
SKEETO_UI(DissolveSetupToggleCi, L"[2] Common / Witch particle dissolve %s")
SKEETO_UI(DissolveSetupType, L"[3] Dissolve type 【%s】")
SKEETO_UI(DissolveSetupMs, L"[4] Duration 【%d ms】")
SKEETO_UI(DissolveSetupRagClean, L"[5] Dissolve-compatible ragdoll clean %s")
SKEETO_UI(DissolveTypeNormal, L"Normal")
SKEETO_UI(DissolveTypeElectrical, L"Electrical")
SKEETO_UI(DissolveTypeElectricalLight, L"Electrical Light")
SKEETO_UI(DissolveTypeCore, L"Core")
SKEETO_UI(DissolveSetupHintType, L"    Engine built-in types. Electrical / Electrical Light spawn spark particles; Normal is mostly fade; Core is rarer. Press 3 to cycle.")
SKEETO_UI(DissolveSetupHintTracer, L"    Dissolve sparks share effects/spark with tracers: change your tracer mod to change dissolve particle style.")
SKEETO_UI(DissolveSetupHintMs, L"    Scales fade and spark lifetime together. Default 550 ms. Press - / + to adjust.")
SKEETO_UI(DissolveSetupHintCi, L"    Commons match via hit Trace / death origin (client HP/lifeState unreliable). Witch shares the Common switch.")
SKEETO_UI(DissolveSetupHintRagClean, L"    Periodic cl_destroy_ragdolls to skip vanilla ragdoll fade. Works alone — dissolve not required; also fine alongside dissolve.")
SKEETO_UI(AcidOutlineEntry, L"[2] Spitter acid range estimate outline, Spitter Acid Range Outline 【Open settings】 %s")
SKEETO_UI(AcidOutlineHint, L"    Client-only: estimate ground range from insect_swarm fire nodes (does not change damage). Separate from acid residue countdown.")
SKEETO_UI(AcidOutlineSetupTitle, L"Spitter acid range estimate settings  v%s")
SKEETO_UI(AcidOutlineSetupToggle, L"[1] Spitter acid range estimate outline %s")
SKEETO_UI(AcidOutlineSetupRadius, L"[2] Outline radius [%d]")
SKEETO_UI(AcidOutlineSetupWidth, L"[3] Outline width [%d px]")
SKEETO_UI(AcidOutlineRadiusHint, L"    Press 2 or -/+ (60/75/90/105, default 75; unverified).")
SKEETO_UI(AcidOutlineWidthHint, L"    Press 3 to cycle (1/2/3/4, default 3).")
SKEETO_UI(AcidOutlineColorHint, L"    Press 4 to cycle colors.")
SKEETO_UI(AcidOutlinePluginWarning, L"    Warning: On servers with plugins that fix spit range/damage, Skeeto may overstate the outline. These plugins often narrow server-side damage checks without changing the replicated acid nodes, so the client cannot read the adjusted radius. We recommend leaving this off on those servers.")
SKEETO_UI(AcidOutlineBindHint, L"    Bind to toggle: bind <key> skeeto_acid_outline_toggle. Add 0 or 1 to set explicitly; no argument toggles.")
SKEETO_UI(AcidOutlineColorOption, L"[4] Outline color [%s]")
SKEETO_UI(AcidOutlineColorGreen, L"Green")
SKEETO_UI(AcidOutlineColorCyan, L"Cyan")
SKEETO_UI(AcidOutlineColorBlue, L"Blue")
SKEETO_UI(AcidOutlineColorYellow, L"Yellow")
SKEETO_UI(AcidOutlineColorOrange, L"Orange")
SKEETO_UI(AcidOutlineColorRed, L"Red")
SKEETO_UI(AcidOutlineColorMagenta, L"Magenta")
SKEETO_UI(AcidOutlineColorWhite, L"White")
SKEETO_UI(DmgCritThreshDesc, L"    Use crit style when the shown value reaches the threshold; not a headshot check")
SKEETO_UI(DmgCritThreshOff, L"Off")
SKEETO_UI(DmgCritThreshGe, L"≥%d")
SKEETO_UI(AmmoGfxHudName, L"Graphic ammo HUD")
SKEETO_UI(AmmoGfxHudHint, L"Segments / straight bar; independent of the extra ammo number HUD")
SKEETO_UI(AmmoGfxModeOff, L"Off")
SKEETO_UI(AmmoGfxModeSeg, L"Segments")
SKEETO_UI(AmmoGfxModeBar, L"Straight bar")
SKEETO_UI(AmmoGfxModeRow, L"[1] Style [%s]")
SKEETO_UI(AmmoGfxPosUp, L"Up")
SKEETO_UI(AmmoGfxPosDown, L"Down")
SKEETO_UI(AmmoGfxPosLeft, L"Left")
SKEETO_UI(AmmoGfxPosRight, L"Right")
SKEETO_UI(AmmoGfxPosRow, L"[3] Side [%s]")
SKEETO_UI(AmmoGfxScaleRow, L"[2] Size [%d%%]  `-`/`=`")
SKEETO_UI(AmmoGfxModeHint, L"    Straight bar: solid track + fill + cursor; segments: stepped white/yellow/red")
SKEETO_UI(AmmoGfxPosHint, L"    Bar default right of crosshair; right side depletes top to bottom")
SKEETO_UI(AmmoGfxNeedStyle, L"    Pick segments or straight bar first; no drawing while Off")
SKEETO_UI(AmmoGfxSegMoveHint, L"    Arrow keys move freely (hold Shift for large steps); close this page to hide the preview frame")
SKEETO_UI(AmmoGfxSegResetPos, L"[3] Reset position")
SKEETO_UI(AmmoGfxEntryName, L"Graphic ammo HUD")
SKEETO_UI(AmmoGfxEntryDesc, L"    Segments / straight bar beside the crosshair; independent of the extra ammo number HUD")
SKEETO_UI(SktUiReloadName, L"Reload all SKT UI")
SKEETO_UI(SktUiReloadDesc, L"    Rebuilds Skeeto font and UI texture caches. Use if fonts disappear after changing display mode in game. Console: skeeto_ui_reload")
SKEETO_UI(SktUiReloadDone, L"SKT UI reloaded")
SKEETO_UI(SktUiReloadRow, L"[%d] %s  [reloaded]")
SKEETO_UI(SktUiVideomodeReloadName, L"Reset display mode and restore SKT UI")
SKEETO_UI(SktUiVideomodeReloadDesc, L"    The game freezes for a while and then recovers; this can trigger a resource reload. Console: skeeto_ui_videomode_reload")
SKEETO_UI(DisclaimerEntry, L"[2] Disclaimer [Info]")
SKEETO_UI(DisclaimerTitle, L"Disclaimer  v%s")
SKEETO_UI(DisclaimerBody1, L"    Skeeto reads and modifies game process memory. Some features use memory patches and hooks, techniques similar to those used by injected cheats. I cannot guarantee how a server or anti-cheat will classify it.")
SKEETO_UI(DisclaimerBody2, L"    My rule is to use Skeeto only on servers that allow mods and to develop only features I consider reasonable. That judgment is subjective; yours may differ.")
SKEETO_UI(DisclaimerBody3, L"    Follow the rules of the server you join. Allowing mods does not mean every feature is welcome. If unsure, turn the relevant feature off. The default-on key list only displays bindable commands; it does not enable gameplay features.")
SKEETO_UI(DisclaimerBody4, L"    Skeeto features are off by default. No one is forcing you to use any of them. The menu marks more sensitive or risky features with conspicuous red warnings.")
SKEETO_UI(DisclaimerBody5, L"    Use these features with care. You are responsible for consequences from breaking server rules, anti-cheat action, or problems after game updates.")
SKEETO_UI(BwSelfHudEntryName, L"Own black-and-white HUD")
SKEETO_UI(BwSelfHudEntryDesc, L"    While you are black-and-white (revived after two incaps), show the cracked-heart ECG on screen until recovered. Off by default.")
SKEETO_UI(BwSelfHudPage, L"Own black-and-white  v%s")
SKEETO_UI(BwSelfHudHint, L"    Same cracked-heart ECG as the world HUD teammate badge, drawn as a screen HUD while you are on third strike.")
SKEETO_UI(BwSelfHudToggle, L"[1] Show own black-and-white HUD %s")
SKEETO_UI(BwSelfHudScale, L"[2] Overall size [%d%%]  `-`/`=`")
SKEETO_UI(BwSelfHudResetPos, L"[3] Reset position (below crosshair)")
SKEETO_UI(ClientDmgNumPageTitle, L"Damage numbers")
SKEETO_UI(ClientDmgNumPageHint, L"Client-side floating damage on SI / Tank models")
SKEETO_UI(ClientDmgNumToggle, L"[1] Damage numbers %s")
SKEETO_UI(ClientDmgNumToggleHint, L"    Master switch. Crit style below can be off without disabling numbers")
SKEETO_UI(ClientDmgNumEnter, L"    Open settings for on/off and crit threshold")
SKEETO_UI(DmgCritThreshPageRow, L"[2] Crit threshold 1 %s")
SKEETO_UI(DmgCritThresh2PageRow, L"[3] Crit threshold 2 >%d")
SKEETO_UI(DmgCritThresh2Desc, L"    Above threshold 2: 1.4x size, red, with !!! after the number")
SKEETO_UI(DmgCritThreshOrderHint, L"    Tier 2 cannot be lower than tier 1; turning tier 1 off disables both")
SKEETO_UI(KillFeedStyle, L"[6] Appearance [%s]")
SKEETO_UI(KillFeedStyleClassic, L"Classic")
SKEETO_UI(KillFeedStyleCs2, L"CS2")
SKEETO_UI(KillFeedStyleHint, L"    Classic keeps the glass rows. CS2 uses comic cut panels, atlas weapons, streaks and optional gold special kills.")
SKEETO_UI(KillFeedStyleAssetHint, L"    CS2-style kill feed needs an updated preset asset pack.")
SKEETO_UI(KillFeedIconStyle, L"[7] Weapon icon style [%s]")
SKEETO_UI(KillFeedIconStyleNew, L"Clear silhouette")
SKEETO_UI(KillFeedIconStyleGame, L"Game / mod")
SKEETO_UI(KillFeedIconStyleGameMirror, L"Game / mod (mirrored)")
SKEETO_UI(KillFeedIconStyleHint, L"    Classic only. Cycles: Clear silhouette -> Game / mod -> Game / mod (mirrored). Mirrored flips the muzzle to the right. Game/mod uses the same vgui/hud mats as teammate primary (workshop icon_* overrides). CS2 style uses the reference atlas.")
SKEETO_UI(KillFeedIconStyleCs2Hint, L"    CS2 appearance uses the reference atlas; classic icon styles are inactive.")
SKEETO_UI(XhairHideOnReach, L"[6] Hide Skeeto crosshair during reach %s")
// L"  - / + 调节"
SKEETO_UI(TextXhairAdjustMark, L"  - / + adjust")
SKEETO_UI(XhairHideOnReachHint, L"    Auto engine reach only (pickups, helping downed teammates). Necola/L4N/mod active inspect not supported. Restores ~100ms after. Default off.")
SKEETO_UI(XhairStyleX, L"X")
SKEETO_UI(XhairStyleXDot, L"X + Dot")
SKEETO_UI(XhairStyleXRing, L"X + Ring")
SKEETO_UI(XhairStyleXRingDot, L"X + Ring + Dot")
SKEETO_UI(TongueCutEntry, L"[3] Tongue Cut Trainer HUD, Tongue Cut Trainer 【Open settings】 %s")
SKEETO_UI(TongueCutHint, L"    Cues the timing when a Smoker locks onto you: CUT! with a knife / machete / fireaxe / katana / crowbar, DODGE! with anything else.")
SKEETO_UI(TongueCutSetupTitle, L"Tongue Cut Trainer HUD settings  v%s")
SKEETO_UI(TongueCutSetupHint, L"A looping preview (ready > CUT!/DODGE! > result) shows while this page is open. Text follows the weapon in hand.")
SKEETO_UI(TongueCutToggle, L"[1] Tongue Cut Trainer HUD %s")
SKEETO_UI(TongueCutToggleHint, L"    Works with any weapon once a tongue locks onto you: knife / machete / fireaxe / katana / crowbar show CUT!, other weapons show Tongue locked / DODGE!.")
SKEETO_UI(TongueCutReadyRow, L"[2] Ready cue + countdown bar %s")
SKEETO_UI(TongueCutReadyHint, L"    Amber badge and countdown bar while you are locked; swing / dodge when the bar empties. Off = only the flash text.")
SKEETO_UI(TongueCutResultRow, L"[3] Result feedback (Cut / Pulled / Dodged) %s")
SKEETO_UI(TongueCutNetCompRow, L"[4] Auto latency compensation %s")
SKEETO_UI(TongueCutNetCompHint, L"    Fires the cue earlier by lerp + ping, sampled once per attempt. Ping is near 0 on your own server; the formula still needs in-game calibration.")
SKEETO_UI(TongueCutOffsetRow, L"[5] %sManual offset [%+d ms]")
SKEETO_UI(TongueCutOffsetHint, L"    Positive = earlier, negative = later. Press 5 / 12 / 8 to select, then - / + to adjust.")
SKEETO_UI(TongueCutFlashRow, L"[6] %sCUT! / DODGE! flash duration [%d ms]")
SKEETO_UI(TongueCutStyleRow, L"[7] Style [%s]")
SKEETO_UI(TongueCutScaleRow, L"[8] %sScale [%d%%]")
SKEETO_UI(TongueCutResetRow, L"[9] Reset position (top center)")
SKEETO_UI(TongueCutCue, L"CUT!")
SKEETO_UI(TongueCutDodgeCue, L"DODGE!")
SKEETO_UI(TongueCutLockLabel, L"Tongue locked")
SKEETO_UI(TongueCutResultCut, L"Cut")
SKEETO_UI(TongueCutResultPulled, L"Pulled")
SKEETO_UI(TongueCutResultDodged, L"Dodged")
// ---- Skeeto TAB board ----
SKEETO_UI(TabBoardEntryName, L"Skeeto TAB Board HUD")
SKEETO_UI(TabBoardEntryDesc, L"    Hold a bound key for a scoreboard (up to 32 players, adjustable): health / temp health, down / ledge / black-and-white / pinned, weapons and items, kills, ping and avatars. Versus shows Survivors and Infected as two groups; campaign can list special infected. Choose either Skeeto-first or vanilla-first single/double tap behavior in Edit Keys. Off by default.")
SKEETO_UI(TabBoardEntryFmt, L"[%d] %s 【Open settings】 %s")
SKEETO_UI(TabBoardSetupTitle, L"Skeeto TAB Board HUD settings  v%s")
SKEETO_UI(TabBoardSetupHint, L"While in game, this page previews the board in the screen center with live match data.")
SKEETO_UI(TabBoardToggle, L"[1] Skeeto TAB Board HUD %s")
SKEETO_UI(TabBoardBindHint, L"    In Edit Keys, bind either Skeeto on a single tap / vanilla on a double tap, or vanilla on a single tap / Skeeto on a double tap. The options below can also bind TAB directly. If this board is off or all HUDs are hidden, the key falls back to the vanilla scoreboard. Restore TAB with [7] before uninstalling Skeeto.")
SKEETO_UI(TabBoardScaleRow, L"[2] Overall size [%d%%]  `-`/`=`")
SKEETO_UI(TabBoardSiStatusRow, L"[4] Show special infected status %s")
SKEETO_UI(TabBoardSiStatusHint, L"    On: campaign also lists living AI special infected; versus shows infected class, health and state. Off: versus lists only human infected with name, ping and kills; campaign lists no infected.")
SKEETO_UI(TabBoardMaxRowsRow, L"[5] Max players shown [%d]")
SKEETO_UI(TabBoardResScaleRow, L"[6] Resolution scaling %s")
SKEETO_UI(TabBoardResScaleHint, L"    Scales this panel by screen height on its own (on by default). Does not stack with the custom-HUD master switch: when the master is on both use the same factor; when the master is off this option can still enlarge the panel alone.")
SKEETO_UI(TabBoardRestoreTab, L"[7] Restore TAB to the vanilla scoreboard")
SKEETO_UI(TabBoardBindTab, L"[8] TAB single: Skeeto, double: vanilla")
SKEETO_UI(TabBoardBindVanillaTab, L"[9] TAB single: vanilla, double: Skeeto")
SKEETO_UI(TabBoardCmdDone, L" [Done]")
SKEETO_UI(TabBoardDataHint, L"    Kills come from events the client receives, reset every round; kills before you joined are not counted. Ping comes from PlayerResource, bots show BOT. Avatars are requested from Steam and cached, and may appear a moment later the first time.")
SKEETO_UI(KbListSectionToggle, L"[3] Skeeto section in the key bindings list %s")
SKEETO_UI(KbListSectionHint, L"    Adds common Skeeto commands (navigation, TAB board, spotlight night vision, hide HUD, ...) to Options > Keyboard/Mouse > Edit Keys, after the L4N section and before the stock entries. Does not modify game files. Takes effect after reopening the options window; turning it off removes it.")
SKEETO_UI(TabBoardZcSmoker, L"Smoker")
SKEETO_UI(TabBoardZcBoomer, L"Boomer")
SKEETO_UI(TabBoardZcHunter, L"Hunter")
SKEETO_UI(TabBoardZcSpitter, L"Spitter")
SKEETO_UI(TabBoardZcJockey, L"Jockey")
SKEETO_UI(TabBoardZcCharger, L"Charger")
SKEETO_UI(TabBoardCapSmoker, L"Smoked")
SKEETO_UI(TabBoardCapHunter, L"Pounced")
SKEETO_UI(TabBoardCapJockey, L"Jockeyed")
SKEETO_UI(TabBoardCapCharger, L"Carried")
SKEETO_UI(TabBoardCapPummel, L"Pummeled")
SKEETO_UI(TabBoardModeCoop, L"Campaign")
SKEETO_UI(TabBoardModeRealism, L"Realism")
SKEETO_UI(TabBoardModeVersus, L"Versus")
SKEETO_UI(TabBoardModeSurvival, L"Survival")
SKEETO_UI(TabBoardModeScavenge, L"Scavenge")
SKEETO_UI(TabBoardModeMutation, L"Mutation")
SKEETO_UI(TabBoardChapterOf, L"Chapter %d/%d")
SKEETO_UI(TabBoardChapter, L"Chapter %d")
SKEETO_UI(TabBoardHumansBots, L"Human %d / BOT %d")
SKEETO_UI(TabBoardChipSpec, L"Spectators %d")
SKEETO_UI(TabBoardChipInf, L"Infected %d")
SKEETO_UI(TabBoardChipSurv, L"Survivors %d")
SKEETO_UI(TabBoardGroupInf, L"INFECTED")
SKEETO_UI(TabBoardGroupSurv, L"SURVIVORS")
SKEETO_UI(TabBoardColName, L"NAME")
SKEETO_UI(TabBoardColHp, L"HEALTH")
SKEETO_UI(TabBoardColStatus, L"STATUS")
SKEETO_UI(TabBoardColPri, L"PRIMARY")
SKEETO_UI(TabBoardColSec, L"SECONDARY")
SKEETO_UI(TabBoardColItems, L"ITEMS")
SKEETO_UI(TabBoardColCi, L"CI")
SKEETO_UI(TabBoardColSi, L"SI")
SKEETO_UI(TabBoardColPing, L"PING")
SKEETO_UI(TabBoardColSk, L"KILLS")
SKEETO_UI(TabBoardMore, L"%d more")
SKEETO_UI(TabBoardDiffEasy, L"Easy")
SKEETO_UI(TabBoardDiffNormal, L"Normal")
SKEETO_UI(TabBoardDiffHard, L"Advanced")
SKEETO_UI(TabBoardDiffExpert, L"Expert")
SKEETO_UI(TabBoardNoData, L"No player data yet")
SKEETO_UI(TabBoardStRespawn, L"Respawn")
SKEETO_UI(TabBoardStGhost, L"Ghost")
SKEETO_UI(TabBoardStAlive, L"Alive")
SKEETO_UI(TabBoardStDead, L"Dead")
SKEETO_UI(TabBoardStHang, L"Ledge")
SKEETO_UI(TabBoardStDown, L"Down")
SKEETO_UI(TabBoardStRevive, L"Reviving")
SKEETO_UI(TabBoardStBw, L"B&W")
SKEETO_UI(TabBoardSpecPrefix, L"Spectating: ")
SKEETO_UI(ChapStatsEntryName, L"Chapter Stats Panel HUD")
SKEETO_UI(ChapStatsEntryDesc, L"    Freezes this chapter's stats when the safe room door closes on a regular chapter and shows them on the transition screen: optional score overview, common / special kills, headshots, team contribution, and an optional special-ops page, rotating by theme with the best player marked. Hold a key to see the running totals at any time. Off by default.")
SKEETO_UI(ChapStatsSetupTitle, L"Chapter Stats Panel HUD settings  v%s")
SKEETO_UI(ChapStatsSetupHint, L"Freezes this chapter's stats when the safe room door closes on a regular chapter, shows them on the transition screen and closes once the next chapter is live; finales / rescue escapes are not settled, nor is an attempt that ends in a wipe. While in game, this page previews the panel in the screen center with live chapter data.")
SKEETO_UI(ChapStatsToggle, L"[1] Chapter Stats Panel HUD %s")
SKEETO_UI(ChapStatsRowsRow, L"[1] Players shown [%d]")
SKEETO_UI(ChapStatsScaleRow, L"[1] Overall size [%d%%]  `-`/`=`")
SKEETO_UI(ChapStatsResScaleRow, L"[2] Resolution scaling %s")
SKEETO_UI(ChapStatsViewRow, L"[2] Transition display [%s]")
SKEETO_UI(ChapStatsViewAll, L"Everyone")
SKEETO_UI(ChapStatsViewSelf, L"Self only")
SKEETO_UI(ChapStatsViewNone, L"Hidden")
SKEETO_UI(ChapStatsPageSecRow, L"[8] Page dwell [%d sec]")
SKEETO_UI(ChapStatsLayoutRow, L"[3] Layout [%s]")
SKEETO_UI(ChapStatsLayoutTable, L"Table")
SKEETO_UI(ChapStatsLayoutCards, L"Survivor cards")
SKEETO_UI(ChapStatsLayoutTactical, L"Tactical briefing")
SKEETO_UI(ChapStatsLayoutGrid, L"Player card grid")
SKEETO_UI(ChapStatsLayoutArcade, L"Arcade leaderboard")
SKEETO_UI(ChapStatsMorePlayers, L"%d more players not shown")
SKEETO_UI(ChapStatsScoreHudEntry, L"[4] Score change HUD [Open settings]")
SKEETO_UI(ChapStatsSizeEntry, L"[6] Overall size and resolution scaling [Open settings]")
SKEETO_UI(ChapStatsDisplayEntry, L"[7] Stats pages, players shown and transition display [Open settings]")
SKEETO_UI(ChapStatsSizeTitle, L"Chapter Stats Panel size settings  v%s")
SKEETO_UI(ChapStatsDisplayTitle, L"Chapter Stats Panel display settings  v%s")
SKEETO_UI(ChapStatsOverviewRow, L"[3] Score overview page %s")
SKEETO_UI(ChapStatsKillsRow, L"[4] Kills & headshots page %s")
SKEETO_UI(ChapStatsDmgRow, L"[5] Tank/Witch damage page %s")
SKEETO_UI(ChapStatsSiRow, L"[6] Special infected page %s")
SKEETO_UI(ChapStatsTeamRow, L"[7] Team contribution page %s")
SKEETO_UI(ChapStatsSpecDisplayRow, L"[8] Special ops page %s")
SKEETO_UI(ChapStatsPagesHint, L"Hiding a stats page also removes its score: Kills covers common/Tank/Witch, Damage scores Tank/Witch damage, Special Infected covers the six SI types (including headshot points); Team and Special score separately. Overview only controls display.")
SKEETO_UI(ChapStatsScoreHudSetupTitle, L"Score change HUD settings  v%s")
SKEETO_UI(ChapStatsScoreHudToggle, L"[1] Score change HUD %s")
SKEETO_UI(ChapStatsScoreHudPrereqHint, L"    Turn on both the Chapter Stats HUD and score system first. Until then, this HUD collects and displays no score changes.")
SKEETO_UI(ChapStatsScoreHudScaleRow, L"[2] Overall size [%d%%]  `-`/`=`")
SKEETO_UI(ChapStatsScoreHudMoveHint, L"    Arrow keys move it; hold to repeat, Shift moves farther. This page previews its position.")
SKEETO_UI(ChapStatsScoreHudReset, L"[3] Reset position (original location)")
SKEETO_UI(ChapStatsScoreHudHint, L"Shows each score change and its cause, including special infected types and kill bonuses. The total follows the chapter score rules.")
SKEETO_UI(ChapStatsScoreTotal, L"Chapter total")
SKEETO_UI(ChapStatsScoreLabelCommon, L"Common infected kill")
SKEETO_UI(ChapStatsScoreLabelCommonHeadshot, L"Common infected headshot kill")
SKEETO_UI(ChapStatsScoreSiKillFmt, L"%s kill")
SKEETO_UI(ChapStatsScoreSiHeadshotFmt, L"%s headshot kill")
SKEETO_UI(ChapStatsScoreBonusMelee, L" (melee)")
SKEETO_UI(ChapStatsScoreBonusSkeet, L" (skeet)")
SKEETO_UI(ChapStatsScoreBonusMeleeSkeet, L" (melee skeet)")
SKEETO_UI(ChapStatsScoreBonusChargerLevel, L" (charger level)")
SKEETO_UI(ChapStatsScoreBonusCrownMelee, L" (melee Witch)")
SKEETO_UI(ChapStatsScoreLabelSpecial, L"Score change")
SKEETO_UI(ChapStatsScoreReasonRevive, L"Revived teammate")
SKEETO_UI(ChapStatsScoreReasonDefib, L"Defibrillated teammate")
SKEETO_UI(ChapStatsScoreReasonHeal, L"Healed teammate")
SKEETO_UI(ChapStatsScoreReasonRescue, L"Freed teammate")
SKEETO_UI(ChapStatsScoreRescueSmoker, L"Freed teammate from Smoker")
SKEETO_UI(ChapStatsScoreRescueHunter, L"Freed teammate from Hunter")
SKEETO_UI(ChapStatsScoreRescueJockey, L"Freed teammate from Jockey")
SKEETO_UI(ChapStatsScoreRescueCharger, L"Freed teammate from Charger")
SKEETO_UI(ChapStatsScoreReasonPinned, L"Pinned by special infected")
SKEETO_UI(ChapStatsScorePinnedSmoker, L"Caught by Smoker")
SKEETO_UI(ChapStatsScorePinnedHunter, L"Pounced by Hunter")
SKEETO_UI(ChapStatsScorePinnedJockey, L"Ridden by Jockey")
SKEETO_UI(ChapStatsScorePinnedChargerPummel, L"Pummeled by Charger")
SKEETO_UI(ChapStatsScoreReasonIncap, L"Incapacitated")
SKEETO_UI(ChapStatsScoreReasonDeath, L"Died")
SKEETO_UI(ChapStatsScoreReasonTongueCut, L"Cut Smoker tongue")
SKEETO_UI(ChapStatsScoreReasonMeleeSi, L"Melee special infected kill")
SKEETO_UI(ChapStatsScoreReasonSkeet, L"Skeet kill")
SKEETO_UI(ChapStatsScoreReasonMeleeSkeet, L"Melee skeet kill")
SKEETO_UI(ChapStatsScoreReasonCrownMelee, L"Melee Witch kill")
SKEETO_UI(ChapStatsScoreReasonChargerLevel, L"Charger level")
SKEETO_UI(ChapStatsScoreRow, L"[2] Score system %s")
SKEETO_UI(ChapStatsCardKills, L"Kills")
SKEETO_UI(ChapStatsCardScore, L"Score")
SKEETO_UI(ChapStatsCardPageScore, L"Page score")
SKEETO_UI(ChapStatsCardBest, L"Best this page")
SKEETO_UI(ChapStatsPageHint, L"← → flip pages")
SKEETO_UI(ChapStatsTitle, L"Chapter Stats")
SKEETO_UI(ChapStatsTitleLive, L"This Chapter (live)")
SKEETO_UI(ChapStatsPageScore, L"Score Overview")
SKEETO_UI(ChapStatsPageKills, L"Kills & Headshots")
SKEETO_UI(ChapStatsPageDmg, L"Tank/Witch Damage")
SKEETO_UI(ChapStatsPageSi, L"Special Infected")
SKEETO_UI(ChapStatsPageTeam, L"Team Contribution")
SKEETO_UI(ChapStatsPageSpec, L"Special Ops")
SKEETO_UI(ChapStatsColCi, L"CI")
SKEETO_UI(ChapStatsColSi, L"SI")
SKEETO_UI(ChapStatsColTank, L"Tank")
SKEETO_UI(ChapStatsColWitch, L"Witch")
SKEETO_UI(ChapStatsColHs, L"HS")
SKEETO_UI(ChapStatsColHsRate, L"HS kill rate")
SKEETO_UI(ChapStatsColTankDmg, L"Tank damage")
SKEETO_UI(ChapStatsColWitchDmg, L"Witch damage")
SKEETO_UI(ChapStatsColScoreDmg, L"Damage score")
SKEETO_UI(ChapStatsColPinned, L"Pinned")
SKEETO_UI(ChapStatsColIncap, L"Downs")
SKEETO_UI(ChapStatsColDeath, L"Deaths")
SKEETO_UI(ChapStatsColRevive, L"Revives")
SKEETO_UI(ChapStatsColDefib, L"Defibs")
SKEETO_UI(ChapStatsColHeal, L"Heals")
SKEETO_UI(ChapStatsColRescue, L"Rescues")
SKEETO_UI(ChapStatsColScoreKills, L"Kill pts")
SKEETO_UI(ChapStatsColScoreHs, L"HS bonus")
SKEETO_UI(ChapStatsColScoreSupport, L"Support")
SKEETO_UI(ChapStatsColScorePenalty, L"Penalty")
SKEETO_UI(ChapStatsColScoreSpecial, L"Special")
SKEETO_UI(ChapStatsColScoreTotal, L"Total")
SKEETO_UI(ChapStatsColMeleeSi, L"Melee SI")
SKEETO_UI(ChapStatsColSkeet, L"Skeet")
SKEETO_UI(ChapStatsColMeleeSkeet, L"Melee skeet")
SKEETO_UI(ChapStatsColCrownMelee, L"Melee Witch")
SKEETO_UI(ChapStatsColTongueCut, L"Tongue cut")
SKEETO_UI(ChapStatsColChargerLevel, L"Level")
SKEETO_UI(ChapStatsNoData, L"No stats for this chapter yet")
SKEETO_UI(PanelStyleEntry, L"[%d] Panel style [Settings]")
SKEETO_UI(PanelStyleTitleTab, L"TAB Scoreboard - Panel style  v%s")
SKEETO_UI(PanelStyleTitleChap, L"Chapter Stats - Panel style  v%s")
SKEETO_UI(PanelStyleHint, L"Changes are saved immediately. While in game, this page previews the panel in the screen center with live data.")
SKEETO_UI(PanelStyleBgRow, L"[1] Background color [%s]")
SKEETO_UI(PanelStyleAlphaRow, L"[2] Background opacity [%d%%]")
SKEETO_UI(PanelStyleAccentRow, L"[3] Accent / highlight [%s]")
SKEETO_UI(PanelStyleTextRow, L"[4] Text color [%s]")
SKEETO_UI(PanelStyleBorderRow, L"[5] Border [%s]")
SKEETO_UI(PanelStyleZebraRow, L"[6] Row stripes %s")
SKEETO_UI(PanelStyleReset, L"[7] Reset style to default")
SKEETO_UI(PanelStyleCustomHint, L"    Accent sets the top rule, headers and active tab; highlight marks best values and your row. Select Custom, then enter %hs R G B in the console to set the panel background (each value 0-255; current %d %d %d). Row stripes are derived by lightening or darkening the base color.")
SKEETO_UI(PsBgDefault, L"Slate (default)")
SKEETO_UI(PsBgBlack, L"Black")
SKEETO_UI(PsBgBlue, L"Navy")
SKEETO_UI(PsBgPurple, L"Plum")
SKEETO_UI(PsBgGreen, L"Pine")
SKEETO_UI(PsBgCustom, L"Custom")
SKEETO_UI(PsAccTeal, L"Teal + amber (default)")
SKEETO_UI(PsAccBlue, L"Sky + gold")
SKEETO_UI(PsAccViolet, L"Violet + pink")
SKEETO_UI(PsAccOrange, L"Orange + yellow")
SKEETO_UI(PsAccSilver, L"Silver + cyan")
SKEETO_UI(PsTextCool, L"Cool white (default)")
SKEETO_UI(PsTextWhite, L"Pure white")
SKEETO_UI(PsTextWarm, L"Warm white")
SKEETO_UI(PsTextGray, L"Light gray")
SKEETO_UI(PsBorderNone, L"None")
SKEETO_UI(PsBorderThin, L"Thin (default)")
SKEETO_UI(PsBorderThick, L"Thick")
SKEETO_UI(KbSecMenu, L"Open / close Skeeto menu")
SKEETO_UI(KbSecTabSkeeto, L"Single tap: Skeeto board; double tap: vanilla scoreboard")
SKEETO_UI(KbSecTabVanilla, L"Single tap: vanilla scoreboard; double tap: Skeeto board")
SKEETO_UI(KbSecNavGoal, L"Cycle safehouse route target (console: skeeto_nav_goal auto or candidate number)")
SKEETO_UI(KbSecSpotlight, L"Spotlight night vision on / off")
SKEETO_UI(KbSecHud, L"Hide / show Skeeto custom HUD")
SKEETO_UI(KbSecWorldHud, L"Hide / show Skeeto world HUD")
SKEETO_UI(KbSecXhair, L"Hide / show Skeeto crosshair")
SKEETO_UI(KbSecAcid, L"Spitter acid range outline on / off")
SKEETO_UI(KbSecCfgNext, L"Next Config Override preset")
SKEETO_UI(KbSecCfgPrev, L"Previous Config Override preset")
SKEETO_UI(KbSecBgToggle, L"Main menu background engine on / off")
SKEETO_UI(KbSecBgNext, L"Next main menu background")
SKEETO_UI(KbSecUiReload, L"Reload Skeeto UI (fonts / textures)")
SKEETO_UI(KbSecChapStats, L"Hold: this chapter's stats (chapter stats panel)")
SKEETO_UI(KbSecNav, L"Navigation and supplies on / off")
SKEETO_UI(KbSecViewModelMirror, L"Global viewmodel mirroring on / off")
SKEETO_UI(MainScriptEntry, L"[7] Skeetoscript")
SKEETO_UI(MainBackgroundEntry, L"[8] Skeeto Background Engine")
SKEETO_UI(ScriptPageTitle, L"Skeetoscript (%d/%d)  v%s")
SKEETO_UI(BackgroundPageTitle, L"Skeeto Background Engine  v%s")
SKEETO_UI(CareerArchivePageTitle, L"Skeeto Career Archive (%d/%d)  v%s")
SKEETO_UI(NavRoot, L"[9] Minimap, routes and supplies")
SKEETO_UI(NavTitle, L"Minimap, routes and supplies")
SKEETO_UI(NavSourceCredit, L"Source code provided by Ragdoll, with permission. Skeeto handles integration and some optimizations.")
SKEETO_UI(NavDisplayRoot, L"[1] Minimap display and layout [Settings]")
SKEETO_UI(NavRouteRoot, L"[2] Route guidance and safehouse [Settings]")
SKEETO_UI(NavSuppliesRoot, L"[3] Supply filters [Settings]")
SKEETO_UI(NavWeaponsRoot, L"[4] Specific weapon filters [Settings]")
SKEETO_UI(NavDisplayTitle, L"Minimap display and layout")
SKEETO_UI(NavRouteTitle, L"Route guidance and safehouse")
SKEETO_UI(NavSuppliesTitle, L"Supply filters (%d/%d)")
SKEETO_UI(NavWeaponsTitle, L"Specific weapons (%d/%d)")
SKEETO_UI(NavRootHint, L"Minimap: skeeto_nav [0|1]. For world-route-only display, enable both Route guidance and World route cues. Safehouse: skeeto_nav_goal cycles candidates; use 'skeeto_nav_goal auto' to restore Auto.")
SKEETO_UI(NavAdjustPageHint, L"[-/+] Adjust %s    Left/Right: switch subpage")
SKEETO_UI(NavDisplayHint, L"[-/+] Adjust %s    Edit position on the next page")
SKEETO_UI(NavEdit, L"[3] Edit minimap position")
SKEETO_UI(NavReload, L"[4] Reload NAV")
SKEETO_UI(NavStatus, L"Status")
SKEETO_UI(NavLabel0, L"Show minimap")
SKEETO_UI(NavLabel1, L"Map size")
SKEETO_UI(NavLabel2, L"Map radius (game units)")
SKEETO_UI(NavLabel3, L"Rotate with view")
SKEETO_UI(NavLabel4, L"Teammates")
SKEETO_UI(NavLabel5, L"Horizontal position")
SKEETO_UI(NavLabel6, L"Vertical position")
SKEETO_UI(NavLabel7, L"Opacity")
SKEETO_UI(NavLabel8, L"Route guidance")
SKEETO_UI(NavLabel9, L"Safehouse candidate")
SKEETO_UI(NavLabel10, L"Supplies")
SKEETO_UI(NavLabel11, L"Supply radius (game units)")
SKEETO_UI(NavLabel12, L"Ammo pile")
SKEETO_UI(NavLabel13, L"Bile jar")
SKEETO_UI(NavLabel14, L"Molotov")
SKEETO_UI(NavLabel15, L"Pills")
SKEETO_UI(NavLabel16, L"First aid kit")
SKEETO_UI(NavLabel17, L"Pipe bomb")
SKEETO_UI(NavLabel18, L"Adrenaline")
SKEETO_UI(NavLabel19, L"Defibrillator")
SKEETO_UI(NavLabel20, L"Laser sight")
SKEETO_UI(NavLabel21, L"All weapon supplies")
SKEETO_UI(NavLabel22, L"Unknown weapon")
SKEETO_UI(NavLabel23, L"AK rifle")
SKEETO_UI(NavLabel24, L"M4 rifle")
SKEETO_UI(NavLabel25, L"Military sniper")
SKEETO_UI(NavLabel26, L"Hunting rifle")
SKEETO_UI(NavLabel27, L"MAC SMG")
SKEETO_UI(NavLabel28, L"UZI SMG")
SKEETO_UI(NavLabel29, L"AWP sniper")
SKEETO_UI(NavLabel30, L"Scout sniper")
SKEETO_UI(NavLabel31, L"SCAR rifle")
SKEETO_UI(NavLabel32, L"Grenade launcher")
SKEETO_UI(NavLabel33, L"M60 machine gun")
SKEETO_UI(NavLabel34, L"Pistol")
SKEETO_UI(NavLabel35, L"Magnum")
SKEETO_UI(NavLabel36, L"Pump shotgun")
SKEETO_UI(NavLabel37, L"Chrome shotgun")
SKEETO_UI(NavLabel38, L"Auto shotgun")
SKEETO_UI(NavLabel39, L"SPAS shotgun")
SKEETO_UI(NavLabel40, L"MP5 SMG")
SKEETO_UI(NavLabel41, L"SG552 rifle")
SKEETO_UI(NavLabel42, L"Melee weapon")
SKEETO_UI(NavLabel43, L"Chainsaw")
SKEETO_UI(NavLabel44, L"World route cues")
SKEETO_UI(NavLabel45, L"Minimap and route refresh rate (FPS)")
SKEETO_UI(NavLabel46, L"Minimap style")
SKEETO_UI(NavRefreshRow, L"[%d] %s [%d]  - / + adjust")
SKEETO_UI(NavRefreshHint, L"Defaults to 10 FPS, up to 60. Updates minimap and world route data; higher rates cost more processing.")
SKEETO_UI(NavMapStyleClassic, L"Classic")
SKEETO_UI(NavMapStyleTactical, L"Tactical night")

SKEETO_UI(NavHud0, L"Route: orange segments")
SKEETO_UI(NavHud1, L"Route: check orange segments")
SKEETO_UI(NavHud2, L"Route via ladder")
SKEETO_UI(NavHud3, L"Route ready")
SKEETO_UI(NavHud4, L"At candidate area")
SKEETO_UI(NavHud5, L"Searching: direction")
SKEETO_UI(NavHud6, L"Searching: direction only")
SKEETO_UI(NavHud7, L"No position: direction")
SKEETO_UI(NavHud8, L"No position: direction only")
SKEETO_UI(NavHud9, L"No route: direction only")
SKEETO_UI(NavHud10, L"Incomplete route: direction")
SKEETO_UI(NavHud11, L"Incomplete route: direction only")
SKEETO_UI(NavHud12, L"No safehouse marker")
SKEETO_UI(NavHud13, L"Select safehouse candidate")
SKEETO_UI(NavHud14, L"Candidate unavailable")
SKEETO_UI(NavHud15, L"Navigation unavailable")
SKEETO_UI(NavHud16, L"No NAV map")
SKEETO_UI(NavHud17, L"Preparing navigation")
SKEETO_UI(NavHud18, L"Turn left ahead")
SKEETO_UI(NavHud19, L"Turn right ahead")
SKEETO_UI(NavHud20, L"Go up ahead")
SKEETO_UI(NavHud21, L"Go down ahead")
SKEETO_UI(NavHud22, L"Climb ladder ahead")
SKEETO_UI(NavHud23, L"Descend ladder ahead")
SKEETO_UI(NavHud24, L"Special section ahead")
SKEETO_UI(NavHud25, L"At destination")
SKEETO_UI(NavHud26, L"Follow ground arrows")
SKEETO_UI(NavHud27, L"Rescue point")
SKEETO_UI(NavHud28, L"Safehouse")
SKEETO_UI(NavHud29, L"World view unavailable")
SKEETO_UI(NavHud30, L"World guidance unavailable")
SKEETO_UI(NavHud31, L"Route behind you: turn around")
SKEETO_UI(NavHud32, L"Route on the left")
SKEETO_UI(NavHud33, L"Route on the right")
SKEETO_UI(NavHud39, L"Map detail reduced")
SKEETO_UI(NavHud41, L"Supplies unavailable")
SKEETO_UI(NavHud47, L"Arrows move (Shift: big step), B/S zoom")
SKEETO_UI(NavHud48, L"Press 2, Enter or Esc to save")
SKEETO_UI(ClientCombatTitle, L"Combat and audiovisual enhancements  v%s")
SKEETO_UI(ClientConvenienceTitle, L"Fixes and conveniences  v%s")
SKEETO_UI(ClientConvenienceHint, L"These features run on your client and do not change server settings.")
SKEETO_UI(BackgroundUsageHint, L"Use: put PNG, JPG/JPEG, H.264 MP4 or BIK files in left4dead2/skeeto/backgrounds/. Press [6] to rescan after adding files. 4K MP4 is not recommended: decoding is demanding and conversion is less efficient, which can cause abnormal playback speed.")
SKEETO_UI(LocalCharacterEntry, L"[8] Character switching [Settings]")
SKEETO_UI(LocalCharacterEntryHint, L"    Open this page to enable character switching and choose a survivor. Enabling it precaches all eight models and may affect frame rate; off by default.")
SKEETO_UI(LocalCharacterPickEntry, L"[2] Choose survivor [Open settings]")
SKEETO_UI(LocalCharacterPickPageTitle, L"Choose survivor  v%s")
SKEETO_UI(LocalCharacterPickHint, L"Choose Nick, Rochelle, Coach, Ellis, Bill, Zoey, Francis or Louis.")
SKEETO_UI(LocalCharacterPickLockedHint, L"    Enable character switching on a local listen server first.")
SKEETO_UI(LocalDebugCvarEntry, L"[9] Debug Cvar options [Settings]")
SKEETO_UI(LocalBugEntry, L"[1] Show invincible bug spots [Settings]")
SKEETO_UI(LocalListenDirectorEntry, L"[2] Full director HUD [Settings]")
SKEETO_UI(LocalNoFall, L"[3] No fall damage %s")
SKEETO_UI(LocalNoFallHint, L"    Local server only. Fall damage is not applied on this listen server. Remote servers are unchanged.")
SKEETO_UI(LocalMeleeScriptTiming, L"[4] Decouple melee rate from anim length %s")
SKEETO_UI(LocalMeleeScriptTimingHint, L"    Local server only. Fixes listen-host melee attack rate depending on animation length instead of weapon-script values. Dedicated servers are unchanged.")
SKEETO_UI(LocalPausable, L"[5] Allow unconditional local-server pause %s")
SKEETO_UI(LocalPausableHint, L"    Listen host only. ESC freezes the whole session like official Solo, including map loads, lobby local games, and local multiplayer (connected friends pause too). Off by default. Locked on remote servers.")
SKEETO_UI(LocalBugPageTitle, L"Invincible bug spots  v%s")
SKEETO_UI(LocalBugToggle, L"[1] Show invincible bug spots %s")
SKEETO_UI(LocalBugBoxes, L"[2] Pink quads %s")
SKEETO_UI(LocalBugTeleport, L"[3] Teleport to the nearest invincible bug spot (local server only)")
SKEETO_UI(LocalBugStatusScan, L"Searching")
SKEETO_UI(LocalBugStatusNone, L"None on this map")
SKEETO_UI(LocalBugStatusReady, L"Found %u")
SKEETO_UI(LocalBugStatusNoStart, L"This map has no usable start")
SKEETO_UI(LocalBugStatusNoNav, L"This map has no navigation data")
SKEETO_UI(LocalDirectorPageTitle, L"Full director HUD  v%s")
SKEETO_UI(LocalDirectorToggle, L"[1] Listen director panel %s")
SKEETO_UI(LocalDirectorScale, L"[2] Size [%d%%]  - / +     Font [%d]  Shift + - / +")
SKEETO_UI(LocalDirectorReset, L"[3] Reset position (top right)")
SKEETO_UI(LocalDirectorMoveHint, L"    Arrow keys move it. Hold to keep moving, Shift for a large step. This page previews sample rows.")
SKEETO_UI(LocalDirectorToggleHint, L"    Your own listen server only. During relax, the countdown or the distance ends the phase, whichever comes first. Once the distance is used up, the countdown hides. Lock means the director is holding the tempo.")
SKEETO_UI(LocalDirectorPhase, L"Phase  %s")
SKEETO_UI(LocalDirectorCountdown, L"Countdown  %d s")
SKEETO_UI(LocalDirectorLock, L"Lock  %s")
SKEETO_UI(LocalDirectorLockOn, L"Yes")
SKEETO_UI(LocalDirectorLockOff, L"No")
SKEETO_UI(LocalDirectorFlow, L"Distance  %d yd")
SKEETO_UI(LocalCharacterPageTitle, L"Character switching settings  v%s")
SKEETO_UI(LocalCharacterToggleRow, L"[1] Character switching %s")
SKEETO_UI(LocalCharacterToggleHint, L"    Enabling precaches all eight survivor models and may affect frame rate; off by default. Console: skeeto_character_change 1/0")
SKEETO_UI(NavLoad0, L"Waiting for map")
SKEETO_UI(NavLoad1, L"Invalid map name")
SKEETO_UI(NavLoad2, L"NAV filesystem unavailable")
SKEETO_UI(NavLoad3, L"NAV is too large")
SKEETO_UI(NavLoad4, L"Reading map")
SKEETO_UI(NavLoad5, L"NAV read failed")
SKEETO_UI(NavLoad6, L"Unsupported NAV")
SKEETO_UI(NavLoad7, L"Parsing map")
SKEETO_UI(NavLoad8, L"Map ready")
SKEETO_UI(NavDisabled, L"Disabled")
SKEETO_UI(NavReady, L"Map ready")
SKEETO_UI(NavItem0, L"")
SKEETO_UI(NavItem1, L"Kit")
SKEETO_UI(NavItem2, L"Defib")
SKEETO_UI(NavItem3, L"Pills")
SKEETO_UI(NavItem4, L"Adren")
SKEETO_UI(NavItem5, L"Molotov")
SKEETO_UI(NavItem6, L"Pipe")
SKEETO_UI(NavItem7, L"Bile")
SKEETO_UI(NavItem8, L"Ammo")
SKEETO_UI(NavItem9, L"Laser")
SKEETO_UI(NavItem10, L"Unknown")
SKEETO_UI(NavItem11, L"ak")
SKEETO_UI(NavItem12, L"m4")
SKEETO_UI(NavItem13, L"30")
SKEETO_UI(NavItem14, L"15")
SKEETO_UI(NavItem15, L"mac")
SKEETO_UI(NavItem16, L"uzi")
SKEETO_UI(NavItem17, L"awp")
SKEETO_UI(NavItem18, L"scout")
SKEETO_UI(NavItem19, L"scar")
SKEETO_UI(NavItem20, L"GL")
SKEETO_UI(NavItem21, L"m60")
SKEETO_UI(NavItem22, L"Pistol")
SKEETO_UI(NavItem23, L"Magnum")
SKEETO_UI(NavItem24, L"Pump")
SKEETO_UI(NavItem25, L"Chrome")
SKEETO_UI(NavItem26, L"Auto")
SKEETO_UI(NavItem27, L"spas")
SKEETO_UI(NavItem28, L"mp5")
SKEETO_UI(NavItem29, L"sg552")
SKEETO_UI(NavItem30, L"Melee")
SKEETO_UI(NavItem31, L"Saw")
SKEETO_UI(NavAdjustHint, L"[-/+] Adjust: %s")
SKEETO_UI(NavPageHint, L"Select a setting or action")
SKEETO_UI(NavLabel47, L"World route arrow color")
SKEETO_UI(NavLabel48, L"Route HUD horizontal position")
SKEETO_UI(NavLabel49, L"Route HUD vertical position")
SKEETO_UI(NavLabel50, L"Minimap shape")
SKEETO_UI(NavMapShapeDisc, L"Disc")
SKEETO_UI(NavMapShapeSquare, L"Square")
SKEETO_UI(NavArrowColorRow, L"[%d] %s [%s]  - / + Adjust")
SKEETO_UI(NavArrowColor0, L"Teal")
SKEETO_UI(NavArrowColor1, L"Green")
SKEETO_UI(NavArrowColor2, L"Yellow")
SKEETO_UI(NavArrowColor3, L"Orange")
SKEETO_UI(NavArrowColor4, L"Blue")
SKEETO_UI(NavArrowColor5, L"Purple")
SKEETO_UI(NavArrowColor6, L"White")
SKEETO_UI(NavWorldEdit, L"[6] Adjust route HUD position")
SKEETO_UI(NavWorldEditHint, L"Arrow keys move; hold to repeat. Shift moves farther.")
SKEETO_UI(NavWorldEditExitHint, L"Press Enter or Esc to finish.")
// L"    部分服务器上，此 HUD 可能导致闲置接管时游戏频繁闪退。遇到此情况，请关闭此 HUD 并启用游戏原有的队友血条。"
SKEETO_UI(TeamHudIdleCrashWarning, L"    On some servers, this HUD may cause frequent crashes when taking over an idle player. If this happens, turn off this HUD and enable the game's original teammate health bars.")
SKEETO_UI(MemoryHudCardTitle, L"MEMORY")
SKEETO_UI(MemoryClearNow, L"[6] Clear datacache manually")
SKEETO_UI(MemoryInfoMode, L"[5] Information [%s]")
SKEETO_UI(MemoryCacheLocked, L"Locked")
SKEETO_UI(MemoryLargestFree, L"Max free block")
SKEETO_UI(MemoryProcessPair, L"WS")
SKEETO_UI(MemoryLastClear, L"CLR")
SKEETO_UI(MemoryClearHistory, L"%.0f MB / %us")
SKEETO_UI(MemoryNeverCleared, L"--")
SKEETO_UI(KbSecClearCache, L"Clear unlocked datacache")

SKEETO_UI(KbListSectionConflict, L"    The added entries may be hidden by vocalizer mods containing kb_act.lst.")

SKEETO_UI(FeedbackJsonLimit, L"[3] JSON catalog limit [%d styles]")
SKEETO_UI(FeedbackJsonLimitHint, L"    Cycle: 64 / 96 / 128, default 64. Applied on returning to the game main menu; no in-match rescan.")
SKEETO_UI(ParticleManifestLimit, L"[6] Particle list limit [%d lists]")
SKEETO_UI(ParticleManifestLimitHint, L"    Cycle: 16 / 32 / 48, default 32. Counts Skeeto particle lists registered at startup. A change applies on the next launch. Nothing is registered while the platform is off.")
SKEETO_UI(MemoryCacheUnlocked, L"Unlocked")
SKEETO_UI(MemoryPrivateCommit, L"Private commit")

SKEETO_UI(MemoryCacheExplain, L"    Locked: protected game cache that cannot be evicted. Unlocked: reclaimable cache; clearing only targets this part.")
SKEETO_UI(MemoryProcessExplain, L"    Working set: process pages currently resident in physical memory. Private commit: committed memory exclusive to this process, backed by RAM or the page file.")
SKEETO_UI(MemoryLargestExplain, L"    Max free block: largest contiguous free virtual address range. Small blocks may prevent large allocations; this differs from total available space.")

SKEETO_UI(MemoryHudBrand, L"Skeeto")
SKEETO_UI(OverlayHudEntry, L"[4] Overlay HUD")
SKEETO_UI(OverlayHudHint, L"Standalone transparent HUDs. Their switches, size and position are independent of the in-game HUD modules.")
SKEETO_UI(OverlayHudAmmoTarget, L"Ammo arc")
SKEETO_UI(OverlayHudReloadTarget, L"Reload ring")
SKEETO_UI(OverlayHudControls, L"Arrow keys move 8px; hold Shift to move 32px. Use -/+ to resize.")

SKEETO_UI(TeamWorldNames, L"Always show survivor names")
SKEETO_UI(TeamWorldNamesDesc, L"    Show other survivors through obstacles, regardless of distance or crosshair; unavailable in Realism")

SKEETO_UI(OverlayHudPageTitle, L"Overlay HUD %s  v%s")
SKEETO_UI(OverlayHudFireTarget, L"Reload / fire cooldown ring")
SKEETO_UI(OverlayHudAmmoEntry, L"[1] Ammo arc [Settings]")
SKEETO_UI(OverlayHudReloadEntry, L"[2] Reload ring [Settings]")
SKEETO_UI(OverlayHudFireEntry, L"[3] Reload / fire cooldown ring [Settings]")
SKEETO_UI(OverlayHudEnabled, L"[1] Enable %s")
SKEETO_UI(OverlayHudPageSize, L"[2] Size: %d%s (-/+)")
SKEETO_UI(OverlayHudPageReset, L"[3] Reset size and position")
SKEETO_UI(OverlayHudSide, L"[4] Arc direction: %s")
SKEETO_UI(OverlayHudFireHint, L"Outer arc: reload progress. Inner arc: remaining time until the next shot.")
SKEETO_UI(OverlayHudPagePreview, L"This page previews only this HUD. Its own switch controls display after closing the menu.")

SKEETO_UI(OverlayColorWhite, L"White")
SKEETO_UI(OverlayColorCyan, L"Cyan")
SKEETO_UI(OverlayColorGold, L"Gold")
SKEETO_UI(OverlayColorGreen, L"Green")
SKEETO_UI(OverlayColorPurple, L"Purple")
SKEETO_UI(OverlayColorCoral, L"Coral red")
SKEETO_UI(OverlayCursorColor, L"[4] Ammo cursor color: %s")
SKEETO_UI(OverlayProgressColor, L"[4] Reload progress color: %s")
SKEETO_UI(OverlayReloadHideXhair, L"[5] Hide Skeeto crosshairs while shown %s")
SKEETO_UI(OverlayReloadHideHint, L"With this option enabled, the reload ring hides all Skeeto crosshairs while shown and restores them afterwards.")

SKEETO_UI(OverlayAmmoText, L"[5] Show ammo count text %s")


SKEETO_UI(WorldNamesSettings, L"Survivor name style")
SKEETO_UI(WorldNamesDefaultColor, L"Default pale blue")
SKEETO_UI(WorldNamesSizeRow, L"[%d] Text size [%d]  - / + adjust")
SKEETO_UI(WorldNamesStyleReset, L"[4] Restore default color and size")
SKEETO_UI(WorldNamesCommandHint, L"Exact color: skeeto_world_names_color R G B; size: skeeto_world_names_size 12..22")


SKEETO_UI(WorldNamesCycleRow,L"[2] Text color [%s]")
SKEETO_UI(WorldNamesCustomColor,L"Custom")

SKEETO_UI(NativeWorldNamesRow,L"[9] Always show stock survivor IDs %s")
SKEETO_UI(NativeWorldNamesHint,L"    Keep stock styling without occlusion, distance or crosshair limits. Up to 8 survivors; unavailable in Realism. Custom name HUD takes priority while active.")

// Custom homepage: native menu routes, flyouts and dedicated Skeeto pages.
SKEETO_UI(MainUiModeCoop, L"CAMPAIGN")
SKEETO_UI(MainUiModeRealism, L"REALISM")
SKEETO_UI(MainUiModeVersus, L"VERSUS")
SKEETO_UI(MainUiModeRealismVersus, L"REALISM VERSUS")
SKEETO_UI(MainUiModeSurvival, L"SURVIVAL")
SKEETO_UI(MainUiModeSurvivalVersus, L"VERSUS SURVIVAL")
SKEETO_UI(MainUiModeScavenge, L"SCAVENGE")
SKEETO_UI(MainUiModeMutation, L"MUTATIONS")
SKEETO_UI(MainUiQuickMatch, L"Quick match")
SKEETO_UI(MainUiCustomMatch, L"Play online")
SKEETO_UI(MainUiFriendsMatch, L"Play with friends")
SKEETO_UI(MainUiGroupServer, L"Steam group servers")
SKEETO_UI(MainUiSoloMode, L"Single player")
SKEETO_UI(MainUiTeamMatch, L"Team game")
SKEETO_UI(MainUiLeaderboards, L"Survivor rankings")
SKEETO_UI(MainUiChooseMutation, L"Choose a mutation")
SKEETO_UI(MainUiCurrentMutation, L"Play current mutation")
SKEETO_UI(MainUiMutationHardEight, L"Hard Eight")
SKEETO_UI(MainUiMutationLoneGunman, L"Lone Gunman")
SKEETO_UI(MainUiVideo, L"Video settings")
SKEETO_UI(MainUiAudio, L"Audio settings")
SKEETO_UI(MainUiKeyboardMouse, L"Keyboard / mouse")
SKEETO_UI(MainUiMultiplayer, L"Multiplayer settings")
SKEETO_UI(MainUiSteamController, L"Steam Controller")
SKEETO_UI(MainUiCloud, L"Steam Cloud")
SKEETO_UI(MainUiCommentary, L"Developer commentary")
SKEETO_UI(MainUiCredits, L"Credits")
SKEETO_UI(MainUiNativeShort, L"Stock home")
SKEETO_UI(MainUiGameOptions, L"GAME SETTINGS")
SKEETO_UI(MainUiAddons, L"ADD-ONS")
SKEETO_UI(MainUiAddonsLoading, L"Loading...")
SKEETO_UI(MainUiAchievements, L"Achievements")
SKEETO_UI(MainUiExtras, L"EXTRAS")
SKEETO_UI(MainUiAdvanced, L"SKEETO ADVANCED")
SKEETO_UI(MainUiVolumes, L"VOLUME MIXER")
SKEETO_UI(MainUiCareer, L"CAREER")
SKEETO_UI(MainUiBackHome, L"Back home")
SKEETO_UI(MainUiBackgroundEngine, L"Background engine")
SKEETO_UI(MainUiOn, L"Enabled")
SKEETO_UI(MainUiOff, L"Disabled")
SKEETO_UI(MainUiNextBackground, L"Next background")
SKEETO_UI(MainUiReloadBackground, L"Rescan backgrounds")
SKEETO_UI(MainUiFit, L"Display mode")
SKEETO_UI(MainUiRandomBackground, L"Random selection on startup")
SKEETO_UI(MainUiBackgroundVolume, L"Background volume")
SKEETO_UI(MainUiBackgroundVolumeHint, L"Independent MP4 volume; static images have no audio.")
SKEETO_UI(MainUiVolumeMaster, L"Category volume master switch")
SKEETO_UI(MainUiVolumeGunfire, L"Gunfire")
SKEETO_UI(MainUiVolumeHandling, L"Weapon handling")
SKEETO_UI(MainUiVolumeMelee, L"Melee")
SKEETO_UI(MainUiVolumeChainsaw, L"Chainsaw")
SKEETO_UI(MainUiVolumeFootstep, L"Footsteps")
SKEETO_UI(MainUiVolumeVoice, L"Survivor voice")
SKEETO_UI(MainUiVolumeSpecial, L"Special infected")
SKEETO_UI(MainUiVolumeCommon, L"Common infected")
SKEETO_UI(MainUiVolumeTank, L"Tank")
SKEETO_UI(MainUiVolumeWitch, L"Witch")
SKEETO_UI(MainUiVolumeExplosion, L"Explosions")
SKEETO_UI(MainUiVolumeThrowable, L"Throwables")
SKEETO_UI(MainUiVolumeAmbient, L"Ambient / world")
SKEETO_UI(MainUiVolumeUi, L"Interface / menu")
SKEETO_UI(MainUiVolumeVehicle, L"Vehicles / rescue")
SKEETO_UI(MainUiVolumeItem, L"Items")
SKEETO_UI(MainUiVolumeHit, L"Hit feedback")
SKEETO_UI(MainUiVolumeSafe, L"Safe-room music")
SKEETO_UI(MainUiAuxVolumeHint, L"These three independent volumes bypass the category master switch.")
SKEETO_UI(MainUiSliderStep, L"Cycle adjustment step")
SKEETO_UI(MainUiCareerHint, L"Existing Steam statistics; unavailable values display --.")
SKEETO_UI(MainUiCareerTime, L"Play time")
SKEETO_UI(MainUiCareerKills, L"Infected kills")
SKEETO_UI(MainUiCareerGames, L"Games played")
SKEETO_UI(MainUiCareerFinales, L"Finales survived")
SKEETO_UI(MainUiCareerRevives, L"Teammates revived")
SKEETO_UI(MainUiCareerProtected, L"Teammates protected")
SKEETO_UI(MainUiAccentLabel, L"Accent color (cycle presets)")
SKEETO_UI(MainUiChangeAccent, L"Change accent color")
SKEETO_UI(MainUiNativeUnavailable, L"Native entry failed compatibility checks. Use the stock main menu entry.")
SKEETO_UI(MainUiVolumePageState, L"Page %d / 3    Adjustment step %d%%")

SKEETO_UI(MainUiTestMaps, L"Quick test maps")
SKEETO_UI(MainUiRebuildAudio, L"Rebuild audio cache")
SKEETO_UI(MainUiBackdrop, L"Panel backdrop opacity")
SKEETO_UI(MainUiBackdropHint, L"0%: text only; adjustment step: 5%.")
SKEETO_UI(MainUiTumtara, L"TUMTARA")
SKEETO_UI(MainUiTumtaraL4d1, L"TUMTARA (L4D1)")
SKEETO_UI(MainUiRange, L"Gun Range")
SKEETO_UI(MainUiRangeL4d1, L"Gun Range (L4D1)")
SKEETO_UI(MainUiCodmRange, L"CODM Range")
SKEETO_UI(MainUiTrainingDay, L"Special-infected training (day)")
SKEETO_UI(MainUiTrainingNight, L"Special-infected training (night)")
SKEETO_UI(MainUiHstm, L"HSTM")
SKEETO_UI(MainUiMaldives, L"Maldives >")
SKEETO_UI(MainUiMaldivesTitle, L"Maldives")
SKEETO_UI(MainUiSeaSunset, L"Sea / Sunset")
SKEETO_UI(MainUiSeaNight, L"Sea / Night")
SKEETO_UI(MainUiSeaDay, L"Sea / Day")
SKEETO_UI(MainUiBeachDay, L"Beach / Day")
SKEETO_UI(MainUiBeachNight, L"Beach / Night")
SKEETO_UI(MainUiBeachSunset, L"Beach / Sunset")
SKEETO_UI(MainUiBackTests, L"< Back to test maps")
SKEETO_UI(MainUiTestMapsHint, L"Install the corresponding maps; Maldives includes sea and beach scenes.")
SKEETO_UI(MainUiRebuildAudioHint, L"Runs the game audio-cache rebuild; wait for it to finish.")

SKEETO_UI(MainUiFavoriteServer, L"Server entry")
SKEETO_UI(MainUiFavoriteMap, L"Favorite map")
SKEETO_UI(MainUiEditorTitle, L"Main menu editor")
SKEETO_UI(MainUiEditorHint, L"Drag to move; bottom-right corner: resize. Arrows: nudge; Ctrl: size; Shift: faster.")
SKEETO_UI(MainUiEditorActionHint, L"Choose an action; fill its content or add submenu items.")
SKEETO_UI(MainUiEditorParameter, L"Edit action parameter")
SKEETO_UI(MainUiEditorNoParameter, L"No parameters needed for this action.")
SKEETO_UI(MainUiEditorVisible, L"Button visible")
SKEETO_UI(MainUiEditorHidden, L"Button hidden")
SKEETO_UI(MainUiEditorSmall, L"Small text")
SKEETO_UI(MainUiEditorNormal, L"Normal text")
SKEETO_UI(MainUiEditorLarge, L"Large text")
SKEETO_UI(MainUiEditorColor, L"Cycle text color")
SKEETO_UI(MainUiEditorOpacity, L"Cycle backdrop")
SKEETO_UI(MainUiEditorAppearance, L"Color %d / 12   Backdrop %d%% (-1: inherit)")
SKEETO_UI(MainUiEditorLayerBack, L"Move earlier")
SKEETO_UI(MainUiEditorLayerFront, L"Move later")
SKEETO_UI(MainUiEditorDecorOn, L"Welcome heading: ON")
SKEETO_UI(MainUiEditorDecorOff, L"Welcome heading: OFF")
SKEETO_UI(MainUiEditorSnapOn, L"8-pixel snap: ON (Shift pauses)")
SKEETO_UI(MainUiEditorSnapOff, L"8-pixel snap: OFF")
SKEETO_UI(MainUiEditorAdd, L"Add button")
SKEETO_UI(MainUiEditorDuplicate, L"Duplicate")
SKEETO_UI(MainUiEditorDelete, L"Delete button and children")
SKEETO_UI(MainUiEditorReset, L"Restore defaults")
SKEETO_UI(MainUiEditorApply, L"Apply and save")
SKEETO_UI(MainUiEditorLimit, L"Button limit reached: %d. Adjust it in main menu settings.")
SKEETO_UI(MainUiLimit256, L"Button limit: 256 (click to change)")
SKEETO_UI(MainUiLimit512, L"Button limit: 512 (click to change)")
SKEETO_UI(MainUiLimitHint, L"Includes children/hidden items. Remove extra buttons before lowering to 256.")
SKEETO_UI(MainUiEditorInvalid, L"Invalid or long content. Check action parameters or main menu profile format.")
SKEETO_UI(MainUiEditorPasteError, L"Paste failed. Use shorter plain text and try again.")
SKEETO_UI(MainUiEditorTextHint, L"Type / Ctrl+V: paste (Chinese supported) / Ctrl+A: select all. Enter: apply; Esc: cancel.")
SKEETO_UI(MainUiEditorPaste, L"Paste from clipboard")
SKEETO_UI(MainUiEditorTextApply, L"Apply text")
SKEETO_UI(MainUiEditorChooseAction, L"Choose button action")
SKEETO_UI(MainUiConsoleAction, L"Console command")
SKEETO_UI(MainUiCustomMenu, L"Custom submenu")
SKEETO_UI(MainUiCustomEmpty, L"No items yet. Enter the editor and add buttons.")
SKEETO_UI(MainUiCustomBack, L"Back")
SKEETO_UI(MainUiCustomNext, L"Next page")
SKEETO_UI(MainUiEditorCommand, L"Edit console command")
SKEETO_UI(MainUiEditorHomeParent, L"Main menu")
SKEETO_UI(MainUiEditorParent, L"Button parent: %ls (click to change)")
SKEETO_UI(MainUiEditHomepage, L"Edit main menu")

SKEETO_UI(MainUiServerCenter, L"Server center")
SKEETO_UI(MainUiBattleHistory, L"Recent battle reports")
SKEETO_UI(MainUiHudProfiles, L"Main menu profiles")
SKEETO_UI(MainUiChangelog, L"Skeeto changelog")
SKEETO_UI(MainUiCommunity, L"Skeeto Workshop")
SKEETO_UI(MainUiAssetFolder, L"Open asset folder")
SKEETO_UI(MainUiHomeServices, L"Main menu services")
SKEETO_UI(MainUiWebAction, L"Open website")
SKEETO_UI(MainUiPathAction, L"Open file / folder")
SKEETO_UI(MainUiSteamAction, L"Open Steam link")
SKEETO_UI(MainUiLoadHudAction, L"Load profile (filename)")
SKEETO_UI(MainUiRecentOn, L"Server history: ON")
SKEETO_UI(MainUiRecentOff, L"Server history: OFF")
SKEETO_UI(MainUiQueryOn, L"Query status: ON")
SKEETO_UI(MainUiQueryOff, L"Query status: OFF")
SKEETO_UI(MainUiFavoritesTab, L"Favorite servers")
SKEETO_UI(MainUiRecentTab, L"Server play history")
SKEETO_UI(MainUiBattleOn, L"Record reports: ON")
SKEETO_UI(MainUiBattleOff, L"Record reports: OFF")
SKEETO_UI(MainUiOpenReport, L"Open detailed browser report")
SKEETO_UI(MainUiProfileSlot, L"Next save slot")
SKEETO_UI(MainUiSaveProfile, L"Save current profile")
SKEETO_UI(MainUiExportProfile, L"Export / share")
SKEETO_UI(MainUiImportProfile, L"Import profile")
SKEETO_UI(MainUiHubNext, L"Next page")
SKEETO_UI(MainUiHubEmpty, L"No records. Enable recording before playing; add favorites in the editor.")
SKEETO_UI(MainUiBattleHint, L"Chapter stats must be enabled; only cleared chapters (with squad stats) are recorded.")
SKEETO_UI(MainUiProfileHint, L"Changes main menu buttons, submenus and appearance; leaves in-game HUD intact.")
SKEETO_UI(MainUiServerHint, L"Click to reconnect. Append @query-port if needed; timeout does not prove offline.")
SKEETO_UI(MainUiQueryIpOnly, L"Query requires a valid IPv4 / IPv6 address")
SKEETO_UI(MainUiQueryUnknown, L"Unknown / not queried")
SKEETO_UI(MainUiSettled, L"Chapter settled")
SKEETO_UI(MainUiFailed, L"Failed")
SKEETO_UI(MainUiIncomplete, L"Incomplete")
SKEETO_UI(MainUiHubBusy, L"Task queue busy; please retry shortly.")
SKEETO_UI(MainUiProfileApplied, L"Main menu profile applied.")

SKEETO_UI(MainUiHubIoError, L"Could not read, write or open file; check the path, file and permissions.")

SKEETO_UI(MainUiProfilesEmpty, L"No profiles. Save current main menu or import a main menu profile.")
SKEETO_UI(MainUiBattlesEmpty, L"No cleared chapters yet. Enable recording and chapter stats, then clear a chapter.")
SKEETO_UI(MainUiRecordedScore, L"Score %d")
SKEETO_UI(MainUiScoreDisabled, L"Scoring disabled")

SKEETO_UI(MainUiExportUi, L"Export main menu (choose destination)")
SKEETO_UI(MainUiImportUi, L"Import main menu (choose file)")

SKEETO_UI(MainUiFolderSample, L"Open cfg folder")
SKEETO_UI(MainUiAssetsSample, L"Preset assets")
SKEETO_UI(MainUiMapSample, L"Map: Dead Center")
SKEETO_UI(MainUiProfileFolderSample, L"Main menu profile folder")

SKEETO_UI(MainUiFavoritesListAction, L"Favorite servers list")
SKEETO_UI(MainUiEditorServerEntryHint, L"Title: name. Address: IP:port.")
SKEETO_UI(MainUiEditorServerAddress, L"Edit server IP / port")
SKEETO_UI(MainUiEditorServerListHint, L"List shortcut; no IP here.")
SKEETO_UI(MainUiEditorAddServerHint, L"To add, choose Server entry.")
SKEETO_UI(MainUiEditorAddChildHint, L"Select menu, then Add button.")
SKEETO_UI(MainUiEditorChildHint, L"New items belong to this menu.")

SKEETO_UI(MainUiAddServer, L"Add server")
SKEETO_UI(MainUiNewServer, L"New server (click to rename)")
SKEETO_UI(MainUiServerQueryOff, L"Status queries disabled")

SKEETO_UI(FireGateReloadToggle, L"[5] Reload progress %s")
SKEETO_UI(FireGateFireToggle, L"[6] Fire cooldown %s")
SKEETO_UI(MainUiWebPresets, L"Useful websites")
SKEETO_UI(MainUiWebSoda, L"Kitasoda server list")
SKEETO_UI(MainUiWebMaps, L"Community map downloads")
SKEETO_UI(MainUiWebSherlock, L"Sherlock server list")
SKEETO_UI(MainUiWebCat, L"Cat server list")
SKEETO_UI(MainUiUnbound, L"Unbound")
SKEETO_UI(MainUiNavigationHint, L"Mouse / Arrows / Tab / Enter    Left/Right: sliders    Esc: back. Customize shortcuts in Main menu settings.")
SKEETO_UI(MainUiHintsOn, L"Bottom key hints: shown")
SKEETO_UI(MainUiHintsOff, L"Bottom key hints: hidden")
SKEETO_UI(MainUiVersionOn, L"Skeeto version: shown")
SKEETO_UI(MainUiVersionOff, L"Skeeto version: hidden")
SKEETO_UI(MainUiGameNameOn, L"Author credit: shown")
SKEETO_UI(MainUiGameNameOff, L"Author credit: hidden")
SKEETO_UI(MainUiBindNative, L"Stock main menu: %s")
SKEETO_UI(MainUiBindHome, L"Skeeto main menu: %s")
SKEETO_UI(MainUiBindEditor, L"Editor: %s")
SKEETO_UI(MainUiBindCapture, L"Press a key; Backspace / Delete: unbind. Esc: cancel.")
SKEETO_UI(MainUiBindHint, L"Fixed keys cannot be rebound. Disable them to use key capture or bind / unbind.")
SKEETO_UI(MainUiCvarSlider, L"ConVar slider")
SKEETO_UI(MainUiGroupModes, L"Game modes and quick start")
SKEETO_UI(MainUiGroupServers, L"Servers and maps")
SKEETO_UI(MainUiGroupGame, L"Game dialogs and tools")
SKEETO_UI(MainUiGroupFeatures, L"Skeeto features")
SKEETO_UI(MainUiGroupLayout, L"Main menu editing and presets")
SKEETO_UI(MainUiGroupCustom, L"Custom actions / Submenus / Sliders")
SKEETO_UI(MainUiEditorHideHidden, L"Hidden buttons shown (click to hide)")
SKEETO_UI(MainUiEditorShowHidden, L"Show hidden buttons (restore)")
SKEETO_UI(MainUiCvarFormat, L"Set variable name, minimum, maximum and step separately")
SKEETO_UI(MainUiCvarExample, L"Fill name / minimum / maximum / step separately; the name field also accepts all four values.")
SKEETO_UI(MainUiCareerOverview, L"Overview")
SKEETO_UI(MainUiCareerTeam, L"Teamwork")
SKEETO_UI(MainUiCareerWeapons, L"Weapons")
SKEETO_UI(MainUiCareerSurvivors, L"Survivors")
SKEETO_UI(MainUiCareerModes, L"Game modes")
SKEETO_UI(MainUiCareerSourceHint, L"Same Steam statistics as the in-game archive; unavailable entries stay --.")
SKEETO_UI(MainUiCareerRefresh, L"Refresh statistics")

SKEETO_UI(MainUiCvarName, L"Variable name")
SKEETO_UI(MainUiCvarMin, L"Minimum")
SKEETO_UI(MainUiCvarMax, L"Maximum")
SKEETO_UI(MainUiCvarStep, L"Step")

SKEETO_UI(MainUiSettingsAppearance, L"Appearance")
SKEETO_UI(MainUiSettingsDisplay, L"Display")
SKEETO_UI(MainUiSettingsKeys, L"Shortcuts")

SKEETO_UI(MainUiChooseCategory, L"Choose a category on the left, then an action.")

SKEETO_UI(MainUiTextScaleOn, L"Resolution-scaled text: On")
SKEETO_UI(MainUiTextScaleOff, L"Resolution-scaled text: Off")

SKEETO_UI(MainUiCreateLobby, L"Quick lobby creation")
SKEETO_UI(MainUiMutationSpecialDelivery, L"Special Delivery")
SKEETO_UI(MainUiLogoOn, L"Top-left Skeeto logo: Show")
SKEETO_UI(MainUiLogoOff, L"Top-left Skeeto logo: Hide")
SKEETO_UI(MainUiAccentLineOn, L"Accent line below logo: Show")
SKEETO_UI(MainUiAccentLineOff, L"Accent line below logo: Hide")

SKEETO_UI(MainUiOfficialMaps, L"Official campaigns")

SKEETO_UI(MainUiParamQuick, L"Mode: Quick match")
SKEETO_UI(MainUiParamCustom, L"Mode: Custom match")
SKEETO_UI(MainUiParamFriends, L"Mode: Friends game")
SKEETO_UI(MainUiParamGroup, L"Mode: Steam group servers")
SKEETO_UI(MainUiParamSolo, L"Mode: Solo map selection")
SKEETO_UI(MainUiParamLobby, L"Create lobby (preset parameters)")
SKEETO_UI(MainUiParamPage, L"Game settings/content page (page code)")
SKEETO_UI(MainUiParamMap, L"Start map / mode / difficulty")
SKEETO_UI(MainUiParamSet, L"ConVar: Set value")
SKEETO_UI(MainUiParamToggle, L"ConVar: Toggle two values")
SKEETO_UI(MainUiParamCycle, L"ConVar: Cycle values")
SKEETO_UI(MainUiParamDemo, L"Play demo")
SKEETO_UI(MainUiParamTeam, L"Mode: Team friends game")
SKEETO_UI(MainUiParamNative, L"Game menu entry (entry code)")
SKEETO_UI(MainUiParamModeHint, L"Mode ID: coop / realism / versus / survival / scavenge; mutations: community1 / mutation4 / mutation17. Team example: teamversus.")
SKEETO_UI(MainUiParamPageHint, L"Native page: Video / Audio / KeyboardMouse / MultiplayerSettings / Addons / StatsAndAchievements / CloudSettings / Credits. See tutorial.")
SKEETO_UI(MainUiParamMapHint, L"Map mode [difficulty], e.g. c1m1_hotel coop Hard. Optional difficulty: Easy / Normal / Hard / Impossible.")
SKEETO_UI(MainUiParamCvarHint, L"Set: volume 0.5; toggle: cl_showfps 0 1; cycle: volume 0 0.25 0.5 1. Game permissions apply.")
SKEETO_UI(MainUiParamDemoHint, L"Demo filename, e.g. my_match; relative paths accepted. Do not enter playdemo. The file must exist in game search paths.")
SKEETO_UI(MainUiParamNativeHint, L"Supported native action, e.g. SoloPlay_mutation17 / FriendsMatch_coop / Video. Not a console command or function address. See tutorial.")

SKEETO_UI(MainUiSystemInput, L"Skeeto text input (IME supported)")

SKEETO_UI(MainUiRelated, L"Skeeto links")
SKEETO_UI(MainUiAutoexec, L"Open autoexec.cfg")
SKEETO_UI(MainUiWorkshopL4d2, L"Workshop")
SKEETO_UI(MainUiExactRgb, L"Exact RGB")
SKEETO_UI(MainUiRgbHint, L"Enter R G B, each 0-255, e.g. 105 222 235.")
SKEETO_UI(MainUiPathHint, L"Enter a game-root relative or absolute local path, e.g. left4dead2\\cfg\\autoexec.cfg. Spaces allowed; no quotes.")
SKEETO_UI(MainUiSteamHint, L"Enter a steam:// URI, e.g. steam://openurl/https://steamcommunity.com/app/550/workshop/.")
SKEETO_UI(MainUiWebHint, L"Enter a complete http:// or https:// URL.")
SKEETO_UI(MainUiProfileNameHint, L"Managed profile filename, e.g. night.hud, under skeeto/mainmenu/profiles. Load reads; save replaces that file.")
SKEETO_UI(MainUiGameVolume, L"Game volume")
SKEETO_UI(MainUiMusicVolume, L"Music volume")
SKEETO_UI(MainUiExportButton, L"Export main menu")
SKEETO_UI(MainUiImportButton, L"Import main menu")
SKEETO_UI(MainUiManageProfilesAction, L"Manage menu profiles (open page)")

SKEETO_UI(MainUiElementWelcome, L"Welcome title (text element)")
SKEETO_UI(MainUiElementSubtitle, L"Welcome subtitle (text element)")
SKEETO_UI(MainUiElementLogo, L"Skeeto logo (text element)")
SKEETO_UI(MainUiElementVersion, L"Version (text element)")
SKEETO_UI(MainUiElementGame, L"Game name (text element)")
SKEETO_UI(MainUiElementHint, L"Controls hint (text element)")
SKEETO_UI(MainUiElementNotice, L"Status notice")
SKEETO_UI(MainUiElementFriends, L"Friends playing L4D2")
SKEETO_UI(MainUiGroupElements, L"Text and friends components")
SKEETO_UI(MainUiExactFont, L"Exact font size")
SKEETO_UI(MainUiFontHint, L"Reference font size: 8-96. Scales with the layout when resolution scaling is enabled.")
SKEETO_UI(MainUiFriendUnknown, L"Steam friend")
SKEETO_UI(MainUiFriendsEmpty, L"No friends currently playing L4D2")
SKEETO_UI(MainUiFriendsUnavailable, L"Steam friends are currently unavailable")

SKEETO_UI(MainUiParamLobbyHint, L"Mode [campaign ID chapter difficulty access server network]; e.g. coop L4D2C1 1 Hard friends official LIVE. See the tutorial.")

SKEETO_UI(MainUiBackgroundView, L"Enjoy background (Esc returns)")

SKEETO_UI(MainUiBackdropFollow, L"Panel color: follow button")
SKEETO_UI(MainUiBackdropUniform, L"Panel color: uniform")
SKEETO_UI(MainUiBackdropColor, L"Uniform color: %d %d %d (cycle)")

SKEETO_UI(MainUiFixedSetup, L"[5] Fixed F3/F4/F5 shortcuts %s")
SKEETO_UI(MainUiFixedOn, L"Fixed F3/F4/F5 shortcuts: ON")
SKEETO_UI(MainUiFixedOff, L"Fixed F3/F4/F5 shortcuts: OFF")

SKEETO_UI(OverlayAmmoSide, L"[6] Arc orientation: %s")

SKEETO_UI(LastMusicNoticeRow, L"[7] Will we win? %s (5 sec)")
SKEETO_UI(LastMusicNoticeSize, L"[8] Notice size [%d%%] - / + to adjust")
SKEETO_UI(LastMusicNoticePos, L"[9] Notice position: arrow keys; Shift faster; press 9 again to reset")
SKEETO_UI(LastMusicNoticeText, L"Will we win?")

SKEETO_UI(TankTargetHudEntryName, L"Tank focus target HUD")
SKEETO_UI(TankTargetHudEntryDesc, L"    Shows the survivor currently targeted by the Tank. Your own name turns red when the Tank targets you. Default off.")
SKEETO_UI(TankTargetHudTitle, L"Tank aggro target")
SKEETO_UI(TankTargetHudRow, L"Tank  >  %s")
SKEETO_UI(TankTargetHudPreviewName, L"Nick")
SKEETO_UI(TankTargetHudFallbackName, L"Survivor")
SKEETO_UI(TankTargetHudEnabled, L"[1] Tank aggro target HUD %s")
SKEETO_UI(TankTargetHudSize, L"[2] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(TankTargetHudReset, L"[3] Reset position")
SKEETO_UI(TankTargetHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther. It samples client player slots every 250 ms only while enabled.")
SKEETO_UI(TankTargetHudPageHint, L"In-game testing confirmed this is the Tank's current aggro target. Your own name turns red when targeted.")

SKEETO_UI(DamageSourceHudEntryName, L"Damage source HUD")
SKEETO_UI(DamageSourceHudEntryDesc, L"    Shows who damaged you and how much. Defaults to teammates only; can include all sources. Default off.")
SKEETO_UI(DamageSourceHudTitle, L"Damage source")
SKEETO_UI(DamageSourceHudRow, L"%s  -%d HP")
SKEETO_UI(DamageSourceHudPreviewName, L"Coach")
SKEETO_UI(DamageSourceHudEnvironment, L"World / unknown")
SKEETO_UI(DamageSourceHudCommon, L"Common infected")
SKEETO_UI(DamageSourceHudEnabled, L"[1] Damage source HUD %s")
SKEETO_UI(DamageSourceHudMode, L"[2] Source filter [%s]")
SKEETO_UI(DamageSourceHudTeammates, L"Teammates only")
SKEETO_UI(DamageSourceHudAll, L"All sources")
SKEETO_UI(DamageSourceHudSize, L"[3] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(DamageSourceHudReset, L"[4] Reset position (top right)")
SKEETO_UI(DamageSourceHudPositionHint, L"Arrow keys move anywhere on screen; hold Shift to move farther.")
SKEETO_UI(DamageSourceHudPageHint, L"Uses the client-received player_hurt_concise event to show the attacker and health lost.")

SKEETO_UI(FriendlyFireHudEntryName, L"Friendly-fire damage tip HUD")
SKEETO_UI(FriendlyFireHudEntryDesc, L"    Shows the teammate and health lost when you hit them. Repeated hits briefly combine. Default off.")
SKEETO_UI(FriendlyFireHudTitle, L"Friendly-fire damage tip")
SKEETO_UI(FriendlyFireHudRow, L"You hit %s for %d HP")
SKEETO_UI(FriendlyFireHudPreviewName, L"Coach")
SKEETO_UI(FriendlyFireHudEnabled, L"[1] Friendly-fire damage tip HUD %s")
SKEETO_UI(FriendlyFireHudSize, L"[2] Size [%d%%]  - / +")
SKEETO_UI(FriendlyFireHudReset, L"[3] Reset position")
SKEETO_UI(FriendlyFireHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther.")
SKEETO_UI(FriendlyFireHudPageHint, L"Uses the client-received player_hurt_concise event when you damage a teammate.")

SKEETO_UI(WorldFriendlyFireDamageNum, L"Teammate FF damage numbers")
SKEETO_UI(WorldFriendlyFireDamageNumDesc, L"    When you hit a teammate, show a large yellow/red damage number above them. 15 damage or more is heavy. Default off.")
SKEETO_UI(ChapStatsColFfDamageTaken, L"FF received")

SKEETO_UI(WorldFriendlyFireLabel, L"Friendly fire")
SKEETO_UI(WorldFriendlyFireOption, L"Teammate friendly-fire warning")
SKEETO_UI(WorldFriendlyFireOptionDesc, L"    When a teammate damages you, the existing red warning appears above the attacker for about 2 seconds.")

SKEETO_UI(ChapStatsColFfDamage, L"FF dealt")
SKEETO_UI(ChapStatsScoreReasonFriendlyFire, L"Friendly-fire damage penalty")

SKEETO_UI(SiAggroHudEntryName, L"SI hate display")
SKEETO_UI(SiAggroHudEntryDesc, L"    Lists special infected currently locked onto you (Smoker, Hunter, ...). Turns red while any is targeting you. Default off.")
SKEETO_UI(SiAggroHudTitle, L"SI hate")
SKEETO_UI(SiAggroHudRowLabel, L"%s  (%s)")
SKEETO_UI(SiAggroHudPreviewLabel, L"Nick")
SKEETO_UI(SiAggroHudEnabled, L"[1] SI hate display %s")
SKEETO_UI(SiAggroHudSize, L"[2] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(SiAggroHudReset, L"[3] Reset position")
SKEETO_UI(SiAggroHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther. It samples client player slots every 250 ms only while enabled.")
SKEETO_UI(SiAggroHudPageHint, L"Shows special infected (not Tank) whose current look-at target is you. Uses the same client field as the Tank target HUD.")

SKEETO_UI(WitchRageHudEntryName, L"Witch startle HUD")
SKEETO_UI(WitchRageHudEntryDesc, L"    Shows how close a Witch is to being startled, with an optional thin bar for wandering rage. Hides after about 2 seconds without change. Default off.")
SKEETO_UI(WitchRageHudTitle, L"Witch startle")
SKEETO_UI(WitchRageHudRow, L"Witch  %d%%")
SKEETO_UI(WitchRageHudEnabled, L"[1] Witch startle HUD %s")
SKEETO_UI(WitchRageHudWander, L"[2] Wander rage bar %s")
SKEETO_UI(WitchRageHudSize, L"[3] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(WitchRageHudReset, L"[4] Reset position")
SKEETO_UI(WitchRageHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther. While enabled it checks known Witches and at most 48 entities every 250 ms.")
SKEETO_UI(WitchRageHudPageHint, L"The main bar is Witch startle rage; the thin blue bar is wander rage. Shown when rage is above zero or changing.")

SKEETO_UI(BossDmgBoardEntryName, L"Boss damage ranking HUD")
SKEETO_UI(BossDmgBoardEntryDesc, L"    After a Tank or Witch dies, shows each survivor's damage to that fight for 10 seconds (top 8). Default off.")
SKEETO_UI(BossDmgBoardTitleTank, L"Tank damage")
SKEETO_UI(BossDmgBoardTitleWitch, L"Witch damage")
SKEETO_UI(BossDmgBoardEnabled, L"[1] Boss damage ranking HUD %s")
SKEETO_UI(BossDmgBoardSize, L"[2] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(BossDmgBoardReset, L"[3] Reset position")
SKEETO_UI(BossDmgBoardPositionHint, L"Arrow keys move this panel; hold Shift to move farther. It reads survivors' checkpoint damage only when a Tank or Witch dies.")
SKEETO_UI(BossDmgBoardPageHint, L"Damage is the change in the checkpoint Tank/Witch damage totals since the previous boss kill or round start.")

SKEETO_UI(IgniteHudEntryName, L"Boss ignite toast HUD")
SKEETO_UI(IgniteHudEntryDesc, L"    Shows a short toast naming who set a Witch or Tank on fire. Default off.")
SKEETO_UI(IgniteHudUnknownSource, L"Fire")
SKEETO_UI(IgniteHudPreviewName, L"Nick")
SKEETO_UI(IgniteHudVerb, L" ignited ")
SKEETO_UI(IgniteHudEnabled, L"[1] Boss ignite toast HUD %s")
SKEETO_UI(IgniteHudSize, L"[2] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(IgniteHudReset, L"[3] Reset position")
SKEETO_UI(IgniteHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther.")
SKEETO_UI(IgniteHudPageHint, L"Uses the zombie_ignited game event when the victim is a Witch or Tank. The toast lasts about 3 seconds.")

SKEETO_UI(WitchHpHudEntryName, L"Witch HP HUD")
SKEETO_UI(WitchHpHudEntryDesc, L"    Shows an estimated Witch HP bar when she takes damage (uses survivor checkpoint Witch damage; Witch entities have no client m_iHealth). Hides after about 2 seconds without change. Default off.")
SKEETO_UI(WitchHpHudTitle, L"Witch HP")
SKEETO_UI(WitchHpHudEnabled, L"[1] Witch HP HUD %s")
SKEETO_UI(WitchHpHudSize, L"[2] Size [%d%%]  - / +; Shift changes font [%d]")
SKEETO_UI(WitchHpHudReset, L"[3] Reset position")
SKEETO_UI(WitchHpHudPositionHint, L"Arrow keys move this panel; hold Shift to move farther. While enabled it reads survivor checkpoint Witch damage every 250 ms.")
SKEETO_UI(WitchHpHudPageHint, L"Screen HUD only; does not follow the Witch model. Client has no Witch entity health; estimated from checkpoint damage when she is hurt.")

SKEETO_UI(CrashGuardHomeNav, L"Left: previous page    Right: next page")
SKEETO_UI(CrashGuardTitle, L"Crash prevention patches")
SKEETO_UI(CrashGuardPlatformHint, L"Skeeto integrates some commonly used L4D2 crash patches. If another platform already provides a corresponding feature, please leave that option disabled here.")
SKEETO_UI(CrashGuardCacheRow, L"[1] Datacache limit [%s]")
SKEETO_UI(CrashGuardIndexRow, L"[2] index_buffer_size [%s]")
SKEETO_UI(CrashGuardCachePlatformHint, L"Similar to the Datacache limit unlock in Left4Neko.")
SKEETO_UI(CrashGuardCacheDetails, L"Default: No change. Restart after changing. Check the limit with the memory HUD; no memory is preallocated.")
SKEETO_UI(CrashGuardIndexPlatformHint, L"Similar to the vertex limit unlock in Left4Neko/Necola/L4D2Fix.")
SKEETO_UI(CrashGuardIndexDetectHint, L"Live capacity check: skeeto_crashguard_status")
SKEETO_UI(CrashGuardIndexDetails, L"Default: No change. Restart after changing. 65535 aligns to 65536; lower the preset if visual artifacts occur.")
SKEETO_UI(CrashGuardCacheState, L"Datacache this launch: %s")
SKEETO_UI(CrashGuardIndexState, L"Index buffer this launch: %s")
SKEETO_UI(CrashGuardNoChange, L"No change")
SKEETO_UI(CrashGuardPending, L"Waiting for startup application")
SKEETO_UI(CrashGuardArmed, L"Waiting for native initialization")
SKEETO_UI(CrashGuardApplied, L"Applied")
SKEETO_UI(CrashGuardUnsupported, L"Game DLL version mismatch; unchanged")
SKEETO_UI(CrashGuardConflict, L"Original state mismatch; skipped")
SKEETO_UI(CrashGuardLate, L"Buffer already initialized; skipped")
SKEETO_UI(CrashGuardFailed, L"Application or verification failed")
SKEETO_UI(CrashGuardProtectionWarningFormat, L"%s (failed to restore original memory page protection; restart the game)")
SKEETO_UI(CrashGuardReportCache, L"Datacache: saved=%d MB, launch=%d MB, state=%s (0=No change)")
SKEETO_UI(CrashGuardReportCacheLive, L"Datacache live usage=%.1f MB, live limit=%.1f MB")
SKEETO_UI(CrashGuardCacheUnavailable, L"Datacache: interface unavailable; live limit cannot be verified.")
SKEETO_UI(CrashGuardReportIndex, L"index_buffer_size: saved=%d, launch=%d, state=%s (0=No change)")
SKEETO_UI(CrashGuardIndexUnavailable, L"Index buffer: module missing or version mismatch; cannot verify.")
SKEETO_UI(CrashGuardReportIndexLive, L"Live index capacity=%u, embedded buffer capacity=%u, render query limit=%u (0=not created yet)")
SKEETO_UI(CrashGuardIndexVerifyPending, L"Index capacity check: runtime buffer not created yet; cannot confirm.")
SKEETO_UI(CrashGuardIndexVerifyPass, L"Index capacity check: all three runtime capacities match the preset for this launch.")
SKEETO_UI(CrashGuardIndexVerifyMismatch, L"Index capacity check: runtime capacities differ from the preset for this launch; check the state above.")
SKEETO_UI(CrashGuardReportIndexExplain, L"The engine aligns 65535 to 65536. These capacities come from runtime objects, not the ini values; they do not prove all crashes are fixed.")
SKEETO_UI(CacheOffloadEntry, L"[3] Cache offload [Settings]")
SKEETO_UI(CacheOffloadTitle, L"Cache offload")
SKEETO_UI(CacheOffloadToggle, L"[1] Cache offload %s")
SKEETO_UI(CacheOffloadRestart, L"Default: Off. Fully exit and restart after changing the switch.")
SKEETO_UI(CacheOffloadDescription, L"Offloads some idle model vertices, large animation blocks and complete sounds; restores them on access. Small animation pools and locked data stay resident. Hundreds of MB of savings are not guaranteed.")
SKEETO_UI(CacheOffloadStateRow, L"This launch: %s")
SKEETO_UI(CacheOffloadLiveRow, L"Currently offloaded %.2f MB / %u items; restored %u times")
SKEETO_UI(CacheOffloadReclaim, L"[2] Reclaim offloaded cache")
SKEETO_UI(CacheOffloadReclaimBusy, L"[2] Reclaim offloaded cache [In progress]")
SKEETO_UI(CacheOffloadReclaimHint, L"Reclaims this module's idle offloaded cache in batches; the game reloads it on access.")
SKEETO_UI(CacheOffloadStatusAction, L"[3] Print cache status to console")
SKEETO_UI(CacheOffloadStatusHint, L"skeeto_cache_status shows each source's binding, resident cache, locks and restore counts.")
SKEETO_UI(CacheOffloadOff, L"Off")
SKEETO_UI(CacheOffloadStarting, L"Waiting for cache interfaces")
SKEETO_UI(CacheOffloadReady, L"Enabled")
SKEETO_UI(CacheOffloadUnsupported, L"Game DLL version mismatch; disabled")
SKEETO_UI(CacheOffloadConflict, L"Cache interfaces modified; offloading stopped")
SKEETO_UI(CacheOffloadHostFailed, L"Helper unavailable; offloading stopped")
SKEETO_UI(CacheOffloadFailed, L"Initialization failed; disabled")
SKEETO_UI(CacheOffloadReportLaunch, L"Cache offload: saved=%d, launch=%d, state=%s")
SKEETO_UI(CacheOffloadReportBytes, L"Currently offloaded=%u items / %.2f MB; model=%.2f MB, animation=%.2f MB, sound=%.2f MB")
SKEETO_UI(CacheOffloadReportHost, L"Helper PID=%u, live mapped=%.1f MB, pending offload=%u items / %.1f MB")
SKEETO_UI(CacheOffloadReportCounts, L"Restores=%u, failures=%u, retained unprotected/unobserved=%u items, reclaimed=%.2f MB")
SKEETO_UI(CacheOffloadMeasureHint, L"Current offload is data not yet restored. Cumulative VA releases count independent pages actually returned during offload, including repeats, not net AV savings. Reclaim discards offloaded items and is counted separately. Other locks, naked pointers, old animation slabs and unsupported assets remain resident.")
SKEETO_UI(CacheOffloadReportHistory, L"This launch: cumulative offload=%.2f MB / %u moves, peak offload=%.2f MB")
SKEETO_UI(CacheOffloadModel, L"Model")
SKEETO_UI(CacheOffloadAnimation, L"Animation")
SKEETO_UI(CacheOffloadSound, L"Sound")
SKEETO_UI(CacheOffloadReportSource, L"%s: bound=%d, resident=%.2f MB, sampled locks=%.2f MB / %u items, tracked=%u items, retained unprotected/unobserved=%u items")
SKEETO_UI(CacheOffloadReportRetained, L"%s: recently used=%.2f MB, retained unprotected/unobserved=%.2f MB, old animation slab=%.2f MB; native cumulative lock fields=%u bytes / %u items")
SKEETO_UI(CacheOffloadReportAudio, L"Sound DLL verified=%d, releasable animation pages=%d; current idle threshold=%u seconds, allowing only verified model-owned cached pins to be released")
SKEETO_UI(CacheOffloadReportSchedule, L"Offload: up to 16 in-flight items, total 512 KB copied per maintenance pass, 0.5 ms budget (individual system operations can exceed it); idle threshold is 3 seconds for the first 30 seconds in a map, then 10 seconds. Map warmup, pending copies and explicit reclamation advance each Paint; other scans run at most 20 Hz. The host wakes on events.")
SKEETO_UI(CacheOffloadReportAudioHistory, L"Sound WaveData: peak offload=%.2f MB, cumulative offload=%.2f MB, cumulative restore=%.2f MB, metadata queries=%u (while offloaded=%u); this section only, not all sound memory.")
SKEETO_UI(CacheOffloadReportVa, L"Independent buffers VA=%.2f MB, cumulative VA actually released on offload=%.2f MB, allocation/release failures=%u; repeats included, not net AV gains or native heap releases.")
SKEETO_UI(CacheOffloadReportMeshPin, L"Model-owned cached pins released=%u, frame-end commits=%u, frame-end commit peak=%.3f ms; other locks and naked pointers remain protected.")
SKEETO_UI(CacheOffloadReportInventory, L"Lock sample valid=%d, scan cycle=%u ms across frames, not CPU time; items are sampled separately. Unprotected includes naked pointers and unobserved pre-install items. Reasons can overlap.")
SKEETO_UI(CacheOffloadReportTiming, L"HUD timing: recent maintenance avg/peak=%.3f/%.3f ms, observed restore peak this launch=%.3f ms")
SKEETO_UI(CacheOffloadTimingHint, L"Maintenance covers this module's Paint work; restore includes mapping, allocation and copying, not full frames, other hooks or helper cost. Timing stops when the HUD is off or hidden.")
SKEETO_UI(CacheHudDemo, L"DEMO")
SKEETO_UI(CacheHudRestart, L"RESTART")
SKEETO_UI(CacheHudReclaiming, L"RECLAIMING")
SKEETO_UI(CacheHudRunning, L"RUNNING")
SKEETO_UI(CacheHudWaiting, L"WAITING")
SKEETO_UI(CacheHudUnsupported, L"UNSUPPORTED")
SKEETO_UI(CacheHudConflict, L"MODIFIED")
SKEETO_UI(CacheHudHostFailed, L"HOST OFFLINE")
SKEETO_UI(CacheHudUnavailable, L"UNAVAILABLE")
SKEETO_UI(CacheHudCurrent, L"Current offload")
SKEETO_UI(CacheHudPeakMoved, L"Peak / cumulative")
SKEETO_UI(CacheHudKinds, L"Model / anim / sound")
SKEETO_UI(CacheHudSourceLocked, L"Resident / sampled locks")
SKEETO_UI(CacheHudCounts, L"Restore / fail / kept")
SKEETO_UI(CacheHudAvReclaimed, L"Free AV / VA released total")
SKEETO_UI(CacheHudTiming, L"Maint avg / peak / restore peak")
SKEETO_UI(CacheHudCurrentValue, L"%.2f MB / %u items")
SKEETO_UI(CacheHudToggle, L"[4] Cache offload HUD %s")
SKEETO_UI(CacheHudToggleHint, L"Default: On; requires the cache-offload master switch. Current offload is not net AV savings. The settings page shows data labeled DEMO.")
SKEETO_UI(CacheHudSize, L"[5] Scale [%d%%]  `-`/`=`  Font [%d]  Shift+`-`/`=`")
SKEETO_UI(CacheHudSizeHint, L"Arrow keys move the HUD; Shift moves faster. Maintenance avg/peak cover the recent window; restore peak is the maximum observed this launch. Only this module's timing scope is measured.")
SKEETO_UI(CacheHudReset, L"[6] Reset HUD position (top right)")
SKEETO_UI(CrashGuardOffloadPlatformHint, L"Similar to the cache reclamation feature in L4D2Evolved.")
SKEETO_UI(CrashGuardNoDecalsRow, L"[4] Disable decals %s")
SKEETO_UI(CrashGuardClearDecalsEntry, L"[7] Auto-clear bullet holes & blood [Open settings] %s")
SKEETO_UI(CrashGuardClearDecalsEntryHint, L"    Open the sub-page to toggle timed clearing and set the interval.")
SKEETO_UI(CrashGuardClearDecalsTitle, L"Auto-clear bullet holes & blood")
SKEETO_UI(CrashGuardAutoClearDecalsRow, L"[1] Auto-clear bullet holes & blood %s")
SKEETO_UI(CrashGuardAutoClearDecalsHint, L"    Periodically runs r_cleardecals to wipe world bullet holes and blood. May flash briefly. Independent from Disable decals.")
SKEETO_UI(CrashGuardClearDecalsIntervalRow, L"[2] Clear interval [%d sec]")
SKEETO_UI(CrashGuardClearDecalsIntervalHint, L"    Cycle 30 / 60 / 90 / 120 seconds. Default 60.")
SKEETO_UI(RadarRootEntry, L"[1] Supply radar")
SKEETO_UI(RadarHomeAudio, L"[2] Audio volume mixer")
SKEETO_UI(RadarHomeClient, L"[3] Combat, audio and visuals")
SKEETO_UI(RadarHomeConvenience, L"[4] Fixes and conveniences")
SKEETO_UI(RadarHomeCrash, L"[5] Crash prevention")
SKEETO_UI(RadarHomeLocal, L"[6] Local server improvements")
SKEETO_UI(RadarHomeCareer, L"[7] L4D2 career archive")
SKEETO_UI(RadarHomeStyle, L"[8] Menu appearance")
SKEETO_UI(RadarHomeChanges, L"[9] Open Skeeto changelog")
SKEETO_UI(RadarHomeHelp, L"[1] Help and debugging")
SKEETO_UI(RadarTitle, L"Supply radar")
SKEETO_UI(RadarItemsTitle, L"Supply radar - item filters")
SKEETO_UI(RadarLayoutTitle, L"Supply radar - item layouts")
SKEETO_UI(RadarLabel0, L"Supply radar")
SKEETO_UI(RadarLabel1, L"Detection range (m)")
SKEETO_UI(RadarLabel2, L"Show quantity")
SKEETO_UI(RadarLabel3, L"Overall scale (%)")
SKEETO_UI(RadarLabel4, L"Icon opacity (%)")
SKEETO_UI(RadarLabel5, L"Show direction arrow")
SKEETO_UI(RadarLabel6, L"Show height difference")
SKEETO_UI(RadarLabel7, L"Show while spectating")
SKEETO_UI(RadarLabel8, L"Special map supply compatibility")
SKEETO_UI(RadarLabel9, L"Digit font (0 classic, 1 heart, 2 cat)")
SKEETO_UI(RadarLabel10, L"Weapon row limit")
SKEETO_UI(RadarLabel11, L"Weapon columns")
SKEETO_UI(RadarLabel12, L"Medical category")
SKEETO_UI(RadarLabel13, L"Throwable category")
SKEETO_UI(RadarLabel14, L"Weapon category")
SKEETO_UI(RadarAdjustRow, L"[%d] %s [%d]  - / + adjust")
SKEETO_UI(RadarFilterEntry, L"[9] Item filters [Settings]")
SKEETO_UI(RadarLayoutEntry, L"[8] Edit each item layout [Settings]")
SKEETO_UI(RadarLayoutRow, L"[%d] %s [Settings]")
SKEETO_UI(RadarUnsupportedRow, L"[%d] %s [Unsupported]")
SKEETO_UI(RadarResetLayout, L"[9] Restore default layouts")
SKEETO_UI(RadarResetSettings, L"[1] Restore all defaults (turns radar off)")
SKEETO_UI(RadarStatusAction, L"[2] Print status to console")
SKEETO_UI(RadarCredit, L"Source code by Ragdoll. Skeeto handles integration and optimization only.")
SKEETO_UI(RadarLayoutEmpty, L"No enabled items. Enable a category and its item filters first.")
SKEETO_UI(RadarPageHint, L"Numbers select; - / + adjust values; arrows change pages. Independent switch; follows Skeeto custom HUD visibility.")
SKEETO_UI(RadarConsoleStatus, L"[Skeeto] Supply radar: %s; range=%dm; status=%s")
SKEETO_UI(RadarConsoleOn, L"ON")
SKEETO_UI(RadarConsoleOff, L"OFF")
SKEETO_UI(RadarItem1, L"First aid kit")
SKEETO_UI(RadarItem2, L"Defibrillator")
SKEETO_UI(RadarItem3, L"Pain pills")
SKEETO_UI(RadarItem4, L"Adrenaline")
SKEETO_UI(RadarItem5, L"Molotov")
SKEETO_UI(RadarItem6, L"Pipe bomb")
SKEETO_UI(RadarItem7, L"Bile jar")
SKEETO_UI(RadarItem8, L"Ammo pile")
SKEETO_UI(RadarItem9, L"Laser sight")
SKEETO_UI(RadarItem10, L"Unknown weapon")
SKEETO_UI(RadarItem11, L"AK-47")
SKEETO_UI(RadarItem12, L"M16")
SKEETO_UI(RadarItem13, L"Military sniper")
SKEETO_UI(RadarItem14, L"Hunting rifle")
SKEETO_UI(RadarItem15, L"MAC-10")
SKEETO_UI(RadarItem16, L"UZI")
SKEETO_UI(RadarItem17, L"AWP")
SKEETO_UI(RadarItem18, L"Scout")
SKEETO_UI(RadarItem19, L"SCAR")
SKEETO_UI(RadarItem20, L"Grenade launcher")
SKEETO_UI(RadarItem21, L"M60")
SKEETO_UI(RadarItem22, L"Pistol")
SKEETO_UI(RadarItem23, L"Magnum")
SKEETO_UI(RadarItem24, L"Pump shotgun")
SKEETO_UI(RadarItem25, L"Chrome shotgun")
SKEETO_UI(RadarItem26, L"Auto shotgun")
SKEETO_UI(RadarItem27, L"SPAS")
SKEETO_UI(RadarItem28, L"MP5")
SKEETO_UI(RadarItem29, L"SG552")
SKEETO_UI(RadarItem30, L"Melee")
SKEETO_UI(RadarItem31, L"Chainsaw")
SKEETO_UI(RadarItem32, L"Gas can")
SKEETO_UI(RadarItem33, L"Fireworks crate")
SKEETO_UI(RadarItem34, L"End safehouse")
SKEETO_UI(RadarEditTitle, L"Layout: %s  %d%%")
SKEETO_UI(RadarEditHint1, L"Arrows: move and snap   Shift: faster   Ctrl: no snap   +/-: resize")
SKEETO_UI(RadarEditHint2, L"Applies and saves instantly   Enter/Esc: exit   R: reset   Tab: next")
SKEETO_UI(RadarStatusOff, L"Disabled")
SKEETO_UI(RadarStatusWaiting, L"Waiting for a valid player")
SKEETO_UI(RadarStatusNoItems, L"No nearby supplies")
SKEETO_UI(RadarStatusScanning, L"Scanning supplies")
SKEETO_UI(RadarStatusReady, L"Ready")
SKEETO_UI(RadarStatusUnsupported, L"Unsupported game build")

SKEETO_UI(PauseSamplingName, L"Pause all HUDs and data sampling")
SKEETO_UI(PauseSamplingDesc, L"    Custom HUDs, world HUDs, stats and hit feedback all pause; menu and crosshair stay.")
SKEETO_UI(HudPausedSuffix, L"(paused)")
SKEETO_UI(BootstrapUnsupportedLayout, L"This update needs manual action or a newer update protocol. Installed files were not changed. Keep your settings and custom files, read the staged package instructions, and use its maintenance entry if needed.")
SKEETO_UI(BootstrapUpdateReady, L"Update validated and staged.\nExit the game completely; installation finishes automatically after exit. Failures keep the stage and retry on the next exit.")
SKEETO_UI(BootstrapApplyFailed, L"Automatic install after exit did not finish (often a locked critical file, quiet-wait timeout, or verification failure).\nThe stage was kept and will retry after a full exit; the game is never force-killed.\nOr close the game and run Setup_安装 维护 卸载.bat from the package, or follow the Workshop/desktop package manual steps.")
SKEETO_UI(BootstrapMigrationNotice, L"Skeeto Steam migration is incomplete. The legacy entry remains available. Exit the game and extract the complete package into the game root. Afterwards launch through Steam. Move old ini cmdline options to Steam launch options yourself. Settings and custom resources are preserved.")
SKEETO_UI(BootstrapConfirmUpdate, L"A Skeeto Workshop update is available (%hs -> %hs). Install it?\n\nThe updater shipped in the new package validates and installs it. Manual migration is reported explicitly; your current installation remains intact.")

SKEETO_UI(IdlePersonRestoreName, L"[9] Restore camera mode after idle takeover %s")
SKEETO_UI(IdlePersonRestoreHint, L"    Restores your previous first/third-person mode once after taking back your character. Does not change the idle spectator camera.")
SKEETO_UI(WModelScaleEntry, L"[1] World-model scale [Open settings]")
SKEETO_UI(WModelScaleTitle, L"World-model scale")
SKEETO_UI(WModelScaleHint, L"    Scales special and common infected world models only. No survivors, no collision changes. Separate from first-person viewmodel scale. 1.00 is stock size.")
SKEETO_UI(WModelScaleHostHint, L"    AI specials/commons/witch: uniform bone scale in BuildTransformations with the field held at 1.0. Player specials: client m_flModelScale + RecvProxy. Never touches viewmodels, survivors, or collision hulls.")
SKEETO_UI(WModelScaleToggle, L"[1] World-model scale %s")
SKEETO_UI(WModelScaleSi, L"[2] Special infected scale %.2f  -/+ adjust")
SKEETO_UI(WModelScaleCommon, L"[3] Common infected scale %.2f  -/+ adjust")
SKEETO_UI(ProgressMenuName, L"[6] Campaign Completion Display [Settings]")
SKEETO_UI(ProgressMenuDesc, L"    Estimates your personal reference-route progress; each 25 / 50 / 75 / 90% notice lasts 5 seconds and plays the native notification sound.")
SKEETO_UI(ProgressMenuTitle, L"Campaign Completion Display  v%s")
SKEETO_UI(ProgressMenuToggle, L"[1] Campaign Completion Display %s")
SKEETO_UI(ProgressMenuHint, L"    Each 25 / 50 / 75 / 90% threshold appears once per round for 5 seconds with the native sound. Estimated from your route; enabling mid-chapter starts there. Reaching the finale target counts as route completion.")
SKEETO_UI(ProgressMoveHint, L"    Arrow keys move the HUD; Shift takes larger steps. A 50% demo is shown here.")
SKEETO_UI(ProgressResetPosition, L"[2] Reset position")
SKEETO_UI(ProgressDiagnostic, L"[3] Print progress diagnostics to console")
SKEETO_UI(ProgressToast, L"Chapter route approx. %d%%")
SKEETO_UI(ProgressWaitingPlayer, L"Waiting for your living survivor")
SKEETO_UI(ProgressWaitingRoute, L"Waiting for an initial route distance")
SKEETO_UI(ProgressReady, L"Route estimate ready")
SKEETO_UI(BossHitTrigger, L"[4] Tank/Witch Hit trigger [%s]")
SKEETO_UI(BossHitRecentAttack, L"Recent attack + damage")
SKEETO_UI(BossHitAnyDamage, L"Any damage")
SKEETO_UI(BossHitTriggerHint, L"    Default: require recent local gunfire or melee to reduce repeated burn feedback. Any damage restores the previous behavior. Damage numbers and kill feedback are unaffected.")
SKEETO_UI(SiSoundEmptyHint, L"    An empty sound in the selected source stays silent; the other pack is not used automatically.")
SKEETO_UI(HideBackItems, L"[7] Hide items on the back [Settings]")
SKEETO_UI(HideBackItemsHint, L"Master off keeps the original display. An enabled category hides its attached models; held and ground models stay visible.")
SKEETO_UI(HideBackItemsUnavailable, L"    The addon model interface is unavailable; original models are retained.")
SKEETO_UI(HideBackTitle, L"Hide character addon models  v%s")
SKEETO_UI(HideBackMaster, L"[1] Hide back items: master %s")
SKEETO_UI(HideBackMedicine, L"[2] Hide medicine (pills / adrenaline) %s")
SKEETO_UI(HideBackMedical, L"[3] Hide medkits / defibrillators %s")
SKEETO_UI(HideBackThrowables, L"[4] Hide throwables %s")
SKEETO_UI(HideBackTongue, L"[5] Hide Smoker tongue accessory %s")
SKEETO_UI(HideBackTongueHint, L"The tongue model wrapped around a survivor after a Smoker grabs them")
SKEETO_UI(ViewModelRootEntry, L"[3] Advanced viewmodel adjustment")
SKEETO_UI(ViewModelTitle, L"Advanced viewmodel adjustment (%d/%d)")
SKEETO_UI(ViewModelEnabled, L"Viewmodel adjustment")
SKEETO_UI(ViewModelPosX, L"Right / left X")
SKEETO_UI(ViewModelPosY, L"Forward / back Y")
SKEETO_UI(ViewModelPosZ, L"Up / down Z")
SKEETO_UI(ViewModelPitch, L"Pitch")
SKEETO_UI(ViewModelYaw, L"Yaw")
SKEETO_UI(ViewModelRoll, L"Roll")
SKEETO_UI(ViewModelMoveStep, L"Move step")
SKEETO_UI(ViewModelAngleStep, L"Rotation step")
SKEETO_UI(ViewModelPivotX, L"Pivot right / left X")
SKEETO_UI(ViewModelPivotY, L"Pivot forward / back Y")
SKEETO_UI(ViewModelPivotZ, L"Pivot up / down Z")
SKEETO_UI(ViewModelScale, L"Viewmodel scale")
SKEETO_UI(ViewModelSlot, L"Edit model")
SKEETO_UI(ViewModelMainHand, L"Held model")
SKEETO_UI(ViewModelHelpingHand, L"Auxiliary model")
SKEETO_UI(ViewModelResetPosition, L"Reset current position")
SKEETO_UI(ViewModelResetRotation, L"Reset current rotation")
SKEETO_UI(ViewModelResetAll, L"Reset all changes for this model")
SKEETO_UI(ViewModelReload, L"Reload saved profiles")
SKEETO_UI(ViewModelCurrentModel, L"Currently editing:")
SKEETO_UI(ViewModelToggleFormat, L"[%d] %s %s")
SKEETO_UI(ViewModelSlotFormat, L"[%d] %s %s")
SKEETO_UI(ViewModelActionFormat, L"[%d] %s")
SKEETO_UI(ViewModelAxisFormat, L"[%d] %s%s %.4f  -/+ adjust")
SKEETO_UI(ViewModelEditHint, L"Number: select; -/+: adjust, hold to repeat; Shift: fine adjustment. X right/left, Y forward/back, Z up/down. Scale resizes the whole first-person hands and gun; 1.0 is stock size.")
SKEETO_UI(ViewModelSaveHint, L"Live preview and auto-save; no manual save needed. Profiles are per model. Disable to restore stock or L4N position. Press a selected step's number again to cycle presets. Scale does not change third-person or what others see.")
SKEETO_UI(VmStatusInitial, L"Profiles load when this page opens or adjustment is enabled.")
SKEETO_UI(VmStatusLoading, L"Loading model profiles...")
SKEETO_UI(VmStatusReady, L"Profiles auto-save per model; adjustments add to L4N.")
SKEETO_UI(VmStatusSaving, L"Saving model profiles...")
SKEETO_UI(VmStatusSaved, L"Model profiles saved.")
SKEETO_UI(VmStatusEdited, L"Edited; auto-save pending.")
SKEETO_UI(VmStatusLoadError, L"Cannot read profiles. Original file preserved; fix it and reload on the last page.")
SKEETO_UI(VmStatusSaveError, L"Auto-save failed; original file preserved. Resolve file access and edit again to retry.")
SKEETO_UI(VmStatusHookError, L"Game interface mismatch; adjustment unavailable.")
SKEETO_UI(VmStatusCapacity, L"Reached the 256 model profile limit.")
SKEETO_UI(VmStatusBusy, L"Auto-saving current edits; reload after saving completes.")
SKEETO_UI(FindAboutFeedback,L"Choose visuals and sounds for hits and kills, with separate settings for different targets.")
SKEETO_UI(FindAboutCrosshair,L"Choose static, dynamic or image crosshairs and adjust their appearance.")
SKEETO_UI(FindAboutHud,L"Choose on-screen information widgets and adjust their placement and style.")
SKEETO_UI(FindAboutOverlay,L"Show ammo, reload and firing rhythm in a separate transparent overlay.")
SKEETO_UI(FindAboutWorld,L"Show health bars, names, damage and notices at characters or world positions.")
SKEETO_UI(FindAboutInterface,L"Customize the main menu, ESC menu, chat box and console.")
SKEETO_UI(FindAboutScript,L"Provide selected notices, sounds and gameplay aids on the client.")
SKEETO_UI(FindAboutLoading,L"Configure resource loading and optional experiments; experimental options default to off.")
SKEETO_UI(FindAboutBackground,L"Choose main-menu images or videos and adjust playback and display.")
SKEETO_UI(FindAboutNavigation,L"Show the minimap, routes, safe-room direction and supply markers.")
SKEETO_UI(FindAboutRadar,L"Show nearby supplies on screen, filter categories and adjust the layout.")
SKEETO_UI(FindAboutViewmodel,L"Adjust weapon-model position, angles, pivot and mirroring per model.")
SKEETO_UI(FindAboutSound,L"Adjust gunfire, footsteps, infected and other sound categories separately.")
SKEETO_UI(FindAboutCombat,L"Configure outlines, camera behavior and combat audiovisual effects.")
SKEETO_UI(FindAboutConvenience,L"Configure idle recovery, bot names and running configs after map transitions.")
SKEETO_UI(FindAboutStability,L"Choose crash-prevention measures and read their applicability notes.")
SKEETO_UI(FindAboutNecola,L"Control whether Necola loads alongside Skeeto at game startup.")
SKEETO_UI(FindAboutLocal,L"Adjust local-server gameplay and testing options; these do not affect remote servers.")
SKEETO_UI(FindAboutCareer,L"View Steam career, weapon and game-mode statistics.")
SKEETO_UI(FindAboutMenuStyle,L"Adjust the Skeeto numbered menu's font, colors, background and size.")
SKEETO_UI(FindAboutHelp,L"Read usage instructions, the disclaimer and related help.")
SKEETO_UI(FindAboutPlugins,L"View the plugin platform switch, loaded plugins and their menu entries.")
SKEETO_UI(FindAboutChat,L"Adjust chat text, background, placement and message lifetime.")
SKEETO_UI(FindAboutConsole,L"Adjust console text, background, borders and colors.")
SKEETO_UI(LocalListenTickRow,L"[%d] Tick [%s]")
SKEETO_UI(VmNoModel, L"Enter a game and equip the weapon or item to adjust.")
SKEETO_UI(ViewModelCompatibility, L"Compatible with L4N positioning: both adjustments add together. Profiles automatically save to left4dead2/skeeto/viewmodel/profiles.ini.")
SKEETO_UI(ViewModelMirror, L"Global viewmodel mirroring")
SKEETO_UI(ViewModelMirrorError, L"Mirror interface mismatch; mirroring stopped. Position and angles remain available.")
SKEETO_UI(ViewModelMirrorConflict, L"Different per-model mirror settings with shared or unverified weapon handles pause this pair to keep culling and bones aligned.")
SKEETO_UI(ViewModelMirrorModel, L"Mirror current model")
SKEETO_UI(ViewModelMirrorModeFormat, L"Mirror control mode: %s")
SKEETO_UI(ViewModelMirrorModeGlobal, L"Global")
SKEETO_UI(ViewModelMirrorModePerModel, L"Per model")
SKEETO_UI(LaserRoundNoticeName, L"Laser round notice")
SKEETO_UI(LaserRoundNoticeDesc, L"    Shows one 10-second notice per chapter when a laser sight pickup is first confirmed. May appear later; only entities received by the client. Position and size adjustable. Default off.")
SKEETO_UI(LaserRoundNoticeText, L"Laser sights are on this chapter. Take them.")
SKEETO_UI(LaserRoundNoticeTitle, L"Laser round notice  v%s")
SKEETO_UI(LaserRoundNoticePageHint, L"Shows a 10-second notice on the first confirmed pickable laser sight this chapter. May appear later; only entities received by the client.")
SKEETO_UI(LaserRoundNoticeEnabled, L"[1] Show laser round notice %s")
SKEETO_UI(LaserRoundNoticeSize, L"[2] Overall [%d%%]  `-`/`=`; Shift for font size [%d]")
SKEETO_UI(LaserRoundNoticeReset, L"[3] Reset position (top center)")
SKEETO_UI(LaserRoundNoticePosHint, L"Arrow keys move; hold Shift for larger steps.")
SKEETO_UI(FastLoadEntry, L"[8] Engine resource loading acceleration (Beta)")
SKEETO_UI(FastLoadTitle, L"Engine resource loading acceleration (Beta)")
SKEETO_UI(FastLoadEnabled, L"[1] Master switch %s")
SKEETO_UI(FastLoadMasterHint, L"With the master ON, only enabled options take effect. Mesh and collision async default to ON; animation async defaults to OFF. The loading timer is separate and defaults to ON.")
SKEETO_UI(FastLoadPaths, L"[2] Reuse repeated path splits %s")
SKEETO_UI(FastLoadPathsHint, L"Loading repeatedly splits paths into directory, filename and extension. Keep only the last input and split result; reuse exact matches without skipping resource loads or caching resource/VPK contents. One historical run reused about 12.99 million of 13 million calls, with about 15,300 native splits. More repetition may help; speedup is not guaranteed.")
SKEETO_UI(FastLoadVpk, L"[3] Skip consecutive unrelated VPKs %s")
SKEETO_UI(FastLoadVpkHint, L"Directory summaries, hit/miss checks and consecutive skips work as one mechanism. Bypass VPKs that cannot contain the target, preserving mod priority and native file confirmation. No pack merging or file-content cache.")
SKEETO_UI(FastLoadAsyncMesh, L"[4] Async model mesh loading %s")
SKEETO_UI(FastLoadAsyncMeshHint, L"Read model meshes asynchronously during loading, finish submitted work and restore the previous setting before play. Requires the master switch. Defaults to ON.")
SKEETO_UI(FastLoadAsyncCollide, L"[5] Async collision model loading %s")
SKEETO_UI(FastLoadAsyncCollideHint, L"Read collision models asynchronously during loading, finish submitted work and restore the previous setting before play. Requires the master switch. Defaults to ON.")
SKEETO_UI(FastLoadAsyncAnims, L"[1] Async model animation loading %s")
SKEETO_UI(FastLoadAsyncAnimsHint, L"Read model animations asynchronously during loading, finish submitted work and restore the previous setting before play. Requires the master switch. Defaults to OFF.")
SKEETO_UI(FastLoadBackground, L"[6] Skip unfocused loading waits %s")
SKEETO_UI(FastLoadBackgroundHint, L"Skip engine sleeps only while loading without focus. Foreground loading usually gains nothing. Restore normal waits when loading ends or this option is disabled.")
SKEETO_UI(FastLoadEarlyView, L"[7] Show game scene early %s")
SKEETO_UI(FastLoadEarlyViewHint, L"After native client preparation, show the scene while the server still waits or freezes the team. Does not unfreeze players, shorten server waits or allow earlier control. The waiting view may not be positioned yet. Client-side mechanism for remote servers; plugins and special modes still need testing.")
SKEETO_UI(FastLoadLocalReady, L"[8] Skip local start countdown %s")
SKEETO_UI(FastLoadLocalReadyHint, L"Solo or self-hosted rooms only; affects the whole room. Remove the fixed extra 3-second countdown after the server allows play. Keep waiting for teammates or the native timeout; skip no resource preparation. No effect on remote servers.")
SKEETO_UI(FastLoadLocalTransition, L"[5] Skip local chapter transition wait (Beta) %s")
SKEETO_UI(FastLoadLocalTransitionHint, L"Solo or self-hosted rooms only; affects the whole room. Set the native extra 8-second wait after chapter completion, before the map change begins, to 0; the engine still schedules the map change normally. Remote servers need dedicated plugin support and are usually unaffected.\nEnabling this causes a black background during chapter-transition loading; the chapter stats HUD remains visible.")
SKEETO_UI(FastLoadLocalTransitionBug, L"This feature currently has a bug and may trigger an unexpected load after leaving the safe room.")
SKEETO_UI(FastLoadDiagnostics, L"[2] Loading diagnostics %s")
SKEETO_UI(FastLoadMeasureHint, L"Requires the master switch. Count calls and sample timings. Turn OFF when comparing net loading gains to exclude diagnostic cost.")
SKEETO_UI(FastLoadReport, L"[3] Print counters to console")
SKEETO_UI(FastLoadReportHint, L"Print collected counters without enabling any optimization.")
SKEETO_UI(FastLoadReset, L"[4] Reset counters")
SKEETO_UI(FastLoadResetHint, L"Reset only these counters; keep mechanism switches unchanged.")
SKEETO_UI(FastLoadChatTime, L"[1] Loading timer %s")
SKEETO_UI(FastLoadChatTimeHint, L"From the loading screen until the game view appears, chat shows [Skeeto] Finished loading in and the seconds. Independent of the master switch. Defaults to ON. With early view on, it ends when the view appears.\nThis is a program-measured duration and may differ from the loading time you perceive.")
SKEETO_UI(FastLoadChatTimeResult, L"[Skeeto] Finished loading in %.2fs")
SKEETO_UI(FastLoadAttachFailed, L"Not connected. Enabled loading optimizations are inactive: %s")
SKEETO_UI(FastLoadStatus, L"Status: %s")
SKEETO_UI(FastLoadReady, L"Connected; detailed counters are in the console")
SKEETO_UI(FastLoadUnsupported, L"Unsupported game call site; inactive")
SKEETO_UI(FastLoadConflict, L"Call site changed by another module; inactive")
SKEETO_UI(FastLoadFailed, L"Could not modify call site; inactive")
SKEETO_UI(FastLoadOff, L"Off")
SKEETO_UI(ProgressDisplayMode, L"[4] Display mode [%s]")
SKEETO_UI(ProgressModeLive, L"Live HUD")
SKEETO_UI(ProgressModeThreshold, L"Threshold notices")

SKEETO_UI(CampaignOutlineEntry, L"[1] Campaign danger outlines 【Open settings】")
SKEETO_UI(CampaignOutlineTitle, L"Campaign danger outlines")
SKEETO_UI(CampaignOutlineCar, L"[1] Tank hittable outlines %s")
SKEETO_UI(CampaignOutlineWitch, L"[2] Persistent Witch outlines %s")
SKEETO_UI(CampaignOutlineTankBody, L"[3] Persistent Tank outlines %s")
SKEETO_UI(CampaignOutlineCarColor, L"[4] Hittable color [%s]")
SKEETO_UI(CampaignOutlineWitchColor, L"[5] Witch color [%s]")
SKEETO_UI(CampaignOutlineTankBodyColor, L"[6] Tank color [%s]")
SKEETO_UI(CampaignOutlineRange, L"[7] Display range [%d game units]")
SKEETO_UI(CampaignOutlineHint, L"    For a living local survivor in coop campaign or Realism. While a living Tank is present, outline every prop that Tank can punch, including alarm cars, ordinary cars and non-vehicle models. Nothing is outlined when no Tank is present. Living Tanks and Witches stay outlined, including startled Witches. Warnings remain visible within range even behind buildings. Existing server outlines are preserved.")
SKEETO_UI(CampaignOutlineUnavailable, L"    Client glow interface verification failed; this feature is inactive.")
SKEETO_UI(CampaignOutlineBindHint, L"    Bindable toggles: skeeto_tank_prop_outline / skeeto_witch_outline / skeeto_tank_outline; add 0 or 1 to turn off or on explicitly.")
SKEETO_UI(LaserSightOutline, L"[1] Persistent laser sight outlines %s")
SKEETO_UI(LaserSightOutlineHint, L"    Outlines stock-model laser sights on the ground through walls within 2000 game units. Picked-up or weapon-mounted sights are not outlined. Only a living local survivor can see them. Default off. Bind skeeto_laser_outline.")
SKEETO_UI(InteractScanEntry, L"[2] Interactable item scan 【Open settings】")
SKEETO_UI(InteractScanEntryHint, L"    After binding a key, briefly outlines nearby buttons, doors, dynamic props and some physics/breakable props. False positives are expected. Default off. Bind skeeto_interact_scan. Sound and effect follow the Supply Scan toggles.")
SKEETO_UI(InteractScanTitle, L"Interactable item scan")
SKEETO_UI(InteractScanToggle, L"[1] Interactable item scan %s")
SKEETO_UI(InteractScanToggleHint, L"    After enabling, press the bound key to scan. Console: bind <key> skeeto_interact_scan. Outlines buttons, doors, dynamic props, physics and breakable props; false positives expected. Ground weapons/supplies are excluded. Sound and effect follow the Supply Scan toggles.")
SKEETO_UI(InteractScanRadius, L"[2] Scan radius [%d]  - / + adjust")
SKEETO_UI(InteractScanRadiusHint, L"    Maximum scan distance. Default 2000, range 400-3200.")
SKEETO_UI(InteractScanHold, L"[3] Outline duration [%d sec]  - / + adjust")
SKEETO_UI(InteractScanHoldHint, L"    How long outlines stay. Default 6 seconds, range 1-15. Visible through walls.")
SKEETO_UI(InteractScanColor, L"[4] Outline color [%s]")

SKEETO_UI(SkeetoDeveloperEntry, L"[3] Skeeto developer")
SKEETO_UI(SkeetoDeveloper, L"Skeeto developer")
SKEETO_UI(SkeetoOpenSourceEntry, L"[4] Skeeto open source repo")
SKEETO_UI(SkeetoOpenSource, L"Skeeto open source repo")

SKEETO_UI(NavAdjustAutoRow, L"[%d] %s [%s]  - / + adjust")
SKEETO_UI(NoGroundCorpses, L"[1] Disable ground corpses %s")
SKEETO_UI(NoGroundCorpsesHint, L"    Sets cl_ragdoll_maxcount to 0 without disabling ragdoll creation. The engine controls corpse retention. Restores the previous value when disabled.")
SKEETO_UI(ScorchDecals, L"[2] Molotov ground scorch, Molotov Scorch Marks %s")
SKEETO_UI(ScorchDecalsHint, L"    Ground burned by a Molotov shows scorch marks.")
SKEETO_UI(ScriptIncapRoll, L"[8] Incapacitated view roll %s")
SKEETO_UI(ScriptIncapRollHint, L"    Corrects the camera angle while a player is incapacitated.")
SKEETO_UI(ScriptLadderStep, L"[9] First-person ladder steps %s")
SKEETO_UI(ScriptJockeyLeap, L"[1] Jockey leap audio cue %s")
SKEETO_UI(ScriptJockeyLeapHint, L"    Plays a cue when a Jockey leaps.")
SKEETO_UI(ScriptGunReverb, L"[3] Dynamic gun reverb %s")
SKEETO_UI(ScriptGunReverbHint, L"    Plays gunshot reverb for the current space. Files are sound\\skeeto\\reverb\\room1.wav through room4.wav, from loose files or a mounted VPK. Turns off while spatial reverb lock is on.")
SKEETO_UI(ScriptGunFeelEntry, L"[3] Gunshot feel 【Open settings】")
SKEETO_UI(ScriptGunFeelEntryHint, L"    Keeps the original gunshot and adds a clearly pitched extra shot. Punch is higher, Low is deeper, Short tail adds a second hit, and Bright is sharper. Turns off spatial-audio lock.")
SKEETO_UI(ScriptGunFeelTitle, L"Gunshot feel  v%s")
SKEETO_UI(ScriptGunFeelHint, L"    Keeps the original gunshot and adds a clearly pitched extra shot. Punch is higher, Low is deeper, Short tail adds a second hit, and Bright is sharper. One treatment for every gun. Turns off spatial-audio lock. Can stay on with gun reverb. Defaults off.")
SKEETO_UI(ScriptGunFeelMaster, L"[1] Gunshot feel %s")
SKEETO_UI(ScriptGunFeelPreset, L"[2] Treatment 【%s】")
SKEETO_UI(ScriptGunFeelPunch, L"Punch")
SKEETO_UI(ScriptGunFeelLow, L"Low")
SKEETO_UI(ScriptGunFeelTail, L"Short tail")
SKEETO_UI(ScriptGunFeelBright, L"Bright")
SKEETO_UI(ScriptGunFeelGain, L"[3] Layer strength 【%d%%】 - / +")
SKEETO_UI(ScriptParryEntry, L"[4] Parry flash, PARRY 【Open settings】")
SKEETO_UI(ScriptParryEntryHint, L"    The white flash can be disabled and lasts 0.1 to 0.6 seconds (default 0.2). A cue plays on a parry, rock break, or selected kill. Cues are in sound\\skeeto\\parry\\. A listen server also slows the game briefly.")
SKEETO_UI(ScriptPageBoth, L"← previous / → next")
SKEETO_UI(ScriptParryTitle, L"Parry flash (%d/%d)  v%s")
SKEETO_UI(ScriptParryHint, L"    White flash and one cue from sound\\skeeto\\parry\\. The flash defaults on and its duration defaults to 0.2 seconds; set it from 0.1 to 0.6 seconds or turn it off. Choose one cue file or play at random. Loose files and mounted VPKs, WAV or MP3. On a listen server the game drops to one tenth speed for about 0.6 seconds, including lobby local games; another server does not change game speed. Master defaults off.")
SKEETO_UI(ScriptParrySound, L"[2] Cue 【%s】")
SKEETO_UI(ScriptParryWhiteFlash, L"[3] White flash %s")
SKEETO_UI(ScriptParryFlashDuration, L"[4] White flash duration 【0.%d s】")
SKEETO_UI(ScriptParrySoundRandom, L"Random")
SKEETO_UI(ScriptParrySoundEmpty, L"    sound\\skeeto\\parry\\ has no WAV or MP3 yet.")
SKEETO_UI(ScriptParryMaster, L"[1] Parry flash, PARRY %s")
SKEETO_UI(ScriptParryHunter, L"[5] Hunter deadstop %s")
SKEETO_UI(ScriptParryJockey, L"[6] Jockey shove %s")
SKEETO_UI(ScriptParryLevel, L"[7] Charger level %s")
SKEETO_UI(ScriptParryRock, L"[8] Tank rock destroyed %s")
SKEETO_UI(ScriptParryTongue, L"[9] Tongue cut %s")
SKEETO_UI(ScriptParryBoomer, L"[1] Boomer killed %s")
SKEETO_UI(ScriptParryWitch, L"[2] Witch killed %s")
SKEETO_UI(ScriptParrySkeet, L"[3] Airborne kill, including headshot %s")
SKEETO_UI(ScriptParryMeleeSkeet, L"[4] Melee airborne kill %s")
SKEETO_UI(BloodSplat, L"[5] Hit flesh sound %s")
SKEETO_UI(BloodSplatHint, L"    Plays a flesh sound on a hit, kill, melee kill, or headshot kill. Hits follow the hit option; kills follow kill feedback. Requires the master sound switch.")
SKEETO_UI(EscUiTitle, L"Game menu")
SKEETO_UI(EscUiTitleFont, L"[4] Title font size [%d]")
SKEETO_UI(EscUiBodyFont, L"[5] Button font size [%d]")
SKEETO_UI(EscUiEdit, L"[6] Edit ESC layout")
SKEETO_UI(EscIoReady, L"Position, size, text and colors are saved independently.")
SKEETO_UI(EscIoSaving, L"Saving or loading profile...")
SKEETO_UI(EscIoSaved, L"Current layout saved")
SKEETO_UI(EscIoReadError, L"Cannot read layout. Original kept; replacement needs confirmation.")
SKEETO_UI(EscIoSaveError, L"Save failed. Original kept; remove file locks and apply again.")
SKEETO_UI(EscIoBusy, L"File operation is still running. Please retry shortly.")
SKEETO_UI(EscIoProfileSaved, L"Profile slot saved")
SKEETO_UI(EscIoProfileLoaded, L"Profile loaded into draft. Apply to use it.")
SKEETO_UI(EscIoProfileMissing, L"This profile slot has no file yet.")
SKEETO_UI(EscIoBadProfile, L"Cannot read profile. Draft unchanged.")
SKEETO_UI(EscIoTextError, L"Text is too long or contains unsupported control characters.")
SKEETO_UI(EscIoNeedResume, L"Keep at least one element.")
SKEETO_UI(EscIoLimit, L"Up to 128 elements are supported.")
SKEETO_UI(EscIoConfirm, L"Click again to confirm replacement.")
SKEETO_UI(EscEdApply, L"Apply and save")
SKEETO_UI(EscEdCancel, L"Discard draft")
SKEETO_UI(EscEdNext, L"Element %d / %d: choose from list")
SKEETO_UI(EscEdZh, L"Edit Chinese text")
SKEETO_UI(EscEdEn, L"Edit English text")
SKEETO_UI(EscEdAction, L"Action: %s")
SKEETO_UI(EscEdVisible, L"Visible: %s")
SKEETO_UI(EscEdColor, L"Text color: %s")
SKEETO_UI(EscEdFont, L"Font style: %s")
SKEETO_UI(EscEdBack, L"Move backward / earlier in navigation")
SKEETO_UI(EscEdFront, L"Move forward / later in navigation")
SKEETO_UI(EscEdDuplicate, L"Duplicate element and descendants")
SKEETO_UI(EscEdDelete, L"Delete element and descendants")
SKEETO_UI(EscEdAddButton, L"Add button")
SKEETO_UI(EscEdAddText, L"Add text")
SKEETO_UI(EscEdDefaults, L"Reset draft to default layout")
SKEETO_UI(EscEdProfile, L"Profile slot: %d / 8")
SKEETO_UI(EscEdSaveProfile, L"Save draft to profile slot")
SKEETO_UI(EscEdLoadProfile, L"Load profile slot into draft")
SKEETO_UI(EscEdOpenProfiles, L"Open profiles folder")
SKEETO_UI(EscEdTextCaption, L"Skeeto ESC text editor")
SKEETO_UI(EscEdDecoration, L"Text only")
SKEETO_UI(EscEdYes, L"On")
SKEETO_UI(EscEdNo, L"Hidden")
SKEETO_UI(EscEdTitleSize, L"Title")
SKEETO_UI(EscEdBodySize, L"Button")
SKEETO_UI(EscEdConfirmCurrent, L"Confirm replacing unreadable layout")
SKEETO_UI(EscEdConfirmProfile, L"Confirm save (replace this profile slot)")
SKEETO_UI(EscEdResume, L"Resume editing draft")
SKEETO_UI(EscEdRoot, L"Edit ESC layout")
SKEETO_UI(EscEdTitle, L"Drag to move / corner to resize / arrows to nudge")
SKEETO_UI(EscEdHint, L"Apply saves. ESC keeps this session's draft.")
SKEETO_UI(EscUiSetupTitle, L"Custom ESC menu UI")
SKEETO_UI(EscUiEnabled, L"[1] Custom ESC menu %s")
SKEETO_UI(EscUiAccent, L"[2] Hover border color: %s")
SKEETO_UI(EscUiBackdrop, L"[3] Background dimming [%d%%]")
SKEETO_UI(EscUiShow, L"[7] Return to custom ESC menu")
SKEETO_UI(EscUiUseNative, L"[8] Use native ESC menu this time")
SKEETO_UI(EscUiCvarRemember, L"[9] Remember session pins across launches %s")
SKEETO_UI(EscUiCvarRememberHint, L"    Session sliders and skt_cvar keep their last value after you quit and start the game again. A mod-ban server still pauses them. Presets stay separate named snapshots.")
SKEETO_UI(EscUiCvarClear, L"[1] Clear remembered session pins")
SKEETO_UI(EscUiSetupHint, L"Press ESC in game. Enter a map to edit the layout. Native settings and confirmations remain available.")
SKEETO_UI(EscUiResume, L"Resume game")
SKEETO_UI(EscUiAudio, L"Audio settings")
SKEETO_UI(EscUiVideo, L"Video settings")
SKEETO_UI(EscUiKeyboard, L"Keyboard / mouse")
SKEETO_UI(EscUiSkeeto, L"Skeeto settings")
SKEETO_UI(EscUiExit, L"Disconnect")
SKEETO_UI(EscUiNative, L"Use native ESC menu this time")

SKEETO_UI(EscUiIdle, L"Idle")
SKEETO_UI(EscUiInvite, L"Invite friends")
SKEETO_UI(EscUiBoot, L"Kick player")
SKEETO_UI(EscUiChapter, L"Vote to change chapter")
SKEETO_UI(EscUiStats, L"Stats and achievements")
SKEETO_UI(EscUiAddons, L"Add-ons")
SKEETO_UI(EscEdCommand, L"Console command")
SKEETO_UI(EscEdParameter, L"Edit action parameters")
SKEETO_UI(EscEdParameterError, L"Invalid parameters: use a non-empty command, a Cvar name/range/positive step, or one of the information component choices listed in its input window.")
SKEETO_UI(EscEdElements, L"Choose element")
SKEETO_UI(EscEdActions, L"Choose action")
SKEETO_UI(EscEdColors, L"Choose text color")
SKEETO_UI(EscEdFonts, L"Choose font style")
SKEETO_UI(EscEdSlots, L"Choose profile slot")
SKEETO_UI(EscEdSliderFields, L"ConVar: name / minimum / maximum / step")
SKEETO_UI(EscEdClose, L"Close picker")
SKEETO_UI(EscEdPrevious, L"Previous page")
SKEETO_UI(EscEdFollowing, L"Next page")
SKEETO_UI(EscEdHiddenSuffix, L" (hidden)")
SKEETO_UI(EscEdWhite, L"Default white")
SKEETO_UI(EscEdAmber, L"Amber")
SKEETO_UI(EscEdCyan, L"Cyan")
SKEETO_UI(EscEdGreen, L"Green")
SKEETO_UI(EscEdPurple, L"Purple")
SKEETO_UI(EscEdOrange, L"Orange")
SKEETO_UI(EscEdSilver, L"Silver")
SKEETO_UI(EscEdSlotChoice, L"Profile slot %d")
SKEETO_UI(EscEdApplyParameters, L"Apply these parameters")
SKEETO_UI(EscEdPickHint, L"Click a choice directly. Picking never moves elements or runs game actions.")

SKEETO_UI(EscEdFlyout, L"Flyout submenu")
SKEETO_UI(EscEdPinnedSlider, L"Session ConVar slider (skt_cvar)")
SKEETO_UI(EscEdParent, L"Parent menu: %s")
SKEETO_UI(EscEdEnter, L"Edit this submenu")
SKEETO_UI(EscEdAddMenu, L"Add flyout submenu")
SKEETO_UI(EscEdPickParent, L"Choose parent menu")
SKEETO_UI(EscEdHomepage, L"Homepage")
SKEETO_UI(EscEdBackMenu, L"Back")
SKEETO_UI(EscEdLayoutError, L"Invalid hierarchy or missing homepage Resume. Move child items before changing a parent action.")
SKEETO_UI(EscEdPinError, L"Value not applied: unavailable variable, server ownership or session table limit.")
SKEETO_UI(EscEdPinDeferred, L"Server restrictions apply. Value recorded for this session only.")

SKEETO_UI(EscEdBackground, L"Button background / shared colors / opacity")
SKEETO_UI(EscEdBgGlobal, L"Use shared background")
SKEETO_UI(EscEdBgClear, L"Transparent")
SKEETO_UI(EscEdBgFollow, L"Follow text color")
SKEETO_UI(EscEdBgItem, L"This button background")
SKEETO_UI(EscEdBgUniform, L"Shared background color")
SKEETO_UI(EscEdBgOpacity, L"Shared background transparency: 0% solid, 100% clear")
SKEETO_UI(EscEdBgItemValue, L"This button: %s")
SKEETO_UI(EscEdBgUniformValue, L"Shared background: %s")
SKEETO_UI(EscEdBgOpacityValue, L"Shared background transparency: %d%%")
SKEETO_UI(EscEdBgAll, L"Use shared background and transparency for all buttons")

SKEETO_UI(EscEdItemOpacity, L"Button background transparency: %s")

SKEETO_UI(EscEdTransparencyValue, L"Transparency: %d%%")
SKEETO_UI(EscEdOpacityInherit, L"Inherit")
SKEETO_UI(EscEdTransparentBorder, L"Show border when transparent: %s")

SKEETO_UI(EscUiVote, L"Call a vote")
SKEETO_UI(EscUiQuit, L"Quick exit")
SKEETO_UI(EscEdRgbInput, L"Enter RGB: R G B, each from 0 to 255")
SKEETO_UI(EscEdRgbError, L"Enter three integers from 0 to 255, separated by spaces.")
SKEETO_UI(GameUiRootEntry, L"[6] Custom game UI")
SKEETO_UI(GameUiTitle, L"Custom game UI")
SKEETO_UI(GameUiMainEntry, L"[1] Custom main menu UI [Open settings]")
SKEETO_UI(GameUiEscEntry, L"[2] Custom ESC menu UI [Open settings]")

SKEETO_UI(EscUiDifficulty, L"Change difficulty")
SKEETO_UI(EscUiScenario, L"Start a new campaign")
SKEETO_UI(EscUiRestart, L"Restart campaign")
SKEETO_UI(EscUiLobby, L"Return to lobby")
SKEETO_UI(EscUiAllTalk, L"Change alltalk")

SKEETO_UI(EscUiNativeVote, L"Native call vote")

SKEETO_UI(EscPresetSaved, L"ConVar preset saved")
SKEETO_UI(EscPresetSaveError, L"Preset save failed; the original file was retained")
SKEETO_UI(EscPresetBusy, L"Preset busy or invalid file; operation was not saved")
SKEETO_UI(EscPresetSkipped, L"Preset saved; %d unreadable slider variables were omitted")
SKEETO_UI(EscInfoMissing, L"Unavailable")
SKEETO_UI(EscInfoServer, L"Server: %.150s")
SKEETO_UI(EscInfoMap, L"Map: %s")
SKEETO_UI(EscInfoPing, L"Ping: %.0f ms")
SKEETO_UI(EscInfoPingMissing, L"Ping: %s")
SKEETO_UI(EscInfoPlayersMax, L"Players: %d/%d")
SKEETO_UI(EscInfoPlayers, L"Players: %d")
SKEETO_UI(EscInfoPlayersMissing, L"Players: %s")
SKEETO_UI(EscInfoAddress, L"IP: %s")
SKEETO_UI(EscInfoNoAddress, L"No connection address is available")
SKEETO_UI(EscInfoCopyError, L"Clipboard unavailable; command was not copied")
SKEETO_UI(EscPresetLimit, L"At most 64 different variables per preset; nothing was saved")
SKEETO_UI(EscPresetEmptyCapture, L"No readable ConVar sliders in the current layout")
SKEETO_UI(EscPresetRename, L"Name ConVar preset")
SKEETO_UI(EscPresetEmpty, L"Empty preset; save current slider values first")
SKEETO_UI(EscPresetApplied, L"Requested %d values; %d were not applied. Game/server rules decide final values")
SKEETO_UI(EscServerInfo, L"Server information")
SKEETO_UI(EscCvarPresets, L"ConVar presets")
SKEETO_UI(EscBattleReport, L"Recent battle report")
SKEETO_UI(EscReportQueued, L"Opening recent battle report")
SKEETO_UI(EscReportBusy, L"Report service busy; please try again later")
SKEETO_UI(EscPresetTile, L"%d / %s / %d values")
SKEETO_UI(EscPresetUnnamed, L"Unnamed preset")
SKEETO_UI(EscPresetExecute, L"Apply selected preset")
SKEETO_UI(EscPresetCapture, L"Save current slider values")
SKEETO_UI(EscPresetClose, L"Close presets")
SKEETO_UI(EscPresetHint, L"Select a slot, then save actual values of all sliders in this layout. Presets never run automatically.")
SKEETO_UI(EscMapInfo, L"Campaign / chapter / map code")
SKEETO_UI(EscClock, L"World clock")
SKEETO_UI(EscRoundTimer, L"Round timer")
SKEETO_UI(EscTimerSeconds, L"Round %u s")
SKEETO_UI(EscTimerValue, L"Round %02u:%02u:%02u")

SKEETO_UI(EscEdAccent, L"Hover border color: %s")
SKEETO_UI(EscCommandCopied, L"Command copied")
SKEETO_UI(EscMapMissing, L"No map code is available")

SKEETO_UI(EscAccentFollow, L"Follow current button color")
SKEETO_UI(EscAccentBlue, L"Light blue")
SKEETO_UI(EscNativeOptions, L"Options")

SKEETO_UI(EscStateShown, L"Shown")
SKEETO_UI(EscStateHidden, L"Hidden")
SKEETO_UI(EscStateRunning, L"Not paused")
SKEETO_UI(EscStatePaused, L"Paused")
SKEETO_UI(EscGeometrySelection, L"X %d  Y %d  W %d  H %d  /  Selected %d")
SKEETO_UI(EscAlignLeft, L"Align left")
SKEETO_UI(EscAlignCenter, L"Align horizontal centers")
SKEETO_UI(EscAlignRight, L"Align right")
SKEETO_UI(EscAlignTop, L"Align top")
SKEETO_UI(EscAlignMiddle, L"Align vertical centers")
SKEETO_UI(EscAlignBottom, L"Align bottom")
SKEETO_UI(EscMatchWidth, L"Match widths")
SKEETO_UI(EscMatchHeight, L"Match heights")
SKEETO_UI(EscMatchSize, L"Match size")
SKEETO_UI(EscSpaceHorizontal, L"Distribute horizontal gaps")
SKEETO_UI(EscSpaceVertical, L"Distribute vertical gaps")
SKEETO_UI(EscArrangeTools, L"Align and size (Ctrl-click to select)")
SKEETO_UI(EscSnapOn, L"Snapping: on / hold Alt to suspend")
SKEETO_UI(EscSnapOff, L"Snapping: off")
SKEETO_UI(EscArrangeHint, L"Align/size uses the active item; distribution keeps both ends fixed and needs 3+ items.")
SKEETO_UI(EscArrangeError, L"Select multiple items on this page (3+ to distribute); no change if space or canvas bounds prevent it.")

SKEETO_UI(EscArrangeTitle, L"Align and size")

SKEETO_UI(EscStateShownPaused, L"Shown / Paused")
SKEETO_UI(EscStateHiddenPaused, L"Hidden / Paused")

SKEETO_UI(FastLoadExperimentsEntry, L"[9] Experimental options [Settings]")
SKEETO_UI(FastLoadExperimentsTitle, L"Experimental loading options")
SKEETO_UI(FastLoadExperimentsHint, L"Experimental features whose effectiveness and reliability need further verification. Open this page to select and enable options for your own tests.")
SKEETO_UI(FastLoadLessModelData, L"[2] Reduce forced model caching %s")
SKEETO_UI(FastLoadLessModelDataHint, L"Stop forcing model data into cache only during loading; restore on completion or disable.")
SKEETO_UI(FastLoadLessTouch, L"[3] Reduce model warm-up %s")
SKEETO_UI(FastLoadLessTouchHint, L"Skip proactive model-data touching only during loading; restore on completion or disable.")
SKEETO_UI(FastLoadLessPreload, L"[4] Reduce forced preloading %s")
SKEETO_UI(FastLoadLessPreloadHint, L"Disable forced client preloading only during loading; required resources remain engine-controlled. Restore on completion or disable. No proven full-interval net gain. Mods/scripts may re-enable it; this option does not repeatedly override them. Defaults to OFF.")

SKEETO_UI(FastLoadMapPacks, L"[6] Skip other map packs %s")
SKEETO_UI(FastLoadMapPacksHint, L"During loading, skip clearly identified VPKs for other maps/campaigns. Keep current-campaign parts and pure material, model and sound packs. No Mod files are changed or unmounted; assumes independent campaigns do not supply each other. Defaults to OFF.")
SKEETO_UI(FastLoadFileFilter, L"[7] Finer file filtering %s")
SKEETO_UI(FastLoadLessTextureWarmup, L"[8] Reduce forced texture warm-up %s")
SKEETO_UI(FastLoadLessTextureWarmupHint, L"Only during loading, skip forcing managed textures into GPU memory one by one. Native texture reads, creation and normal drawing remain; existing ConVar values are unchanged. Some uploads may move to the first visible use and cause a short hitch after loading. Works on remote servers; test effectiveness and reliability yourself. Defaults to OFF.")
SKEETO_UI(FastLoadFileFilterHint, L"For adjacent repeated resource paths, reuse native-confirmed file lookup results within the current mount generation and skip absent VPKs. No full-pack index, resource contents or open results are cached; native order and final confirmation remain. First queries still cost work; compare results yourself. Both VPK experiments use directory candidates. Defaults to OFF.")

SKEETO_UI(GameUiChatEntry, L"[3] Custom chat box [Open settings]")
SKEETO_UI(GameUiConsoleEntry, L"[4] Custom console panel [Open settings]")
SKEETO_UI(NativeUiChatTitle, L"Custom chat box")
SKEETO_UI(NativeUiConsoleTitle, L"Custom console panel")
SKEETO_UI(NativeUiEnabled, L"Custom style")
SKEETO_UI(NativeUiFontOverride, L"Override native font")
SKEETO_UI(NativeUiFontFace, L"Font family")
SKEETO_UI(NativeUiFontSize, L"Font size")
SKEETO_UI(NativeUiBold, L"Bold")
SKEETO_UI(NativeUiShadow, L"Text shadow")
SKEETO_UI(NativeUiOutline, L"Text outline")
SKEETO_UI(NativeUiAutoScale, L"Scale font with resolution")
SKEETO_UI(NativeUiChatText, L"Chat message color")
SKEETO_UI(NativeUiText, L"Command echo color")
SKEETO_UI(NativeUiInput, L"Input text color")
SKEETO_UI(NativeUiBackground, L"Background color")
SKEETO_UI(NativeUiBgAlpha, L"Background opacity")
SKEETO_UI(NativeUiChatBackground, L"Chat background (while typing)")
SKEETO_UI(NativeUiChatBgAlpha, L"Chat background opacity (while typing)")
SKEETO_UI(NativeUiSayTimeHidden, L"0 (hidden; open chat to view)")
SKEETO_UI(NativeUiInputBackground, L"Input background color")
SKEETO_UI(NativeUiInputBgAlpha, L"Input background opacity")
SKEETO_UI(NativeUiGeometry, L"Override native position and size")
SKEETO_UI(NativeUiPositionEntry, L"Move chat box [Open settings]")
SKEETO_UI(NativeUiPositionTitle, L"Chat box position")
SKEETO_UI(NativeUiPositionHint, L"Arrow keys move; Shift moves faster. Moving enables layout override. Blue outline previews the target.")
SKEETO_UI(NativeUiPositionReset, L"[1] Reset chat position")
SKEETO_UI(NativeUiWidth, L"Width (% of screen)")
SKEETO_UI(NativeUiHeight, L"Height (% of screen)")
SKEETO_UI(NativeUiSayTime, L"Message duration (seconds)")
SKEETO_UI(NativeUiSurvivor, L"Survivor name color")
SKEETO_UI(NativeUiInfected, L"Infected name color")
SKEETO_UI(NativeUiDeveloper, L"Developer output color")
SKEETO_UI(NativeUiTitleText, L"Title text color")
SKEETO_UI(NativeUiTitleBackground, L"Title background color")
SKEETO_UI(NativeUiReset, L"Reset this style to defaults")
SKEETO_UI(NativeUiWaiting, L"Waiting for native controls (open chat / console)")
SKEETO_UI(NativeUiActive, L"Applying style to native controls")
SKEETO_UI(NativeUiUnsupported, L"Some interfaces do not match this game; related properties were not applied")
SKEETO_UI(NativeUiFault, L"Controls rebuilt or font setup failed; reopen the native panel and check")
SKEETO_UI(NativeUiOff, L"Disabled: restore native style captured before enabling")
SKEETO_UI(NativeUiMenuHint, L"Number keys select values; -/+ adjusts. Click toggles switches or cycles colors/fonts.")
SKEETO_UI(NativeUiAlphaStep, L"Opacity adjustment step")
SKEETO_UI(NativeUiConsoleRgbHint, L"Exact RGB: skeeto_console_ui color text 153 204 255 (see tutorial)")
SKEETO_UI(NativeUiChatRgbHint, L"Exact RGB: skeeto_chat_ui color text 153 204 255 (see tutorial)")
SKEETO_UI(NativeUiBorder, L"Native theme border")
SKEETO_UI(PluginPlatformEntry, L"[5] Skeeto plugin platform")
SKEETO_UI(PluginPlatformTitle, L"Skeeto plugin platform")
SKEETO_UI(PluginStartupOption, L"[%d] Load plugins at startup %s")
SKEETO_UI(PluginLoaded, L"Loaded; waiting for initialization")
SKEETO_UI(PluginActive, L"Running")
SKEETO_UI(PluginLoadError, L"Load failed (image or dependency)")
SKEETO_UI(PluginEntryMissing, L"Loaded; plugin entry missing")
SKEETO_UI(PluginApiMismatch, L"Loaded; incompatible API")
SKEETO_UI(PluginDuplicateId, L"Loaded; duplicate plugin ID")
SKEETO_UI(PluginInvalidDescriptor, L"Loaded; invalid descriptor")
SKEETO_UI(PluginInitError, L"Initialization failed")
SKEETO_UI(PluginPageError, L"Menu data or callback failed")
SKEETO_UI(PluginActionError, L"Action callback failed")
SKEETO_UI(PluginSubmenu, L"[Open settings]")
SKEETO_UI(PluginRestartHint, L"Startup changes apply next launch. Loaded plugins remain until the game fully exits.")
SKEETO_UI(PluginLaunchHint, L"Launch option -skeeto_plugin_platform 0/1 overrides only this startup and does not change the saved switch.")
SKEETO_UI(PluginLoading, L"Loading plugins")
SKEETO_UI(PluginSessionOn, L"Platform started this process; place entry DLLs in bin\\skeeto\\plugins\\.")
SKEETO_UI(PluginSessionOff, L"Platform was not started; no plugin directory scan or plugin execution.")
SKEETO_UI(PluginLimitHint, L"Plugin limit reached; at most 32 entry DLLs are processed per launch.")
SKEETO_UI(PluginEmpty, L"No plugin entry DLLs found.")
SKEETO_UI(PluginScanError, L"Could not read plugin directory; system error: %u")
SKEETO_UI(PluginWorking, L"Updating plugin menu")
SKEETO_UI(PluginNoMenu, L"This plugin does not provide a menu page.")
SKEETO_UI(PluginAttribution, L"Author: %s    Version: %s")
SKEETO_UI(PluginResult, L"State: %s    Result code: %u")
SKEETO_UI(PluginKeysHint, L"Number keys select; -/+ adjusts numeric values; R refreshes; 0 goes back.")

SKEETO_UI(FindRootSearch, L"[1] Search features")
SKEETO_UI(FindLocatedRow, L"Located: %s / item [%d]")
SKEETO_UI(FindLocatedPage, L"Located: %s / settings")
SKEETO_UI(FindSearchTitle, L"Search features")
SKEETO_UI(FindQuery, L"Query: %s")
SKEETO_UI(FindEmptyQuery, L"All features")
SKEETO_UI(FindEditQuery, L"[1] Edit query (up to 64 characters)")
SKEETO_UI(FindWaiting, L"Confirm or cancel in the input window.")
SKEETO_UI(FindNoResults, L"No results. Try a feature name or a common term.")
SKEETO_UI(FindLocalTag, L" [Local server only]")
SKEETO_UI(FindExperimentalTag, L" [Experimental]")
SKEETO_UI(FindPath, L"Path: %s")
SKEETO_UI(FindInputBusy, L"Text input is busy. Finish the other input window first.")
SKEETO_UI(FindInputFailed, L"Unable to open or read the input window. Try again.")

SKEETO_UI(FindAboutChangelog, L"Open the version changelog installed with Skeeto.")

SKEETO_UI(SupplyScanEntry, L"[5] Supply scan, Survivor Sense 【Open settings】")
SKEETO_UI(SupplyScanEntryHint, L"    Press the bound key and pickups inside the radius are outlined by type through walls until the chosen time ends. The sweep is only a visual cue. Console bind: bind <key> skeeto_supply_scan.")
SKEETO_UI(AlarmCarGlowEntry, L"[6] Armed alarm-car outline, Alarm Car Glow 【Open settings】")
SKEETO_UI(AlarmCarGlowEntryHint, L"    Outlines alarm cars that can still trigger; does not change trigger range. Default off.")
SKEETO_UI(AlarmCarGlowTitle, L"Armed alarm-car outline  v%s")
SKEETO_UI(AlarmCarGlowHint, L"    Outlines alarm cars that can still trigger; does not change trigger range. Default off.")
SKEETO_UI(AlarmCarGlowToggle, L"[1] Armed alarm-car outline %s")
SKEETO_UI(AlarmCarGlowToggleHint, L"    When on, outlines still-armed prop_car_alarm cars; clears after they disarm or you turn this off.")
SKEETO_UI(AlarmCarGlowRange, L"[2] Outline range [%d game units]")
SKEETO_UI(AlarmCarGlowRangeHint, L"    Press 2 or -/+ to cycle Glow range (1000/1500/2000/2500/3000, default 2000).")
SKEETO_UI(AlarmCarGlowColor, L"[3] Outline color [%s]")
SKEETO_UI(AlarmCarGlowColorHint, L"    Press 3 to cycle the palette; default orange.")
SKEETO_UI(AlarmCarGlowUnavailable, L"    Client outline interface check failed; this feature is inactive.")
SKEETO_UI(AlarmCarGlowApproxHint, L"    Client has no networked m_bDisabled; only glass_alarm windows count. After trigger the stock instance disables that glass and enables ordinary glass, and the outline clears with it.")
SKEETO_UI(ClientPerfEntry, L"[7] Client performance presets, Client Perf Presets 【Open settings】")
SKEETO_UI(ClientPerfEntryHint, L"    Writes ConVars per group; turning a group off restores that group's snapshot. Does not touch fast-load mod_load_*. Gains vary by machine; default off.")
SKEETO_UI(ClientPerfTitle, L"Client performance presets  v%s")
SKEETO_UI(ClientPerfHint, L"    Writes ConVars per group; turning a group off restores that group's snapshot. Does not touch fast-load mod_load_*. Gains vary by machine; default off.")
SKEETO_UI(ClientPerfParticles, L"[1] Particle / effect load reduction %s")
SKEETO_UI(ClientPerfParticlesHint, L"    Particle fallback multipliers and mat_reduceparticles.")
SKEETO_UI(ClientPerfBones, L"[2] Bone / physics / animation load reduction %s")
SKEETO_UI(ClientPerfBonesHint, L"    Physics ticks, bone blending and SIMD-related options.")
SKEETO_UI(ClientPerfRender, L"[3] Render load reduction %s")
SKEETO_UI(ClientPerfRenderHint, L"    Shadow / fog / framebuffer-copy style render options.")
SKEETO_UI(ClientPerfThreads, L"[4] Thread / queue optimization %s")
SKEETO_UI(ClientPerfThreadsHint, L"    Client threads and queues; leaf_system=0 may hurt. No mod_load_*.")
SKEETO_UI(ClientPerfSound, L"[5] Sound optimization %s")
SKEETO_UI(ClientPerfSoundHint, L"    Async sound and dsp_slow_cpu (may hurt audio quality).")
SKEETO_UI(ClientPerfListenNet, L"[6] Listen-host net interp and server opts %s")
SKEETO_UI(ClientPerfListenNetHint, L"    Listen-host pure/consistency and cl_interp_npcs; does not change tick/nb/lerp main path. Allow-0-lerp owns min_interp_ratio first.")
SKEETO_UI(ClientPerfListenNetLocked, L"    Listen host only; locked while you are not the host.")
SKEETO_UI(SupplyScanTitle, L"Supply scan, Survivor Sense (%d/%d)")
SKEETO_UI(SupplyScanToggle, L"[1] Supply scan %s")
SKEETO_UI(SupplyScanToggleHint, L"    When this is on, the bound key starts a scan. Console bind: bind <key> skeeto_supply_scan.")
SKEETO_UI(SupplyScanRadius, L"[2] Scan radius 【%d】  - / + adjust")
SKEETO_UI(SupplyScanRadiusHint, L"    How far the wave travels. Default 1600, range 400-3200.")
SKEETO_UI(SupplyScanHold, L"[3] Outline time 【%d s】  - / + adjust")
SKEETO_UI(SupplyScanHoldHint, L"    How long pickup outlines stay. Default 5 seconds, range 1-15. Items behind walls are included.")
SKEETO_UI(SupplyScanLimit, L"[1] Outlines kept 【%d】  - / + adjust")
SKEETO_UI(SupplyScanLimitHint, L"    How many pickups can stay outlined at once. Default 192, range 32-512. Nearer ones are kept first.")
SKEETO_UI(SupplyScanFilterEntry, L"[2] Item filter 【Open settings】")
SKEETO_UI(SupplyScanFilterEntryHint, L"    Every pickup is scanned unless you turn a category or item off. The category switch is separate from each item.")
SKEETO_UI(SupplyScanFilterTitle, L"Item filter")
SKEETO_UI(SupplyScanFilterHint, L"    Everything is scanned until you turn a category or item off.")
SKEETO_UI(SupplyScanFilterMedical, L"[1] Medical 【Open settings】")
SKEETO_UI(SupplyScanFilterThrow, L"[2] Throwables 【Open settings】")
SKEETO_UI(SupplyScanFilterGun, L"[3] Guns 【Open settings】")
SKEETO_UI(SupplyScanFilterMelee, L"[4] Melee 【Open settings】")
SKEETO_UI(SupplyScanFilterAmmo, L"[5] Ammo and upgrades 【Open settings】")
SKEETO_UI(SupplyScanFilterAll, L"[1] Whole category %s")
SKEETO_UI(SupplyScanFilterRow, L"[1] %s %s")
SKEETO_UI(SupplyScanColorHealth, L"[4] Healing items 【%s】  - / + adjust")
SKEETO_UI(SupplyScanColorAmmo, L"[5] Ammo and upgrades 【%s】  - / + adjust")
SKEETO_UI(SupplyScanColorThrow, L"[6] Throwables 【%s】  - / + adjust")
SKEETO_UI(SupplyScanColorWeapon, L"[7] Weapons 【%s】  - / + adjust")
SKEETO_UI(SupplyScanSound, L"[8] Scan sound %s")
SKEETO_UI(SupplyScanSoundHint, L"    Plays sound\\skeeto\\scan\\scan.wav when a scan starts. Loose files and mounted VPKs both work.")
SKEETO_UI(SupplyScanFx, L"[9] Scan effect %s")
SKEETO_UI(KbSecSupplyScan, L"Supply scan (Survivor Sense)")
SKEETO_UI(KbSecInteractScan, L"Interactable item scan")
SKEETO_UI(KbSecBugTeleport, L"Teleport to the nearest invincible bug spot (local server only)")
SKEETO_UI(SupplyScanHudEntry, L"[3] Scan result HUD [Settings]")
SKEETO_UI(SupplyScanHudEntryHint, L"    Items and counts from this scan. Icons are the supply-radar set. On by default.")
SKEETO_UI(SupplyScanHudTitle, L"Scan result  v%s")
SKEETO_UI(SupplyScanHudToggle, L"[1] Scan result %s")
SKEETO_UI(SupplyScanHudToggleHint, L"    Shows the pickups found by this scan, with counts. Arrow keys move it. Shift is a large step.")
SKEETO_UI(SupplyScanHudSize, L"[2] Size [%d%%]  - / +     Font [%d]  Shift + - / +")
SKEETO_UI(SupplyScanHudReset, L"[3] Reset position (right)")
SKEETO_UI(SupplyScanHudHeader, L"This scan  %d")
SKEETO_UI(SupplyScanHudMore, L"%d more")
