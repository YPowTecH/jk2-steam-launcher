#!/usr/bin/env bash
# Build the drop-in jk2-steam-launcher folder (and a zip of it) from a
# Release build:
#
#   dist/jk2-steam-launcher/jk2-steam-launcher.exe
#   dist/jk2-steam-launcher/README.txt
#
# Usage: tools/package.sh [--install]
#   --install  also put the folder into the Steam install
#              ($GAMEDATA/jk2-steam-launcher), replacing whatever is in it
#              (the launcher itself never writes there)
set -euo pipefail

NAME="jk2-steam-launcher"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXE="${EXE:-$ROOT/build/Release/$NAME.exe}"
DIST="$ROOT/dist/$NAME"
GAMEDATA="${GAMEDATA:-/c/Program Files (x86)/Steam/steamapps/common/Jedi Outcast/GameData}"

[ -f "$EXE" ] || { echo "no build at $EXE (build the Release config first)" >&2; exit 1; }

rm -rf "$DIST"
mkdir -p "$DIST"
cp "$EXE" "$DIST/"
cp "$ROOT/tools/dist-README.txt" "$DIST/README.txt"

(cd "$ROOT/dist" && rm -f "$NAME.zip" && powershell.exe -NoProfile -Command "Compress-Archive -Path $NAME -DestinationPath $NAME.zip")
echo "Packaged $DIST and dist/$NAME.zip"

if [ "${1:-}" = "--install" ]; then
	[ -f "$GAMEDATA/jk2mp.exe" ] || { echo "no Jedi Outcast install at $GAMEDATA" >&2; exit 1; }
	rm -rf "${GAMEDATA:?}/$NAME"
	cp -r "$DIST" "$GAMEDATA/$NAME"
	echo "Installed to $GAMEDATA/$NAME"
fi
