#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

export PATH := /c/devkitPro/devkitARM/bin:/c/devkitPro/tools/bin:$(PATH)

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

# Override LD to use gcc as the linker driver
LD = $(DEVKITARM)/bin/arm-none-eabi-gcc.exe

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary data
# GRAPHICS is a list of directories containing files to be processed by grit
#---------------------------------------------------------------------------------
TARGET    := SuperMetroidDSi
OUTPUT    := $(TARGET)
BUILD     := build
SOURCES   := src
INCLUDES  := include
DATA      := data
GRAPHICS  :=
GRAPHICS  :=

CFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
DATAFiles := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))

OFILES := $(patsubst %,$(BUILD)/%,$(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(SFILES:.s=.o) $(DATAFiles:.bin=.o))

INCLUDE := $(foreach dir,$(INCLUDES),-I$(dir)) \
           $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
           -I$(BUILD)

LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

VPATH  := $(foreach dir,$(SOURCES),$(dir)) $(foreach dir,$(DATA),$(dir))

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -marm -mthumb-interwork -march=armv5te -mtune=arm946e-s

CFLAGS   := -g -Wall -O2 \
            $(ARCH) $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g
LDFLAGS  = -specs=ds_arm9.specs -g -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS := -lnds9

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS := $(LIBNDS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
.PHONY: all build clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

all: $(OUTPUT).nds

build:
	@mkdir -p $(BUILD)

$(BUILD)/%.o: %.c
	@echo $(notdir $<)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s
	@echo $(notdir $<)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: %.cpp
	@echo $(notdir $<)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD)/converted_tiles_bin.o: data/converted_tiles.bin
	@echo $(notdir $<)
	@bin2s $< > $(BUILD)/converted_tiles_bin.s
	$(AS) $(ASFLAGS) -o $@ $(BUILD)/converted_tiles_bin.s

$(BUILD)/converted_tiles_bin.h: $(BUILD)/converted_tiles_bin.o
	@echo "extern const u8 converted_tiles_bin[];" > $@
	@echo "extern const u32 converted_tiles_bin_size;" >> $@

$(BUILD)/main.o: $(BUILD)/converted_tiles_bin.h

$(OUTPUT).elf: $(OFILES)
	@echo linking $(notdir $@)
	$(LD) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(OUTPUT).nds: $(OUTPUT).elf
	@echo building $(notdir $@)
	ndstool -c $@ -9 $<

#---------------------------------------------------------------------------------
# clean:
# 	@echo clean ...
# 	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds
#
# #---------------------------------------------------------------------------------
# else

# DEPENDS := $(OFILES:.o=.d)

# #---------------------------------------------------------------------------------
# # main targets
# #---------------------------------------------------------------------------------
# $(OUTPUT).nds: $(OUTPUT).elf
# $(OUTPUT).elf: $(OFILES)

# #---------------------------------------------------------------------------------
# %.nds: %.elf
# 	ndstool -c $@ -9 $<

# -include $(DEPENDS)

# #---------------------------------------------------------------------------------
# endif
#---------------------------------------------------------------------------------

