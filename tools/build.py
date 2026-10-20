#!/usr/bin/env python3
"""Build the Worth per Weight mod: appends "[2k ₭/kg]" to item names in every game language.

Reads item Cost/Weight straight from the installed game, so re-run after every game patch.

    python3 -I tools/build.py [--game DIR] [--repak PATH] [--retoc PATH] [--symbol ₭] [--format short|full] [--newline] [--out build]

Output is build/workshop/, a Steam Workshop content folder in Zone Kit layout (see tools/zonekit.py).
Tools, looked up in .bin/ first, then PATH:
  repak  cargo install --git https://github.com/trumank/repak repak_cli
  retoc  https://github.com/trumank/retoc @885a8da with tools/retoc-zonekit.patch applied
         (cargo build --locked --release -p retoc_cli)
"""
import argparse, csv, os, shutil, subprocess, sys, tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bin2cfg, locres, zonekit  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent

NS = 'ST_S2BaseGameLocalization'
DEFAULT_GAME = '/data/steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl'  # or --game / STALKER2_DIR
SKIP_TYPES = {'EItemType::Info', 'EItemType::None', None}  # quest documents etc. cannot be sold
ITEM_PAKS = {  # pak (relative to game dir) -> ItemPrototypes.cfg.bin inside it
    'Stalker2/Content/Paks/pakchunk0-Windows.pak': 'Stalker2/Content/GameLite/GameData/ItemPrototypes.cfg.bin',
    'Stalker2/Content/Paks/pakchunk101-Windows.pak': 'Stalker2/Content/GameLite/DLCGameData/PreOrder/ItemPrototypes.cfg.bin',
    'Stalker2/Content/Paks/pakchunk102-Windows.pak': 'Stalker2/Content/GameLite/DLCGameData/Deluxe/ItemPrototypes.cfg.bin',
    'Stalker2/Content/Paks/pakchunk104-Windows.pak': 'Stalker2/Content/GameLite/DLCGameData/Ultimate/ItemPrototypes.cfg.bin',
    'DLC1/S2_DLC1Stalker2-Windows.pak': 'Stalker2/Content/GameLite/DLCGameData/DLC1/ItemPrototypes.cfg.bin',
}
LOC_PAK = 'Stalker2/Content/Paks/pakchunk0-Windows.pak'
LOC_DIR = 'Stalker2/Content/Localization/Game'
# Every rated item is pointed (via cfg patch) at a copy of its texts under a new LocalizationSID with this prefix,
# the approach DecoupledRanks uses. Overriding the original keys directly would also work (see FINDINGS.md).
LOC_PREFIX = 'WPW_'
LOC_SUFFIXES = ('name', 'description', 'short_name', 'short_description')
PATCH_NAME = 'WorthPerWeight'


def tool(name):
    """Look for a helper binary in .bin/ first, then on PATH."""
    local = ROOT / '.bin' / name
    return str(local) if local.exists() else shutil.which(name) or name


def unpack(repak, pak, pattern, out):
    subprocess.run([repak, 'unpack', '-q', '-f', '-i', pattern, '-o', str(out), str(pak)], check=True)


def read_roots(data):
    """Yield (root node, source cfg path) for every root struct of a .cfg.bin."""
    r = bin2cfg.BinaryCursor(data)
    version, pool = bin2cfg.read_binary_header(r)
    bin2cfg.skip_post_pool_padding(r, version)
    for _ in range(r.read_int32()):
        pos = r.position
        block = bin2cfg.read_binary_cfg_config(r, len(pool), version)
        r.position = pos
        node = bin2cfg.read_binary_struct(r, pool, version)
        src = bin2cfg.get_binary_string(block.values[3], pool) if len(block.values) > 3 else ''
        yield node, src


def load_items(repak, game, tmp):
    """Sellable items with resolved Cost/Weight/Type/LocalizationSID and the cfg files that define them."""
    fields, parent, sources = {}, {}, {}
    for pak, inner in ITEM_PAKS.items():
        if not (game / pak).exists():
            continue
        unpack(repak, game / pak, inner, tmp)
        for node, src in read_roots((tmp / inner).read_bytes()):
            sid = node.get('SID') or node.__internal__.rawName
            f = fields.setdefault(sid, {})
            f.update({k: v for k, v in node.entries() if not isinstance(v, bin2cfg.Node)})
            if src and src not in sources.setdefault(sid, []):
                sources[sid].append(src)
            if node.__internal__.refkey:
                parent[sid] = str(node.__internal__.refkey).strip('[]')
            elif sid not in parent:
                parent[sid] = None

    def get(sid, key, depth=0):
        while sid is not None and depth < 32:
            f = fields.get(sid)
            if f is None:
                return None
            if key in f:
                return f[key]
            sid, depth = parent.get(sid), depth + 1
        return None

    items = []
    for sid in fields:
        if sid.startswith('Template') or sid in ('Empty', '0'):
            continue
        cost, weight, typ = get(sid, 'Cost'), get(sid, 'Weight'), get(sid, 'Type')
        if not isinstance(cost, float) or not isinstance(weight, float) or cost <= 0 or weight <= 0:
            continue
        if typ in SKIP_TYPES:
            continue
        items.append({'SID': sid, 'Type': str(typ).replace('EItemType::', ''), 'Cost': cost, 'Weight': weight,
                      'PerKg': cost / weight, 'LocSID': get(sid, 'LocalizationSID') or sid,
                      'Sources': sources.get(sid, [])})
    return items


