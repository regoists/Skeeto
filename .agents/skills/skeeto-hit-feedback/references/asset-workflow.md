# Asset production, size, and acceptance

This page covers designing, producing, converting, and optimizing real assets. All required methods are here; users do not need a development workspace. First make observable images, animation, or audio, then wire them to JSON events. Hit and kill feedback share this production pipeline but remain separate event systems—keep their timing, priority, and visual jobs distinct unless the user asks otherwise.

## Design one trigger first

List the image, sound, particles, duration, and priority for each event. Confirm whether the picture is a pure overlay, a world particle, or both; the same `r_screenoverlay` slot cannot stack two materials. If a glow and an icon must appear together, precompose them into one frame; particles may dispatch separately. Decide whether rapid retriggers replace or continue the previous effect—do not judge from a static screenshot alone.

Before using someone else's assets, record source, author, license scope, and whether modification/redistribution is allowed. Generated images or audio also need usable rights checked; do not take material from an installed game pack and assume redistributing it is allowed. Put attribution, license notes, and install instructions in the pack readme—not in JSON fields the game parses.

## From design to a static image

1. Decide on-screen position, display size, and trigger cadence. Short feedback usually locks icon silhouette first, then adds glow, outline, or text; avoid covering the crosshair and enemies. Preview against a game screenshot, not only a pure black background.
2. Prefer the user's original art when available. New graphics may use vector tools or image generation; complex illustration may use generation and then editing. When a generator is available, actually generate and inspect results—do not treat a prompt as finished art. Keep Alpha for transparent backgrounds; do not threshold black backgrounds away, which damages black detail and glow edges.
3. Keep icon, glow, and text on separate layers. Overlay layers that must appear together are precomposed at the same coordinates before export; lay out text in the project—do not rely on an image generator to typeset. Export full-size RGBA PNG first and check fine lines, text, translucent edges, and both black and white backgrounds.
4. Choose texture size from real on-screen display size; do not stretch art just to hit a power-of-two size—transparent padding is fine. Downscaling can lose detail; when the user requires lossless visible pixels, keep visible pixel size and only remove empty canvas. Keep PNG project art and VTF game textures separately.

## Animation from GIF, video, or layered projects

- GIF frames may contain only dirty regions; restore the full canvas and apply each frame's disposal rules before exporting RGBA frames—do not turn incremental dirty frames into materials directly. Single-bit GIF transparency cannot invent original high-quality Alpha.
- Video usually has no Alpha. Greenscreen needs real-background keying and spill checks; pure-black glow can use additive blending, but black solid shapes or text usually cannot. Prefer a high-quality source that already has Alpha over compressing to video and keying back.
- ffmpeg can extract frames, for example `ffmpeg -i input.mov -vf "fps=25" frames/frame_%03d.png`; that uniformly resamples, does not keep GIF variable frame timing, and does not invent transparency. When converting to GIF, inspect source frame durations and choose a sensible uniform cadence rather than mechanically locking 25 FPS.
- `overlay_sequence` allows at most 30 frames at a fixed `overlay_step_ms`. Example: a 0.6 s animation uses 15 frames at 40 ms. Over the limit, prefer shortening the animation or resampling with a reason; if the user requires every original frame kept, explain the limit and consider multi-frame VTF re-trigger differences—do not silently drop frames and claim full fidelity.
- All frames share one canvas, one crop rect, and one material transform. Crop from the union of visible content across frames and leave margin for glow/blur; per-frame tight crops cause jitter. Build a contact sheet for order, then a preview that plays at the target timing.
- For fades, author per-frame Alpha/brightness changes; JSON has no general fade curve. Design animation duration with audio hit points; the last frame can hold via total duration without duplicating identical textures.

## VTF export and ready-to-use materials

Discover available VTFEdit/VTFCmd, ffmpeg, image editors, or script libraries on this machine; fetch tools for the actual task when missing. Download or look up new tools from official releases and verify usage—do not invent CLI flags from memory. Dependencies may install into an isolated tools directory or Python venv; delivery notes list required tools. Players do not need those authoring tools to run the game.

For new assets: check color space and Alpha; continuous transparency usually uses DXT5; strict detail may use RGBA8888/BGRA8888 at larger size. DXT5 is lossy even without resizing—inspect the decoded image. Save a post-export PNG comparison, focusing on text and glow edges; for strict fidelity, compare pixels, not eyes alone.

Typical translucent overlay VMT:

