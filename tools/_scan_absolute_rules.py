# -*- coding: utf-8 -*-
"""审核规则文件和源码注释中的绝对口吻；不参与构建或运行时。"""

from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
files = [root / "AGENTS.md", root / "docs" / "START_HERE.txt", root / "src" / "skeeto" / "core" / "skeeto_proxy.cpp"]
for p in (root / "src" / "skeeto").rglob("*"):
    if p.suffix.lower() in {".h", ".hpp", ".cpp", ".c"} and p != files[2]:
        files.append(p)

pat = re.compile(
    r"(禁止|严禁|永远|绝对|不得|勿因此|不要新增|不要为了|Never |Do not |DON'T |Forbidden|MUST NOT|must not|"
    r"No SaveSettings|No Steam|不得吞|不得为了|不要 kill|硬禁|一票否决|一律|决不要|千万不要|"
    r"Do NOT|NEVER |FORBIDDEN|不得杀|未明确授权|禁止反复)",
    re.I,
)

def is_commentish(line, path):
    s = line.strip()
    if path.suffix.lower() in {".md", ".txt"}:
        return True
    return s.startswith("//") or s.startswith("/*") or s.startswith("*") or " // " in line

rows = []
for path in files:
    if not path.exists():
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        text = path.read_text(encoding="gb18030")
    rel = str(path.relative_to(root)).replace("\\", "/")
    for i, line in enumerate(text.splitlines(), 1):
        if not pat.search(line):
            continue
        if path.suffix.lower() in {".cpp", ".c", ".h", ".hpp"} and not is_commentish(line, path):
            continue
        s = line.strip()
        if len(s) > 220:
            s = s[:220] + "..."
        rows.append((rel, i, s))

def is_main(rel):
    return rel in {"AGENTS.md", "docs/START_HERE.txt", "src/skeeto/core/skeeto_proxy.cpp"}

main = [r for r in rows if is_main(r[0])]
other = [r for r in rows if not is_main(r[0])]

# de-dup
seen=set(); main2=[]
for r in main:
    k=(r[0],r[1],r[2])
    if k in seen: continue
    seen.add(k); main2.append(r)

out=[]
out.append("# 绝对口吻死规则清单（审核用）")
out.append("")
out.append("扫自：`AGENTS.md`、`docs/START_HERE.txt`、`src/skeeto/**` 注释。")
out.append("目标：找出说得特别绝对、易被当成永久硬禁的条文，供你决定 **留 / 改软 / 删**。")
out.append("范围不只 ±attack：凡「禁止/不得/Never/Do not/一律…」类绝对口吻都进表。")
out.append("")
out.append("---")
out.append("")
out.append("## 0. 总览（先看这个）")
out.append("")
out.append("| 桶 | 含义 | 数量级 |")
out.append("| --- | --- | --- |")
out.append("| A 真边界 | 热路径/钩内安全/发版流程/技术事实 | 宜多留 |")
out.append("| B 场景禁令被读成全局 | 某条路径旁「Do not X」像全项目禁 X | **优先改软** |")
out.append("| C 历史吓阻 | 键鼠失灵等已修问题推成永久拒做 | **对齐「已修勿挡」或删吓阻语气** |")
out.append("| D 文档双口径 | 旧方案仍写死禁，新参考已改 | 删旧或归档 |")
out.append("")
out.append("**结论预告：** AGENTS **没有**「永远禁止 ClientCmd±attack」；真正点名 ±attack 的是 proxy 焦点恢复注释。更多绝对句是「某钩/热路径里禁止 SaveSettings/ClientCmd/磁盘」——精神对，但措辞容易被助手读成全局死规则。")
out.append("")
out.append("---")
out.append("")
out.append("## 1. AGENTS.md / START_HERE")
out.append("")
out.append("| 文件 | 行 | 原文 | 我标的桶 | 建议 |")
out.append("| --- | ---: | --- | --- | --- |")

