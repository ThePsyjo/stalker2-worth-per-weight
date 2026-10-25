#!/bin/sh
# Build and cook the Worth per Weight Blueprint mod with a stock UE 5.5 (Linux) editor and the stub project ue/Stalker2.
# Usage: tools/build_bp.sh [UE_ROOT]   (default /data/ue)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
UE=${1:-${UE_ROOT:-/data/ue}}
PROJ="$ROOT/ue/Stalker2/Stalker2.uproject"
LOGS="$ROOT/work/logs"
FLAGS="-unattended -nopause -nosplash -NullRHI -nocrashreports -stdout"
mkdir -p "$LOGS"

"$UE/Engine/Build/BatchFiles/Linux/Build.sh" Stalker2Editor Linux Development -Project="$PROJ" -WaitMutex > "$LOGS/ubt.log" 2>&1 \
	|| { tail -30 "$LOGS/ubt.log"; exit 1; }

# generated assets are rebuilt from code every time; stale ones would be "partially loaded" and refuse to save
rm -f "$ROOT/ue/Stalker2/Plugins/ValuePerKilo/Content/"*.uasset
"$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJ" -run=WPWBuild $FLAGS > "$LOGS/wpwbuild.log" 2>&1 \
	|| { grep -E 'LogWPW|Error' "$LOGS/wpwbuild.log" | grep -v LogInit | tail -30; exit 1; }
grep 'LogWPW' "$LOGS/wpwbuild.log" | grep -v LogInit

rm -rf "$ROOT/ue/Stalker2/Saved/Cooked"
PKGS=""
for f in "$ROOT/ue/Stalker2/Plugins/ValuePerKilo/Content/"*.uasset; do PKGS="$PKGS -PACKAGE=/ValuePerKilo/$(basename "$f" .uasset)"; done
"$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJ" -run=cook -targetplatform=Linux -unversioned $PKGS -cooksinglepackagenorefs $FLAGS \
	> "$LOGS/cook.log" 2>&1 || { grep -E 'Error' "$LOGS/cook.log" | tail -30; exit 1; }
find "$ROOT/ue/Stalker2/Saved/Cooked" -path '*ValuePerKilo/Content/*' -type f
