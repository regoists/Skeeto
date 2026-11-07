Skeeto 使用说明
==============

此说明适用于 Skeeto 2.0 架构。
This guide is for the Skeeto 2.0 layout.

Skeeto（SKT）是《求生之路 2》的客户端增强框架。本地安装后，多数功能可在服务器内使用；对抗、写实等模式会有不同程度限制。它使用 Hook、内存补丁等技术，原理与外挂同类，请自行判断是否使用。
Skeeto (SKT) is a Left 4 Dead 2 client enhancement. After a local install, most features work on servers; Versus, Realism, and similar modes have extra limits. It uses hooks and memory patches like cheats do—decide for yourself whether to use it.

问题反馈：bilibili @regoists 或者 Skeeto的创意工坊页面
Feedback: bilibili @regoists or the Skeeto Workshop page

创意工坊：https://steamcommunity.com/sharedfiles/filedetails/?id=3787239937
Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3787239937


菜单快速查找
------------

按 [ 打开 Skeeto 数字菜单，首页 [1] 为“搜索功能”，[2] 为“功能总览”。搜索支持中英文名称、常用叫法和多个关键词，例如“镜像”“viewmodel 镜像”“聊天 字号”；直接确认空关键词可浏览全部条目。
输入确认后，按结果编号进入相应设置页；按 [0] 返回时保留查询、分类和翻页位置。选择结果只定位，不替你开关功能或执行动作。取消修改关键词会保留原查询。
总览按玩家用途分类。列表显示名称、用途和原菜单路径，并随字号及可用高度调整每页条数。搜索覆盖内置功能及其设置；第三方插件内部菜单和素材包名称不在本版搜索范围。

Press [ to open the Skeeto numbered menu. [1] opens Search Features and [2] opens Feature Overview. Search accepts Chinese or English names, common terms and multiple keywords. Confirm an empty query to browse all entries.
Select a result by number to locate its settings. [0] returns with the query, category and page position preserved. Selecting a result never enables a feature or runs its action; cancelling a query edit keeps the previous query.
The overview groups features by purpose. Entries show their name, purpose and original menu path, with fewer rows when font size or available height requires it. Search covers built-in features and settings, excluding third-party plugin menus and asset-pack names.


安装与迁移
----------

首次安装：把完整包解压到游戏根目录，再从 Steam 启动。
First install: extract the full package into the game root, then launch from Steam.

从 1.x 升到 2.x 必须手动迁移一次，不能自动升级：退出游戏，把新包解压到游戏目录外，再运行「Setup_安装 维护 卸载.bat」选 1 并核对游戏根目录；不要先覆盖旧 skeeto.dll。手动路径及 Necola 保留步骤见根目录 Skeeto_更新日志.txt 最前面的说明。迁移后从 Steam 启动；旧 Skeeto 启动器文件保留，确认无用后自行删除。
1.x to 2.x requires one manual migration; automatic upgrade is unavailable. Exit the game, extract the new package outside the game folder, run Setup_安装 维护 卸载.bat, choose 1 and check the game root. Do not overwrite the old skeeto.dll first. See the top of Skeeto_更新日志.txt for the manual path and Necola preservation steps. Launch through Steam afterward; obsolete Skeeto launchers are kept until you remove them yourself.


常见问题
--------

1) 如何更新？
How do I update?

保持Skeeto的工坊页面为订阅状态即可自动更新。创意工坊：https://steamcommunity.com/sharedfiles/filedetails/?id=3787239937
此自动更新说明适用于完成迁移后的 2.0；1.x 必须先按上面的步骤手动迁移。
Automatic updates apply after migrating to 2.0. Users on 1.x must migrate manually first.
收到更新准备完成提示后，请完全退出游戏及旧启动器，等待退出后安装完成再启动。关键文件占用或短暂失败时会保留暂存，之后再次退出可重试；仍无法完成时，关闭游戏后运行包内维护入口或按本说明手工覆盖。
After the update-ready notice, fully exit the game and legacy launchers and allow installation to finish before relaunching. Locks or temporary failures retain staging for another exit-time retry. If it still fails, close the game and use the packaged maintenance entry or the manual installation steps.


2) 装了没效果，或按 [ 打不开菜单？
Installed but nothing works, or [ does not open the menu?

先运行 Setup_安装 维护 卸载.bat，再从 Steam 启动。检查游戏根目录的 skeeto.dll 与 bin/wininet.dll；若被杀毒隔离，先恢复明确属于 Skeeto 的文件。有不明代理 DLL 冲突时先查清，不要强行覆盖。
Run Setup_安装 维护 卸载.bat, then launch from Steam. Check root skeeto.dll and bin/wininet.dll; restore Skeeto files if antivirus quarantined them. Do not overwrite unknown proxy DLL conflicts until you know what they are.


3) 还能用 Steam 启动吗？
Can I still launch from Steam?

可以。用任何正常的启动游戏的方式都可以。
Yes. You can use any normal method to launch the game.


4) 命中反馈里「普感/特感素材包」一直关闭？
Hit-feedback Common/Special Infected packs stay off?

