# JK2 Steam Launcher

A Steam launcher for **Star Wars Jedi Knight II: Jedi Outcast** that makes Steam's Play button start modern clients - [EternalJK2MV](https://github.com/TomArrow/jk2mv) ("Tommyternal"), [JK2MV](https://jk2mv.org) or NWH for multiplayer & [OpenJO](https://github.com/JACoders/OpenJK) for singleplayer.

It doesn't change the game or the clients. It's a small exe that starts the right one and waits until you quit, so Steam still tracks your playtime and the overlay keeps working.

Unofficial; not affiliated with Lucasfilm, Valve, JK2MV, EternalJK2MV, NWH or OpenJO.

## Install

1. Download [jk2-steam-launcher.zip](https://github.com/YPowTecH/jk2-steam-launcher/releases/latest/download/jk2-steam-launcher.zip) from the [latest release](https://github.com/YPowTecH/jk2-steam-launcher/releases/latest).
2. Unzip the `jk2-steam-launcher` folder into your `Jedi Outcast\GameData` folder.
3. In Steam, open Jedi Outcast's properties and set the launch options to:

```
"C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" %command%
```

Change the path if your Steam library is somewhere else.

## How it works

When you press Play, Steam replaces `%command%` with the exe it would normally run, so the launcher knows which option you picked:

- Launch Multiplayer starts the first multiplayer client it finds (see below).
- Launch Single Player starts OpenJO if it's installed, otherwise the original `jk2sp.exe`.

Anything after `%command%` is passed on to the game. If you stop the game from Steam, the launcher closes it too. The launcher doesn't write any files.

## Multiplayer clients

The launcher looks for these in order and starts the first one it finds:

| Name | Client | How to install |
|---|---|---|
| `tommy` | EternalJK2MV ("Tommyternal") | Installer from its [releases](https://github.com/TomArrow/jk2mv/releases/tag/latest-postxp) |
| `jk2mv` | JK2MV | Installer from [jk2mv.org](https://jk2mv.org) |
| `nwh` | NWH | No installer, put its files in `GameData` |

To pick one yourself, add `-client` and its name before `%command%`:

```
"...\jk2-steam-launcher.exe" -client jk2mv %command%
```

You can also give the full path to any client exe, or use `-client stock` for the original `jk2mp.exe`.

Portable versions of these clients can't load the game files from the Steam folder, so they have to live in `GameData` (or keep their own copy of `assets0-5.pk3`). They also come with their own `OpenAL32.dll`, which replaces the game's when you copy them into `GameData`. The installers don't have this problem.

EternalJK2MV and JK2MV both keep their settings and downloaded maps in `Documents\jk2mv`.

## Singleplayer

Download `OpenJO-windows-x86_64.zip` from [OpenJK's latest build](https://github.com/JACoders/OpenJK/releases/tag/latest) and unzip it into `GameData\OpenJO`. The launcher tells OpenJO where the game files are, so nothing in `GameData` gets replaced and the original singleplayer still works if you remove OpenJO.

OpenJO keeps its saves in `Documents\My Games\OpenJO`, separate from the original game's saves.

## Building

You need Visual Studio 2022 and CMake.

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
tools/package.sh            # creates dist/jk2-steam-launcher.zip
tools/package.sh --install  # also copies it into your Steam install
```

The build treats all warnings as errors and runs the MSVC code analyzer. `tools/lint.sh` runs clang-format and clang-tidy (needs LLVM), and `tools/lint.sh --fix` applies the formatting.

To support another client, add a line to the `MP_CLIENTS` or `SP_CLIENTS` table at the top of [`src/jk2-steam-launcher.cpp`](src/jk2-steam-launcher.cpp).

## License

Copyright (C) 2026 YPowTecH. Licensed under the [GPL v3](LICENSE) or later. The launcher doesn't contain any code from the game or the clients, it only starts them.
