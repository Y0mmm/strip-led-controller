#######################
# SDCC Makefile for the STM8 LED controller project.
#######################

CC               = sdcc
LD               = sdcc
OPTIMIZE         =
CFLAGS           = -mstm8 --std-sdcc99 --std-c99 $(OPTIMIZE)
LFLAGS           = -mstm8 -lstm8 --out-fmt-ihx

OUTPUT_DIR       = SDCC
TARGET           = $(OUTPUT_DIR)/main.ihx
MAP_FILE         = $(OUTPUT_DIR)/main.map
DEVICE_FLASH_BYTES = 8192
DEVICE_RAM_BYTES   = 1024

SRC_DIRS         = . src/config src/app src/drivers
INC_DIRS         = . src/config src/app src/drivers
SOURCE           = main.c \
                   src/config/led_config.c \
                   src/app/led_state.c \
                   src/app/led_features.c \
                   src/app/ir_decoder.c \
                   src/app/ir_commands.c \
                   src/drivers/peripherals.c
HEADER           = $(foreach d,$(INC_DIRS),$(wildcard $(d)/*.h))
OBJECTS          = $(addprefix $(OUTPUT_DIR)/,$(SOURCE:.c=.rel))
INCLUDE          = $(foreach d,$(INC_DIRS),$(addprefix -I,$(d)))

vpath %.c $(SRC_DIRS)
vpath %.h $(INC_DIRS)

.PHONY: clean all default

.PRECIOUS: $(TARGET) $(OBJECTS)

default: $(OUTPUT_DIR) $(TARGET)

all: default

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(TARGET): $(OBJECTS)
	$(LD) $(LFLAGS) -o $@ $(OBJECTS)
	@perl -e 'my ($$map, $$ihx, $$flash_limit, $$ram_limit) = @ARGV; open my $$fh, "<", $$map or die $$!; my ($$flash, $$ram) = (0, 0); while (<$$fh>) { if (/^(CODE|CONST|HOME|GSINIT|GSFINAL|INITIALIZER)\s+[0-9A-F]+\s+([0-9A-F]+)\s+=/) { $$flash += hex($$2); } elsif (/^(DATA|INITIALIZED)\s+[0-9A-F]+\s+([0-9A-F]+)\s+=/) { $$ram += hex($$2); } } my $$ihx_bytes = -s $$ihx; my $$flash_pct = $$flash_limit ? 100 * $$flash / $$flash_limit : 0; my $$ram_pct = $$ram_limit ? 100 * $$ram / $$ram_limit : 0; printf "Firmware size: flash=%d/%d bytes (%.1f%%), ram=%d/%d bytes (%.1f%%), ihx=%d bytes\n", $$flash, $$flash_limit, $$flash_pct, $$ram, $$ram_limit, $$ram_pct, $$ihx_bytes;' $(MAP_FILE) $@ $(DEVICE_FLASH_BYTES) $(DEVICE_RAM_BYTES)

$(OUTPUT_DIR)/%.rel: %.c $(HEADER)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -fr $(OUTPUT_DIR)/*

flash: 
	stm8flash -c stlinkv2 -w SDCC/main.ihx -p stm8s003j3