菜单只显示已成功读到的资源。若一直关且开不了，多半是工坊订阅的预设素材包挂载偏晚或进图太快导致 JSON 没读到。不要继续靠工坊订阅加载该素材包；把已下载的 VPK 挪到 addons（不要放在 workshop）再本地加载。
The menu only shows resources that loaded. If those options stay off, the Workshop preset pack often mounts too late—especially if you join a map quickly—so the JSON never loads. Stop relying on that Workshop subscription; move the downloaded VPK into addons (not workshop) for local loading.




持枪视角、挂件与快速测试图
------------------------

主菜单的“更强大的持枪视角调节”默认关闭。位置 X 为左右、Y 为前后、Z 为上下；数字键选中后用 - / + 调整，Shift 提供细调。可调整角度与旋转原点，支持编辑手持模型和辅助模型。调整与 L4N 位置叠加，不改写 L4N 配置。不同武器 MOD 按模型路径与模型校验值保存到 left4dead2/skeeto/viewmodel/profiles.ini，自动保存，无需手动保存。
Advanced Viewmodel Adjustment is disabled by default. X controls left/right, Y forward/back and Z up/down. Select with number keys, adjust with - / +, and hold Shift for fine adjustments. Angles, pivots, held models and auxiliary models are supported. Changes add to L4N positioning without editing L4N files. Profiles auto-save per model path and checksum to left4dead2/skeeto/viewmodel/profiles.ini.

完整 V 模镜像包含武器和双手。全局镜像与当前模型镜像为互斥控制模式；控制台 skeeto_viewmodel_mirror 0|1|toggle 选择全局模式，也可在游戏键盘 / 鼠标设置中绑定。逐模型模式下，两套共用武器脚本的 V 模如果要求不同镜像状态，会暂停这对模型的镜像，避免骨骼与面剔除不一致。
Full viewmodel mirroring includes weapons and hands. Global and per-model controls are mutually exclusive. skeeto_viewmodel_mirror 0|1|toggle selects global mode and can be bound in Keyboard / Mouse settings. If two viewmodels share weapon script data but request opposite per-model mirror states, mirroring pauses for that pair to keep bones and face culling aligned.

Skeetoscript 的“隐藏角色挂件模型”默认总开关及分类开关均关闭，可按需隐藏药品、医疗包 / 电击器、投掷物与 Smoker 舌头挂件。
Hide Items on the Back in Skeetoscript has its master and category switches disabled by default. Choose medicine, medkits / defibrillators, throwables or Smoker tongue accessories as needed.

默认预设的快速测试图入口保留专家难度，出生后自动给予小刀、消音冲锋枪、医疗包、止痛药、燃烧瓶和冲锋枪激光升级，并开启 sv_cheats。此行为只适用于这些本地测试图入口。
Quick test maps in the default preset retain Expert difficulty, give a knife, silenced SMG, medkit, pills, Molotov and SMG laser sights after spawning, and enable sv_cheats. This behavior applies only to these local test map entries.


自定义局内 ESC 菜单
------------------

