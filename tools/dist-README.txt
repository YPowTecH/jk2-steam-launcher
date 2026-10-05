JK2 Steam Launcher
==================

Makes Steam's Play button for Star Wars Jedi Knight II: Jedi Outcast start
EternalJK2MV ("Tommyternal"), JK2MV or NWH for multiplayer and OpenJO for
singleplayer. Steam still tracks your playtime and the overlay still works.
It doesn't change the game or the clients.

Install
-------
1. Copy this "jk2-steam-launcher" folder into your Jedi Outcast GameData
   folder, usually:
     C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData

2. In Steam, open Jedi Outcast's properties and set the launch options to:

     "C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" %command%

   Change the path if your Steam library is somewhere else.

3. Press Play and pick Launch Multiplayer or Launch Single Player.

You'll also need a multiplayer client:
  EternalJK2MV: https://github.com/TomArrow/jk2mv/releases (the installer)
  JK2MV:        https://jk2mv.org (the installer)
  NWH:          no installer, put its files in the GameData folder

For singleplayer, download OpenJO-windows-x86_64.zip from
https://github.com/JACoders/OpenJK/releases/tag/latest and unzip it into
GameData\OpenJO. Without it, Launch Single Player starts the original game.

Picking a client
----------------
The launcher uses the first client it finds: EternalJK2MV, then JK2MV, then
NWH. To pick one yourself, add -client and its name before %command%:

     "...\jk2-steam-launcher.exe" -client jk2mv %command%

The names are tommy, jk2mv and nwh. You can also give the full path to any
client exe, or use -client stock for the original jk2mp.exe.

Anything after %command% (like +set fs_game ProAt) is passed on to the game.

Portable clients
----------------
Portable versions of the clients can't load the game files from the Steam
folder, so put them in GameData, or keep a copy of assets0.pk3, assets1.pk3,
assets2.pk3 and assets5.pk3 in their own base folder. The installed versions
don't have this problem.

OpenJO keeps its saves in Documents\My Games\OpenJO, separate from the
original game's saves.

Uninstall
---------
Clear the launch options in Steam and delete the jk2-steam-launcher folder.

License
-------
Copyright (C) 2026 YPowTecH. Licensed under the GPL v3 or later, see
LICENSE.txt. Source code: https://github.com/YPowTecH/jk2-steam-launcher
