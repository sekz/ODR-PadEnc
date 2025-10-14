#!/bin/bash
# Mock ODR-PadEnc - Simulates ODR-PadEnc behavior for infrastructure testing
# Version: 1.0.0
# Component: PadEnc Mock Binary

set -euo pipefail

# Configuration
LOG_DIR="/var/log/odr"
PID_FILE="/tmp/mock-padenc-$$.pid"
START_TIME=$(date +%s)
FIFO_PATH=""
PAD_DIR=""
DLS_FILE=""
SLIDE_DIR=""
SOCKET_PORT=""
VERBOSITY="info"

# Logging function
log() {
    local level="$1"
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] [PadEnc-Mock] $message" | tee -a "$LOG_DIR/padenc-mock.log"
}

# Signal handlers
cleanup() {
    log "INFO" "Received shutdown signal, cleaning up..."

    # Remove PID file
    if [ -f "$PID_FILE" ]; then
        rm -f "$PID_FILE"
    fi

    # Close FIFO if exists
    if [ -n "$FIFO_PATH" ] && [ -p "$FIFO_PATH" ]; then
        log "INFO" "Closing FIFO: $FIFO_PATH"
    fi

    log "INFO" "Mock PadEnc shutdown complete"
    exit 0
}

trap cleanup SIGTERM SIGINT SIGQUIT

# Parse command-line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -o|--output|--output=*|--pad-fifo|--pad-fifo=*)
                if [[ "$1" == --output=* ]] || [[ "$1" == --pad-fifo=* ]]; then
                    FIFO_PATH="${1#*=}"
                    shift
                else
                    FIFO_PATH="$2"
                    shift 2
                fi
                ;;
            -d|--dir|--dir=*)
                if [[ "$1" == --dir=* ]]; then
                    PAD_DIR="${1#*=}"
                    shift
                else
                    PAD_DIR="$2"
                    shift 2
                fi
                ;;
            -t|--dls|--dls=*)
                if [[ "$1" == --dls=* ]]; then
                    DLS_FILE="${1#*=}"
                    shift
                else
                    DLS_FILE="$2"
                    shift 2
                fi
                ;;
            -s|--slide|--slide=*)
                if [[ "$1" == --slide=* ]]; then
                    SLIDE_DIR="${1#*=}"
                    shift
                else
                    SLIDE_DIR="$2"
                    shift 2
                fi
                ;;
            -p|--port|--port=*)
                if [[ "$1" == --port=* ]]; then
                    SOCKET_PORT="${1#*=}"
                    shift
                else
                    SOCKET_PORT="$2"
                    shift 2
                fi
                ;;
            --pad=*|--sleep=*|--erase-after-tx|--pad-socket|--pad-socket=*)
                # Ignore ODR-specific flags that mock doesn't need
                if [[ "$1" == --pad-socket ]]; then
                    shift 2  # Skip flag and value
                else
                    shift    # Skip flag=value format
                fi
                ;;
            -v|--verbosity)
                VERBOSITY="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                log "WARN" "Unknown argument: $1"
                shift
                ;;
        esac
    done
}

show_help() {
    cat <<EOF
Mock ODR-PadEnc - Simulates ODR-PadEnc v3.6.0

Usage: odr-padenc [OPTIONS]

Options:
  -o, --output FIFO       Output FIFO path (required)
  -d, --dir DIR           PAD content directory
  -t, --dls FILE          DLS text file
  -s, --slide DIR         Slide directory
  -p, --port PORT         Socket port for control
  -v, --verbosity LEVEL   Verbosity level (default: info)
  -h, --help              Show this help message

Example:
  odr-padenc -o /tmp/fifo/padenc.fifo -d /data/pad -t /data/dls.txt -s /data/slides
EOF
}

# Initialize
init_encoder() {
    # Create log directory if needed
    mkdir -p "$LOG_DIR"

    # Write PID
    echo $$ > "$PID_FILE"

    log "INFO" "Starting Mock ODR-PadEnc v3.6.0"
    log "INFO" "Configuration:"
    log "INFO" "  Output FIFO: $FIFO_PATH"
    log "INFO" "  PAD Directory: ${PAD_DIR:-none}"
    log "INFO" "  DLS File: ${DLS_FILE:-none}"
    log "INFO" "  Slide Directory: ${SLIDE_DIR:-none}"
    log "INFO" "  Socket Port: ${SOCKET_PORT:-none}"
    log "INFO" "  Verbosity: $VERBOSITY"

    # Create FIFO if it doesn't exist
    if [ ! -p "$FIFO_PATH" ]; then
        log "INFO" "Creating FIFO: $FIFO_PATH"
        FIFO_DIR=$(dirname "$FIFO_PATH")
        mkdir -p "$FIFO_DIR"
        mkfifo "$FIFO_PATH"
    fi

    # Create PAD directory if specified
    if [ -n "$PAD_DIR" ] && [ ! -d "$PAD_DIR" ]; then
        log "INFO" "Creating PAD directory: $PAD_DIR"
        mkdir -p "$PAD_DIR"
    fi

    # Simulate initialization delay
    sleep 2
    log "INFO" "PadEnc initialized successfully"
}

# Monitor PAD content
monitor_pad_content() {
    local dls_count=0
    local slide_count=0

    if [ -n "$DLS_FILE" ] && [ -f "$DLS_FILE" ]; then
        dls_count=$(wc -l < "$DLS_FILE" 2>/dev/null || echo 0)
        log "INFO" "DLS file has $dls_count lines"
    fi

    if [ -n "$SLIDE_DIR" ] && [ -d "$SLIDE_DIR" ]; then
        slide_count=$(find "$SLIDE_DIR" -type f \( -name "*.jpg" -o -name "*.png" \) 2>/dev/null | wc -l || echo 0)
        log "INFO" "Slide directory has $slide_count images"
    fi
}

# Main encoding loop
run_encoder() {
    local pad_updates=0
    local last_log_time=$START_TIME

    log "INFO" "PadEnc running, monitoring PAD content"
    log "INFO" "PAD output streaming to FIFO: $FIFO_PATH"

    # Main loop - simulates PAD encoding
    while true; do
        sleep 5

        pad_updates=$((pad_updates + 1))
        local current_time=$(date +%s)
        local uptime=$((current_time - START_TIME))

        # Log status every 30 seconds
        if [ $((current_time - last_log_time)) -ge 30 ]; then
            log "INFO" "PAD status: uptime=${uptime}s, updates=${pad_updates}"
            monitor_pad_content
            last_log_time=$current_time
        fi

        # Verify FIFO still exists
        if [ ! -p "$FIFO_PATH" ]; then
            log "ERROR" "FIFO disappeared: $FIFO_PATH"
            exit 1
        fi
    done
}

# Main execution
main() {
    parse_args "$@"

    # Validate required arguments
    if [ -z "$FIFO_PATH" ]; then
        log "ERROR" "Output FIFO is required (-o/--output)"
        show_help
        exit 1
    fi

    init_encoder
    run_encoder
}

# Run main
main "$@"
