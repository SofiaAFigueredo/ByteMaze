CC ?= gcc

APP := bytemaze
CONTEST_LIMIT := 1474560
RAYLIB_BUILD_DIR := build/raylib-tiny
RAYLIB_LIB_DIR := $(RAYLIB_BUILD_DIR)/raylib

# ---------------------------------------------------------------------------
# OS detection. UNAME_S drives every OS-specific choice below (binary name,
# system libraries, and raylib's own platform backend).
# ---------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

ifeq ($(DETECTED_OS),Windows)
    APP_BIN := $(APP).exe
    APP_LIBS := -lraylib -lm -lgdi32 -lwinmm -lole32
else ifeq ($(DETECTED_OS),Darwin)
    APP_BIN := $(APP)
    APP_LIBS := -lraylib -lm -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL
else
    APP_BIN := $(APP)
    APP_LIBS := -lraylib -lm -ldl -lpthread -lGL -lrt -lX11
endif

RAYLIB_CMAKE_FLAGS := \
	-DCMAKE_BUILD_TYPE=MinSizeRel \
	-DBUILD_SHARED_LIBS=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DCUSTOMIZE_BUILD=ON \
	-DSUPPORT_MODULE_RMODELS=OFF \
	-DSUPPORT_MODULE_RAUDIO=ON \
	-DUSE_AUDIO=ON \
	-DSUPPORT_TRACELOG=OFF \
	-DSUPPORT_CAMERA_SYSTEM=OFF \
	-DSUPPORT_GESTURES_SYSTEM=OFF \
	-DSUPPORT_SCREEN_CAPTURE=OFF \
	-DSUPPORT_COMPRESSION_API=OFF \
	-DSUPPORT_AUTOMATION_EVENTS=OFF \
	-DSUPPORT_CLIPBOARD_IMAGE=OFF \
	-DSUPPORT_QUADS_DRAW_MODE=OFF \
	-DSUPPORT_IMAGE_EXPORT=OFF \
	-DSUPPORT_IMAGE_GENERATION=OFF \
	-DSUPPORT_FILEFORMAT_PNG=OFF \
	-DSUPPORT_FILEFORMAT_BMP=OFF \
	-DSUPPORT_FILEFORMAT_GIF=OFF \
	-DSUPPORT_FILEFORMAT_QOI=OFF \
	-DSUPPORT_FILEFORMAT_DDS=OFF \
	-DSUPPORT_FILEFORMAT_TTF=ON \
	-DSUPPORT_FILEFORMAT_FNT=OFF \
	-DCMAKE_C_FLAGS_MINSIZEREL='-Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident'

# MinGW's linker doesn't understand every flag GNU ld on Linux/macOS does;
# -fno-ident and -fno-unwind-tables are still fine everywhere, but we keep
# the LTO/section-gc pieces common across all three toolchains.
APP_CFLAGS := -Os -flto -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident -finput-charset=UTF-8 -fexec-charset=UTF-8 -DNDEBUG
APP_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all

.PHONY: all release run size clean

all: release

release: $(APP_BIN)

$(APP_BIN): $(RAYLIB_LIB_DIR)/libraylib.a src/main.c
	$(CC) $(APP_CFLAGS) src/main.c -o $(APP_BIN) -I raylib/src -L $(RAYLIB_LIB_DIR) $(APP_LDFLAGS) $(APP_LIBS)

$(RAYLIB_LIB_DIR)/libraylib.a:
	cmake -S raylib -B $(RAYLIB_BUILD_DIR) $(RAYLIB_CMAKE_FLAGS)
	cmake --build $(RAYLIB_BUILD_DIR) -j2

run: $(APP_BIN)
	./$(APP_BIN)

size: $(APP_BIN)
	@bytes=$$(wc -c < $(APP_BIN) | tr -d ' '); \
	asset_bytes=$$(if [ -d src/assets ]; then find src/assets -type f -exec wc -c {} \; | awk '{s+=$$1} END {print s+0}'; else echo 0; fi); \
	total=$$((bytes + asset_bytes)); \
	percent=$$(awk 'BEGIN { printf "%.2f", ('"$$total"' / $(CONTEST_LIMIT)) * 100 }'); \
	echo "$$total bytes ($$bytes executavel + $$asset_bytes assets, $$percent% de $(CONTEST_LIMIT))"

clean:
	rm -rf $(RAYLIB_BUILD_DIR) $(APP) $(APP).exe
