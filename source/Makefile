# Makefile for executables and other data to be packaged.
#
# Tools needed:
# - PlaydateSDK from https://play.date/dev
#
# - ARM GNU toolchain, available here:
#   https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain
#
#   This is the one I used:
#   arm-gnu-toolchain-14.3.rel1-mingw-w64-x86_64-arm-none-eabi.exe
#
# - GCC, MingW, and libpng.  All available through Cygwin.
#
#   GCC and libpng are used during the build process.  MingW is needed to
#   build Windows binaries.


# ......................................................................
# {{{ Settings.

# Check environment variables.
# PLAYDATE_SDK_PATH is where Playdate SDK is installed, e.g.:
# "C:\users\omoikane\Documents\PlaydateSDK"
#
# TL_PATH is where ARM SDK is installed, e.g.:
# "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.3 rel1\bin"
ifeq ($(PLAYDATE_SDK_PATH),)
$(error need to set PLAYDATE_SDK_PATH environment)
endif
ifeq ($(TL_PATH),)
$(error need to set TL_PATH environment)
endif

INC_PATH = "$(PLAYDATE_SDK_PATH)/C_API"

# Tool settings to build for windows simulator, using MingW on Cygwin.
SIM_PREFIX = x86_64-w64-mingw32-
SIM_EXT = dll
SIM_BUILD_DIR = sim_build
SIM_CC = $(SIM_PREFIX)gcc
SIM_CP = $(SIM_PREFIX)objcopy
SIM_AS = $(SIM_PREFIX)gcc -x assembler-with-cpp
SIM_STRIP = $(SIM_PREFIX)strip
SIM_TOOL_ROOT = /usr/x86_64-w64-mingw32/sys-root/mingw

SIM_ASFLAGS =
SIM_CFLAGS = \
	-DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 \
	-O2 -Wall -Wstrict-prototypes -Wno-unknown-pragmas -Wdouble-promotion \
	-flto
SIM_LFLAGS = -shared -fPIC -flto

# Tool settings to build for the device.
DEVICE_PREFIX = "$(TL_PATH)/arm-none-eabi-"
DEVICE_BUILD_DIR = device_build
DEVICE_CC = $(DEVICE_PREFIX)gcc
DEVICE_CP = $(DEVICE_PREFIX)objcopy
DEVICE_AS = $(DEVICE_PREFIX)gcc -x assembler-with-cpp
DEVICE_STRIP = $(DEVICE_PREFIX)strip

HEAP_SIZE = 8388208
STACK_SIZE = 61800
MCFLAGS = -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -D__FPU_USED=1

DEVICE_ASFLAGS = \
	-x assembler-with-cpp \
	-D__HEAP_SIZE=$(HEAP_SIZE) \
	-D__STACK_SIZE=$(STACK_SIZE)
DEVICE_CFLAGS = \
	-DNDEBUG \
	-DTARGET_PLAYDATE=1 -DTARGET_EXTENSION=1 \
	-O2 -Wall -Wno-unknown-pragmas -Wdouble-promotion \
	-flto \
	-gdwarf-2 \
	-falign-functions=16 -fomit-frame-pointer \
	-fverbose-asm \
	-ffunction-sections -fdata-sections \
	-fno-common -fno-exceptions \
	-mword-relocations \
	$(MCFLAGS)
DEVICE_LFLAGS = \
	-nostartfiles \
	$(MCFLAGS) \
	-flto \
	-Tlink_map.ld \
	"-Wl,-Map=$(DEVICE_BUILD_DIR)/game.map,--cref,--gc-sections,--no-warn-mismatch,--emit-relocs" \
	--entry eventHandlerShim

# Tool settings for various binaries that are not directly included in the
# final executables.
BUILD_DIR = build
CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -pedantic -march=native

# }}}

# ......................................................................
# {{{ Primary targets.

# Toplevel target.
all: $(SIM_BUILD_DIR)/pdex.$(SIM_EXT) $(DEVICE_BUILD_DIR)/pdex.elf

# Compile rules.
#
# Note every individual .o file depends on all .h files in the current
# directory, due to "$(wildcard *.h)".  Every other way to get a more
# accurate set of header dependencies is more complicated than what we
# wanted, so we just rebuild everything whenever any header file changes.
SRCS = main.c setup.c bgm.c slime.c world.c
OBJS = $(SRCS:.c=.o)
SIM_OBJS = $(addprefix $(SIM_BUILD_DIR)/, $(OBJS))
DEVICE_OBJS = $(addprefix $(DEVICE_BUILD_DIR)/, $(OBJS))

$(SIM_BUILD_DIR)/%.o: %.c $(wildcard *.h) | make_sim_build_dir
	$(SIM_CC) $(SIM_CFLAGS) -I $(INC_PATH) -c $< -o $@

$(SIM_BUILD_DIR)/%.s: %.c $(wildcard *.h) | make_sim_build_dir
	$(SIM_CC) $(substr -flto,,$(SIM_CFLAGS)) -I $(INC_PATH) -S -fverbose-asm -c $< -o $@

