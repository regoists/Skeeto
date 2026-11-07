# L4D2 live verification and UI operations

This guide is for agents that drive Left 4 Dead 2 through Computer Use, reproduce features, and record runtime evidence. Project red lines and build/deploy flow follow root `AGENTS.md`; this guide only covers how to operate and accept the game reliably. When the user clearly says they will test themselves, stop operating the game on their behalf and wait for feedback.

## Before you start: confirm the test subject

- Check the current branch, HEAD, workspace, and `VERSION.txt`. Code build, game-root deploy, and what the game actually loaded are different states; a successful build command does not mean an in-use game-root DLL was replaced.
- After deploy, compare SHA256 of game-root `skeeto.dll` with this round's build artifact. Matching hashes only prove the on-disk files match; testing must start from a game process launched after deploy finished—do not attribute old-process results to the new DLL.
- Confirm the foreground window's process path is the machine's target L4D2. Use screenshots to confirm whether you are in gameplay, console, main menu, or a system dialog; when Steam or a crash dialog covers the window, do not blindly send keys to the game.
- Fix this round's conditions and briefly record them: map/chapter, mode, solo or local/remote server, players and bots, critical mods and related scripts. Do not change these mid-comparison.
- Before testing, record the Skeeto options you will change and their current values. After testing, restore settings you temporarily changed this round; do not casually reset the user's other settings, mods, or system virtual memory.
- Check memory-protection conditions already present on this machine. When a stop threshold is hit, a persistent black screen appears, or a crash dialog shows, stop repeating input and keep evidence; do not generalize one machine's one-test values into a fixed standard for every PC.

## Remote-server test entry

- [KitaSoda server list](https://www.kitasoda.com/serverList). For later authorized remote-server verification, you can pick an idle server's IP and port here without waiting for the user to provide a test IP. Prefer servers with 0 players, and re-read player count, map, and server status before connecting.
- That page loads data dynamically. Confirmed 2026-10-23: no login is required to read the first `serverDetailUpdate` event from the page's public SSE endpoint `https://api.kitasoda.com/subscribeServerDetailAll`; `serverDetailAll` includes `status`, `address` (IP:port), `numPlayers`, `maxPlayers`, `name`, and `map`. After reading the initial list, close the connection; a standing subscription is unnecessary. The API and addresses may change—follow the live page, and do not treat historical IPs or idle status as real-time.
- The user said these servers may be used for Skeeto verification and currently sees little VAC/LAC risk; generalized worry is not a reason to stop testing. This records the user's test authorization and experience judgment, not a detection conclusion about each server's anti-cheat state.

## Computer Use habits

- After every launch, load, menu open/close, console open/close, return to main menu, or other UI transition, re-read the screen before deciding the next step. Act on the currently visible page; do not reuse mouse coordinates from a previous resolution or layout.
- When the menu has numbers or stable keyboard ops, prefer key navigation; after every page turn, confirm the page title and option numbers. If a remembered route has uncertain steps, confirm page by page—do not blind-fire a whole key sequence.
- When console commands establish a fixed map or test state, commands only prepare the scene. If acceptance targets menu input, key interception, mouse interaction, HUD layout, or real game events, the target action must still be done through real in-game input.
- Treat press, hold, and release as different operations. After holding attack then switching weapons, releasing a throw key, or closing the console, separately confirm what actually happened; after sending a key, confirm it was released so a held key is not mistaken for a single click.
- Execute one observable transition or action at a time. If the expected page does not appear, first check focus, console state, and system dialogs; do not spam keys hoping to "bump" into the target page.
- Confirm long loads and async transitions by screen state; do not infer that the map is acceptance-ready from process start, a fixed wait, or the console already accepting input.

## Accept by feature type

- **Menus and HUD:** Capture key screens before enable, after enable, and after disable. Confirm page/map, option numbers, text, and input behavior; then confirm that after disable the cue disappears and after re-enable it can restore. For HUD that needs occlusion relationships, check both unoccluded views and views blocked by world geometry.
- **Entities and events:** Be clear that a screenshot only proves "seen / not seen at that moment." It alone cannot prove a network event fired, a projectile was spawned, or server state synced. When those need verification, combine controllable real events, console/logs, or existing state observation; actions bots will not perform cannot be replaced by bot results when a human teammate test is required.
- **Local vs remote differences:** Mark whether the current setup is a local listen server or a remote server. Client-observed state does not prove server-private data is readable; local script values also do not represent script values changed on a remote server. When network conditions are abnormal, mark only the affected network conclusions invalid; independent screen observations may be recorded under their own conditions.
- **Render changes:** Besides the target HUD, at least inspect console, server browser or panels, settings/editor UI, and main menu that this path can affect. Changing lower-level drawing may also affect VGUI, input focus, or mouse state; the target element appearing is not enough to pass.
- **CVar and state restore:** Separately confirm initial value, target value after enable, and restore value after disable; then check related boundaries such as master switch off, map change, or disconnect. Do not treat "it looks restored on screen" as verification of values or ownership.
- **Load or performance:** Fix map, launch method, mods, and comparison order; change only the option under test. Distinguish cold/warm cache and the full target interval; record full load, first use after enter, frame time, and similar metrics. Corner FPS or subjective feel can be clues, not sole proof of net speedup or no side effects.

## Lifecycle and abnormal wrap-up

- In-map exit, normal return to main menu, disconnect, chapter restart, demo playback end, console close, and process exit are different paths; cover the scenes required by this feature's resource lifecycle—they do not substitute for each other.
- If a problem happens at the end of a sequence, keep the full order. Example: "record demo → stop → playback finishes → return to main menu with console still open → close console". You may separately repeat playback-end and console-close steps to find the trigger edge; do not directly blame "returned to main menu" as the playback crash cause.
- After a normal exit, separately observe whether the game process ended, overlays disappeared, and mouse/input restored. No dialog does not mean the process exited; a leftover name in Task Manager also cannot alone prove another Skeeto child process. Prefer normal exit; process operations follow current user authorization and project exit rules.
- On black screen, dialog, or input loss of control, first stop Computer Use input and record the screen and time; avoid continuous Alt+Tab, Esc, or mouse clicks that create more state changes. Module names and offsets in a crash dump are location clues, not sole root-cause proof; analyze them with the matching-time console.log, operation order, and DLL identity.
- If Steam login, connect, or server errors appear, judge whether this run is still meaningful against the target assertion. It may only affect online/return-to-menu paths, or it may invalidate remote-event tests outright; record the impact scope clearly instead of declaring the whole test invalid or passed.

## Recording live results

Each test only needs enough to reproduce and judge; do not copy the whole user config:

```text
Build: branch / commit / VERSION / game-root DLL SHA256
Environment: map and chapter / mode / local or remote / players and bots / related mods
Prep: launch and map-enter method / initial values of options under test / machine resource-protection state
Steps: keys, menu pages, console commands, and each transition in actual order
Results: expected / observed behavior / result after switch off
Wrap-up: return to menu, console, normal exit, leftover process, overlay and input state
Evidence: screenshots and times / related console.log ranges / dump files and times
Conclusion bounds: which conditions this result supports; which maps, modes, servers, or player behaviors remain unverified
```

Before each test, check whether a valid offline verification/build result already exists for the same source and DLL, to avoid duplication. Offline fixtures, compile success, disk hashes, screenshots, logs, and actual behavior each explain different things; do not let one replace the others. When a build is blocked by a holding process, a test protection stop, or a network break, record the intermediate state and recovery result; do not write an attempt as a pass.
