jk2x - launch Tommyternal / JK2MV / NWH from Steam's Jedi Outcast
=================================================================

jk2x makes Steam's "Play" button for Star Wars Jedi Knight II: Jedi Outcast
start your multiplayer client (EternalJK2MV, JK2MV, NWH, ...) while Steam
keeps tracking it (playtime, overlay, friends list). Singleplayer still
starts the original game. Nothing of the client or the game is changed.

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
By default jk2x starts the first client it finds, in this order:
EternalJK2MV (Tommyternal), JK2MV, NWH.

To pick one, add -client and its name before %command%:

     "...\GameData\jk2x\jk2x.exe" -client tommy %command%
     "...\GameData\jk2x\jk2x.exe" -client jk2mv %command%
     "...\GameData\jk2x\jk2x.exe" -client nwh %command%

or the full path to any client exe:

     "...\GameData\jk2x\jk2x.exe" -client "D:\Games\SomeClient\client.exe" %command%

Clients are found where their installer put them, or as a portable exe in
the GameData folder. The client is started from its own folder, exactly as
if you opened it.

Portable clients: NWH, and portable builds of EternalJK2MV or JK2MV, can't
read the game files from the Steam folder. They need copies of
GameData\base\assets0.pk3, assets1.pk3, assets2.pk3 and assets5.pk3 in their
own base folder, or to be placed in GameData itself. The installed versions
don't have this problem.

Anything after %command% (like +set fs_game ProAt) is passed on to the game.

Uninstall
---------
Clear the Steam launch option and delete the jk2x folder.