```text
"UnlitGeneric"
{
    "$basetexture" "skeeto/my_style/kill_00"
    "$translucent" "1"
    "$ignorez" "1"
    "$vertexcolor" "1"
    "$vertexalpha" "1"
    "$basetexturetransform" "center .5 .5 scale 4 4 rotate 0 translate 0 0"
}
```

This example centers the texture in the middle quarter of the screen. Texture edges must be transparent, and Clamp S/T must be enabled in the VTF. Omit the transform when no scale is needed; values are examples, not fixed for every style. Pure glows may use `$additive "1"` with pure-black borders—do not mix both blend modes and guess the result.

Placement example: design canvas 1920×1080, art at top-left (832,412), cropped to 256×256 keeping original pixel size. The formula below yields `scale 7.5 4.21875 rotate 0 translate 0 0`, restoring the original canvas center. When padding to a power-of-two size, use final texture w/h and the padded crop origin for x/y. Existing rotation or scale must not be blindly overwritten.

Each sequence frame has its own VMT; change only `$basetexture` to the matching frame and keep other transform/blend parameters identical. Do not rename a PNG to `.vtf`, and do not put `.vtf` paths in overlay fields.

## Lossless optimization of existing large textures

First emit an inventory: VTF size, format, frame count, mip count, footprint, referencing VMTs, and per-frame visible bounds. Crop only true transparent/pure-black empty space; translucent shadows and very dark glows are still content—do not threshold them away as empty.

- Uncompressed RGBA/BGRA textures may copy rectangular pixel bytes.
- Already-compressed DXT1/DXT5 textures must copy existing 4×4 compression blocks; crop origin and size align to block boundaries. Decoding to PNG and re-encoding to DXT5 loses detail again and is not lossless.
- Each mip level has its own block alignment and coordinates; do not crop only the top level then casually rebuild or drop mips. Without reliable mip handling, keep the original texture and finish other proven-safe optimizations first.
- Multi-frame, atlas, and particle textures need layout confirmed first. After overlay optimization, compensate VMT transforms; do not default the same method onto particles, atlases, and crosshairs.

Acceptance first compares original vs optimized decoded visible pixels, then compares rendered previews under the same background, resolution, and timing. Record footprint reduction and the actual method used. "Did not reduce resolution" is not "did not lose sharpness." Without in-game live tests you can confirm pixels and config only—not identical sampling.

## Audio production and cleanup

New audio may use licensed sources, recording, or generation. Pick one clear short hit point; do not play long background music on every hit. Keep lossless project audio, then export game-supported WAV/MP3. When compatibility is unclear, start from PCM WAV, 44100 Hz, 16-bit; finally confirm in-game playback—those parameters are not the only supported format.

Trim meaningless leading/trailing silence while keeping useful tails and reverb; add a few milliseconds of fade at abrupt cut boundaries to avoid clicks, without shaving the main transient. Check peaks, clipping, and channels; avoid unnecessary phase inversion; do not "fix" already-clipped sources by only lowering gain. Compare loudness across a batch, leave peak headroom, then check whether rapid in-game retriggers are harsh or mask footsteps.

Use ffmpeg/ffprobe to check codec, duration, sample rate, and actual decode; a file that exists but fails to decode is still a delivery error. MP3 has encoder delay—prefer WAV when sync must be tighter. `sounds` picks one at random, not multi-track layering; mix layers in the audio project first when sync is required.

## Practical particle bounds

When reusing particles, first confirm effect name, load path, PCF, and material dependencies. New particles need a usable Source particle editor and a runtime that can load that PCF. Ordinary image tools cannot produce a valid PCF—do not fake empty files.

With an editor, start from a verified template under an independent effect name; set emission, lifetime, size, Alpha/color, and screen/world space; test single fire then rapid kills. Prefer short lifetime and few particles; avoid endlessly accumulating emitters. Accept screen-space and world-space effects separately. After exporting the PCF, list every VMT/VTF dependency. When load path is unclear, state that a usable particle part cannot be delivered yet; finish overlay/audio the user accepts first, and do not overwrite global manifests on your own.

## Crop placement formula and fidelity rules

Use an available VTF converter (for example installed VTFEdit/VTFCmd) or another tool whose output you can verify; do not assume a specific program is already on the user's machine. When downloading, use trusted sources, record tool version and export settings, and verify with a viewer plus decode comparison—extension alone does not prove a usable VTF. For VPK packing, prefer game-bundled `vpk.exe` or a confirmed-compatible tool, then read the directory listing back.

