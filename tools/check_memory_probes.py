#!/usr/bin/env python3
"""Skeeto 防回归：禁止在源码里新增未登记的 VirtualQuery（及等价）调用。

背景（docs/feature-plans/SKT P0 跨章节掉帧 - VirtualQuery 根治方案.md）：
VirtualQuery 的耗时随“同属性大区域的剩余大小”增长，热路径上逐字段调用会在大块连续
提交内存里变成约 100us/次。读内存统一走 src/skeeto/core/skeeto_memread.h：
受保护读（skmem::Probe/Copy/...）；确需页查询的冷路径用 skmem::Query，它会被
skeeto_memprobe 计数。

用法：
    python tools/check_memory_probes.py            # 扫描 src/skeeto，有未登记调用则退出码 1
    python tools/check_memory_probes.py --self-test

检测方式：先去掉注释、字符串和字符字面量（保留换行和长度），再按“标识符”查找，
所以跨行调用、宏别名、函数指针都会被发现；再用花括号跟踪确定外围函数。白名单按
（文件，函数名）登记并带次数上限；lambda 内的调用记为“函数::<lambda>”，不继承外围
函数的豁免。无法归属的调用（宏、文件作用域）不会被豁免。
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SCAN_DIR = os.path.join(ROOT, "src", "skeeto")
SKIP_DIRS = {"obj", "build", "out"}
EXTS = (".cpp", ".h", ".inc", ".hpp", ".c")

# 整个文件豁免：封装页查询本身的头文件（Query、FaultFilter 的重挂 guard、冷路径范围检查）。
FILE_ALLOWED = {
    "src/skeeto/core/skeeto_memread.h",
}

# (相对路径, 外围函数名) -> (允许的最多调用处数, 理由)
ALLOWED = {
    ("src/skeeto/core/skeeto_proxy.cpp", "MemoryHudWorkerPump"):
        (1, "内存 HUD 的地址空间空闲区扫描（显式遍历区域，不是探测），默认关闭，只在既有 worker 上运行"),
    ("src/skeeto/core/skeeto_proxy.cpp", "CrashLooksLikeOurs"):
        (1, "崩溃过滤器，仅崩溃时执行一次"),
}

NAMES = ("VirtualQuery", "VirtualQueryEx", "NtQueryVirtualMemory", "ZwQueryVirtualMemory")
IDENT_RE = re.compile(r"\b(?:%s)\b" % "|".join(NAMES))
STRING_NAME_RE = re.compile(r"^(?:%s)$" % "|".join(NAMES))
LAMBDA_RE = re.compile(r"\[[^\[\]]*\]\s*(?:\([^()]*\))?\s*(?:mutable\s*)?(?:noexcept\s*)?"
                       r"(?:->\s*[\w:<>,\s\*&]+)?\s*$")
TYPE_KEYWORD_RE = re.compile(r"\b(?:namespace|struct|class|union|enum)\b|\bextern\b")


def sanitize(text):
    """去掉注释、字符串与字符字面量内容（换行和总长度不变）。返回 (干净文本, [(位置, 字符串内容)])。"""
    out = list(text)
    strings = []
    i, n = 0, len(text)

    def blank(a, b):
        for k in range(a, b):
            if out[k] not in "\r\n":
                out[k] = " "

    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            j = i
            while j < n and text[j] != "\n":
                # 行尾反斜杠续行
                if text[j] == "\\" and j + 1 < n and text[j + 1] in "\r\n":
                    j += 1
                    if text[j] == "\r" and j + 1 < n and text[j + 1] == "\n":
                        j += 1
                j += 1
            blank(i, j)
            i = j
        elif c == "/" and nxt == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
        elif c == '"':
            raw = i > 0 and text[i - 1] == "R"
            if raw:
                open_paren = text.find("(", i)
                delim = text[i + 1:open_paren] if open_paren > 0 else ""
                close = text.find(")" + delim + '"', open_paren)
                j = n if close < 0 else close + len(delim) + 2
                strings.append((i, text[open_paren + 1:close] if close > 0 else ""))
            else:
                j = i + 1
                while j < n and text[j] != '"' and text[j] != "\n":
                    j += 2 if text[j] == "\\" else 1
                strings.append((i, text[i + 1:j]))
                j = min(j + 1, n)
            blank(i + 1, max(i + 1, j - 1))
            i = j
        elif c == "'":
            prev = text[i - 1] if i > 0 else ""
            if prev.isalnum() and prev not in "uULR":  # 数字分隔符，如 1'000
                i += 1
                continue
            j = i + 1
            while j < n and text[j] != "'" and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            blank(i + 1, max(i + 1, j))
            i = min(j + 1, n)
        else:
            i += 1
    return "".join(out), strings


def preprocessor_mask(clean):
    """返回每个字符是否属于预处理行（含反斜杠续行）。"""
    mask = [False] * len(clean)
    pos = 0
    in_pp = False
    for line in clean.splitlines(keepends=True):
        starts = in_pp or line.lstrip().startswith("#")
        if starts:
            for k in range(pos, pos + len(line)):
                mask[k] = True
        stripped = line.rstrip("\r\n")
        in_pp = starts and stripped.endswith("\\")
        pos += len(line)
    return mask


def last_paren_group_name(header):
    """header 中最后一个深度 0 括号组前面的标识符；没有则 None。"""
    depth = 0
    last_open = -1
    for k, ch in enumerate(header):
        if ch == "(":
            if depth == 0:
                last_open = k
            depth += 1
        elif ch == ")":
            depth = max(0, depth - 1)
    if last_open < 0:
        return None
    m = re.search(r"([A-Za-z_~][A-Za-z_0-9:~]*|operator\s*\S+)\s*$", header[:last_open])
    return m.group(1) if m else None


def classify_open_brace(header, in_function, parent_kind):
    h = " ".join(header.split())
    if in_function:
        return ("lambda", None) if LAMBDA_RE.search(h) else ("block", None)
    if parent_kind == "block":  # 文件作用域初始化列表里的子花括号
        return ("block", None)
    if "(" not in h and TYPE_KEYWORD_RE.search(h):
        return ("transparent", None)
    if "(" in h:
        name = last_paren_group_name(h)
        if name:
            return ("func", name)
    return ("block", None)


def scan_text(text):
    """返回 [(行号, 归属名)]。归属名为函数名、函数名::<lambda>、<preprocessor>、<dynamic lookup> 或 <file scope>。"""
    clean, strings = sanitize(text)
    pp = preprocessor_mask(clean)
    hits = []  # (position, forced_name or None)
    for m in IDENT_RE.finditer(clean):
        hits.append((m.start(), "<preprocessor>" if pp[m.start()] else None))
    for pos, content in strings:
        if STRING_NAME_RE.match(content.strip()):
            hits.append((pos, "<dynamic lookup>"))
    hits.sort()

    structural = "".join(" " if pp[k] and ch not in "\r\n" else ch for k, ch in enumerate(clean))
    result = []
    stack = []  # (kind, name)
    header_start = 0
    hi = 0
    line_starts = [0]
    for k, ch in enumerate(text):
        if ch == "\n":
            line_starts.append(k + 1)

    def line_of(pos):
        lo, hi_ = 0, len(line_starts) - 1
        while lo < hi_:
            mid = (lo + hi_ + 1) // 2
            if line_starts[mid] <= pos:
                lo = mid
            else:
                hi_ = mid - 1
        return lo + 1

    def attribution():
        func = None
        lam = False
        for kind, name in stack:
            if kind == "func" and func is None:
                func = name
            elif kind == "lambda" and func is not None:
                lam = True
        if func is None:
            return "<file scope>"
        return func + ("::<lambda>" if lam else "")

    n = len(structural)
    for k in range(n + 1):
        while hi < len(hits) and hits[hi][0] <= k:
            pos, forced = hits[hi]
            result.append((line_of(pos), forced or attribution()))
            hi += 1
        if k == n:
            break
        ch = structural[k]
        if ch == "{":
            in_function = any(kind == "func" for kind, _ in stack)
            parent = stack[-1][0] if stack else None
            stack.append(classify_open_brace(structural[header_start:k], in_function, parent))
            header_start = k + 1
        elif ch == "}":
            if stack:
                stack.pop()
            header_start = k + 1
        elif ch == ";":
            header_start = k + 1
    # 位置恰在文末之前的命中已在循环内处理；剩余的按当前归属记录。
    while hi < len(hits):
        pos, forced = hits[hi]
        result.append((line_of(pos), forced or attribution()))
        hi += 1
    return result


def evaluate(rel, hits, allowed):
    """返回 (问题列表, 已使用的白名单键集合)。"""
    problems = []
    used = {}
    for line_no, func in hits:
        key = (rel, func)
        limit = allowed.get(key, (0, ""))[0]
        used[key] = used.get(key, 0) + 1
        if used[key] > limit:
            if limit:
                problems.append("%s:%d %s() 里的 VirtualQuery 超过登记次数 %d" % (rel, line_no, func, limit))
            else:
                problems.append("%s:%d 在 %s 里使用 VirtualQuery 或等价接口" % (rel, line_no, func))
    return problems, set(k for k in used if k in allowed)


def scan_tree():
    problems = []
    used = set()
    for base, dirs, files in os.walk(SCAN_DIR):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for name in files:
            if not name.endswith(EXTS):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            if rel in FILE_ALLOWED:
                continue
            with open(path, "rb") as fh:
                text = fh.read().decode("utf-8", errors="replace")
            p, u = evaluate(rel, scan_text(text), ALLOWED)
            problems.extend(p)
            used |= u
    stale = sorted(set(ALLOWED) - used)
    return problems, stale


def self_test():
    def names(src):
        return [n for _, n in scan_text(src)]

    # 基本：函数归属、注释和字符串里的名字被忽略
    assert scan_text("static bool Hot(void* p) {\n  MEMORY_BASIC_INFORMATION m{};\n  return VirtualQuery(p, &m, sizeof(m)) != 0;\n}\n") == [(3, "Hot")]
    assert names("// VirtualQuery(\n/* VirtualQuery( */\nvoid F() { const char* s = \"VirtualQuery(x)\"; }\n") == []
    # 跨行：名字与括号分行
    assert names("void F() {\n  VirtualQuery\n    (p, &m, 1);\n}\n") == ["F"]
    # 同一行里的 https:// 不能把后面的真实调用当作注释
    assert names("void F() { const char* u = \"https://x\"; VirtualQuery(p, &m, 1); }\n") == ["F"]
    # 宏别名与函数指针
    assert names("#define VQ VirtualQuery\nvoid F() {}\n") == ["<preprocessor>"]
    assert names("void F() { auto q = &VirtualQuery; }\n") == ["F"]
    # 动态获取
    assert names("void F() { GetProcAddress(h, \"VirtualQuery\"); }\n") == ["<dynamic lookup>"]
    # 缩进的函数定义不会并入前一个函数
    src = "static void A() {\n  int x;\n}\n  static void Evil() {\n  VirtualQuery(p, &m, 1);\n}\n"
    assert names(src) == ["Evil"]
    # 函数前带 __declspec(...) 的定义取到真正的函数名
    assert names("__declspec(noinline) inline bool Probe(const void* p) {\n  VirtualQuery(p, 0, 1);\n}\n") == ["Probe"]
    # lambda 不继承外围函数的名字
    assert names("void Cold() {\n  auto f = [&](int x) {\n    VirtualQuery(p, &m, 1);\n  };\n}\n") == ["Cold::<lambda>"]
    # struct 内的成员函数、命名空间内的函数
    assert names("namespace n {\nstruct S {\n  bool M() {\n    VirtualQuery(p, &m, 1);\n  }\n};\n}\n") == ["M"]
    # 文件作用域
    assert names("static auto g = VirtualQuery;\n") == ["<file scope>"]
    # 数字分隔符不会让后面的整行被当成字符字面量
    assert names("void F() { int a = 1'000; VirtualQuery(p, &m, 1); }\n") == ["F"]

    # 白名单：登记次数是上限；超过、同名其他函数里的、lambda 里的都要报
    allowed = {("a.cpp", "Cold"): (1, "test")}
    ok, _ = evaluate("a.cpp", [(3, "Cold")], allowed)
    assert not ok
    over, _ = evaluate("a.cpp", [(3, "Cold"), (9, "Cold")], allowed)
    assert len(over) == 1
    lam, _ = evaluate("a.cpp", [(3, "Cold::<lambda>")], allowed)
    assert len(lam) == 1
    other, _ = evaluate("b.cpp", [(3, "Cold")], allowed)
    assert len(other) == 1

    # 真实源码：无未登记调用，白名单没有过期条目
    problems, stale = scan_tree()
    assert not problems, problems
    assert not stale, "白名单里有已不存在的条目：%s" % stale
    print("check_memory_probes self-test: ok")


def main():
    if "--self-test" in sys.argv:
        self_test()
        return 0
    problems, stale = scan_tree()
    for p in problems:
        print("ERROR:", p)
    for s in stale:
        print("WARN: 白名单条目已无对应调用，请清理：%s::%s" % s)
    if problems:
        print("发现 %d 处未登记的 VirtualQuery。热路径请改用 skeeto_memread.h；冷路径请用 skmem::Query，"
              "确需直接调用时在本脚本 ALLOWED 登记理由。" % len(problems))
        return 1
    print("check_memory_probes: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
