"""Capture/compare local Skeeto evidence. Never builds, restores, stages or deletes.

Python 3.10+, Git, standard library only. Capture writes one new state.json;
compare and list are read-only. Hashes identify content, not task ownership or
runtime correctness. This is evidence collection, not a source-code backup.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path


SCHEMA = 1
STORE = ".codex/handoffs"
DEFAULT_FOCUS = ("AGENTS.md", "docs/START_HERE.txt", "VERSION.txt")
MAX_JSON_BYTES = 2 * 1024 * 1024


def git(root, *args, allowed=(0,)):
    env = dict(os.environ, GIT_OPTIONAL_LOCKS="0")
    result = subprocess.run(
        ["git", "-c", "core.quotepath=false", "-C", str(root), *args],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, timeout=20,
        shell=False,
    )
    if result.returncode not in allowed:
        # Do not echo arbitrary config, URLs, credentials or user file contents.
        raise RuntimeError(f"Git {args[0]} failed (exit {result.returncode}); verify the selected repository.")
    if len(result.stdout) > 4 * 1024 * 1024:
        raise RuntimeError("Git output exceeds the evidence budget; use a narrower worktree or manual collection.")
    return result.stdout


def repo_root(root):
    resolved = Path(git(Path(root).resolve(), "rev-parse", "--show-toplevel").decode("utf-8").strip()).resolve()
    markers = (resolved / "src/skeeto", resolved / "docs/START_HERE.txt", resolved / "VERSION.txt")
    if not (resolved / "AGENTS.md").is_file() or sum(p.exists() for p in markers) < 2:
        raise ValueError("Selected root does not look like the Skeeto workspace; check --root.")
    return resolved


def relative_path(root, value):
    path = Path(value)
    # Permit absolute paths only when they designate this selected repository.
    if not path.is_absolute():
        if ".." in path.parts:
            raise ValueError(f"Parent traversal is not allowed: {value}")
        path = root / path
    lexical = Path(os.path.abspath(path))
    if not lexical.is_relative_to(root):
        raise ValueError("Focus path is outside the selected workspace.")
    if not lexical.parent.resolve().is_relative_to(root):
        raise ValueError("Focus parent resolves outside the selected workspace.")
    if not lexical.is_symlink() and not lexical.resolve().is_relative_to(root):
        raise ValueError("Focus path resolves outside the selected workspace.")
    name = lexical.relative_to(root).as_posix()
    if not name or name == "." or len(name) > 1024:
        raise ValueError("Focus must designate a bounded repository-relative file path.")
    return name


def is_output(name):
    return name == STORE or name.startswith(STORE + "/")


def changes(root):
    chunks = git(root, "status", "--porcelain=v1", "-z", "--untracked-files=all").split(b"\0")
    rows, i = [], 0
    while i < len(chunks) and chunks[i]:
        row = chunks[i].decode("utf-8")
        i += 1
        if len(row) < 4 or row[2] != " ":
            raise ValueError("Cannot parse Git status; do not infer a clean workspace.")
        entry = {"xy": row[:2], "path": row[3:]}
        if "R" in entry["xy"] or "C" in entry["xy"]:
            if i >= len(chunks) or not chunks[i]:
                raise ValueError("Truncated rename/copy status.")
            entry["old_path"] = chunks[i].decode("utf-8")
            i += 1
        # A rename crossing out of/into the output store still changes source.
        if not is_output(entry["path"]) or (entry.get("old_path") and not is_output(entry["old_path"])):
            rows.append(entry)
    return sorted(rows, key=lambda row: (row["path"], row.get("old_path", "")))


def index_entries(root, names):
    entries = {}
    # Keep argv below Windows command-line limits; literal pathspecs prevent
    # brackets/globs in user filenames from selecting unrelated files.
    batches, batch, length = [], [], 0
    for name in names:
        arg = ":(literal)" + name
        if batch and length + len(arg) > 6000:
            batches.append(batch)
            batch, length = [], 0
        batch.append(arg)
        length += len(arg) + 4
    if batch:
        batches.append(batch)
    for batch in batches:
        for raw in git(root, "ls-files", "--stage", "-z", "--", *batch).split(b"\0"):
            if not raw:
                continue
            meta, name = raw.decode("utf-8").split("\t", 1)
            mode, blob, stage = meta.split()
            entries.setdefault(name, []).append({"mode": mode, "blob": blob, "stage": int(stage)})
    return entries


def fingerprint(root, name, file_limit, remaining):
    path = root / name
    item = {"path": name}
    try:
        if path.is_symlink():
            item.update(kind="symlink", sha256=hashlib.sha256(os.readlink(path).encode("utf-8")).hexdigest())
            return item, 0
        if not path.resolve().is_relative_to(root):
            item["kind"] = "outside-root"
            return item, 0
        if not path.exists():
            item["kind"] = "missing"
            return item, 0
        if not path.is_file():
            item["kind"] = "not-a-file"
            return item, 0
        before = path.stat()
        if before.st_size > file_limit:
            item.update(kind="oversize", size=before.st_size)
            return item, 0
        if before.st_size > remaining:
            item.update(kind="budget-exceeded", size=before.st_size)
            return item, 0
        digest, read_bytes = hashlib.sha256(), 0
        with path.open("rb") as stream:
            while True:
                block = stream.read(min(1024 * 1024, file_limit - read_bytes + 1, remaining - read_bytes + 1))
                if not block:
                    break
                read_bytes += len(block)
                if read_bytes > file_limit or read_bytes > remaining:
                    item.update(kind="changed-during-read", size=read_bytes)
                    return item, read_bytes
                digest.update(block)
        after = path.stat()
        if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
            item["kind"] = "changed-during-read"
        else:
            item.update(kind="file", size=read_bytes, sha256=digest.hexdigest())
        return item, read_bytes
    except OSError:
        item["kind"] = "unreadable"
        return item, 0


def head(root):
    return git(root, "rev-parse", "--verify", "HEAD", allowed=(0, 128)).decode("utf-8").strip() or None


def collect(root, focus=(), max_files=256, max_file_bytes=16 * 1024 * 1024, max_total_bytes=64 * 1024 * 1024):
    if not 1 <= max_files <= 512 or not 1 <= max_file_bytes <= 64 * 1024 * 1024 or not 1 <= max_total_bytes <= 256 * 1024 * 1024:
        raise ValueError("Evidence limits are outside the supported bounds.")
    root = repo_root(root)
    focus = sorted(set(relative_path(root, value) for value in (*DEFAULT_FOCUS, *focus)))
    if len(focus) > max_files:
        raise ValueError("Too many focus files for the evidence budget.")
    old_head, old_changes = head(root), changes(root)
    branch = git(root, "symbolic-ref", "--quiet", "--short", "HEAD", allowed=(0, 1)).decode("utf-8").strip() or None
    names = set(focus)
    for row in old_changes:
        names.add(relative_path(root, row["path"]))
        if row.get("old_path"):
            names.add(relative_path(root, row["old_path"]))
    ordered = focus + sorted(names - set(focus))
    warnings = []
    if old_head is None:
        warnings.append("HEAD is unavailable (unborn branch or unreadable commit); history remains unverified.")
    if len(ordered) > max_files or len(old_changes) > max_files:
        warnings.append("File/status list truncated by max_files; unlisted content remains unknown.")
    ordered = ordered[:max_files]
    old_index = index_entries(root, ordered)
    files, consumed = [], 0
    for name in ordered:
        item, count = fingerprint(root, name, max_file_bytes, max(0, max_total_bytes - consumed))
        consumed += count
        item["index"] = old_index.get(name, [])
        files.append(item)
        if item["kind"] not in ("file", "symlink", "missing"):
            warnings.append(f"Incomplete file evidence: {name} ({item['kind']}).")
    if old_head != head(root) or old_changes != changes(root) or old_index != index_entries(root, ordered):
        warnings.append("Git state changed during capture; this snapshot cannot be treated as consistent.")
    # The version is descriptive. Content hashes remain the comparison evidence.
    version_file = root / "VERSION.txt"
    version = None
    if version_file.is_file() and version_file.stat().st_size <= 1024:
        version = version_file.read_text(encoding="utf-8-sig").strip()
    return {
        "schema_version": SCHEMA,
        "captured_at": datetime.now(timezone.utc).isoformat(),
        "root_hint": str(root), "branch": branch, "head": old_head,
        "version": version, "focus": focus,
        "changes": old_changes[:max_files], "change_count": len(old_changes),
        "files": files, "complete": not warnings, "warnings": warnings,
        "limits": {"max_files": max_files, "max_file_bytes": max_file_bytes, "max_total_bytes": max_total_bytes},
    }


def compare(saved, root):
    if not isinstance(saved, dict) or saved.get("schema_version") != SCHEMA:
        raise ValueError("Unsupported snapshot schema; inspect manually instead of declaring a match.")
    if not isinstance(saved.get("files"), list) or len(saved["files"]) > 512:
        raise ValueError("Invalid snapshot file evidence.")
    root = repo_root(root)
    names = [relative_path(root, row["path"]) for row in saved["files"]]
    limits = saved.get("limits", {})
    if set(limits) != {"max_files", "max_file_bytes", "max_total_bytes"}:
        raise ValueError("Invalid snapshot limits.")
    if not isinstance(saved.get("complete"), bool) or not isinstance(saved.get("changes"), list):
        raise ValueError("Invalid snapshot consistency evidence.")
    current = collect(root, focus=names, **limits)
    old_files = {row["path"]: row for row in saved["files"]}
    new_files = {row["path"]: row for row in current["files"]}
    # Capture times, mtimes and root_hint are not evidence of changed content.
    changed = sorted(name for name, old in old_files.items()
                     if {k: v for k, v in old.items() if k != "index"} !=
                     {k: v for k, v in new_files.get(name, {}).items() if k != "index"})
    staged = sorted(name for name, old in old_files.items()
                    if old.get("index") != new_files.get(name, {}).get("index"))
    metadata = {key: {"before": saved.get(key), "now": current.get(key)}
                for key in ("head", "branch", "version") if saved.get(key) != current.get(key)}
    status_changed = saved["changes"] != current["changes"] or saved.get("change_count") != current["change_count"]
    incomplete = not saved["complete"] or not current["complete"]
    result = "incomplete" if incomplete else ("drift" if changed or staged or metadata or status_changed else "match")
    return {"result": result, "metadata_changed": metadata, "status_changed": status_changed,
            "files_changed": changed, "index_changed": staged,
            "warnings": saved.get("warnings", []) + current["warnings"],
            "meaning": "Content identity only; this does not validate permissions, ownership, build or game behavior."}


def read_state(path):
    path = Path(path)
    if path.stat().st_size > MAX_JSON_BYTES:
        raise ValueError("Snapshot exceeds the supported JSON size.")
    state = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(state, dict):
        raise ValueError("Snapshot must be a JSON object.")
    return state


def capture(root, topic, focus):
    root = repo_root(root)
    state = collect(root, focus=focus)
    # No user-controlled output path, replacement of existing notes, or deletion.
    slug = re.sub(r"[^\w-]+", "-", topic, flags=re.UNICODE).strip("-_")[:48] or "skeeto"
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    folder = root / STORE / f"{timestamp}-{slug}-{uuid.uuid4().hex[:8]}"
    if not folder.resolve().is_relative_to(root):
        raise ValueError("Handoff store resolves outside the workspace.")
    folder.mkdir(parents=True, exist_ok=False)
    state["topic"] = topic[:256]
    state_path = folder / "state.json"
    with state_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(state, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    return {"state_path": str(state_path), "relative_folder": folder.relative_to(root).as_posix(),
            "complete": state["complete"], "warnings": state["warnings"]}


def list_states(root, limit):
    root = repo_root(root)
    if not 1 <= limit <= 50:
        raise ValueError("List limit must be 1-50.")
    store = root / STORE
    if not store.resolve().is_relative_to(root):
        raise ValueError("Handoff store resolves outside the workspace.")
    candidates = sorted(store.glob("*/state.json"), reverse=True)[:200]
    rows = []
    for path in candidates:
        try:
            if not path.resolve().is_relative_to(root):
                continue
            state = read_state(path)
            rows.append({"folder": path.parent.relative_to(root).as_posix(),
                         "topic": state.get("topic"), "captured_at": state.get("captured_at"),
                         "branch": state.get("branch"), "head": state.get("head"),
                         "has_handoff": (path.parent / "handoff.md").is_file(),
                         "complete": state.get("complete")})
        except (OSError, ValueError):
            rows.append({"folder": path.parent.relative_to(root).as_posix(), "error": "Unreadable snapshot; inspect manually."})
        if len(rows) >= limit:
            break
    return {"candidates": rows, "note": "Newest is not automatically the correct task or worktree."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    cap = sub.add_parser("capture", help="Create only a new local state.json; fill handoff.md separately.")
    cap.add_argument("--root", default=".")
    cap.add_argument("--topic", required=True)
    cap.add_argument("--focus", action="append", default=[], help="Exact task file; repeat as needed, including local evidence.")
    cmp = sub.add_parser("compare", help="Read-only content/status comparison against the selected current workspace.")
    cmp.add_argument("state")
    cmp.add_argument("--root", default=".")
    lst = sub.add_parser("list", help="List a bounded set of candidate snapshots without choosing a task.")
    lst.add_argument("--root", default=".")
    lst.add_argument("--limit", type=int, default=10)
    args = parser.parse_args()
    try:
        if args.command == "capture":
            result = capture(args.root, args.topic, args.focus)
        elif args.command == "compare":
            result = compare(read_state(args.state), args.root)
        else:
            result = list_states(args.root, args.limit)
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 2 if result.get("result") == "incomplete" or result.get("complete") is False else 0
    except (OSError, ValueError, TypeError, KeyError, subprocess.SubprocessError) as exc:
        print(json.dumps({"error": str(exc), "meaning": "Evidence unavailable; do not infer success or a clean state."}, ensure_ascii=False))
        return 2
    except RuntimeError as exc:
        print(json.dumps({"error": str(exc)}, ensure_ascii=False))
        return 2


if __name__ == "__main__":
    sys.exit(main())
