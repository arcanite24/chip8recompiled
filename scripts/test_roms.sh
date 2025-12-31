#!/bin/bash
#
# CHIP-8 Recompiler ROM Compatibility Test Script
# Tests all ROMs and generates a report
#
# Features:
# - Parallel test execution for speed
# - Progress bar with ETA
# - Multiple ROM source directories
# - Configurable test sets
#

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
ROMS_DIR="$PROJECT_ROOT/roms"
CHIP8_ARCHIVE_DIR="$PROJECT_ROOT/chip8Archive/roms"
REPORT_FILE="$PROJECT_ROOT/COMPATIBILITY_REPORT.md"
RECOMPILER="$BUILD_DIR/recompiler/chip8recomp"

# Parallel execution settings
MAX_JOBS=${MAX_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'
BOLD='\033[1m'

# Command line options
VERBOSE=0
TEST_SET="all"  # all, classic, archive, quick, chip8only
CLEAN_AFTER=1
CHIP8_ARCHIVE_JSON="$PROJECT_ROOT/chip8Archive/programs.json"

usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  -j N          Run N jobs in parallel (default: $MAX_JOBS)
  -s SET        Test set: all, classic, archive, quick, chip8only (default: all)
  -v            Verbose output (show build errors)
  -k            Keep test outputs (don't clean up)
  -h            Show this help

Test Sets:
  all       - All ROMs from roms/ and chip8Archive/
  classic   - Only ROMs from roms/ directory
  archive   - Only ROMs from chip8Archive/
  chip8only - All ROMs, but skip SCHIP/XO-CHIP from chip8Archive
  quick     - Small subset for quick validation

Examples:
  $(basename "$0")              # Test all ROMs with max parallelism
  $(basename "$0") -j 8         # Use 8 parallel jobs
  $(basename "$0") -s quick     # Quick test with subset
  $(basename "$0") -s archive   # Test only chip8Archive ROMs
  $(basename "$0") -s chip8only # Skip SCHIP/XO-CHIP ROMs
EOF
    exit 0
}

while getopts "j:s:vkh" opt; do
    case $opt in
        j) MAX_JOBS="$OPTARG" ;;
        s) TEST_SET="$OPTARG" ;;
        v) VERBOSE=1 ;;
        k) CLEAN_AFTER=0 ;;
        h) usage ;;
        *) usage ;;
    esac
done

echo ""
echo -e "${BOLD}══════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}    CHIP-8 Recompiler Compatibility Test Suite${NC}"
echo -e "${BOLD}══════════════════════════════════════════════════════${NC}"
echo ""

# Ensure recompiler exists
if [[ ! -f "$RECOMPILER" ]]; then
    echo -e "${CYAN}Building recompiler first...${NC}"
    mkdir -p "$BUILD_DIR"
    pushd "$BUILD_DIR" >/dev/null
    cmake .. -G Ninja >/dev/null 2>&1
    cmake --build . >/dev/null 2>&1
    popd >/dev/null
    
    if [[ ! -f "$RECOMPILER" ]]; then
        echo -e "${RED}Failed to build recompiler${NC}"
        exit 1
    fi
fi

# Check if a ROM from chip8Archive is standard CHIP-8 (not SCHIP or XO-CHIP)
is_chip8_platform() {
    local rom_name="$1"
    local base_name="${rom_name%.ch8}"
    
    # If no JSON file, assume it's CHIP-8
    [[ ! -f "$CHIP8_ARCHIVE_JSON" ]] && return 0
    
    # Check platform in JSON (using grep for speed)
    local platform=$(/usr/bin/grep -A 20 "\"$base_name\":" "$CHIP8_ARCHIVE_JSON" 2>/dev/null | /usr/bin/grep '"platform"' | head -1 | sed 's/.*"platform"[^"]*"\([^"]*\)".*/\1/')
    
    # If not found or "chip8", it's standard CHIP-8
    [[ -z "$platform" || "$platform" == "chip8" ]]
}

