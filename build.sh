#!/bin/bash
#---------------------------------------------------------------------------------
# Super Metroid DSi Port - Build Script
# Checks environment and builds the project
#---------------------------------------------------------------------------------

set -e  # Exit on error

# Color output for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Super Metroid DSi Port - Build Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

#---------------------------------------------------------------------------------
# Environment Checks
#---------------------------------------------------------------------------------

echo -e "${YELLOW}[1/5] Checking environment...${NC}"

# Check if devkitPro is installed
if [ ! -d "/c/devkitPro" ] && [ ! -d "$HOME/devkitPro" ] && [ ! -d "/opt/devkitpro" ]; then
    echo -e "${RED}ERROR: devkitPro not found!${NC}"
    echo "Please install devkitPro from: https://devkitpro.org/wiki/Getting_Started"
    exit 1
fi

# Determine devkitPro path - always use the real installed path
export DEVKITPRO=/c/devkitPro

echo -e "${GREEN}✓ devkitPro found at: $DEVKITPRO${NC}"

# Set devkitARM path
export DEVKITARM=$DEVKITPRO/devkitARM

if [ ! -d "$DEVKITARM" ]; then
    echo -e "${RED}ERROR: devkitARM not found at $DEVKITARM${NC}"
    echo "Please ensure devkitARM is properly installed."
    exit 1
fi

echo -e "${GREEN}✓ devkitARM found at: $DEVKITARM${NC}"

# Set up PATH
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"

# Check for required tools
echo ""
echo -e "${YELLOW}[2/5] Checking build tools...${NC}"

check_tool() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓ $1 found${NC}"
        return 0
    else
        echo -e "${RED}✗ $1 not found${NC}"
        return 1
    fi
}

TOOLS_OK=true
check_tool "arm-none-eabi-gcc" || TOOLS_OK=false
check_tool "arm-none-eabi-g++" || TOOLS_OK=false
check_tool "arm-none-eabi-as" || TOOLS_OK=false
check_tool "arm-none-eabi-objcopy" || TOOLS_OK=false
check_tool "ndstool" || TOOLS_OK=false
check_tool "make" || TOOLS_OK=false

if [ "$TOOLS_OK" = false ]; then
    echo -e "${RED}ERROR: Some required build tools are missing!${NC}"
    exit 1
fi

# Check for libnds
echo ""
echo -e "${YELLOW}[3/5] Checking libraries...${NC}"

if [ ! -d "$DEVKITPRO/libnds" ]; then
    echo -e "${RED}ERROR: libnds not found!${NC}"
    echo "Install with: (dkp-)pacman -S libnds"
    exit 1
fi

export LIBNDS=$DEVKITPRO/libnds
echo -e "${GREEN}✓ libnds found at: $LIBNDS${NC}"

# Check for libfat (needed for save system)
if [ ! -f "$DEVKITPRO/libnds/lib/libfat.a" ]; then
    echo -e "${YELLOW}⚠ libfat not found - save system will not work!${NC}"
    echo "Install with: (dkp-)pacman -S libfat-nds"
    echo -e "${YELLOW}Continuing anyway...${NC}"
else
    echo -e "${GREEN}✓ libfat found${NC}"
fi

#---------------------------------------------------------------------------------
# Clean old build if requested
#---------------------------------------------------------------------------------

if [ "$1" == "clean" ]; then
    echo ""
    echo -e "${YELLOW}[4/5] Cleaning old build...${NC}"
    make clean
    echo -e "${GREEN}✓ Clean complete!${NC}"
    exit 0
fi

#---------------------------------------------------------------------------------
# Build
#---------------------------------------------------------------------------------

echo ""
echo -e "${YELLOW}[4/5] Starting build...${NC}"
echo ""

# Create build directory
mkdir -p build

# Run make with explicit environment variables
if make DEVKITPRO="$DEVKITPRO" DEVKITARM="$DEVKITARM" LIBNDS="$LIBNDS" "$@"; then
    echo ""
    echo -e "${GREEN}✓ Build successful!${NC}"
else
    echo ""
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

#---------------------------------------------------------------------------------
# Verify output
#---------------------------------------------------------------------------------

echo ""
echo -e "${YELLOW}[5/5] Verifying output...${NC}"

if [ ! -f "SuperMetroidDSi.nds" ]; then
    echo -e "${RED}ERROR: Output file SuperMetroidDSi.nds not found!${NC}"
    exit 1
fi

FILE_SIZE=$(stat -c%s "SuperMetroidDSi.nds" 2>/dev/null || stat -f%z "SuperMetroidDSi.nds" 2>/dev/null || echo "0")
echo -e "${GREEN}✓ Output file created: SuperMetroidDSi.nds (${FILE_SIZE} bytes)${NC}"

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}BUILD COMPLETE!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Output: SuperMetroidDSi.nds"
echo "You can now test this file in:"
echo "  - DeSmuME emulator"
echo "  - melonDS emulator"
echo "  - Real DSi hardware (with flashcard)"
echo ""
echo -e "${YELLOW}Note:${NC} Most features are stubs and won't work yet."
echo "This build only tests that the compilation pipeline is working."
