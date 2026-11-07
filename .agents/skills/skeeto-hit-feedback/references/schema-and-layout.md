# Style JSON and directory reference

This page covers the format and limits needed to build asset packs. Loose JSON and `skeeto/skeeto_*.json` inside a VPK both work on current builds; after packing, still confirm the style appears in the menu.

When rebuilding the catalog, loose JSON is scanned first, then JSON under `GAME` is enumerated. For the same ID, a later-loaded style overrides an earlier one. So when debugging a loose file while a VPK with the same ID is still mounted, the result may be the VPK version. Use a unique ID or isolate the old pack, and confirm the real menu style after a full game restart—do not assume file edits hot-reload.

## Files and paths

```text
<pack-root>/skeeto/skeeto_<style-name>.json
<pack-root>/materials/skeeto/<author-or-style>/<frame>.vmt
<pack-root>/materials/skeeto/<author-or-style>/<frame>.vtf
<pack-root>/sound/skeeto/<author-or-style>/<sound>.mp3   # or .wav
<pack-root>/particles/<own-particle>.pcf                 # only when particles are needed
```

`overlay` / `overlay_sequence` write `skeeto/<author-or-style>/<frame>` without `materials/`, `.vmt`, or `.vtf`. The VMT `$basetexture` points at an existing VTF, also without prefix or extension. `sound` / `sounds` are relative to `sound/` and keep the extension. Always use `/`; prefer ASCII path names. Avoid `..`, absolute paths, and material names that collide with other packs. Official preset assets may be referenced without copying, but declare the dependency and minimum requirement. One VTF may serve multiple VMTs or channel JSON files.

## Top-level fields and events

| Field | Meaning |
| --- | --- |
| `id` | Required, unique across the catalog; saved into user settings—avoid changing after publish. At most 47 UTF-8 bytes. Duplicate IDs override a previously loaded style. |
| `name` | Menu display name; at most 63 bytes (not 63 characters). Defaults to `id`. |
| `channel` | Required: `ci`, `si`, `ff`, or `fx`. One JSON belongs to one channel. |
| `sort` | Menu sort order; smaller first. |
| `streak` | `enabled`, `wrap`, `priority`; write only when streak tiers are truly needed. `wrap=0` does not wrap. |
| `hit`, `kill`, `headshot`, `melee` | Ordinary event blocks. Only non-empty resources form an effective result. **Hit and kill are separate event blocks**—do not treat kill-feed icons or kill-only timing as Hit rules. |
| `streak_1` … `streak_10`, `streak_default` | Streak tiers. Missed tiers fall back to default, then to `kill`; `headshot` / `melee` override streak effects only when priority is **strictly higher**. |
| Other top-level event objects | Named events. Mainly for existing `fx` trigger names (such as `skeet`, `crown`); inventing a new name does not invent a trigger. |

Skeeto builds with adjustable catalog capacity default to 64 styles and can switch 64 / 96 / 128 at the end of hit-feedback page 2; returning to the main menu reloads. Older builds still hard-cap at 32; confirm the user's version and menu option first. Count distinct style IDs across all channels; styles over the ceiling may not appear. Assets themselves need no JSON format change. Current preset packs already contain multiple styles—on delivery, check count and ID conflicts against the user's installed combination.

### Event-block fields

| Field | Limits and behavior |
| --- | --- |
| `overlay` | Single fullscreen overlay material path; at most 95 bytes. |
| `overlay_sequence` | 1–30 material paths, each at most 95 bytes; when present, preferred over a single `overlay`. |
| `overlay_step_ms` | Sequence interval; unset or 0 defaults to 40 ms; runtime clamps to 16–1000 ms and may shorten to fit a 5-second total window. Low FPS skips mid-frames that cannot keep up. |
| `overlay_ms` | Total display duration; 0/unset uses the event default; actual clamp 40–5000 ms; a sequence lasts at least through the last frame. `duration` and `overlay_duration` are aliases—use only one. Controls the overlay only, not particle lifetime. |
| `particle`, `particles` | Particle effect names; extra array at most 8 items, each at most 63 bytes, dispatched with the primary particle. Requires a loadable PCF/manifest. |
| `sound`, `sounds` | Paths relative to `sound/`; extra array at most 8 items, each at most 95 bytes. When `sounds` is set, one member (plus a non-empty `sound`) is chosen at random—**not played together**. |
| `priority` | Weight among same-event candidates. Ordinary `kill`/streak usually 10, `headshot` 100, `melee` 110; set for the intended override relationship—do not copy blindly. |
| `world` | `true` when the particle uses world coordinates; otherwise screen space. Named `fx` events dispatched through `FireFX` already place the primary particle at the victim's world coordinates. |

Lengths above are UTF-8 **bytes**. Named-event keys are at most 39 bytes and at most 24 keys. Unsupported JSON keys are skipped, so "file parses" does not mean a field takes effect. `overlay_pos`, `overlay_scale`, `fade_out_start_ms`, and `fade_out_duration_ms` are not parsed today—do not silently depend on them.

## Channel selection

- `ci`: common-infected hit/kill; usually `hit`, `kill`, `headshot`, `melee`. Streaks, when enabled, use the same counter.
- `si`: special-infected hit/kill and streaks. Current Skeeto may assemble effects with the selected `ci` style, especially under SI image/audio per-item switches; record both selections when testing so shared visuals are not misread as content of the `si` JSON.
- `ff`: friendly fire; current code picks `hit` when damage `<15` and `kill` when `>=15`—this does **not** mean a teammate was actually killed.
- `fx`: named special-operation events. Reuse only event names the current implementation calls; `particle` is the real effect name; `overlay` may be empty. `SpriteCard` particle materials are not screen overlays—do not put their VMT paths in `overlay`.

## Minimal example

```json
{
  "id": "ci_example_author",
  "name": "Common · Example",
  "channel": "ci",
  "sort": 210,
  "streak": { "enabled": false, "wrap": 0 },
  "hit": {
    "priority": 0,
    "overlay": "skeeto/example_author/hit",
    "overlay_ms": 110,
    "sound": "skeeto/example_author/hit.mp3"
  },
  "kill": {
    "priority": 10,
    "overlay_sequence": [
      "skeeto/example_author/kill_00",
      "skeeto/example_author/kill_01"
    ],
    "overlay_step_ms": 40,
    "overlay_ms": 240,
    "sound": "skeeto/example_author/kill.mp3"
  }
}
```

To reuse the same look for special infected, write another JSON with a unique `id` and `channel: "si"`; material and sound paths may reuse existing files. Adjust `hit`, `kill`, and streak behavior as needed—do not mechanically copy every field.