# Collect ROMs based on test set
collect_roms() {
    local roms=()
    
    case "$TEST_SET" in
        classic)
            for rom in "$ROMS_DIR"/games/*.ch8 "$ROMS_DIR"/demos/*.ch8 "$ROMS_DIR"/programs/*.ch8; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            ;;
        archive)
            for rom in "$CHIP8_ARCHIVE_DIR"/*.ch8; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            ;;
        chip8only)
            # Classic ROMs are all standard CHIP-8
            for rom in "$ROMS_DIR"/games/*.ch8 "$ROMS_DIR"/demos/*.ch8 "$ROMS_DIR"/programs/*.ch8; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            # Filter chip8Archive to only include standard CHIP-8
            for rom in "$CHIP8_ARCHIVE_DIR"/*.ch8; do
                [[ -f "$rom" ]] || continue
                if is_chip8_platform "$(basename "$rom")"; then
                    roms+=("$rom")
                fi
            done
            ;;
        quick)
            # Quick subset - a few representative ROMs
            local quick_roms=(
                "$ROMS_DIR/games/Pong (1 player).ch8"
                "$ROMS_DIR/games/Breakout (Brix hack) [David Winter, 1997].ch8"
                "$ROMS_DIR/games/Tetris [Fran Dachille, 1991].ch8"
                "$ROMS_DIR/games/Space Invaders [David Winter].ch8"
                "$CHIP8_ARCHIVE_DIR/snake.ch8"
                "$CHIP8_ARCHIVE_DIR/br8kout.ch8"
            )
            for rom in "${quick_roms[@]}"; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            ;;
        all|*)
            for rom in "$ROMS_DIR"/games/*.ch8 "$ROMS_DIR"/demos/*.ch8 "$ROMS_DIR"/programs/*.ch8; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            for rom in "$CHIP8_ARCHIVE_DIR"/*.ch8; do
                [[ -f "$rom" ]] && roms+=("$rom")
            done
            ;;
    esac
    
    printf '%s\n' "${roms[@]}"
}

# Write ROM list to temp file for parallel processing
echo -e "${CYAN}Collecting ROMs (set: $TEST_SET)...${NC}"
ROM_LIST="$TEMP_DIR/rom_list.txt"
collect_roms > "$ROM_LIST"
TOTAL=$(wc -l < "$ROM_LIST" | tr -d ' ')

if [[ $TOTAL -eq 0 ]]; then
    echo -e "${RED}No ROMs found to test${NC}"
    exit 1
fi

echo -e "  Found ${BOLD}$TOTAL${NC} ROMs to test"
echo -e "  Using ${BOLD}$MAX_JOBS${NC} parallel jobs"
echo ""

# Create the test worker script
cat > "$TEMP_DIR/test_worker.sh" << 'WORKER_SCRIPT'
#!/bin/bash
rom="$1"
result_dir="$2"
recompiler="$3"
build_dir="$4"

rom_name=$(basename "$rom")
output_name=$(basename "$rom" .ch8 | tr ' [](),' '_____' | tr -d "'.," | tr -s '_')
output_dir="$build_dir/test_${output_name}_$$"
status="failed"
mode="none"

# Try normal mode first
rm -rf "$output_dir" 2>/dev/null
if "$recompiler" "$rom" -o "$output_dir" >/dev/null 2>&1; then
    if (
        cd "$output_dir" && \
        mkdir -p build && \
        cd build && \
        cmake .. -G Ninja >/dev/null 2>&1 && \
        ninja >/dev/null 2>&1
    ); then
        status="passed"
        mode="normal"
    fi
fi

# If normal failed, try single-function mode
if [[ "$status" == "failed" ]]; then
    rm -rf "$output_dir" 2>/dev/null
    if "$recompiler" "$rom" -o "$output_dir" --single-function >/dev/null 2>&1; then
        if (
            cd "$output_dir" && \
            mkdir -p build && \
            cd build && \
            cmake .. -G Ninja >/dev/null 2>&1 && \
            ninja >/dev/null 2>&1
        ); then
            status="passed"
            mode="single"
        fi
    fi
fi

# Clean up
rm -rf "$output_dir" 2>/dev/null

# Output result (atomic write)
result_file="$result_dir/$(echo "$rom_name" | md5sum | cut -d' ' -f1)"
echo "$status|$mode|$rom_name" > "$result_file"
WORKER_SCRIPT
chmod +x "$TEMP_DIR/test_worker.sh"

# Results directory
RESULTS_DIR="$TEMP_DIR/results"
mkdir -p "$RESULTS_DIR"

# Start time
START_TIME=$(date +%s)

echo -e "${CYAN}Running tests...${NC}"
echo ""

# Run tests in parallel using xargs
cat "$ROM_LIST" | xargs -P "$MAX_JOBS" -I {} "$TEMP_DIR/test_worker.sh" {} "$RESULTS_DIR" "$RECOMPILER" "$BUILD_DIR" &
XARGS_PID=$!

# Show progress while tests run
while kill -0 $XARGS_PID 2>/dev/null; do
    COMPLETED=$(ls -1 "$RESULTS_DIR" 2>/dev/null | wc -l | tr -d ' ')
    PASSED=$(grep -l "passed" "$RESULTS_DIR"/* 2>/dev/null | wc -l | tr -d ' ')
    FAILED_COUNT=$((COMPLETED - PASSED))
    
    # Calculate ETA
    ELAPSED=$(($(date +%s) - START_TIME))
    if [[ $COMPLETED -gt 0 && $ELAPSED -gt 0 ]]; then
        REMAINING=$(( (TOTAL - COMPLETED) * ELAPSED / COMPLETED ))
        ETA=$(printf "%02d:%02d" $((REMAINING / 60)) $((REMAINING % 60)))
    else
        ETA="--:--"
    fi
    
    # Progress bar
    if [[ $TOTAL -gt 0 ]]; then
        PERCENTAGE=$((COMPLETED * 100 / TOTAL))
        BAR_WIDTH=40
        FILLED=$((COMPLETED * BAR_WIDTH / TOTAL))
        EMPTY=$((BAR_WIDTH - FILLED))
        BAR=$(printf "%${FILLED}s" | tr ' ' '=')
        SPACE=$(printf "%${EMPTY}s")
        printf "\r  [${GREEN}%s${NC}%s] %3d%% (%d/%d) ${GREEN}✓%d${NC} ${RED}✗%d${NC} ETA: %s  " \
            "$BAR" "$SPACE" "$PERCENTAGE" "$COMPLETED" "$TOTAL" "$PASSED" "$FAILED_COUNT" "$ETA"
    fi
    
    sleep 0.2
done

# Wait for xargs to finish
wait $XARGS_PID

# Final count
COMPLETED=$(ls -1 "$RESULTS_DIR" 2>/dev/null | wc -l | tr -d ' ')
echo ""
echo ""

# Collect results
PASSED_NORMAL=0
PASSED_SINGLE=0
FAILED=0
NORMAL_ROMS=""
SINGLE_ROMS=""
FAILED_ROMS=""

for result_file in "$RESULTS_DIR"/*; do
    [[ -f "$result_file" ]] || continue
    IFS='|' read -r status mode rom_name < "$result_file"
    
    if [[ "$status" == "passed" ]]; then
        if [[ "$mode" == "normal" ]]; then
            ((PASSED_NORMAL++))
            NORMAL_ROMS="$NORMAL_ROMS$rom_name"$'\n'
        else
            ((PASSED_SINGLE++))
            SINGLE_ROMS="$SINGLE_ROMS$rom_name"$'\n'
        fi
    else
        ((FAILED++))
        FAILED_ROMS="$FAILED_ROMS$rom_name"$'\n'
    fi
done

# Calculate results
PASSED=$((PASSED_NORMAL + PASSED_SINGLE))
if [[ $TOTAL -gt 0 ]]; then
    PERCENTAGE=$((PASSED * 100 / TOTAL))
else
    PERCENTAGE=0
fi

ELAPSED=$(($(date +%s) - START_TIME))
ELAPSED_MIN=$((ELAPSED / 60))
ELAPSED_SEC=$((ELAPSED % 60))

# Summary
echo -e "${BOLD}══════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}                      SUMMARY${NC}"
echo -e "${BOLD}══════════════════════════════════════════════════════${NC}"
echo ""
echo -e "  Test Set:           ${BOLD}$TEST_SET${NC}"
echo -e "  Total ROMs:         ${BOLD}$TOTAL${NC}"
echo -e "  Time Elapsed:       ${BOLD}${ELAPSED_MIN}m ${ELAPSED_SEC}s${NC}"
echo ""
echo -e "  ${GREEN}Normal mode:        $PASSED_NORMAL${NC}"
echo -e "  ${YELLOW}Single-function:    $PASSED_SINGLE${NC}"
echo -e "  ${RED}Failed:             $FAILED${NC}"
echo ""
if [[ $PERCENTAGE -ge 90 ]]; then
    echo -e "  ${GREEN}${BOLD}Compatibility:      $PASSED/$TOTAL ($PERCENTAGE%)${NC}"
elif [[ $PERCENTAGE -ge 70 ]]; then
    echo -e "  ${YELLOW}${BOLD}Compatibility:      $PASSED/$TOTAL ($PERCENTAGE%)${NC}"
else
    echo -e "  ${RED}${BOLD}Compatibility:      $PASSED/$TOTAL ($PERCENTAGE%)${NC}"
fi
echo ""

# Generate report
DATE=$(date '+%Y-%m-%d %H:%M:%S')

cat > "$REPORT_FILE" << EOF
# CHIP-8 Recompiler Compatibility Report

**Generated:** $DATE  
**Test Set:** $TEST_SET  
**Total ROMs Tested:** $TOTAL  
**Compatibility:** $PASSED/$TOTAL ($PERCENTAGE%)  
**Test Duration:** ${ELAPSED_MIN}m ${ELAPSED_SEC}s (${MAX_JOBS} parallel jobs)

## Summary

| Status | Count | Percentage |
|--------|-------|------------|
| ✅ Normal Mode | $PASSED_NORMAL | $((TOTAL > 0 ? PASSED_NORMAL * 100 / TOTAL : 0))% |
| ✅ Single-Function Mode | $PASSED_SINGLE | $((TOTAL > 0 ? PASSED_SINGLE * 100 / TOTAL : 0))% |
| ❌ Failed | $FAILED | $((TOTAL > 0 ? FAILED * 100 / TOTAL : 0))% |

---

## Working ROMs (Normal Mode)

These ROMs compile with standard recompilation:

EOF

echo "$NORMAL_ROMS" | sort | while read -r rom; do
    [[ -n "$rom" ]] && echo "- \`$rom\`" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

---

## Working ROMs (Single-Function Mode)

These ROMs require \`--single-function\` flag:

EOF

echo "$SINGLE_ROMS" | sort | while read -r rom; do
    [[ -n "$rom" ]] && echo "- \`$rom\`" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

---

## Failed ROMs

These ROMs failed to compile:

EOF

if [[ -z "$FAILED_ROMS" ]]; then
    echo "_None - 100% compatibility!_" >> "$REPORT_FILE"
else
    echo "$FAILED_ROMS" | sort | while read -r rom; do
        [[ -n "$rom" ]] && echo "- \`$rom\`" >> "$REPORT_FILE"
    done
fi

cat >> "$REPORT_FILE" << EOF

---

## ROM Sources

- **Classic ROMs:** \`roms/games/\`, \`roms/demos/\`, \`roms/programs/\`
- **chip8Archive:** \`chip8Archive/roms/\` ([JohnEarnest/chip8Archive](https://github.com/JohnEarnest/chip8Archive))

## Usage

\`\`\`bash
# Run full test suite
./scripts/test_roms.sh

# Run with 8 parallel jobs
./scripts/test_roms.sh -j 8

# Test only classic ROMs
./scripts/test_roms.sh -s classic

# Test only chip8Archive ROMs  
./scripts/test_roms.sh -s archive

# Quick validation test
./scripts/test_roms.sh -s quick

# Recompile a ROM
./build/recompiler/chip8recomp path/to/rom.ch8 -o output_dir

# For complex ROMs requiring single-function mode
./build/recompiler/chip8recomp path/to/rom.ch8 -o output_dir --single-function
\`\`\`
EOF

echo -e "Report saved to: ${BOLD}$REPORT_FILE${NC}"

# Clean up test outputs
if [[ $CLEAN_AFTER -eq 1 ]]; then
    echo ""
    echo -e "${CYAN}Cleaning up test outputs...${NC}"
    rm -rf "$BUILD_DIR"/test_*
fi

echo ""
echo -e "${GREEN}Done!${NC}"
