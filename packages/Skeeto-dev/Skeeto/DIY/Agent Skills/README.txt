Skeeto 用户制作 Skill / User creation skill
==========================================

位置：DIY/Agent Skills/skeeto-hit-feedback/
这是给 AI agent 阅读的制作指南，不是游戏模组，安装 Skeeto 不需要把它复制到游戏目录。

使用方法：
1. 在支持 Skills 的 agent 中，按该工具的说明安装整个 skeeto-hit-feedback 文件夹。
   例如 Codex 可放入你的用户目录 .agents/skills/，重新打开会话后调用 $skeeto-hit-feedback。
2. 也可以直接让 agent 阅读文件夹中的 SKILL.md，并按需读取 references 和 scripts。
3. 提供你的目标、现有素材，以及 Skeeto 版本，例如：
   “用这个 Skill 制作普感和特感的击杀反馈。图案放在准星右侧，动画约 0.6 秒，
    保持原素材清晰度，交付可安装素材包。”

涵盖：图片与动画设计、透明度处理、预合成、VMT/VTF 转换和定位、音效、粒子、
素材体积优化、JSON 事件配置、离线校验与游戏内验收。无需 Skeeto 源码。
Python 脚本只用于制作校验，游戏播放不依赖 Python。素材制作可能需要额外工具，
由 agent 根据任务选择。只做离线检查不能代替游戏内的画面、声音和粒子确认。

English:
The skill is in DIY/Agent Skills/skeeto-hit-feedback/. It is an agent guide,
not a game mod. Install the complete folder using your agent's skill instructions,
or ask the agent to read SKILL.md and its linked resources. Provide your Skeeto
version, source assets, and intended result. Skeeto source code is not required.
It covers images, animation, VMT/VTF, audio, particles, asset optimization,
JSON configuration and validation. Python is only needed for the optional checker;
offline checks do not replace in-game visual and audio verification.
