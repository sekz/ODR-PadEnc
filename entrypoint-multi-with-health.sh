#!/bin/bash
# ODR-PadEnc Multi-Instance Entrypoint Script with Health Endpoints
# StreamDAB EncoderManager - Phase 2.1 Enhanced
# Starts multiple PadEnc instances with health monitoring

set -e

echo "========================================"
echo "ODR-PadEnc Multi-Instance Container"
echo "========================================"

# Configuration from environment
INSTANCE_COUNT=${INSTANCE_COUNT:-3}
FIFO_BASE_PATH=${FIFO_BASE_PATH:-/tmp/fifo}
PORT_BASE=${PORT_BASE:-8020}
API_PORT_BASE=${API_PORT_BASE:-9210}
PAD_PORT_BASE=${PAD_PORT_BASE:-9210}
DLS_FILE=${DLS_FILE:-/tmp/dls.txt}
LOG_DIR=${LOG_DIR:-/var/log/odr}

# Binary location (try multiple paths)
BINARY_PATHS=(
    "/usr/bin/odr-padenc"           # Mock binary location
    "/app/src/odr-padenc"           # Volume-mounted development
    "/app/build/bin/odr-padenc"     # Production build
    "/app/odr-padenc"               # Alternative location
    "/usr/local/bin/odr-padenc"     # System install
)

BINARY=""
for path in "${BINARY_PATHS[@]}"; do
    if [ -f "$path" ]; then
        BINARY="$path"
        echo "✓ Found ODR-PadEnc binary: $BINARY"
        break
    fi
done

if [ -z "$BINARY" ]; then
    echo "ERROR: odr-padenc binary not found in: ${BINARY_PATHS[*]}"
    exit 1
fi

# Create required directories
mkdir -p "$FIFO_BASE_PATH"
mkdir -p "$LOG_DIR"
mkdir -p /tmp
chmod 777 "$FIFO_BASE_PATH" 2>/dev/null || true
chmod 777 /tmp 2>/dev/null || true
chmod 755 "$LOG_DIR" 2>/dev/null || true

# Create default DLS file if not exists
if [ ! -f "$DLS_FILE" ]; then
    echo "StreamDAB Thailand DAB+ Broadcasting" > "$DLS_FILE"
fi

# Arrays to track PIDs
PIDS=()
HEALTH_PIDS=()

echo ""
echo "Configuration:"
echo "========================================"
echo "Instance Count: $INSTANCE_COUNT"
echo "FIFO Base Path: $FIFO_BASE_PATH"
echo "Port Base: $PORT_BASE"
echo "API Port Base: $API_PORT_BASE"
echo "PAD Port Base: $PAD_PORT_BASE"
echo "DLS File: $DLS_FILE"
echo "Log Directory: $LOG_DIR"
echo "Binary: $BINARY"
echo "========================================"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Received shutdown signal, stopping all instances..."

    # Stop health servers
    for pid in "${HEALTH_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    # Stop PadEnc instances
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    # Wait for processes to stop
    sleep 2

    # Force kill if needed
    for pid in "${PIDS[@]}" "${HEALTH_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill -9 "$pid" 2>/dev/null || true
        fi
    done

    echo "All instances stopped"
    exit 0
}

trap cleanup SIGTERM SIGINT

