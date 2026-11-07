#pragma once
// Build-time catalog descriptions; names/order/targets remain in the menus.
// Generated catalog references these immutable Chinese strings, with English
// from the same UiId in skeeto_ui_en.inc.
struct MenuDiscoverySummary { int page; UiTextEntry text; };
static constexpr MenuDiscoverySummary kMenuDiscoverySummaries[] = {
    {kMenuPageHitFeedback,{UiId::FindAboutFeedback,L"为命中和击杀选择图像、声音等反馈，并分别配置不同目标。"}},
    {kMenuPageCrosshairHub,{UiId::FindAboutCrosshair,L"选择静态、动态或贴图准星，按需要调整外观。"}},
    {kMenuPageCustomHudHub,{UiId::FindAboutHud,L"选择屏幕上的信息组件，并调整显示位置与样式。"}},
    {kMenuPageOverlayHud,{UiId::FindAboutOverlay,L"以独立透明叠加层显示弹药、换弹和开火节奏。"}},
    {kMenuPageWorldHud,{UiId::FindAboutWorld,L"在人物或场景位置显示血条、名字、伤害与提醒。"}},
    {kMenuPageGameUi,{UiId::FindAboutInterface,L"调整主菜单、ESC 菜单、聊天框和控制台等游戏界面。"}},
    {kMenuPageSkeetoScript,{UiId::FindAboutScript,L"通过客户端提供部分提示、音效和玩法辅助。"}},
    {kMenuPageFastLoad,{UiId::FindAboutLoading,L"配置资源加载方式和可选加载实验；实验项目默认关闭。"}},
    {kMenuPageBackgroundEngine,{UiId::FindAboutBackground,L"选择主菜单背景图片或视频，并调整播放与显示方式。"}},
    {kMenuPageNavigation,{UiId::FindAboutNavigation,L"显示小地图、导航路线、安全屋方向和物资提示。"}},
    {kMenuPageSupplyRadar,{UiId::FindAboutRadar,L"在屏幕上显示附近物资，并按类别筛选和调整布局。"}},
    {kMenuPageViewModel,{UiId::FindAboutViewmodel,L"按模型分别调整持枪位置、角度、旋转中心和镜像。"}},
    {kMenuPageSoundVolume,{UiId::FindAboutSound,L"按开火、脚步、感染者等类别分别调节音量。"}},
    {kMenuPageCombatVisual,{UiId::FindAboutCombat,L"设置描边、镜头和战斗时的视听效果。"}},
    {kMenuPageClientConvenience,{UiId::FindAboutConvenience,L"设置闲置恢复、人机名字、过关执行配置等便利功能。"}},
    {kMenuPageCrashGuard,{UiId::FindAboutStability,L"选择防闪退措施，并查看各项措施的适用说明。"}},
    {kMenuPageNecolaStartup,{UiId::FindAboutNecola,L"控制游戏启动时是否联动加载 Necola。"}},
    {kMenuPageLocalListen,{UiId::FindAboutLocal,L"调整本地服务器玩法与测试功能；不作用于远端服务器。"}},
    {kMenuPageCareerArchive,{UiId::FindAboutCareer,L"查看 Steam 生涯统计、武器和模式等数据。"}},
    {kMenuPageMenuStyle,{UiId::FindAboutMenuStyle,L"调整 Skeeto 数字菜单的字体、配色、背景和尺寸。"}},
    {kMenuPageHelpDebug,{UiId::FindAboutHelp,L"查看操作说明、免责声明与相关帮助。"}},
    {kMenuPagePluginPlatform,{UiId::FindAboutPlugins,L"查看插件平台开关、已加载插件和插件菜单入口。"}},
    {kMenuPageChatUi,{UiId::FindAboutChat,L"调整聊天文字、背景、位置与消息停留时间。"}},
    {kMenuPageConsoleUi,{UiId::FindAboutConsole,L"调整控制台文字、背景、边框与颜色。"}},
    {kMenuPageRoot,{UiId::FindAboutChangelog,L"打开随 Skeeto 安装的版本更新说明。"}},
};