def patch_path(src):
    """cfg patch location for a source file: GameData/X/Y.cfg -> GameData/X/Y/Y_patch_<Mod>.cfg"""
    stem = src[:-len('.cfg')]
    return f"{stem}/{stem.rsplit('/', 1)[-1]}_patch_{PATCH_NAME}.cfg"


def short(x):
    """Thousands, rounded: 112 -> <1k, 1890 -> 2k, 266667 -> 267k."""
    return '<1k' if x < 500 else f'{round(x / 1000):,}k'


def full(x):
    """Two significant figures with separators: 112 -> 110, 1890 -> 1,900, 266667 -> 270,000."""
    if x < 100:
        return str(round(x))
    p = 10 ** (len(str(int(x))) - 2)
    return f'{int(round(x / p) * p):,}'


def tag(per_kg, a):
    value = short(per_kg) if a.format == 'short' else full(per_kg)
    return ('\n' if a.newline else ' ') + f'[{value} {a.symbol}/kg]'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', default=os.environ.get('STALKER2_DIR', DEFAULT_GAME))
    ap.add_argument('--repak', default=tool('repak'))
    ap.add_argument('--retoc', default=tool('retoc'))
    ap.add_argument('--symbol', default='₭', help='currency sign (default ₭)')
    ap.add_argument('--format', choices=('short', 'full'), default='short', help='2k (default) or 1,900')
    ap.add_argument('--newline', action='store_true', help='put the rating on its own line')
    ap.add_argument('--out', default=str(ROOT / 'build'))
    a = ap.parse_args()
    game, out = Path(a.game), Path(a.out)

    with tempfile.TemporaryDirectory() as t:
        tmp = Path(t)
        items = load_items(a.repak, game, tmp / 'items')

        # one rating per name key: prefer the item whose SID owns the key, else the first seen
        rating = {}
        for it in items:
            key = f"sid_items_{it['LocSID']}_name"
            if key not in rating or it['SID'] == it['LocSID']:
                rating[key] = it

        unpack(a.repak, game / LOC_PAK, f'{LOC_DIR}/*', tmp / 'loc')
        entries, stats = {}, {}
        for lang_dir in sorted((tmp / 'loc' / LOC_DIR).iterdir()):
            if not lang_dir.is_dir():
                continue
            table = locres.Locres(lang_dir / 'Game.locres').table(NS)
            n = 0
            for key, it in rating.items():
                e = table.get(key)
                if e is None or not e[3].strip():
                    continue
                for suffix in LOC_SUFFIXES:
                    src = table.get(f"sid_items_{it['LocSID']}_{suffix}")
                    if src is None:
                        continue
                    text = src[3]
                    if suffix == 'name':
                        text = text + tag(it['PerKg'], a)
                        if lang_dir.name == 'en':
                            it['Name'] = text
                    entries.setdefault(f"sid_items_{LOC_PREFIX}{it['LocSID']}_{suffix}", {})[lang_dir.name] = text
                n += 1
            stats[lang_dir.name] = n

        # point every item sharing a rated LocalizationSID at the copied texts
        renamed = {it['LocSID'] for it in rating.values() if it.get('Name')}
        patches = {}
        for it in items:
            if it['LocSID'] not in renamed:
                continue
            for src in it['Sources']:
                patches.setdefault(patch_path(src), []).append(
                    f"{it['SID']} : struct.begin {{bpatch}}\n   LocalizationSID = {LOC_PREFIX}{it['LocSID']}\nstruct.end\n")
        patches = {p: '\n'.join(blocks) for p, blocks in patches.items()}

        out.mkdir(parents=True, exist_ok=True)
        workshop = zonekit.package(entries, patches, ROOT / 'template', out / 'workshop', a.retoc, a.repak, tmp)

        with open(out / 'ratings.csv', 'w', newline='', encoding='utf-8') as f:
            w = csv.DictWriter(f, ['Name', 'SID', 'Type', 'Cost', 'Weight', 'PerKg', 'LocSID'], extrasaction='ignore')
            w.writeheader()
            for it in sorted(rating.values(), key=lambda r: -r['PerKg']):
                if it.get('Name'):
                    w.writerow({**it, 'PerKg': round(it['PerKg'])})

    print(f'{len(items)} sellable items, {len(rating)} name keys, {len(renamed)} renamed, '
          f'{sum(b.count("bpatch") for b in patches.values())} item patches in {len(patches)} files, {len(entries)} text keys')
    print('per language:', ' '.join(f'{k}={v}' for k, v in stats.items()))
    print(f'workshop folder: {workshop}')


if __name__ == '__main__':
    main()
