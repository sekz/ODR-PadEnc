#!/bin/sh
# ODR-PadEnc Entrypoint Script
# Provides default configuration for MOT Slideshow and DLS
# POSIX compliant for maximum compatibility

set -e

# Default configuration
PAD_DIR="${PAD_DIR:-/app/data/slides}"
DLS_FILE="${DLS_FILE:-/app/data/dls.txt}"
# Use FIFO for PAD output (to be consumed by ODR-DabMux)
PAD_FIFO="${PAD_FIFO:-/tmp/pad.fifo}"
SLIDE_INTERVAL="${SLIDE_INTERVAL:-10}"
CHARSET="${CHARSET:-15}"  # UTF-8 for Thai language support
VERBOSE="${VERBOSE:-}"

# Create required directories
mkdir -p "$PAD_DIR"
mkdir -p "$(dirname "$DLS_FILE")"
mkdir -p /app/logs

# Create default DLS file if not exists
if [ ! -f "$DLS_FILE" ]; then
    echo "StreamDAB - Thailand DAB+ Broadcasting" > "$DLS_FILE"
fi

# Validate DLS file is readable
if [ ! -r "$DLS_FILE" ]; then
    echo "ERROR: DLS file exists but is not readable: $DLS_FILE"
    exit 1
fi

# Create FIFO with proper cleanup and permissions
rm -f "$PAD_FIFO"
mkfifo "$PAD_FIFO"
chmod 666 "$PAD_FIFO"

# Start background FIFO reader to prevent blocking
# This prevents ODR-PadEnc from blocking when no consumer is attached
(
    while true; do
        # Read and discard FIFO data when no consumer present
        cat "$PAD_FIFO" > /dev/null 2>&1 || true
        sleep 1
    done
) &
FIFO_READER_PID=$!

# Cleanup function
cleanup() {
    echo "Cleaning up..."
    kill $FIFO_READER_PID 2>/dev/null || true
    rm -f "$PAD_FIFO"
    exit 0
}

trap cleanup INT TERM

# Build command line arguments
# Note: ODR-PadEnc auto-detects FIFO from path, no --output-type needed
ARGS="--dir=$PAD_DIR --output=$PAD_FIFO --dls=$DLS_FILE --charset=$CHARSET --sleep=$SLIDE_INTERVAL"

# Add verbose flag if requested
if [ -n "$VERBOSE" ]; then
    ARGS="$ARGS --verbose"
fi

# Log configuration
echo "========================================"
echo "ODR-PadEnc Configuration"
echo "========================================"
echo "PAD Directory: $PAD_DIR"
echo "DLS File: $DLS_FILE"
echo "PAD FIFO: $PAD_FIFO"
echo "Slide Interval: ${SLIDE_INTERVAL}s"
echo "Character Set: $CHARSET (UTF-8)"
echo "FIFO Reader PID: $FIFO_READER_PID"
echo "========================================"

# Execute ODR-PadEnc
exec /app/build/odr-padenc $ARGS "$@"
