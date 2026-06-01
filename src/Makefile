# Makefile — КЛАД C23 reimplementare
# Target: native Linux + WebAssembly
# Sursa: disassembly/annotated/uknc_klad_1987.asm

CC      := cc
CSTD    := -std=c2x
WARN    := -Wall -Wextra -Wpedantic
OPT     := -O2

RAYLIB_SRC ?= /tmp/raylib-5.5/src
XLIB       ?= /tmp/xlibs/usr/lib/x86_64-linux-gnu

SRCS := main.c game.c map.c player.c enemy.c score.c render.c
INC  := -I$(RAYLIB_SRC) -I. -I../font
LIBS := $(RAYLIB_SRC)/libraylib.a \
        -L$(XLIB) -lGL -lm -lpthread -ldl -lrt -lX11 \
        -lXcursor -lXi -lXinerama -lXrandr

OUT_DIR := ../build
NATIVE  := $(OUT_DIR)/klad

# ── native ────────────────────────────────────────────────────────────────────
.PHONY: all run clean web

all: $(NATIVE)

$(NATIVE): $(SRCS) klad.h map.h player.h enemy.h score.h render.h i18n.h gfx_data.h level_data.h ../font/uknc_font.h
	@mkdir -p $(OUT_DIR)
	$(CC) $(CSTD) $(WARN) $(OPT) $(INC) $(SRCS) -o $@ $(LIBS)

run: $(NATIVE)
	$(NATIVE)

# ── web (emscripten) ──────────────────────────────────────────────────────────
RAYLIB_WEB ?= $(HOME)/raylib/src/libraylib.web.a
WEB_OUT    := $(OUT_DIR)/web

web: $(SRCS) klad.h map.h player.h enemy.h score.h render.h i18n.h gfx_data.h level_data.h ../font/uknc_font.h
	@mkdir -p $(WEB_OUT)
	emcc $(CSTD) $(WARN) $(OPT) $(INC) $(SRCS) \
	  -o $(WEB_OUT)/index.html \
	  $(RAYLIB_WEB) \
	  -DPLATFORM_WEB \
	  -s USE_GLFW=3 \
	  -s ASYNCIFY \
	  -s ALLOW_MEMORY_GROWTH=1 \
	  --shell-file web/shell.html
	@echo "web build → $(WEB_OUT)/index.html"

clean:
	rm -rf $(OUT_DIR)

# ---------- E2E headless movement/collision tests (no raylib) ----------
test:
	$(CC) $(CSTD) $(WARN) -I. ../tools/test_e2e.c map.c player.c enemy.c score.c -lm -o /tmp/klad_e2e
	@/tmp/klad_e2e
