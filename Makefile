#---------------------------------------------------------------------------------
# Super Metroid DSi Port - Makefile
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

# Allow environment variables to be passed from build.sh
DEVKITPRO ?= /c/devkitPro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
LIBNDS ?= $(DEVKITPRO)/libnds
CALICO := $(DEVKITPRO)/calico

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM not set. Please run ./build.sh which sets the environment properly")
endif

# ARM Toolchain
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
AS      := $(PREFIX)as
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
AR      := $(PREFIX)ar

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary data
#---------------------------------------------------------------------------------
TARGET    := SuperMetroidDSi
BUILD     := build
SOURCES   := src src/game src/input
INCLUDES  := include
DATA      := data

#---------------------------------------------------------------------------------
# Game metadata (shown in ROM header)
#---------------------------------------------------------------------------------
GAME_TITLE     := Super Metroid DSi
GAME_SUBTITLE1 := Port by Infernal
GAME_SUBTITLE2 := Built with devkitARM

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -march=armv5te -mtune=arm946e-s

CFLAGS   := -g -Wall -O2 -ffunction-sections -fdata-sections \
            $(ARCH) $(INCLUDE) -DARM9 -D__NDS__ -I$(CALICO)/include
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH) -DARM9
LDFLAGS  = -specs=$(CALICO)/share/ds9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order matters!)
#---------------------------------------------------------------------------------
LIBS := -lcalico_ds9 -lfat -lnds9

#---------------------------------------------------------------------------------
# list of directories containing libraries
#---------------------------------------------------------------------------------
LIBDIRS := $(CALICO) $(LIBNDS)

#---------------------------------------------------------------------------------
# ARM7 binary and icon for .nds creation
#---------------------------------------------------------------------------------
ARM7_ELF := $(CALICO)/bin/ds7_maine.elf
NDS_ICON := $(CALICO)/share/nds-icon.bmp

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

# Automatically find all source files in SOURCES directories
CFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES  := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif
#---------------------------------------------------------------------------------

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD) -I$(CALICO)/include

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo "Cleaning build artifacts..."
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).arm9
	@echo "Clean complete!"

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds: $(OUTPUT).arm9
$(OUTPUT).arm9: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# Build .nds file with ARM7 binary and game metadata
#---------------------------------------------------------------------------------
%.nds: %.arm9
	@echo "Creating .nds file with ARM7 binary: $(notdir $@)"
	@ndstool -c $@ -9 $< -7 $(ARM7_ELF) -b $(NDS_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)"
	@echo "Build successful!"

#---------------------------------------------------------------------------------
%.arm9: %.elf
	@echo "Creating ARM9 binary: $(notdir $@)"
	@$(OBJCOPY) -O binary $< $@

#---------------------------------------------------------------------------------
%.elf:
	@echo "Linking $(notdir $@)..."
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

#---------------------------------------------------------------------------------
# Compile Targets for C/C++
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo "Compiling $(notdir $<)..."
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.c
	@echo "Compiling $(notdir $<)..."
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.s
	@echo "Assembling $(notdir $<)..."
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
# Binary data embedding
#---------------------------------------------------------------------------------
%.bin.o: %.bin
	@echo "Embedding binary data: $(notdir $<)"
	@bin2s $< | $(AS) -o $@

%_bin.h: %.bin
	@echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > $@
	@echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> $@
	@echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
