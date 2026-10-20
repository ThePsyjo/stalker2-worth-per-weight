# Worth per Weight — S.T.A.L.K.E.R. 2 mod

Appends each sellable item's base value per kilogram to its name, e.g. `Medkit [2k ₭/kg]`,
so you can see at a glance what is worth carrying. All 18 game languages, base game + DLC items.

- Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3823156532
- Value = base `Cost` / `Weight` from the item prototypes, in thousands (`<1k` below 500).
  Traders pay a fraction of the base price and less for worn gear: compare items, don't read it as a sale price.
- Quest items and items without price or weight are skipped. Items from other mods are not rated.

## Building

Requirements: Python 3, an installed copy of the game, and two Rust tools in `.bin/` (or on `PATH`):

```sh
# repak (pak files)
cargo install --root . --git https://github.com/trumank/repak repak_cli && mv bin/repak .bin/
# retoc (IoStore containers), patched for Zone Kit compatibility
git clone https://github.com/trumank/retoc /tmp/retoc && cd /tmp/retoc && git checkout 885a8da \
  && git apply /path/to/this/repo/tools/retoc-zonekit.patch && cargo build --locked --release -p retoc_cli \
  && cp target/release/retoc /path/to/this/repo/.bin/
```

Then:

```sh
python3 -I tools/build.py --game "/path/to/S.T.A.L.K.E.R. 2 Heart of Chornobyl"
```

Options: `--format short|full` (`2k` or `1,900`), `--newline` (rating on its own line; breaks list layouts),
`--symbol` (default `₭`). Outputs `build/workshop/` (Workshop content folder) and `build/ratings.csv`.
Re-run after every game patch.

Upload with `workshop/upload.sh <steam_username> "change note"` (SteamCMD; updates the item in `workshop/published_id`).

## How it works

S.T.A.L.K.E.R. 2 loads Steam Workshop mods after it has read `Game.locres`, so replacing that file only works
for loose `~mods` paks. Workshop mods ship texts in a `ModLocalizationDatabaseDataAsset`, the format the Zone Kit
cooks. This repo produces that without the Zone Kit:

1. `tools/bin2cfg.py` decodes `ItemPrototypes.cfg.bin`; `tools/build.py` resolves `refkey` inheritance for
   `Cost`/`Weight`/`LocalizationSID` and records each item's source cfg file.
2. Each rated item gets a cfg patch (`<File>/<File>_patch_WorthPerWeight.cfg`) pointing it at a new
   `LocalizationSID` (`WPW_<old>`), and the new `sid_items_WPW_*` keys get the original texts (from each
   language's `Game.locres`, read by `tools/locres.py`) plus the rating.
3. `tools/zonekit.py` serializes the text database and packs it with the boilerplate packages in `template/`
   into `NewContent` / `OverrideContent` IoStore containers, in the Zone Kit Workshop layout.

Details and pitfalls (package redirects, mount points, hashes) are in [FINDINGS.md](FINDINGS.md).

## Credits

- `tools/bin2cfg.py` by [joric](https://github.com/joric/stalker) (Unlicense), based on sdwvit's S2CfgToJSON.
- [retoc](https://github.com/trumank/retoc) (MIT) and [repak](https://github.com/trumank/repak) (Apache-2.0) by trumank.
- `template/` contains Zone Kit boilerplate (GameFeatureData, Wwise cook anchors, asset registry) derived from a
  Zone Kit-cooked Workshop mod ("True Gun Names") and renamed; it contains none of that mod's texts.

## License

MIT for this repo's own code, see [LICENSE](LICENSE). Third-party files keep their licenses.