$(SIM_BUILD_DIR)/%.o: %.s | make_sim_build_dir
	$(SIM_AS) $(SIM_ASFLAGS) -c $< -o $@

$(DEVICE_BUILD_DIR)/%.o: %.c $(wildcard *.h) | make_device_build_dir
	$(DEVICE_CC) $(DEVICE_CFLAGS) -I $(INC_PATH) -c $< -o $@

$(DEVICE_BUILD_DIR)/%.s: %.c $(wildcard *.h) | make_device_build_dir
	$(DEVICE_CC) $(subst -flto,,$(DEVICE_CFLAGS)) -I $(INC_PATH) -S -fverbose-asm -c $< -o $@

$(DEVICE_BUILD_DIR)/%.o: %.s | make_device_build_dir
	$(DEVICE_AS) $(DEVICE_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c $(wildcard *.h) | make_build_dir
	$(CC) $(CFLAGS) -c $< -o $@

$(DEVICE_BUILD_DIR)/main.o: main.c $(wildcard *.h) $(BUILD_DIR)/version.txt | make_device_build_dir

$(SIM_BUILD_DIR)/main.o: main.c $(wildcard *.h) $(BUILD_DIR)/version.txt | make_sim_build_dir

$(DEVICE_BUILD_DIR)/slime.o: slime.c $(wildcard *.h) $(BUILD_DIR)/velocity_table.txt | make_device_build_dir

$(SIM_BUILD_DIR)/slime.o: slime.c $(wildcard *.h) $(BUILD_DIR)/velocity_table.txt | make_sim_build_dir

$(DEVICE_BUILD_DIR)/world.o: world.c $(wildcard *.h) $(BUILD_DIR)/gray_patterns.txt | make_device_build_dir

$(SIM_BUILD_DIR)/world.o: world.c $(wildcard *.h) $(BUILD_DIR)/gray_patterns.txt | make_sim_build_dir

# Pregenerated data.
$(BUILD_DIR)/velocity_table.txt: generate_velocity_table.pl | make_build_dir
	perl $< > $@

$(BUILD_DIR)/gray_patterns.txt: generate_gray_patterns.pl | make_build_dir
	perl $< > $@

# Build version string from pdxinfo.  We would like to access this
# programmatically, but the C API doesn't have metadata access, so we
# will generate it during the build process.
$(BUILD_DIR)/version.txt: pdxinfo | make_build_dir
	echo -n 'static const char kVersion[] = "' > $@
	sed -ne 's/name=\(.*\)/\1/;T;p' pdxinfo | tr -d '\n' >> $@
	echo -n ' v' >> $@
	sed -ne 's/version=\(.*\)/\1/;T;p' pdxinfo | tr -d '\n' >> $@
	echo '";' >> $@

# Link rules.
$(SIM_BUILD_DIR)/pdex.$(SIM_EXT): $(SIM_BUILD_DIR)/pdex_unstripped.$(SIM_EXT)
	$(SIM_STRIP) --strip-unneeded -g $< -o $@

$(SIM_BUILD_DIR)/pdex_unstripped.$(SIM_EXT): $(SIM_OBJS)
	$(SIM_CC) $(SIM_LFLAGS) -lm -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 $(SIM_OBJS) -o $@

$(DEVICE_BUILD_DIR)/pdex.elf: $(DEVICE_BUILD_DIR)/pdex_unstripped.elf
	$(DEVICE_STRIP) --strip-unneeded -R .comment -g $< -o $@

$(DEVICE_BUILD_DIR)/pdex_unstripped.elf: $(DEVICE_OBJS) link_map.ld
	$(DEVICE_CC) $(DEVICE_LFLAGS) $(DEVICE_OBJS) -o $@

$(BUILD_DIR)/pack_png.exe: $(BUILD_DIR)/pack_png.o
	$(CC) $(CFLAGS) $^ -lpng -o $@

# Maintenance rules.
make_sim_build_dir: $(SIM_BUILD_DIR)

$(SIM_BUILD_DIR):
	mkdir -p $@

make_device_build_dir: $(DEVICE_BUILD_DIR)

$(DEVICE_BUILD_DIR):
	mkdir -p $@

make_build_dir: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

clean:
	-rm -rf $(SIM_BUILD_DIR) $(DEVICE_BUILD_DIR) $(BUILD_DIR)

# }}}

# ......................................................................
# {{{ Tests and random experiments.

$(BUILD_DIR)/%.test_passed: $(BUILD_DIR)/%.exe
	./$< && touch $@

test: \
	$(BUILD_DIR)/common_test.test_passed \
	$(BUILD_DIR)/inline_constants.test_passed \
	$(BUILD_DIR)/strip_lua.test_passed

$(BUILD_DIR)/common_test.exe: $(BUILD_DIR)/common_test.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/inline_constants.test_passed: inline_constants.pl inline_constants_test.sh
	./inline_constants_test.sh $< && touch $@

$(BUILD_DIR)/strip_lua.test_passed: strip_lua.pl strip_lua_test.sh
	./strip_lua_test.sh $< && touch $@

# }}}