# Start each instance
for i in $(seq 1 "$INSTANCE_COUNT"); do
    INSTANCE_ID=$i
    FIFO_PATH="$FIFO_BASE_PATH/audio${INSTANCE_ID}.fifo"
    PORT=$((PORT_BASE + INSTANCE_ID - 1))
    PAD_PORT=$((PAD_PORT_BASE + INSTANCE_ID - 1))
    API_PORT=$((API_PORT_BASE + INSTANCE_ID - 1))
    PAD_IDENT="pad${INSTANCE_ID}"
    LOG_FILE="$LOG_DIR/padenc_instance_${INSTANCE_ID}.log"

    echo "Starting PadEnc instance $INSTANCE_ID..."
    echo "  TCP Port: $PORT"
    echo "  PAD Port: $PAD_PORT"
    echo "  API Port: $API_PORT"
    echo "  FIFO: $FIFO_PATH (waiting for AudioEnc to create)"
    echo "  PAD Socket: /tmp/${PAD_IDENT}.padenc"
    echo "  Log: $LOG_FILE"

    # Wait for FIFO to be created by AudioEnc
    WAIT_COUNT=0
    MAX_WAIT=30
    while [ ! -p "$FIFO_PATH" ] && [ $WAIT_COUNT -lt $MAX_WAIT ]; do
        echo "  Waiting for FIFO to be created by AudioEnc... ($WAIT_COUNT/$MAX_WAIT)"
        sleep 1
        WAIT_COUNT=$((WAIT_COUNT + 1))
    done

    if [ ! -p "$FIFO_PATH" ]; then
        echo "  WARNING: FIFO not found after ${MAX_WAIT}s, starting anyway (may fail)"
    else
        echo "  ✓ FIFO ready"
    fi

    # Start padenc instance in background
    "$BINARY" \
        --pad="$PAD_PORT" \
        --dls="$DLS_FILE" \
        --pad-fifo="$FIFO_PATH" \
        --pad-socket="/tmp/${PAD_IDENT}.padenc" \
        > "$LOG_FILE" 2>&1 &

    INSTANCE_PID=$!
    PIDS+=("$INSTANCE_PID")

    echo "  Started PadEnc with PID: $INSTANCE_PID"

    # Start health endpoint server for this instance
    export INSTANCE_ID="$INSTANCE_ID"
    export API_PORT="$API_PORT"
    export TCP_PORT="$PORT"
    export FIFO_PATH="$FIFO_PATH"
    export PAD_SOCKET="/tmp/${PAD_IDENT}.padenc"
    export ODR_VERSION="v2.5.0"

    # Check if health_server.py exists
    if [ -f "/app/health_server.py" ]; then
        python3 /app/health_server.py > "$LOG_DIR/health_instance_${INSTANCE_ID}.log" 2>&1 &
        HEALTH_PID=$!
        HEALTH_PIDS+=("$HEALTH_PID")
        echo "  Health server started on port $API_PORT (PID: $HEALTH_PID)"
    elif [ -f "/app/src/health_server.py" ]; then
        python3 /app/src/health_server.py > "$LOG_DIR/health_instance_${INSTANCE_ID}.log" 2>&1 &
        HEALTH_PID=$!
        HEALTH_PIDS+=("$HEALTH_PID")
        echo "  Health server started on port $API_PORT (PID: $HEALTH_PID)"
    else
        echo "  WARNING: health_server.py not found, health endpoint not available"
        echo "  Expected locations: /app/health_server.py or /app/src/health_server.py"
    fi

    echo ""

    # Small delay between instances
    sleep 0.5
done

echo "========================================"
echo "All $INSTANCE_COUNT PadEnc instances started"
echo "PadEnc PIDs: ${PIDS[*]}"
echo "Health Server PIDs: ${HEALTH_PIDS[*]}"
echo "========================================"
echo ""
echo "Health endpoints available at:"
for i in $(seq 1 "$INSTANCE_COUNT"); do
    API_PORT=$((API_PORT_BASE + i - 1))
    echo "  Instance $i: http://localhost:$API_PORT/health"
done
echo ""
echo "Monitoring instances (checking every 5 seconds)..."

# Monitor instances
RESTART_COUNT=0
MAX_RESTARTS=5

while true; do
    NEED_RESTART=false

    # Check PadEnc instances
    for i in "${!PIDS[@]}"; do
        pid="${PIDS[$i]}"
        instance_id=$((i + 1))

        if ! kill -0 "$pid" 2>/dev/null; then
            echo "WARNING: PadEnc instance $instance_id (PID: $pid) died!"
            NEED_RESTART=true
            echo "$(date): Instance $instance_id crashed" >> "$LOG_DIR/crashes.log"
        fi
    done

    # Check health servers (restart if died, but don't trigger container restart)
    for i in "${!HEALTH_PIDS[@]}"; do
        pid="${HEALTH_PIDS[$i]}"
        instance_id=$((i + 1))

        if ! kill -0 "$pid" 2>/dev/null; then
            echo "WARNING: Health server for instance $instance_id (PID: $pid) died, restarting..."

            # Restart health server
            API_PORT=$((API_PORT_BASE + instance_id - 1))
            FIFO_PATH="$FIFO_BASE_PATH/audio${instance_id}.fifo"
            PAD_IDENT="pad${instance_id}"

            export INSTANCE_ID="$instance_id"
            export API_PORT="$API_PORT"
            export TCP_PORT=$((PORT_BASE + instance_id - 1))
            export FIFO_PATH="$FIFO_PATH"
            export PAD_SOCKET="/tmp/${PAD_IDENT}.padenc"
            export ODR_VERSION="v2.5.0"

            if [ -f "/app/health_server.py" ]; then
                python3 /app/health_server.py > "$LOG_DIR/health_instance_${instance_id}.log" 2>&1 &
                HEALTH_PIDS[$i]=$!
                echo "  Health server restarted (PID: ${HEALTH_PIDS[$i]})"
            fi
        fi
    done

    if [ "$NEED_RESTART" = true ]; then
        RESTART_COUNT=$((RESTART_COUNT + 1))
        echo "PadEnc instance failure detected (restart count: $RESTART_COUNT/$MAX_RESTARTS)"

        if [ $RESTART_COUNT -ge $MAX_RESTARTS ]; then
            echo "ERROR: Too many restarts ($RESTART_COUNT). Exiting for Docker to restart container."
            exit 1
        else
            echo "Container will exit for Docker to restart all instances."
            exit 1
        fi
    fi

    sleep 5
done