在 Skeeto 菜单的“自定义游戏UI按钮”中，分别设置自定义主菜单 UI 和自定义 ESC 菜单 UI。ESC 模块默认关闭，进入地图后使用；可自定义按钮、文字、滑块、飞出子菜单、服务器/地图信息和时钟，并提供 ConVar 数值预设、原生投票与完整选项入口。默认布局左右两列，中间留空。

编辑器支持中文输入、RGB 文字色、按钮底色与透明度、Ctrl 多选、整体移动、对齐/统一尺寸/等间距和吸附。详细教程：Skeeto\DIY\ESCMenu_ESC菜单UI.txt。ESC 布局、主菜单布局和自定义 HUD 方案互不通用。布局和数值预设保存在 left4dead2\skeeto\escmenu\，不会随安装包替换你的个人设置。

Configure Custom main menu UI and Custom ESC menu UI under Custom game UI buttons. The ESC module is disabled by default and used after entering a map. It supports buttons, text, sliders, flyouts, server/map information, clocks, ConVar value presets, native voting and full settings. The default layout leaves the center clear.

The editor supports RGB text colors, backgrounds/transparency, Ctrl-click selection, group movement, alignment, matching sizes, distribution and snapping. See Skeeto\DIY\ESCMenu_ESC菜单UI.txt. ESC layouts, main-menu layouts and custom HUD profiles are separate. Personal layouts and value presets are stored under left4dead2\skeeto\escmenu\ and are not replaced by this package.


工坊 JSON 补救与原生界面样式
---------------------------

命中反馈设置中的“工坊素材延迟补救”默认关闭。开启后会等待原生素材挂载完成，再分批补读遗漏的 JSON，不会强制重挂模组；已连接服务器时原生挂载仍可能推迟。素材包经常挂载偏晚时，仍建议放入 addons。它是资源发现补救选项，不代表已证明加载提速。
Workshop material recovery in Hit Feedback is disabled by default. It waits for native mounting and incrementally reads missing JSON without forcing addon remounts. Native mounting can still be deferred while connected. Moving persistently late material packs into addons remains recommended; this is resource recovery, not a proven loading-speed improvement.

在“自定义游戏UI按钮”内进入自定义聊天框、控制台面板设置，两项分别开关，默认关闭。聊天框可调整字体、停留时间、颜色、输入时背景、位置和大小；控制台可调整字体、回显与开发输出、背景和标题栏颜色。原生输入、补全、滚动与历史记录继续保留。精确 RGB 等参数见 Skeeto\Commands.txt。
Open the chat and console appearance settings under Custom game UI buttons. Both are independently disabled by default. Chat supports font, lifetime, colors, input-time backgrounds, position and size; console supports font, output, background and title-bar colors. Native input, completion, scrolling and history are retained. See Skeeto\Commands.txt for precise RGB parameters.


Skeeto 插件平台
---------------

平台默认关闭。将兼容 Skeeto Plugin API 的 Windows x86 插件入口 DLL 放入 bin\skeeto\plugins\，在 Skeeto插件平台中打开“启动时加载插件”，再完全退出并重启游戏。开关从下次完整启动生效，不会在当前进程强行卸载插件。插件独立管理自己的设置，安装包不附带或自动开启第三方插件。
The platform is disabled by default. Place compatible Windows x86 Skeeto Plugin API entry DLLs in bin\skeeto\plugins\, enable startup loading in Skeeto Plugin Platform, and fully restart the game. The switch applies on the next process start; loaded plugins are not forcibly unloaded. Plugins manage their own settings, and no third-party plugin is bundled or enabled.

玩家安装教程：Skeeto\DIY\Plugins_插件平台.txt。作者开发资料：Skeeto\PluginSDK\，包含公共头文件与 HelloMenu 独立编译示例。普通玩家不需要编译器；L4N、SourceMod 插件和普通代理 DLL 不能直接放进来使用。
Player guide: Skeeto\DIY\Plugins_插件平台.txt. Author SDK: Skeeto\PluginSDK\, including the public header and an independently buildable HelloMenu sample. Players do not need a compiler; L4N/SourceMod plugins and ordinary proxy DLLs are not directly compatible.
