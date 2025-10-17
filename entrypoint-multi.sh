#!/bin/bash
set -e

# ODR-PadEnc Multi-Instance Entrypoint
# Manages multiple PadEnc instances in a single container
# Each instance reads from a dedicated FIFO created by corresponding AudioEnc instance

echo "=========================================="
echo "ODR-PadEnc Multi-Instance Manager"
echo "=========================================="

# Configuration from environment variables
INSTANCE_COUNT=${INSTANCE_COUNT:-3}
FIFO_BASE_PATH=${FIFO_BASE_PATH:-/tmp/fifo}
PORT_BASE=${PORT_BASE:-8020}
DLS_FILE_BASE=${DLS_FILE:-/app/dls}
PAD_PORT_BASE=${PAD_PORT_BASE:-9210}

# Binary location (try multiple paths for dev and production)
BINARY_PATHS=(
    "/app/src/odr-padenc"           # Volume-mounted development
    "/app/build/bin/odr-padenc"     # Production build
    "/app/odr-padenc"               # Alternative location
    "/usr/local/bin/odr-padenc"     # System install
)

BINARY=""
for path in "${BINARY_PATHS[@]}"; do
    if [ -x "$path" ]; then
        BINARY="$path"
        echo "✓ Found ODR-PadEnc binary: $BINARY"
        break
    fi
done

if [ -z "$BINARY" ]; then
    echo "✗ ERROR: ODR-PadEnc binary not found in any expected location!"
    echo "Searched paths:"
    for path in "${BINARY_PATHS[@]}"; do
        echo "  - $path"
    done
    exit 1
fi

# Verify binary is executable
if [ ! -x "$BINARY" ]; then
    echo "✗ ERROR: Binary exists but is not executable: $BINARY"
    exit 1
fi

echo "Configuration:"
echo "  Instance Count: $INSTANCE_COUNT"
echo "  FIFO Base Path: $FIFO_BASE_PATH"
echo "  Port Base: $PORT_BASE"
echo "  PAD Port Base: $PAD_PORT_BASE"
echo "  DLS File Base: $DLS_FILE_BASE"
echo "  Binary: $BINARY"
echo ""

# Create FIFO base directory if it doesn't exist
mkdir -p "$FIFO_BASE_PATH"
chmod 777 "$FIFO_BASE_PATH"

# Create log directory
LOG_DIR="/var/log/odr-padenc"
mkdir -p "$LOG_DIR"

# Array to store PIDs
declare -a PIDS

# Cleanup function
cleanup() {
    echo ""
    echo "Shutting down all PadEnc instances..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            echo "  Stopping PadEnc instance (PID: $pid)"
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done
    wait
    echo "All instances stopped."
    exit 0
}

trap cleanup SIGTERM SIGINT

# Start each PadEnc instance
echo "Starting $INSTANCE_COUNT PadEnc instances..."
echo ""

for i in $(seq 1 "$INSTANCE_COUNT"); do
    INSTANCE_ID=$i
    FIFO_PATH="$FIFO_BASE_PATH/audio${INSTANCE_ID}.fifo"
    PORT=$((PORT_BASE + INSTANCE_ID - 1))
    PAD_PORT=$((PAD_PORT_BASE + INSTANCE_ID - 1))
    LOG_FILE="$LOG_DIR/padenc-${INSTANCE_ID}.log"
    DLS_FILE="${DLS_FILE_BASE}${INSTANCE_ID}.txt"
    PAD_IDENT="padenc${INSTANCE_ID}"

    echo "Instance $INSTANCE_ID:"
    echo "  FIFO: $FIFO_PATH"
    echo "  Output Port: $PORT"
    echo "  PAD Control Port: $PAD_PORT"
    echo "  DLS File: $DLS_FILE"
    echo "  PAD Identity: $PAD_IDENT"
    echo "  Log: $LOG_FILE"

    # Wait for FIFO to be created by AudioEnc (with timeout)
    WAIT_COUNT=0
    MAX_WAIT=30
    while [ ! -p "$FIFO_PATH" ] && [ $WAIT_COUNT -lt $MAX_WAIT ]; do
        echo "  Waiting for FIFO to be created by AudioEnc... ($WAIT_COUNT/$MAX_WAIT)"
        sleep 1
        WAIT_COUNT=$((WAIT_COUNT + 1))
    done

    if [ ! -p "$FIFO_PATH" ]; then
        echo "  ✗ ERROR: FIFO not found after ${MAX_WAIT}s wait: $FIFO_PATH"
        echo "  AudioEnc instance $INSTANCE_ID may not be running!"
        continue
    fi

    echo "  ✓ FIFO exists"

    # Verify FIFO is readable
    if [ ! -r "$FIFO_PATH" ]; then
        echo "  ✗ ERROR: FIFO is not readable: $FIFO_PATH"
        continue
    fi

    # Create DLS file if it doesn't exist
    if [ ! -f "$DLS_FILE" ]; then
        echo "StreamDAB Instance $INSTANCE_ID" > "$DLS_FILE"
        chmod 666 "$DLS_FILE"
    fi

    # Build command line arguments
    ARGS=(
        --pad="$PORT"
        --dls="$DLS_FILE"
        --pad-fifo="$FIFO_PATH"
        --pad-socket="/tmp/${PAD_IDENT}.padenc"
    )

    # Start PadEnc instance in background
    echo "  Starting: $BINARY ${ARGS[*]}"
    "$BINARY" "${ARGS[@]}" > "$LOG_FILE" 2>&1 &

    INSTANCE_PID=$!
    PIDS+=("$INSTANCE_PID")

    echo "  ✓ Started PadEnc instance $INSTANCE_ID (PID: $INSTANCE_PID)"
    echo ""

    # Small delay between starting instances
    sleep 1
done

echo "=========================================="
echo "All $INSTANCE_COUNT PadEnc instances started"
echo "PIDs: ${PIDS[*]}"
echo "=========================================="
echo ""

# Monitor all instances
echo "Monitoring instances (will restart container if any instance dies)..."
while true; do
    ALL_RUNNING=true

    for i in "${!PIDS[@]}"; do
        pid="${PIDS[$i]}"
        instance_num=$((i + 1))

        if ! kill -0 "$pid" 2>/dev/null; then
            echo "✗ CRITICAL: PadEnc instance $instance_num (PID: $pid) has died!"
            echo "Container will exit for Docker restart..."
            ALL_RUNNING=false
            break
        fi
    done

    if [ "$ALL_RUNNING" = false ]; then
        # Exit container so Docker can restart it
        cleanup
        exit 1
    fi

    # Check every 5 seconds
    sleep 5
done
