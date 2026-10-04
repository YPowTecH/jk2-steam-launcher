jk2x - launch EternalJK2MV / JK2MV from Steam's Jedi Outcast
=============================================================

jk2x makes Steam's "Play" button for Star Wars Jedi Knight II: Jedi Outcast
start your multiplayer client (EternalJK2MV, JK2MV, NWH, ...) while Steam
keeps tracking it (playtime, overlay, friends list). Singleplayer still
starts the original game. Nothing of the client or the game is changed.

You need
--------
- Jedi Outcast from Steam
- a multiplayer client, best installed with its installer:
    EternalJK2MV: https://github.com/TomArrow/jk2mv/releases
                  (the "Windows.Package.Installer" download)
    JK2MV:        https://jk2mv.org

Install
-------
1. Copy this "jk2x" folder into your Jedi Outcast GameData folder, usually:
     C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData

2. In Steam: right-click Jedi Outcast > Properties > General > Launch Options,
   and paste (adjust the path if your Steam library is elsewhere):

     "C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2x\jk2x.exe" %command%

3. Press Play and pick "Launch Multiplayer" or "Launch Single Player".

Which client starts
-------------------
jk2x finds an installed EternalJK2MV or JK2MV by itself (EternalJK2MV first).
Otherwise it uses a portable client in the GameData folder
(eternaljk2mvmp.exe, jk2mvmp.exe, nwhmp.exe). To pick a client yourself, add
-client before %command%:

     "...\GameData\jk2x\jk2x.exe" -client "C:\Games\jk2nwh\nwhmp.exe" %command%

The client is started from its own folder, exactly as if you opened it.

Portable clients: portable builds (EternalJK2MV, JK2MV or NWH) can't read
the game files from the Steam folder. They need copies of
GameData\base\assets0.pk3, assets1.pk3, assets2.pk3 and assets5.pk3 in their
own base folder, or to be placed in GameData itself. The installed versions
don't have this problem.

Anything after %command% (like +set fs_game ProAt) is passed on to the game.

Uninstall
---------
Clear the Steam launch option and delete the jk2x folder.
