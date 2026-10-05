Makes Steam's Play button for Star Wars Jedi Knight II: Jedi Outcast start EternalJK2MV ("Tommyternal"), JK2MV or NWH for multiplayer and OpenJO for singleplayer. Steam still tracks your playtime and the overlay still works.

### Install

1. Download `jk2-steam-launcher.zip` below.
2. Unzip the `jk2-steam-launcher` folder into `...\steamapps\common\Jedi Outcast\GameData`.
3. In Steam, open Jedi Outcast's properties and set the launch options to:
   ```
   "C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\jk2-steam-launcher\jk2-steam-launcher.exe" %command%
   ```

You'll also need a multiplayer client like [EternalJK2MV](https://github.com/TomArrow/jk2mv/releases/tag/latest-postxp) or [JK2MV](https://jk2mv.org). For singleplayer, put [OpenJO](https://github.com/JACoders/OpenJK/releases/tag/latest) in `GameData\OpenJO`. The [README](https://github.com/YPowTecH/jk2-steam-launcher#readme) has the details.

The exe isn't signed, so Windows might show a SmartScreen warning the first time. Click "More info" and then "Run anyway".

SHA-256 of the zip: `@SHA256@`

Licensed under the GPL v3. Unofficial; not affiliated with Lucasfilm, Valve, JK2MV, EternalJK2MV, NWH or OpenJO.
