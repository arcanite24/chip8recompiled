#!/bin/bash
#
# run_test_suite.sh - Run the Timendus CHIP-8 test suite
#
# This script recompiles each test ROM and builds the resulting executable.
# The test suite includes:
#   1. CHIP-8 Logo - Basic drawing test
#   2. IBM Logo - Drawing with ADD instruction
#   3. Corax+ - Comprehensive opcode test
#   4. Flags - VF flag behavior test (the critical one!)
#   5. Quirks - Platform quirk detection
#   6. Keypad - Key input testing (requires manual interaction)
#   7. Beep - Sound testing (requires manual interaction)
#   8. Scrolling - SUPER-CHIP scrolling (not applicable to base CHIP-8)
#
# Usage:
#   ./scripts/run_test_suite.sh [--run] [--test N]
#
# Options:
#   --run       Also run the compiled tests (requires display)
#   --test N    Only run test N (1-8)
#   --help      Show this help

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TEST_SUITE_DIR="$PROJECT_DIR/tests/chip8-test-suite/bin"
BUILD_DIR="$PROJECT_DIR/build"
RECOMPILER="$BUILD_DIR/recompiler/chip8recomp"
OUTPUT_DIR="$BUILD_DIR/test_suite_output"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

RUN_TESTS=false
SINGLE_TEST=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --run)
            RUN_TESTS=true
            shift
            ;;
        --test)
            SINGLE_TEST="$2"
            shift 2
            ;;
        --help|-h)
            head -26 "$0" | tail -24
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check if recompiler exists
if [[ ! -f "$RECOMPILER" ]]; then
    echo -e "${RED}Error: Recompiler not found at $RECOMPILER${NC}"
    echo "Please build the project first:"
    echo "  cd build && cmake --build ."
    exit 1
fi

# Check if test suite exists
if [[ ! -d "$TEST_SUITE_DIR" ]]; then
    echo -e "${RED}Error: Test suite not found at $TEST_SUITE_DIR${NC}"
    echo "Please initialize submodules:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Test ROMs to run (excluding keypad, beep, scrolling which need interaction/SUPER-CHIP)
declare -a TESTS=(
    "1-chip8-logo:CHIP-8 Logo:Basic drawing"
    "2-ibm-logo:IBM Logo:Drawing with ADD"
    "3-corax+:Corax+ Opcodes:Comprehensive opcode test"
    "4-flags:Flags Test:VF flag behavior"
    "5-quirks:Quirks Test:Platform quirk detection"
)

# For manual tests
declare -a MANUAL_TESTS=(
    "6-keypad:Keypad Test:Requires key input"
    "7-beep:Beep Test:Requires audio"
    "8-scrolling:Scrolling Test:SUPER-CHIP only"
)

PASSED=0
FAILED=0
SKIPPED=0

run_test() {
    local rom_file="$1"
    local test_name="$2"
    local description="$3"
    local test_num="${rom_file%%-*}"
    
    # Filter by single test if specified
    if [[ -n "$SINGLE_TEST" && "$test_num" != "$SINGLE_TEST" ]]; then
        return 0
    fi
    
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Test $test_num: $test_name${NC}"
    echo -e "${BLUE}$description${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    local rom_path="$TEST_SUITE_DIR/${rom_file}.ch8"
    local output_name="test_${rom_file//-/_}"
    local output_path="$OUTPUT_DIR/$output_name"
    
    if [[ ! -f "$rom_path" ]]; then
        echo -e "${RED}ROM not found: $rom_path${NC}"
        ((FAILED++))
        return 1
    fi
    
    # Recompile the ROM
    echo -e "${YELLOW}Recompiling...${NC}"
    if ! "$RECOMPILER" "$rom_path" -o "$output_path" --single-function 2>&1; then
        echo -e "${RED}✗ Recompilation failed${NC}"
        ((FAILED++))
        return 1
    fi
    
    # Build the recompiled ROM
    echo -e "${YELLOW}Building...${NC}"
    mkdir -p "$output_path/build"
    cd "$output_path/build"
    
    # Override CHIP8_RECOMPILED_DIR since we're 3 levels deep (build/test_suite_output/test_*)
    if ! cmake -G Ninja .. -DCHIP8_RECOMPILED_DIR="$PROJECT_DIR" > /dev/null 2>&1; then
        echo -e "${RED}✗ CMake configuration failed${NC}"
        ((FAILED++))
        return 1
    fi
    
    if ! cmake --build . > /dev/null 2>&1; then
        echo -e "${RED}✗ Build failed${NC}"
        ((FAILED++))
        return 1
    fi
    
    echo -e "${GREEN}✓ Build successful${NC}"
    ((PASSED++))
    
    # Optionally run the test
    if [[ "$RUN_TESTS" == true ]]; then
        local exe_name="${rom_file//-/_}"
        exe_name="${exe_name//+/_}"  # Replace + with _
        
        if [[ -f "./$exe_name" ]]; then
            echo -e "${YELLOW}Running test (close window when done)...${NC}"
            "./$exe_name" || true
        else
            echo -e "${YELLOW}Executable not found, trying alternative names...${NC}"
            ls -la
        fi
    fi
    
    cd "$PROJECT_DIR"
}

echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║        CHIP-8 Test Suite - Timendus chip8-test-suite         ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Recompiler: $RECOMPILER"
echo "Test ROMs:  $TEST_SUITE_DIR"
echo "Output:     $OUTPUT_DIR"

# Run automated tests
for test_entry in "${TESTS[@]}"; do
    IFS=':' read -r rom_file test_name description <<< "$test_entry"
    run_test "$rom_file" "$test_name" "$description"
done

# Summary
echo -e "\n${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                         SUMMARY                              ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${GREEN}Passed:${NC}  $PASSED"
echo -e "  ${RED}Failed:${NC}  $FAILED"
echo -e "  ${YELLOW}Skipped:${NC} $SKIPPED"
echo ""

if [[ "$RUN_TESTS" != true ]]; then
    echo -e "${YELLOW}Note:${NC} Tests were compiled but not run."
    echo "      Use --run to also execute the tests (requires display)."
    echo ""
    echo "Manual testing required for:"
    for test_entry in "${MANUAL_TESTS[@]}"; do
        IFS=':' read -r rom_file test_name description <<< "$test_entry"
        echo "  - $test_name: $description"
    done
fi

echo ""
echo -e "${BLUE}To run a test manually:${NC}"
echo "  cd $OUTPUT_DIR/test_<name>/build"
echo "  ./<executable>"

if [[ $FAILED -gt 0 ]]; then
    exit 1
fi
exit 0
