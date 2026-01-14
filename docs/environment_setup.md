# Development Environment Setup

## Installation Summary

This document describes the development environment for the Super Metroid DSi Port project.

**Date Completed:** 2026-01-15
**System:** Windows (MSYS2 environment)

---

## Installed Components

### 1. devkitPro/devkitARM Toolchain

**Location:** `/c/devkitPro/` (C:\devkitPro\)

**Components:**
- **devkitARM** - ARM cross-compiler toolchain (GCC 15.2.0)
  - Location: `/c/devkitPro/devkitARM/`
  - Compiler: `arm-none-eabi-gcc`
  - Includes: binutils, gdb, newlib

- **libnds** - Nintendo DS/DSi development library
  - Location: `/c/devkitPro/libnds/`
  - Includes: headers, libraries, maxmod

- **Development Tools:**
  - `grit` v0.9.2 - Graphics Raster Image Transmogrifier
  - `mmutil` v1.10.1 - Maxmod audio utility
  - `ndstool` v2.3.1 - Nintendo DS ROM tool

**Verification:**
```bash
arm-none-eabi-gcc --version
# arm-none-eabi-gcc.exe (devkitARM) 15.2.0

grit -?
# ---grit v0.9.2 ---

mmutil
# Maxmod Utility 1.10.1

ndstool --version
# Nintendo DS rom tool 2.3.1
```

---

### 2. Python Environment

**Version:** Python 3.14.2
**Location:** `/c/Users/Infernal/AppData/Local/Python/bin/python3`

**Installed Packages:**
- **Pillow** 12.1.0 - Image processing library (for asset conversion)
- **pip** 25.3 - Package manager

**Verification:**
```bash
python3 --version
# Python 3.14.2

python3 -c "from PIL import Image; print('Pillow', Image.__version__)"
# Pillow 12.1.0
```

---

### 3. DeSmuME Emulator

**Version:** 0.9.13 (x64)
**Location:** `/c/Users/Infernal/AppData/Local/Microsoft/WinGet/Packages/DeSmuMETeam.DeSmuME_Microsoft.Winget.Source_8wekyb3d8bbwe/`
**Executable:** `DeSmuME_0.9.13_x64.exe`

**Command Line Alias:** `DeSmuME` (after restarting shell)

**Features:**
- Best compatibility for development
- Built-in debugger
- Save state support
- Supports .nds files

**Alternative:** melonDS (more accurate, closer to hardware) can be installed if needed for final testing.

---

## Environment Variables

The following environment variables have been configured in `~/.bashrc`:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH="/c/devkitPro/devkitARM/bin:/c/devkitPro/tools/bin:$PATH"
```

**Note:** A `~/.bash_profile` was automatically created to load `~/.bashrc`.

**Verification:**
```bash
echo $DEVKITPRO
# /opt/devkitpro

echo $DEVKITARM
# /opt/devkitpro/devkitARM

echo $PATH | grep devkitARM
# Should show /c/devkitPro/devkitARM/bin in PATH
```

---

## Build Tools

The following build tools are available through MSYS2:

- **make** - GNU Make 4.4.1
- **bash** - Shell environment
- **git** - Version control (verified - project is in git repo)
- **pacman** - Package manager (for installing additional dependencies)

---

## Directory Structure

```
C:\devkitPro\
├── devkitARM\
│   ├── bin\          # Compiler binaries (arm-none-eabi-*)
│   ├── lib\          # Standard libraries
│   └── include\      # Standard headers
├── libnds\
│   ├── include\      # libnds headers
│   └── lib\          # libnds libraries
├── tools\
│   └── bin\          # grit, mmutil, ndstool
├── examples\         # Example projects
└── msys2\            # MSYS2 environment

