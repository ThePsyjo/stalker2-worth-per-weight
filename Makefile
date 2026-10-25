# Worth per Weight — build pipeline. `make` builds the Workshop folder; see README.md.
#
#   make tools          build repak + patched retoc into .bin/
#   make bp             generate + cook the Blueprint (needs UE 5.5 for Linux)
#   make mod            pack build/workshop/ (default)
#   make install-local  copy build/workshop/ over the local Workshop download (for testing before upload)
#   make preview        regenerate workshop/preview.jpg
#   make upload STEAM_USER=<name> NOTE="change note"
#   make clean

UE_ROOT    ?= /data/ue
GAME       ?= /data/steam/steamapps/common/S.T.A.L.K.E.R. 2 Heart of Chornobyl
WORKSHOP   ?= /data/steam/steamapps/workshop/content/1643320
ITEM_ID    := $(shell cat workshop/published_id 2>/dev/null)
NOTE       ?= update
RETOC_REV  := 885a8da
PYTHON     := python3 -I

BIN    := .bin
WORK   := work
COOKED := ue/Stalker2/Saved/Cooked/Linux/Stalker2/Plugins/ValuePerKilo/Content
UE_SRC := $(shell find ue/Stalker2/Source -type f) ue/Stalker2/Stalker2.uproject
FONT   := $(WORK)/fonts/Stalker2/Content/GameLite/FPS_Game/UIRemaster/Fonts/Oswald-Medium.ufont

.PHONY: all tools bp mod install-local preview upload clean

all: mod

tools: $(BIN)/repak $(BIN)/retoc

$(BIN)/repak:
	mkdir -p $(BIN) $(WORK)
	cargo install --root $(WORK)/repak --git https://github.com/trumank/repak repak_cli
	cp $(WORK)/repak/bin/repak $@

$(BIN)/retoc: tools/retoc-zonekit.patch
	mkdir -p $(BIN) $(WORK)
	rm -rf $(WORK)/retoc-src
	git clone -q https://github.com/trumank/retoc $(WORK)/retoc-src
	cd $(WORK)/retoc-src && git checkout -q $(RETOC_REV) && git apply ../../tools/retoc-zonekit.patch \
		&& cargo build --locked --release -p retoc_cli
	cp $(WORK)/retoc-src/target/release/retoc $@

bp: $(COOKED)/WPW_Init.uasset

$(COOKED)/WPW_Init.uasset: $(UE_SRC) tools/build_bp.sh
	tools/build_bp.sh $(UE_ROOT)

mod: tools bp
	$(PYTHON) tools/build.py --game "$(GAME)"

install-local: mod
	@test -n "$(ITEM_ID)" || { echo "workshop/published_id missing"; exit 1; }
	@test -d "$(WORKSHOP)/$(ITEM_ID)" || { echo "not subscribed: $(WORKSHOP)/$(ITEM_ID)"; exit 1; }
	rm -rf "$(WORKSHOP)/$(ITEM_ID)/Windows" "$(WORKSHOP)/$(ITEM_ID)/Stalker2"
	cp -a build/workshop/Windows "$(WORKSHOP)/$(ITEM_ID)/"

$(FONT): | $(BIN)/repak
	$(BIN)/repak unpack -q -f -i 'Stalker2/Content/GameLite/FPS_Game/UIRemaster/Fonts/Oswald-Medium.ufont' \
		-o $(WORK)/fonts "$(GAME)/Stalker2/Content/Paks/pakchunk0-Windows.pak"

preview: $(FONT) tools/preview.py
	$(PYTHON) tools/preview.py $(FONT) workshop/preview.jpg

upload: mod
	@test -n "$(STEAM_USER)" || { echo "usage: make upload STEAM_USER=<name> NOTE=\"change note\""; exit 1; }
	workshop/upload.sh "$(STEAM_USER)" "$(NOTE)"

clean:
	rm -rf build ue/Stalker2/Saved/Cooked ue/Stalker2/Plugins/ValuePerKilo/Content/*.uasset
