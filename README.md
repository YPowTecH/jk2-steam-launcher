# jk2x

A Steam launcher for **Star Wars Jedi Knight II: Jedi Outcast** that makes Steam's Play button start your multiplayer client — [EternalJK2MV](https://github.com/TomArrow/jk2mv) ("Tommyternal"), [JK2MV](https://jk2mv.org), NWH, or any other JK2 client — in the style of DoomBFA for Doom 3 BFG: the stock game stays untouched and Steam launches jk2x through a launch option, so playtime, the overlay and the friends list keep working.

jk2x does not replace or modify the client or the game. It is one small exe that starts the right program and waits for it.

## What it does

Steam launch option:

```
"C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2x\jk2x.exe" %command%
```

Steam replaces `%command%` with the stock exe it would have run:

| Steam menu | Steam passes | jk2x starts |
|---|---|---|
| Launch Multiplayer | `GameData\jk2mp.exe` | the multiplayer client, from its own folder |
| Launch Single Player | `GameData\jk2sp.exe` | the stock `jk2sp.exe` |
| (started without Steam) | nothing | the multiplayer client |

Other arguments are passed on to the game. jk2x waits on the game's whole process tree (a job object), so Steam sees the game running until it really exits; if jk2x is closed (Steam's Stop button) the game is closed too. jk2x writes no files.

### Which client

Pick one with `-client` before `%command%`, by short name or by path:

```
"...\GameData\jk2x\jk2x.exe" -client tommy %command%
"...\GameData\jk2x\jk2x.exe" -client jk2mv %command%
"...\GameData\jk2x\jk2x.exe" -client "D:\Games\jk2nwh\nwhmp.exe" %command%
```

Without `-client`, jk2x starts the first known client it finds, in the order of the table below. A known client is looked for through its installer's registry entry (`Uninstall\<key>\InstallLocation`), then as a portable exe in `GameData`, then in its default `Program Files (x86)\<key>` folder.

| Short names | Client | Exe | Installer key | Notes |
|---|---|---|---|---|
| `tommy`, `tommyternal`, `eternaljk2mv`, `eternal` | **EternalJK2MV** ("Tommyternal"), TomArrow's JK2MV fork — the main target | `eternaljk2mvmp.exe` | `EternalJK2` | Builds on [GitHub Releases](https://github.com/TomArrow/jk2mv/releases/tag/latest-postxp). **Use the Installer package**: the installed build finds the game files in the Steam folder by itself. |
| `jk2mv`, `mv` | JK2MV | `jk2mvmp.exe` | `JK2MV` | Installed build: same as above. |
| `nwh` | NWH (unmaintained, last v1.2.5 via [Monolith Mods](https://jk2t.ddns.net/)) | `nwhmp.exe` | — | Portable only. |

**Adding a client** is one line in the `CLIENTS` table at the top of [`src/jk2x.cpp`](src/jk2x.cpp): its short names, display name, exe, and installer key (or `NULL`). Its position in the table is its auto-detect priority.

**Portable builds** (any of the above) can't read the game files from the Steam folder — JK2MV compiles `fs_assetspath` out of portable builds, and NWH ignores it — so they need `assets0-5.pk3` in their own `base` folder: either keep copies there, or put the client into `GameData`. Note that portable packages ship their own `OpenAL32.dll`, which would replace the stock game's `openal32.dll` in `GameData`.

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
