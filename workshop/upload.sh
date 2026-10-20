#!/bin/sh
# Upload build/workshop to Steam Workshop. First run creates a new PRIVATE item; afterwards
# workshop/published_id holds its ID and later runs update it.
# Usage: workshop/upload.sh <steam_username> ["change note"]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
USER=${1:?steam username}
NOTE=${2:-update}
STEAMCMD=${STEAMCMD:-$HOME/.local/share/steamcmd/steamcmd.sh}
if [ ! -x "$STEAMCMD" ]; then
	mkdir -p "$(dirname "$STEAMCMD")"
	curl -sSL https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz | tar -xz -C "$(dirname "$STEAMCMD")"
fi
ID=$(cat "$ROOT/workshop/published_id" 2>/dev/null || echo 0)
VDF="$ROOT/build/item.vdf"
python3 -I - "$ROOT" "$ID" "$NOTE" "$VDF" <<'PY'
import sys
root, pid, note, out = sys.argv[1:]
desc = open(f'{root}/workshop/description.txt', encoding='utf-8').read().replace('"', '\\"')
t = open(f'{root}/workshop/item.vdf.in', encoding='utf-8').read()
t = t.replace('@ROOT@', root).replace('@ID@', pid).replace('@DESC@', desc).replace('@NOTE@', note.replace('"', "'"))
open(out, 'w', encoding='utf-8').write(t)
PY
"$STEAMCMD" +login "$USER" +workshop_build_item "$VDF" +quit | tee "$ROOT/build/upload.log"
NEW=$(sed -n 's/.*PublishFileID \([0-9]*\).*/\1/p' "$ROOT/build/upload.log" | tail -1)
[ -n "$NEW" ] && [ "$ID" = 0 ] && echo "$NEW" > "$ROOT/workshop/published_id" && echo "Published file id: $NEW"
