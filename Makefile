CC ?= gcc

APP := bytemaze
CONTEST_LIMIT := 1474560
RAYLIB_BUILD_DIR := build/raylib-tiny
RAYLIB_LIB_DIR := $(RAYLIB_BUILD_DIR)/raylib

RAYLIB_CMAKE_FLAGS := \
	-DCMAKE_BUILD_TYPE=MinSizeRel \
	-DBUILD_SHARED_LIBS=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DCUSTOMIZE_BUILD=ON \
	-DSUPPORT_MODULE_RMODELS=OFF \
	-DSUPPORT_MODULE_RAUDIO=OFF \
	-DUSE_AUDIO=OFF \
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
	-DSUPPORT_FILEFORMAT_TTF=OFF \
	-DSUPPORT_FILEFORMAT_FNT=OFF \
	-DCMAKE_C_FLAGS_MINSIZEREL='-Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident'

APP_CFLAGS := -Os -flto -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident -DNDEBUG
APP_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all
APP_LIBS := -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

.PHONY: all release run size clean

all: release

release: $(APP)

$(APP): $(RAYLIB_LIB_DIR)/libraylib.a src/main.c
	$(CC) $(APP_CFLAGS) src/main.c -o $(APP) -I raylib/src -L $(RAYLIB_LIB_DIR) $(APP_LDFLAGS) $(APP_LIBS)

$(RAYLIB_LIB_DIR)/libraylib.a:
	cmake -S raylib -B $(RAYLIB_BUILD_DIR) $(RAYLIB_CMAKE_FLAGS)
	cmake --build $(RAYLIB_BUILD_DIR) -j2

run: $(APP)
	./$(APP)

size: $(APP)
	@bytes=$$(stat -c '%s' $(APP)); \
	percent=$$(awk 'BEGIN { printf "%.2f", ('"$$bytes"' / $(CONTEST_LIMIT)) * 100 }'); \
	echo "$$bytes bytes ($$percent% de $(CONTEST_LIMIT))"

clean:
	rm -rf $(RAYLIB_BUILD_DIR) $(APP)
