# Skeeto Plugin SDK 1.0

这是可选的客户端 DLL 加载与菜单接口，面向 Windows x86 / MSVC。插件依据 **Plugin API 1.0** 开发，不绑定某个 Skeeto 产品版本。

## 玩家：安装与使用已有插件

1. 确认作者提供的是 **Skeeto Plugin API 1.x、Windows x86 DLL**。L4N 插件、SourceMod 插件或普通代理 DLL 不能直接放进来使用。
2. 完全退出游戏，把插件入口 DLL 放进游戏目录的 `bin\skeeto\plugins\`。如果作者提供带目录的完整包，按其说明合并目录，保留私有依赖。
3. 在 Skeeto 根菜单进入 **Skeeto插件平台**，打开 **启动时加载插件**，完全退出并重新启动游戏。只改开关不会在当前进程卸载/重新加载 DLL。
4. 回到平台列表，查看实际装载状态，选中插件进入自己的设置页。有些插件不提供菜单，只列出名称、版本和状态。
5. 数字键操作；数值项先选中，再用 `-/+` 调整；`R` 刷新当前插件页；`0` 返回；方向键翻页。

更新或删除插件前也应完全退出游戏。平台不自动下载插件，没有插件市场或热重载。平台默认关闭；放入 DLL 本身不会绕过这个总开关。

控制台也可用 `skeeto_plugins open` 打开平台；`skeeto_plugins status` 查看本进程的加载与错误结果。平台菜单以后可以移动位置，这两个命令和插件接入协议不依赖它在根菜单的页码/序号。

## 开发者：头文件与可编译示例

- `include/skeeto_plugin_api.h`：公开 ABI 头文件，相当于开发入口；编译插件时包含它，不在游戏运行时加载这个 `.h`。
- `examples/HelloMenu/plugin.cpp`：完整可编译示例，演示全部六种菜单项和子页。
- `examples/HelloMenu/build.bat`：独立 x86 构建入口；无需链接 Skeeto 私有库。
- 本文：安装、菜单接入、设置保存、线程和兼容契约。

### 第一次构建

1. 安装 Visual Studio C++ Build Tools（含 x86 工具）。整个 `plugin_sdk` 目录可复制到其他位置使用，不依赖 Skeeto 私有头或仓库工具。
2. 执行 `examples\HelloMenu\build.bat`，产物为同目录 `out\HelloMenu.dll`；可传第一个参数指定输出目录。不会自动安装或运行 DLL。
3. 将入口 DLL 手动放到游戏 `bin\skeeto\plugins\`。顶层只放入口 DLL；私有依赖可按 DLL 的正常依赖规则放置，平台不递归扫描子目录，不修改全局 DLL 搜索目录。
4. Skeeto 根菜单 → Skeeto插件平台 → 启动时加载插件，开启后**完全退出并重新启动游戏**。
5. 列表显示本进程实际结果，点击插件进入页面；数字键操作，数值项先选中再 `-/+`，R 刷新，0 返回，方向键翻页。

HelloMenu 展示状态、动作、开关、三档选择、0～1/0.01 数值和子菜单。仅操作示例内存，不改游戏音量、HUD 或玩法，也不写配置；重新启动后恢复示例默认值。

### 菜单怎样接入

插件不调用 Skeeto 私有菜单函数，也不需要知道平台当前在哪一页、第几个选项。宿主通过公开回调向插件请求自己的菜单数据：

1. DLL 导出 `SkeetoPlugin_Query`，填写稳定的插件 `id`、显示名、版本，以及回调指针。
2. `on_host_ready` 收到 API 信息和插件路径，进行必要的轻量就绪处理。
3. 玩家点击插件入口后，宿主调用 `build_page(context, 0, page)`，请求插件根页。
4. 插件填入宿主给定的 `page->items`：文字、开关、选择项、数值项、动作或子菜单。**界面绘制、编号、翻页、返回和按键由 Skeeto 负责。**
5. 玩家操作时，宿主调用 `on_action(context, page_id, item_id, value)`；插件修改自己的状态。返回成功后宿主重新请求页面，显示实际状态。

子菜单只需把条目类型设为 `SKT_MENU_SUBMENU` 并填写自己的 `page_id`，再在 `build_page` 中处理这个页号。`HelloMenu` 已有完整实例，可直接以它为起点改名和增加功能。

这是插件自己的页面入口，不是开放全部 Skeeto 内置设置或菜单内部指针。公开协议不包含宿主页码；移动平台入口不会要求作者重编译。插件ID用于独立数据目录，发布后应保持稳定，不能为了改显示名一起改ID。ID使用1～47个ASCII字母、数字、点、下划线或连字符，不可用点结尾；重复ID按Windows路径规则忽略大小写，以免两个插件共用配置目录。

## 设置保存在哪里

**插件设置不写入 Skeeto 的 `skeeto_setting.ini`。** 该文件只保存宿主自己的设置，包括平台启动总开关 `plugin_platform`。

`SktHostApi::data_directory_utf8` 提供插件独立数据目录，例如插件ID `author.example` 对应：

```text
游戏目录/
  bin/skeeto/plugins/Example.dll
  left4dead2/skeeto/plugins/author.example/settings.ini
