# Skeeto

Left 4 Dead 2 客户端代理源码（Necola 轨注入 `necola.dll`，或独立轨注入 `skeeto.dll`）。

当前版本见 `VERSION.txt`。本仓库只含可编译的核心源码，不含玩家安装包、素材 VPK、Necola 原版与第三方参考工程。

## 编译

- Windows
- Visual Studio 2022（或 Build Tools）C++ 工作负载，**x86** 工具链

在 `src` 目录运行：

```bat
build.bat
```

成功后产物为 `dist/skeeto.dll`。Necola 轨把它拷到游戏根并命名为 `necola.dll`（原版 Necola 改名为 `necola_orig.dll`）；独立轨直接使用 `skeeto.dll`。

## 目录

```
src/skeeto_proxy.cpp     代理主逻辑
src/skeeto_style.cpp     DIY 样式目录
src/skeeto_style.h
src/skeeto_filesystem.h  引擎文件系统接口布局
src/build.bat            MSVC x86 编译
VERSION.txt
更新日志.txt
```

## 说明

菜单、命中反馈、自定义 HUD、本地听服优化等都在 `skeeto_proxy.cpp`。样式 JSON 与粒子/贴图不在本仓库，随安装包分发。
