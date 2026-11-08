"""Exercise handoff evidence against real temporary Git worktrees."""
import importlib.util
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("handoff_state.py")


def load_helper():
    spec = importlib.util.spec_from_file_location("handoff_state", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class HandoffStateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.helper = load_helper()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="skeeto-handoff-check-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "project with spaces"
        self.root.mkdir()
        self.git("init", "-q")
        self.git("config", "user.name", "Fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        for name, content in {
            "AGENTS.md": "Skeeto project rules\n",
            "VERSION.txt": "1.0.0\n",
            "docs/START_HERE.txt": "Project handoff entry\n",
            "src/skeeto/core/模块.cpp": "original\n",
            "src/skeeto/core/other.cpp": "unchanged\n",
            "src/skeeto/core/deleted.cpp": "will be deleted\n",
        }.items():
            self.write(name, content)
        self.git("add", ".")
        self.git("commit", "-qm", "fixture baseline")

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args])

    def write(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def collect(self, **kwargs):
        return self.helper.collect(self.root, **kwargs)

    def test_dirty_content_changes_without_new_status_are_detected(self):
        self.write("src/skeeto/core/模块.cpp", "first dirty version\n")
        state = self.collect()
        status = self.git("status", "--porcelain")
        self.write("src/skeeto/core/模块.cpp", "later dirty version\n")
        self.assertEqual(status, self.git("status", "--porcelain"))
        result = self.helper.compare(state, self.root)
        self.assertEqual("drift", result["result"])
        self.assertIn("src/skeeto/core/模块.cpp", result["files_changed"])

    def test_index_changes_are_detected_even_if_worktree_bytes_match(self):
        self.write("src/skeeto/core/模块.cpp", "staged one\n")
        self.git("add", "src/skeeto/core/模块.cpp")
        self.write("src/skeeto/core/模块.cpp", "working copy\n")
        state = self.collect()
        self.write("src/skeeto/core/模块.cpp", "staged two\n")
        self.git("add", "src/skeeto/core/模块.cpp")
        self.write("src/skeeto/core/模块.cpp", "working copy\n")
        result = self.helper.compare(state, self.root)
        self.assertEqual("drift", result["result"])
        self.assertIn("src/skeeto/core/模块.cpp", result["index_changed"])

    def test_focus_files_include_clean_ignored_and_missing_evidence(self):
        self.write(".gitignore", "local-evidence.txt\n")
        self.write("local-evidence.txt", "local facts\n")
        state = self.collect(focus=["src/skeeto/core/other.cpp", "local-evidence.txt", "missing.txt"])
        by_path = {f["path"]: f for f in state["files"]}
        self.assertEqual("missing", by_path["missing.txt"]["kind"])
        self.assertIn("sha256", by_path["local-evidence.txt"])
        self.write("src/skeeto/core/other.cpp", "now modified\n")
        self.assertIn("src/skeeto/core/other.cpp", self.helper.compare(state, self.root)["files_changed"])

    def test_unicode_rename_delete_and_untracked_are_preserved(self):
        self.git("mv", "src/skeeto/core/模块.cpp", "src/skeeto/core/改名 后.cpp")
        (self.root / "src/skeeto/core/deleted.cpp").unlink()
        self.write("待交接.txt", "user material\n")
        before = self.git("status", "--porcelain=v1", "-z")
        state = self.collect()
        renamed = next(c for c in state["changes"] if c["xy"].startswith("R"))
        self.assertEqual("src/skeeto/core/模块.cpp", renamed["old_path"])
        self.assertEqual("src/skeeto/core/改名 后.cpp", renamed["path"])
        self.assertTrue(any(c["path"] == "待交接.txt" and c["xy"] == "??" for c in state["changes"]))
        self.assertEqual(before, self.git("status", "--porcelain=v1", "-z"))
        self.assertEqual("match", self.helper.compare(state, self.root)["result"])

    def test_handoff_outputs_do_not_invalidate_their_own_snapshot(self):
        state = self.collect()
        self.write(".codex/handoffs/fixture/state.json", json.dumps(state))
        self.write(".codex/handoffs/fixture/handoff.md", "handoff evidence\n")
        self.assertEqual("match", self.helper.compare(state, self.root)["result"])

    def test_root_move_with_identical_evidence_is_not_content_drift(self):
        state = self.collect()
        moved = Path(self.temp.name) / "moved-project"
        shutil.copytree(self.root, moved)
        self.assertEqual("match", self.helper.compare(state, moved)["result"])

    def test_file_and_hash_limits_are_explicitly_incomplete(self):
        self.write("new-a.txt", "a" * 40)
        self.write("new-b.txt", "b" * 40)
        state = self.collect(max_files=3)
        self.assertFalse(state["complete"])
        self.assertEqual("incomplete", self.helper.compare(state, self.root)["result"])
        limited = self.collect(max_file_bytes=8, max_total_bytes=16)
        self.assertFalse(limited["complete"])
        self.assertTrue(limited["warnings"])

    def test_outside_root_focus_is_rejected(self):
        outside = Path(self.temp.name) / "private.txt"
        outside.write_text("must not be read", encoding="utf-8")
        with self.assertRaises(ValueError):
            self.collect(focus=[str(outside)])

    def test_unknown_schema_does_not_get_treated_as_matching(self):
        state = self.collect()
        state["schema_version"] = 999
        with self.assertRaises(ValueError):
            self.helper.compare(state, self.root)

    def test_source_rename_into_output_store_is_not_hidden(self):
        destination = self.root / ".codex/handoffs/fixture"
        destination.mkdir(parents=True)
        self.git("mv", "src/skeeto/core/other.cpp", ".codex/handoffs/fixture/other.cpp")
        state = self.collect()
        self.assertTrue(any(c.get("old_path") == "src/skeeto/core/other.cpp" for c in state["changes"]))

    def test_malformed_record_does_not_break_candidate_listing(self):
        self.write(".codex/handoffs/fixture/state.json", "[]")
        listed = self.helper.list_states(self.root, 10)
        self.assertIn("error", listed["candidates"][0])

    def test_user_file_contents_are_not_exported(self):
        secret = "never-export-this-password-716abc"
        self.write("user-config.txt", secret)
        state = self.collect()
        self.assertNotIn(secret, json.dumps(state))
        self.assertTrue(any(c["path"] == "user-config.txt" for c in state["changes"]))

    def test_commands_in_snapshot_are_never_executed(self):
        state = self.collect()
        marker = self.root / "should-not-exist.txt"
        state["next_command"] = 'python -c "open(\"should-not-exist.txt\",\"w\").write(\"bad\")"'
        self.assertEqual("match", self.helper.compare(state, self.root)["result"])
        self.assertFalse(marker.exists())

    def test_capture_command_only_writes_its_new_snapshot(self):
        before = self.git("status", "--porcelain=v1", "-z")
        output = subprocess.check_output([
            "python", str(SCRIPT), "capture", "--root", str(self.root),
            "--topic", "route-hud", "--focus", "src/skeeto/core/模块.cpp",
        ])
        result = json.loads(output)
        target = Path(result["state_path"])
        self.assertTrue(target.is_file())
        self.assertTrue(target.resolve().is_relative_to(self.root.resolve()))
        after = self.git("status", "--porcelain=v1", "-z")
        self.assertEqual(before, after.replace(b"?? .codex/\x00", b""))
        self.assertEqual("match", self.helper.compare(json.loads(target.read_text(encoding="utf-8")), self.root)["result"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