```

目录按稳定ID划分，不按菜单位置、DLL文件名或Skeeto产品版本划分。`settings.ini` 是建议文件名，插件也可以在自己的目录下使用JSON等格式。宿主只提供路径，**不会替插件自动保存开关/数值，也不会自动创建目录**；作者负责读写自己的配置、公布保存失败状态，并避免覆盖别的插件/宿主数据。

文件IO应交给插件自己的后台任务，菜单/Paint回调仅提交设置快照，不能同步读盘。`data_directory_utf8` 是UTF-8，使用Windows宽字符文件API前应正确转换；不要将其直接当系统ANSI编码。

`HelloMenu` 特意只演示菜单，设置仅在内存中、重启恢复默认。另行适配的 Status HUD 插件实现了独立保存，其配置为 `left4dead2/skeeto/plugins/skeeto.status_hud/settings.ini`；这个插件不属于本最小SDK的必需依赖。

## 平台与其他 Skeeto 功能的边界

- 插件平台默认关闭：启动不扫描目录、不装载第三方DLL、不执行插件回调；共享Paint路径保留固定成本的门控，不启动专属轮询或线程。
- 开启后只在核心正常就绪后发现一次，最多处理32个入口。装载由现有worker完成，不增加后台目录监视。
- 菜单使用独立的页面/导航状态。没有待初始化或交互请求时不回调插件；页面不是每帧重新构建。
- 有限长的页面/条目校验和回调非零返回值会显示该插件失败，不要求核心其他模块一起关闭。
- **这些边界不等于原生代码沙箱。** 插件与游戏/Skeeto在同一进程；DLL初始化、任意内存写、Hook、异常或阻塞回调仍可能影响整个游戏。宿主不能可靠中断任意native代码，也不能保证未知插件绝不影响其他功能。只使用可信插件，作者应遵守下面的线程、成本和生命周期约定。

## 接口与所有权

唯一必需导出名称为 `SkeetoPlugin_Query`，原型见 `include/skeeto_plugin_api.h`。使用 `extern "C"`、`__cdecl`、MSVC x86、默认 `/Zp8`；不要改函数原型、结构顺序或对齐。示例使用 `__declspec(dllexport)`，可用 `dumpbin /exports` 核对未修饰符号名。

宿主传入自己的 API major/minor 和可写结构容量；插件检查容量，只填写自己理解的前缀，写回自身结构大小和所需 API。major 必须相同，所需 minor 不能高于宿主。API 1.0 的 `SktPluginInfo`、`SktHostApi`、`SktMenuPage`、`SktMenuItem` 在 x86 MSVC 下大小分别为 44、16、28、80 字节。描述/宿主表的兼容扩展只能追加且保留旧前缀；**API 1.x 的 SktMenuItem 及其 80 字节数组步长冻结，不能直接追加条目字段**。需要新的条目布局时另起 API major 或单独设计扩展服务。不按 Skeeto semver 绑定插件。

Query 在装载 worker 上调用，负责返回元数据/函数指针；允许一次有明确大小上限的本机配置读取，**不允许目录扫描、联网、等待后台任务，或调用 Source、VGUI、渲染接口**。读取字节数有界不代表磁盘耗时有硬上限，避免把大文件或批量初始化塞进 Query。插件 DllMain 只做必要的模块记录；不从这里读配置、装载游戏后端、等待线程或访问引擎。装载发生在核心正常就绪之后，宿主不提供 D3D/Hook/设备加载通知。

OnHostReady、BuildPage、OnAction 在已确认的 Paint/game 线程串行调用，必须短小、不可阻塞、不可抛出 C++ 异常。需要文件 IO 或耗时任务时由插件安排自身工作；不能在 Paint 回调里同步扫描或联网。宿主没有沙箱，也不会强行中断运行中的原生回调。

OnHostReady 可读取宿主版本和路径。两个路径字符串由宿主持有，持续到进程结束。`data_directory_utf8` 是建议的独立数据目录 `left4dead2\skeeto\plugins\<id>`；宿主不创建它，插件自管配置、目录和线程，不能修改 Skeeto 的 ini 或核心状态。

BuildPage 接收宿主提供的 `items` 与 `capacity`；不要更换它们。写回 title/count 和条目，每项填写 size/id/type，标签和状态为 UTF-8（只有一门语言时另一项可为空指针，使用已有语言）。字符串在回调返回后立即复制；不要返回指向已经离开作用域的栈内存。普通静态字符串、插件持有的字符串均可。

页面 ID 由插件定义，根页固定为 0；条目 ID 同页非零且唯一。每页模型最多 64 项，宿主每屏 9 项、子菜单最多 8 层。中文/英文标签、标题上限 95 个 UTF-16 单元，状态文案 127，选择文案 255；过长、非法 UTF-8 和重复 ID 拒绝整个页面并显示错误。

- INFO：只显示 text，不触发 OnAction。
- ACTION：OnAction 收到 value=0。
- TOGGLE：value 必须为 0/1，操作传入新的值。
- CHOICE：choices 用 `|` 分隔，不允许空选项，最多 32 项；value 为从 0 开始的整数索引。
- NUMBER：value/min_value/max_value/step 必须有限，范围有序且当前值在范围内，step>0；宿主按步长增减并限制范围。
- SUBMENU：page_id 指向插件自己的页面；返回由宿主负责。
- flags 的 `SKT_MENU_DISABLED` 禁止交互。

OnAction 返回 0 后重新 BuildPage，读取插件真实状态；不要假定宿主把提交值直接存成插件配置。没有每帧或自动计时刷新，只有进入、操作、语言变化和 R/refresh 请求生成页面；测量和绘制从不调用插件。

每个回调返回 0 表示成功；非零状态或非法描述显示失败并停止该插件的菜单调用，其他插件/核心初始化不受其返回值影响。任意 native DLL 仍可自行造成进程崩溃/死锁；这不属于宿主可隔离的错误。首次 Query 已发生在 LoadLibrary 之后，接口不兼容也不意味着插件 DllMain 从未执行。

## 装载与退出

Skeeto插件平台默认关闭，需要在Skeeto菜单里手动打开。关闭时不扫描、不装载、不回调。启用后每进程只发现一次，最多处理 32 个入口，不监视目录变化，不自动下载或重试。为启动时装载开关重新启动即可；已载入模块不会热卸载，初始化失败后也不擅自 FreeLibrary。

首版不提供退出或换图回调。不在游戏模块可能已卸载后补调清理，不在宿主 DllMain 等待插件。插件自行创建的线程、Hook 和资源由作者负责；本平台只解决加载入口共存，不管理 Hook 冲突。

## 检查与支持

`skeeto_plugins open` 打开平台；`skeeto_plugins status` 输出本进程状态、各插件结果和 loads/init/builds/actions 计数；`skeeto_plugins on/off` 修改下次启动设置；`skeeto_plugins refresh` 刷新当前插件页。默认不写专属日志或诊断文件。

装载失败的返回码是 Win32 错误；缺入口、API 不兼容、重复 ID、初始化失败和菜单失败分别显示。没有菜单回调的插件也可成功运行，列表保留其元数据。平台遵循现有 Skeeto 菜单的可用/焦点条件，不接管原生 ESC 或其他功能界面。

第三方作者负责其插件功能和兼容性。Skeeto 负责已公开的宿主 ABI、装载与菜单契约。发布时不要把示例默认装到普通玩家游戏中。