C:\Users\Infernal\Documents\Code\SMetroidDSiPort\
├── docs\                    # Documentation (this file)
├── tools\                   # Asset conversion scripts (to be created)
├── raw\                     # Raw data extracted from ROM (to be created)
├── assets\                  # Converted assets (to be created)
├── src\                     # C source code (to be created)
├── include\                 # C headers (to be created)
├── Super Metroid (...).sfc  # Source ROM
└── Makefile                 # Build configuration (to be created)
```

---

## Creating Your First Build

### Step 1: Create a Basic NDS Project

Create a `Makefile` in the project root:

```makefile
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary data
# GRAPHICS is a list of directories containing files to be processed by grit
#---------------------------------------------------------------------------------
TARGET    := SuperMetroidDSi
BUILD     := build
SOURCES   := src
INCLUDES  := include
DATA      :=
GRAPHICS  :=

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -marm -mthumb-interwork -march=armv5te -mtune=arm946e-s

CFLAGS   := -g -Wall -O2 \
            $(ARCH) $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS  = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

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
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
%.nds: %.elf
	ndstool -c $@ -9 $<

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
```

### Step 2: Create a Test Program

Create `src/main.c`:

```c
#include <nds.h>
#include <stdio.h>

int main(void) {
    consoleDemoInit();

    printf("Super Metroid DSi Port\n");
    printf("Press START to exit\n");

    while(1) {
        swiWaitForVBlank();
        scanKeys();

        int keys = keysDown();
        if(keys & KEY_START) break;
    }

    return 0;
}
```

Create `include/` directory:
```bash
mkdir -p include
```

### Step 3: Build

```bash
make
```

This will create `SuperMetroidDSi.nds` which can be run in DeSmuME.

### Step 4: Run in Emulator

```bash
DeSmuME SuperMetroidDSi.nds
```

Or drag-and-drop the .nds file onto the DeSmuME executable.

---

## Testing the Setup

Run these commands to verify your setup:

```bash
# Test 1: Check environment variables
echo $DEVKITPRO $DEVKITARM

# Test 2: Check ARM compiler
arm-none-eabi-gcc --version

# Test 3: Check tools
grit -? && mmutil && ndstool --version

# Test 4: Check Python and Pillow
python3 -c "from PIL import Image; print('OK')"

# Test 5: Check libnds examples
ls $DEVKITPRO/examples/nds/
```

---

## Troubleshooting

### Compiler not found
- Make sure `~/.bashrc` exports are present
- Restart your shell or run `source ~/.bashrc`
- Verify PATH includes `/c/devkitPro/devkitARM/bin`

### Build fails with "DEVKITARM not set"
- Check: `echo $DEVKITARM`
- Should output: `/opt/devkitpro/devkitARM`
- If empty, run: `source ~/.bashrc`

### grit/mmutil not found
- Verify PATH includes `/c/devkitPro/tools/bin`
- Run: `echo $PATH | grep tools`

### Python import errors
- Verify Pillow: `python3 -m pip list | grep Pillow`
- Reinstall if needed: `python3 -m pip install --force-reinstall Pillow`

### DeSmuME not launching
- Full path: `/c/Users/Infernal/AppData/Local/Microsoft/WinGet/Packages/DeSmuMETeam.DeSmuME_Microsoft.Winget.Source_8wekyb3d8bbwe/DeSmuME_0.9.13_x64.exe`
- Or restart shell and use alias: `DeSmuME`

---

## Next Steps

1. **Create project structure:**
   ```bash
   mkdir -p src include tools raw assets/{tilesets,palettes,sprites,rooms,audio}
   ```

2. **Create initial Makefile** (see template above)

3. **Test build system** with a "Hello World" program

4. **Begin ROM extraction** (see Agent C's task list in `Agent_C_Assets_Support.md`)

5. **Start implementing core systems** (see project plan in `SuperMetroid_DSi_Port_Plan.md`)

---

## Resources

- **devkitPro Documentation:** https://devkitpro.org/wiki/Getting_Started
- **libnds Documentation:** https://libnds.devkitpro.org/
- **grit Documentation:** https://www.coranac.com/projects/grit/
- **DeSmuME Documentation:** https://desmume.org/documentation/

---

*Environment setup completed successfully on 2026-01-15*
