# Build Klad-Reimagined for native (dev) and WebAssembly (ship). C23.
#
#   make            -> native binary (build/klad), needs raylib + GL/X11 dev libs
#   make web        -> build/web/index.html (+ .wasm .js), needs emscripten + raylib-web
#   make run        -> build & run native
#   make clean
#
# raylib (native):  apt install libraylib-dev   (or build from source)
# emscripten:       https://emscripten.org  (emsdk); then `emcc` on PATH
# raylib (web):     build raylib with PLATFORM=PLATFORM_WEB, point RAYLIB_WEB at libraylib.a

CC      ?= cc
CSTD    := -std=c2x
WARN    := -Wall -Wextra
SRC     := main.c game.c
OUT     := ../build

# ---------- native ----------
$(OUT)/klad: $(SRC) game.h i18n.h levels_data.h
	@mkdir -p $(OUT)
	$(CC) $(CSTD) $(WARN) $(SRC) -o $@ $(shell pkg-config --cflags --libs raylib 2>/dev/null || echo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11)

.PHONY: run clean web
run: $(OUT)/klad
	$(OUT)/klad

# ---------- web (emscripten) ----------
# Set RAYLIB_WEB to your web-built libraylib.a, and RAYLIB_SRC to its src/ (for raylib.h).
RAYLIB_WEB ?= $(HOME)/raylib/src/libraylib.web.a
RAYLIB_SRC ?= $(HOME)/raylib/src
web: $(SRC) game.h
	@mkdir -p $(OUT)/web
	emcc $(CSTD) $(SRC) -o $(OUT)/web/index.html \
	  -I$(RAYLIB_SRC) $(RAYLIB_WEB) \
	  -DPLATFORM_WEB \
	  -s USE_GLFW=3 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 \
	  --shell-file web/shell.html
	@echo "built $(OUT)/web/index.html — serve with: python3 -m http.server -d $(OUT)/web"

clean:
	rm -rf $(OUT)