agents_start = [r for r in main2 if r[0] in {"AGENTS.md", "docs/START_HERE.txt"}]
for rel,i,s in agents_start:
    bucket="A"
    advice="留"
    if "键鼠失灵" in s:
        bucket="C→已软"; advice="留（已写勿挡）"
    elif "ClientCmd" in s and "热路径" in s:
        bucket="A/B"; advice="留精神；可加「有节流/非钩内可用」以免读成全局禁 ClientCmd"
    elif "IN_KeyEvent" in s:
        bucket="A"; advice="留（钩内场景）"
    elif "禁止" in s or "不得" in s or "未明确" in s:
        bucket="A"; advice="留或略软"
    ss=s.replace("|","\\|")
    out.append(f"| `{rel}` | {i} | {ss} | {bucket} | {advice} |")

out.append("")
out.append("## 2. skeeto_proxy.cpp 绝对口吻注释")
out.append("")
out.append("| 行 | 原文 | 桶 | 建议 |")
out.append("| ---: | --- | --- | --- |")
for rel,i,s in main2:
    if r[0] if False else rel != "src/skeeto/core/skeeto_proxy.cpp":
        continue
    bucket="A"
    advice="留"
    low=s.lower()
    if "attack" in low and ("clientcmd" in low or "do not" in low):
        bucket="B"; advice="改软：限定「焦点恢复解粘」场景，勿像全局硬禁"
    elif "echo" in low:
        bucket="A"; advice="留（技术事实）"
    elif "cheat" in low and "clientcmd" in low:
        bucket="A"; advice="留（技术事实）"
    elif "no savesettings" in low or "no steam" in low or ("clientcmd" in low and ("paint" in low or "detour" in low or "wanted" in low)):
        bucket="A"; advice="留（热路径/钩内）"
    elif "never" in low or "do not" in low:
        bucket="B"; advice="核是否场景限定；过宽则改软"
    ss=s.replace("|","\\|")
    out.append(f"| {i} | {ss} | {bucket} | {advice} |")

out.append("")
out.append("## 3. 其它源文件注释命中（前 40）")
out.append("")
if not other:
    out.append("无。")
else:
    out.append("| 文件 | 行 | 原文 |")
    out.append("| --- | ---: | --- |")
    for rel,i,s in other[:40]:
        out.append(f"| `{rel}` | {i} | {s.replace('|','\\|')} |")
    if len(other)>40:
        out.append(f"| … | | 另有 {len(other)-40} 条 |")

out.append("")
out.append("## 4. 建议你优先拍板的几条（高信号）")
out.append("")
out.append("1. **proxy `InputFocusTick`：`Do not ClientCmd ±attack/±forward`** → 改成场景备注（你已否硬禁）。")
out.append("2. **AGENTS 热路径「不要新增无节流 ClientCmd」** → 留，但加一句「非热路径/有节流可为功能服务」。")
out.append("3. **IN_KeyEvent / Paint 钩内禁 ClientCmd+SaveSettings** → 留（真边界）。")
out.append("4. **键鼠失灵**（AGENTS/START_HERE）→ 已写「已修勿挡」，留。")
out.append("5. **旧稿 `docs/Skeeto_SemiAuto_HoldFire_初版方案.md`** → 删或归档，避免和中文实施方向双口径。")
out.append("6. **流程类**（未授权不 commit/不杀进程等）→ 留。")
out.append("")
out.append("你回：按条号写「留 / 改软（可给一句新措辞）/ 删」。我再动文件。")
out.append("")
out.append(f"统计：主三文件命中 {len(main2)}，其它文件 {len(other)}。")

path = root / "docs" / "_审核_绝对口吻死规则清单.md"
path.write_text("\n".join(out) + "\n", encoding="utf-8")
print("WROTE", path)
print("main", len(main2), "other", len(other))
print("---AGENTS/START---")
for rel,i,s in agents_start:
    print(f"{rel}:{i}: {s}")
print("---PROXY---")
for rel,i,s in main2:
    if rel.endswith("skeeto_proxy.cpp"):
        print(f"{i}: {s}")
