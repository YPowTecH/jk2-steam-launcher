# jk2x

A Steam launcher for **Star Wars Jedi Knight II: Jedi Outcast** that makes Steam's Play button start modern clients — [EternalJK2MV](https://github.com/TomArrow/jk2mv) ("Tommyternal"), [JK2MV](https://jk2mv.org) or NWH for multiplayer, [OpenJO](https://github.com/JACoders/OpenJK) for singleplayer, or any other exe — in the style of DoomBFA for Doom 3 BFG: the stock game stays untouched and Steam launches jk2x through a launch option, so playtime, the overlay and the friends list keep working.

jk2x does not replace or modify the clients or the game. It is one small exe that starts the right program and waits for it.

Unofficial; not affiliated with Lucasfilm, Valve, JK2MV, EternalJK2MV, NWH or OpenJO.

## What it does

Steam launch option:

```
"C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2x\jk2x.exe" %command%
```

Steam replaces `%command%` with the stock exe it would have run:

| Steam menu | Steam passes | jk2x starts |
|---|---|---|
| Launch Multiplayer | `GameData\jk2mp.exe` | a multiplayer client (table below), from its own folder |
| Launch Single Player | `GameData\jk2sp.exe` | OpenJO if it's there, otherwise the stock `jk2sp.exe` |
| (started without Steam) | nothing | a multiplayer client |

Other arguments are passed on to the game. jk2x waits on the game's whole process tree (a job object), so Steam sees the game running until it really exits; if jk2x is closed (Steam's Stop button) the game is closed too. jk2x writes no files.

### Choosing the multiplayer client

Add `-client` before `%command%`, with a short name, a path, or `stock` for the game's own `jk2mp.exe`:

```
"...\GameData\jk2x\jk2x.exe" -client tommy %command%
"...\GameData\jk2x\jk2x.exe" -client jk2mv %command%
"...\GameData\jk2x\jk2x.exe" -client "D:\Games\SomeClient\client.exe" %command%
```

Without `-client`, jk2x starts the first known client it finds, in table order. A known client is looked for through its installer's registry entry (`Uninstall\<key>\InstallLocation`), then as a portable exe in `GameData`, then in its default `Program Files (x86)\<key>` folder.

Singleplayer needs no option: "Launch Single Player" starts OpenJO if it's installed, otherwise the stock `jk2sp.exe`.

The JK2 CTF community ([jk2ctf.com/launcher](https://jk2ctf.com/launcher)) uses all of these: Tommyternal for defrag, FFA and most public servers, NWH (anti-cheat) for organised CTF, JK2MV as the engine the others build on, and OpenJO for the campaign.

**Multiplayer**

| Short names | Client | Exe | Installer key | Notes |
|---|---|---|---|---|
| `tommy`, `tommyternal`, `eternaljk2mv`, `eternal` | **EternalJK2MV** ("Tommyternal"), TomArrow's JK2MV fork — the main target | `eternaljk2mvmp.exe` | `EternalJK2` | Builds on [GitHub Releases](https://github.com/TomArrow/jk2mv/releases/tag/latest-postxp). **Use the Installer package**: the installed build finds the game files in the Steam folder by itself. Keeps its settings (`eternaljk2mv*.cfg`) and downloads in `Documents\jk2mv`, shared with JK2MV. |
| `jk2mv`, `mv` | JK2MV | `jk2mvmp.exe` | `JK2MV` | Installed build: same as above. |
| `nwh` | NWH, the anti-cheat client organised CTF runs on | `nwhmp.exe` | — | Portable only (no installer). Put it in `GameData` (where [soracle-launcher](https://github.com/soradozere/soracle-launcher) puts clients too) so `-client nwh` finds it and it can read the game files, or point `-client` at its exe. |

**Singleplayer**

| Engine | Exe | Where | Notes |
|---|---|---|---|
| **OpenJO**, OpenJK's Jedi Outcast singleplayer engine | `openjo_sp.x86_64.exe`, else `openjo_sp.x86.exe` | `GameData\OpenJO\` (or `GameData`) | Unzip `OpenJO-windows-x86_64.zip` from [OpenJK's Latest Build](https://github.com/JACoders/OpenJK/releases/tag/latest) into `GameData\OpenJO`. jk2x points it at the game files with `+set fs_cdpath "<GameData>"`, so nothing in `GameData` is replaced and the stock `jk2sp.exe` keeps working. Saves and settings go to `Documents\My Games\OpenJO` (separate from the stock game's saves). |

**Adding a client** is one line in the `MP_CLIENTS` (or `SP_CLIENTS`) table at the top of [`src/jk2x.cpp`](src/jk2x.cpp): short names, display name, exe names, installer key, portable sub-folder and the cvar that points it at the game files (`nullptr` where not applicable). Its position in the table is its auto-detect priority.

**Portable JK2MV-based builds** (EternalJK2MV, JK2MV, NWH) can't read the game files from the Steam folder — JK2MV compiles `fs_assetspath` out of portable builds, and NWH ignores it — so they need `assets0-5.pk3` in their own `base` folder: either keep copies there, or put the client into `GameData`. Note that portable packages ship their own `OpenAL32.dll`, which would replace the stock game's `openal32.dll` in `GameData`.

## Build (Windows, Visual Studio 2022)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
tools/package.sh            # dist/jk2x/ and dist/jk2x.zip
tools/package.sh --install  # also copies it into the Steam install
```

Built 32-bit like the clients and the stock game, with a static runtime, so users need no Visual C++ redistributable.

## Code quality

Every build compiles with MSVC `/W4 /WX /permissive-` (all warnings, warnings are errors, strict standard conformance) and runs the MSVC code analyzer (`/analyze`; turn off with `-DJK2X_ANALYZE=OFF`).

On top of that, before committing:

```bash
tools/lint.sh        # clang-format check + clang-tidy
tools/lint.sh --fix  # apply clang-format
```

- [`.clang-tidy`](.clang-tidy): C++ Core Guidelines, CERT, clang static analyzer, bugprone, modernize, performance and readability checks, all as errors, plus naming rules. The two disabled checks are listed there with the reason.
- [`.clang-format`](.clang-format): formatting (tabs for indentation, 120 columns).
- [`.editorconfig`](.editorconfig) / [`.gitattributes`](.gitattributes): UTF-8, LF line endings.

Needs LLVM (clang-format, clang-tidy) — set `LLVM_BIN` if it isn't in `C:\Program Files\LLVM\bin` or on `PATH`.
