#!/usr/bin/env bash
# Build the drop-in jk2x folder (and a zip of it) from a Release build:
#
#   dist/jk2x/jk2x.exe
#   dist/jk2x/README.txt
#
# Usage: tools/package.sh [--install]
#   --install  also put the folder into the Steam install ($GAMEDATA/jk2x),
#              replacing whatever is in it (jk2x itself never writes there)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXE="${EXE:-$ROOT/build/Release/jk2x.exe}"
DIST="$ROOT/dist/jk2x"
GAMEDATA="${GAMEDATA:-/c/Program Files (x86)/Steam/steamapps/common/Jedi Outcast/GameData}"

[ -f "$EXE" ] || { echo "no build at $EXE (build the Release config first)" >&2; exit 1; }

rm -rf "$DIST"
mkdir -p "$DIST"
cp "$EXE" "$DIST/"
cp "$ROOT/tools/dist-README.txt" "$DIST/README.txt"

(cd "$ROOT/dist" && rm -f jk2x.zip && powershell.exe -NoProfile -Command "Compress-Archive -Path jk2x -DestinationPath jk2x.zip")
echo "Packaged $DIST and dist/jk2x.zip"

if [ "${1:-}" = "--install" ]; then
	[ -f "$GAMEDATA/jk2mp.exe" ] || { echo "no Jedi Outcast install at $GAMEDATA" >&2; exit 1; }
	rm -rf "$GAMEDATA/jk2x"
	cp -r "$DIST" "$GAMEDATA/jk2x"
	echo "Installed to $GAMEDATA/jk2x"
fi
