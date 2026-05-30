# Build Klad-Reimagined for native (dev) and WebAssembly (ship). C23.
#
#   make            -> native binary (build/klad), needs raylib + GL/X11 dev libs
#   make web        -> build/web/index.html (+ .wasm .js), needs emscripten + raylib-web
#   make run        -> build & run native
#   make clean
#
# raylib (native):  apt install libraylib-dev
#   OR build from source into RAYLIB_SRC (defaults to /tmp/raylib-5.5/src):
#     cd /tmp && wget -q -O raylib.tar.gz https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
#     tar xzf raylib.tar.gz
#     CUSTOM_CFLAGS="-I/tmp/xlibs/usr/include" make -C /tmp/raylib-5.5/src PLATFORM=PLATFORM_DESKTOP
#   Missing X11 dev headers (if needed without sudo):
#     mkdir -p /tmp/xlibs
#     for p in libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libxrender-dev libxfixes-dev libxext-dev; do
#       apt-get download $p && dpkg-deb -x ${p}_*.deb /tmp/xlibs; done
#     (and the .so files: libxcursor1 libxi6 libxinerama1 libxrandr2)
#
# emscripten:       https://emscripten.org  (emsdk); then `emcc` on PATH
# raylib (web):     build raylib with PLATFORM=PLATFORM_WEB, point RAYLIB_WEB at libraylib.a

CC      ?= cc
CSTD    := -std=c2x
WARN    := -Wall -Wextra
SRC     := main.c game.c
OUT     := ../build

# Local raylib build (fallback when pkg-config not available)
RAYLIB_SRC ?= /tmp/raylib-5.5/src
XLIB       ?= /tmp/xlibs/usr/lib/x86_64-linux-gnu

# ---------- native ----------
$(OUT)/klad: $(SRC) game.h i18n.h levels_data.h
	@mkdir -p $(OUT)
	$(CC) $(CSTD) $(WARN) -I$(RAYLIB_SRC) $(SRC) -o $@ \
	  $(shell pkg-config --cflags --libs raylib 2>/dev/null || \
	    echo "$(RAYLIB_SRC)/libraylib.a -L$(XLIB) -lGL -lm -lpthread -ldl -lrt -lX11 -lXcursor -lXi -lXinerama -lXrandr")

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