1. Save original project art and full-resolution transparent images before exporting VTF. Compare the visible-content union across events and all animation frames; choose a canvas that covers glow/shadow edges. Do not default screen overlays to a mostly empty 1920×1080 or 2048×1024 texture.
2. Soft-edge transparency usually uses DXT5; consider DXT1 only when single-bit transparency truly fits. For absolute-fidelity detail, text, or original pixels, first compare source vs VTF decode—do not blindly lower resolution, switch to DXT1, or re-encode repeatedly for size. VTF size must at least meet format block requirements; power-of-two sizes are usually less painful on older toolchains.
3. When a small image is used as a fullscreen overlay, enable Clamp S/T on the VTF and use `UnlitGeneric` with `$basetexturetransform`. Translucent blend borders should be transparent; `$additive 1` borders should be pure black—otherwise Clamp pulls edge color across the screen. When editing an existing VTF, inspect **all frames** and find borders by the real blend mode: non-black RGB for `$additive`, Alpha for `$translucent`.
4. To losslessly shrink an existing fullscreen asset, prefer copying source DXT blocks or RGBA bytes into an aligned crop, then compensate texture coordinates. Recompression is not pixel-lossless; dropping/rebuilding mipmaps can change downscaled sampling—keep mip levels that can be preserved exactly. After edits, compare visible pixels and coordinates per frame; without in-game comparison, do not claim identical results.
5. Particle `SpriteCard`, atlases, dynamic crosshairs, and UV-dependent materials must check the matching particle definition, atlas layout, and UV usage before cropping; do not mechanically apply overlay transform formulas.

Reference: cropping a `W×H` source canvas from `(x,y)` into `w×h` with no prior transform gives `u'=(W·u−x)/w`, `v'=(H·v−y)/h`. For a VMT `center .5 .5 scale sx sy rotate 0 translate tx ty`, set `sx=W/w`, `sy=H/h`, `tx=(sx−1)/2−x/w`, `ty=(sy−1)/2−y/h`. **Existing transforms must be composed with this one first**; re-check canvas coordinates, UV orientation, and real in-game display. Different aspect ratios especially need stretch checks.

## Animation and performance

- Multi-frame VTF + `AnimatedTexture` proxies suit looping materials, but re-triggers may continue from global material time and are not guaranteed to restart at frame 0.
- `overlay_sequence` switches independent VMT/VTF entries and suits short animations that must restart from the first frame; ceiling is 30 frames. Estimate whether `frames × interval` fits the 5-second window; minimize duplicate frames. At low FPS, overdue frames are skipped rather than backfilled.
- For short feedback, fewer frames and a sensible duration that stays readable come first. Static hold segments can lengthen total display on the last frame—do not duplicate identical VTFs. Do not sync audio/visuals with nonexistent JSON fade fields.
- When compressing, tally whole-pack and per-style bytes; check top resolution plus mips, all frames, and VRAM trend. Optimize only assets that truly dominate size and can be proven not to harm the look.

## Audio and particles (wiring)

- `sound` is one file; `sounds` is a random pool. Keep clear `.mp3` or `.wav` extensions; check decode, duration, clipping, and relative loudness; avoid long cues drowned by the next high-frequency feedback. Do not duplicate identical audio without need.
- `particle` is the **effect name inside the PCF**, not a `.pcf` path; `particles` appends concurrently and is not a random candidate pool. Check the PCF, its VMT/VTF deps, effect name, and load manifest. World particles need victim position, occlusion, map-change cleanup, and frequent-trigger cost. Without PCF parse / in-game evidence, say only "path exists," not "particle verified."
- Avoid covering the game-global `particles_manifest.txt` for custom effects. Existing presets may load particles through Skeeto/Necola own manifests or platform directories; check the user's real runtime. When particles are not ready, delivering overlay/audio first is fine—do not invent particle fields.

## Install and visual acceptance

The output directory should map directly onto `left4dead2/`; VPK roots use the same relative layout. Loose files are fine while iterating; before release, check packed file lists, VPK vs loose priority, same-path collisions, and whether JSON appears in the game menu. Do not pack raw PSD/PNG, uncompressed audio, or scripts into the game asset root unless they have a runtime purpose; keep authoring sources elsewhere.

In a clean environment, select each channel's style and enable the needed audio/visual switches. At least observe: ordinary hit, kill, headshot, melee, streak tiers; CI/SI both selected; light/heavy friendly fire; named special-kill events; rapid retrigger; map change, quit, visuals off; 1080p and other aspect ratios; low-FPS animation. Record events never triggered or not stably reproducible—do not summarize undone checks as "all fine."
