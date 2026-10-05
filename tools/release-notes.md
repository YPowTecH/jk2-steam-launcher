Makes Steam's **Play** button for *Star Wars Jedi Knight II: Jedi Outcast* start modern clients — EternalJK2MV ("Tommyternal"), JK2MV or NWH for multiplayer, OpenJO for singleplayer — while Steam keeps tracking playtime, the overlay and your friends list status. Nothing of the game or the clients is changed.

### Install

1. Download **jk2-steam-launcher.zip** below and unzip the `jk2-steam-launcher` folder into your Jedi Outcast `GameData` folder, usually
   `C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData`
2. In Steam: right-click **Jedi Outcast** → **Properties** → **General** → **Launch Options**, and paste:
   ```
   "C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" %command%
   ```
3. Press **Play** → **Launch Multiplayer** or **Launch Single Player**.

You also need a multiplayer client — [EternalJK2MV](https://github.com/TomArrow/jk2mv/releases/tag/latest-postxp) (installer) or [JK2MV](https://jk2mv.org) — and optionally [OpenJO](https://github.com/JACoders/OpenJK/releases/tag/latest) in `GameData\OpenJO` for the campaign. To pick a client: `-client tommy`, `-client jk2mv`, `-client nwh` before `%command%`. See the [README](https://github.com/YPowTecH/jk2-steam-launcher#readme) for details.

The exe isn't code-signed, so Windows may show a SmartScreen warning the first time (**More info** → **Run anyway**).

**SHA-256** of `jk2-steam-launcher.zip`: `@SHA256@`

*Unofficial; not affiliated with Lucasfilm, Valve, JK2MV, EternalJK2MV, NWH or OpenJO.*
