JK2 Steam Launcher - launch Tommyternal / JK2MV / NWH / OpenJO from Steam's Jedi Outcast
========================================================================================

JK2 Steam Launcher makes Steam's "Play" button for Star Wars Jedi Knight II:
Jedi Outcast start modern clients - EternalJK2MV, JK2MV or NWH for
multiplayer, OpenJO for singleplayer - while Steam keeps tracking them
(playtime, overlay, friends list). Nothing of the clients or the game is
changed.

You need
--------
- Jedi Outcast from Steam
- one or more multiplayer clients:
    EternalJK2MV ("Tommyternal") - defrag, FFA, most public servers
                  https://github.com/TomArrow/jk2mv/releases
                  (the "Windows.Package.Installer" download)
    JK2MV       - https://jk2mv.org (installer)
    NWH         - anti-cheat client for organised CTF; no installer, put
                  its files in the GameData folder
- optionally, for the singleplayer campaign:
    OpenJO      - https://github.com/JACoders/OpenJK/releases/tag/latest
                  unzip OpenJO-windows-x86_64.zip into GameData\OpenJO
                  (without it, Single Player starts the original game)

Install
-------
1. Copy this "jk2-steam-launcher" folder into your Jedi Outcast GameData
   folder, usually:
     C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData

2. In Steam: right-click Jedi Outcast > Properties > General > Launch Options,
   and paste (adjust the path if your Steam library is elsewhere):

     "C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" %command%

3. Press Play and pick "Launch Multiplayer" or "Launch Single Player".

Which program starts
--------------------
Single Player: OpenJO if it's installed, otherwise the original jk2sp.exe.

Multiplayer: the first client it finds - EternalJK2MV (Tommyternal),
JK2MV, NWH. To pick one, add -client and its name before %command%:

     "...\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" -client tommy %command%
     "...\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" -client jk2mv %command%
     "...\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" -client nwh %command%

or "stock" for the game's own jk2mp.exe, or the full path to any client exe:

     "...\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" -client "D:\Games\SomeClient\client.exe" %command%

Clients are found where their installer put them, or as a portable exe in
the GameData folder (OpenJO also in GameData\OpenJO). Each is started from
its own folder, exactly as if you opened it.

Portable clients: NWH, and portable builds of EternalJK2MV or JK2MV, can't
read the game files from the Steam folder. They need copies of
GameData\base\assets0.pk3, assets1.pk3, assets2.pk3 and assets5.pk3 in their
own base folder, or to be placed in GameData itself. The installed versions
don't have this problem, and neither does OpenJO in GameData\OpenJO (the
launcher points it at the game files). OpenJO keeps its saves and settings
in Documents\My Games\OpenJO, separate from the original game's saves.

Anything after %command% (like +set fs_game ProAt) is passed on to the game.

Uninstall
---------
Clear the Steam launch option and delete the jk2-steam-launcher folder.
