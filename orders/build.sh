#!/bin/bash
# Build script for HFT Orders System

set -e

echo "=== Building HFT Orders System ==="

# Load environment variables
if [ -f ".env" ]; then
    echo "Loading environment variables from .env"
    export $(cat .env | grep -v '^#' | xargs)
else
    echo "⚠️  No .env file found. Copy .env.example to .env"
    echo "   cp .env.example .env"
    echo "   Edit .env with your API credentials"
    echo ""
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
BUILD_TYPE="Release"
BUILD_TESTS="ON"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --help)
            echo "Usage: ./build.sh [OPTIONS]"
            echo "Options:"
            echo "  --debug       Build in Debug mode (default: Release)"
            echo "  --no-tests    Skip test build"
            echo "  --help        Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${YELLOW}Build Configuration:${NC}"
echo "  Build Directory: $BUILD_DIR"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Tests: $BUILD_TESTS"
echo ""

# Clean old build
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning old build...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Running CMake configure...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS="$BUILD_TESTS"

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . --config "$BUILD_TYPE" --parallel $(nproc 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"
echo ""

# Run tests if enabled
if [ "$BUILD_TESTS" = "ON" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    ctest --verbose
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${RED}Some tests failed!${NC}"
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Next steps:"
if [ "$BUILD_TESTS" = "ON" ]; then
    echo "  1. Run example: ./build/example_fire_and_forget"
else
    echo "  1. Build with tests: ./build.sh (default)"
    echo "  2. Run example: ./build/example_fire_and_forget"
fi
echo "  3. Review documentation: open docs/QUICKSTART.md"
echo ""
