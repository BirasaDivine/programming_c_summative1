#!/bin/bash
# Linux Server Health Monitor

LOG_FILE="health_monitor.log"
CPU_THRESHOLD=80
MEM_THRESHOLD=80
DISK_THRESHOLD=90
MONITOR_INTERVAL=60
MONITOR_PID=""

# Terminal colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# -------- Utilities --------

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

check_dependencies() {
    local missing=0
    for cmd in top free df ps awk grep; do
        if ! command -v "$cmd" &>/dev/null; then
            echo "Error: required command '$cmd' not found." >&2
            missing=1
        fi
    done
    [ $missing -eq 1 ] && exit 1
}

validate_percent() {
    [[ "$1" =~ ^[0-9]+$ ]] && [ "$1" -ge 1 ] && [ "$1" -le 100 ]
}

validate_interval() {
    [[ "$1" =~ ^[0-9]+$ ]] && [ "$1" -ge 1 ]
}

# -------- Metric Collection --------

get_cpu() {
    # Extract idle% from top, subtract from 100
    local idle
    idle=$(top -bn1 | grep -E "Cpu|cpu" | head -1 \
        | awk '{for(i=1;i<=NF;i++) if($i~/^[0-9.]+$/ && $(i+1)~/id/) {print $i; exit}}')
    if [ -z "$idle" ]; then
        # Fallback: try /proc/stat
        local vals
        vals=$(awk '/^cpu / {print $2,$3,$4,$5,$6,$7,$8}' /proc/stat 2>/dev/null)
        if [ -n "$vals" ]; then
            local idle2 total
            idle2=$(echo "$vals" | awk '{print $4}')
            total=$(echo "$vals" | awk '{t=0; for(i=1;i<=NF;i++) t+=$i; print t}')
            idle=$(awk "BEGIN {printf \"%.1f\", ($idle2/$total)*100}")
        fi
    fi
    [ -z "$idle" ] && { echo "N/A"; return; }
    awk "BEGIN {printf \"%.1f\", 100 - $idle}"
}

get_mem() {
    free | awk '/^Mem:/ {printf "%.1f", $3/$2*100}'
}

get_disk() {
    df / | awk 'NR==2 {print $5}' | tr -d '%'
}

get_procs() {
    ps -e --no-headers 2>/dev/null | wc -l
}

# -------- Alert Check --------

check_alert() {
    local label="$1" value="$2" threshold="$3"
    local ivalue="${value%.*}"
    if [[ "$ivalue" =~ ^[0-9]+$ ]] && [ "$ivalue" -ge "$threshold" ]; then
        echo -e "${RED}[ALERT]${NC} $label is ${value}% — threshold is ${threshold}%"
        log "ALERT: $label=${value}% (threshold ${threshold}%)"
    else
        echo -e "${GREEN}[OK]${NC}    $label is ${value}%"
    fi
}

# -------- Menu Functions --------

show_health() {
    local cpu mem disk procs
    cpu=$(get_cpu)
    mem=$(get_mem)
    disk=$(get_disk)
    procs=$(get_procs)

    echo ""
    echo -e "${CYAN}${BOLD}=== System Health Dashboard ===${NC}"
    printf "%-20s %s\n" "Timestamp:"      "$(date '+%Y-%m-%d %H:%M:%S')"
    printf "%-20s %s\n" "Hostname:"       "$(hostname)"
    printf "%-20s %s\n" "Uptime:"         "$(uptime -p 2>/dev/null || uptime)"
    echo   "-------------------------------------------"
    printf "%-20s ${BOLD}%s%%${NC}\n"  "CPU Usage:"         "$cpu"
    printf "%-20s ${BOLD}%s%%${NC}\n"  "Memory Usage:"      "$mem"
    printf "%-20s ${BOLD}%s%%${NC}\n"  "Disk Usage (/):"    "$disk"
    printf "%-20s ${BOLD}%s${NC}\n"    "Active Processes:"  "$procs"
    echo   "-------------------------------------------"
    echo   "Threshold Status:"
    check_alert "CPU"    "$cpu"  "$CPU_THRESHOLD"
    check_alert "Memory" "$mem"  "$MEM_THRESHOLD"
    check_alert "Disk"   "$disk" "$DISK_THRESHOLD"

    log "HEALTH: CPU=${cpu}% MEM=${mem}% DISK=${disk}% PROCS=${procs}"
}

configure_thresholds() {
    echo ""
    echo -e "${BOLD}=== Configure Alert Thresholds ===${NC}"
    echo "Current: CPU=${CPU_THRESHOLD}%  Memory=${MEM_THRESHOLD}%  Disk=${DISK_THRESHOLD}%"
    echo ""

    local input
    read -rp "New CPU threshold    (1-100) [${CPU_THRESHOLD}]: " input
    if [ -n "$input" ]; then
        if validate_percent "$input"; then
            CPU_THRESHOLD=$input
            echo "  CPU threshold set to ${CPU_THRESHOLD}%"
        else
            echo "  Invalid value — keeping ${CPU_THRESHOLD}%"
        fi
    fi

    read -rp "New Memory threshold (1-100) [${MEM_THRESHOLD}]: " input
    if [ -n "$input" ]; then
        if validate_percent "$input"; then
            MEM_THRESHOLD=$input
            echo "  Memory threshold set to ${MEM_THRESHOLD}%"
        else
            echo "  Invalid value — keeping ${MEM_THRESHOLD}%"
        fi
    fi

    read -rp "New Disk threshold   (1-100) [${DISK_THRESHOLD}]: " input
    if [ -n "$input" ]; then
        if validate_percent "$input"; then
            DISK_THRESHOLD=$input
            echo "  Disk threshold set to ${DISK_THRESHOLD}%"
        else
            echo "  Invalid value — keeping ${DISK_THRESHOLD}%"
        fi
    fi

    log "CONFIG: CPU_THRESH=${CPU_THRESHOLD}% MEM_THRESH=${MEM_THRESHOLD}% DISK_THRESH=${DISK_THRESHOLD}%"
    echo "Thresholds updated."
}

view_logs() {
    echo ""
    echo -e "${BOLD}=== Activity Log: ${LOG_FILE} ===${NC}"
    if [ -s "$LOG_FILE" ]; then
        cat "$LOG_FILE"
    else
        echo "Log is empty or does not exist."
    fi
}

clear_logs() {
    read -rp "Clear all logs? (y/N): " confirm
    case "$confirm" in
        [yY])
            > "$LOG_FILE"
            echo "Log cleared."
            log "Log cleared by user."
            ;;
        *)
            echo "Cancelled."
            ;;
    esac
}

# Background monitoring subprocess
_monitor_loop() {
    log "Auto-monitor started (interval=${MONITOR_INTERVAL}s  CPU=${CPU_THRESHOLD}%  MEM=${MEM_THRESHOLD}%  DISK=${DISK_THRESHOLD}%)"
    while true; do
        local cpu mem disk
        cpu=$(get_cpu)
        mem=$(get_mem)
        disk=$(get_disk)
        log "HEALTH: CPU=${cpu}% MEM=${mem}% DISK=${disk}%"

        local ic="${cpu%.*}" im="${mem%.*}" id="${disk%.*}"
        [[ "$ic" =~ ^[0-9]+$ ]] && [ "$ic" -ge "$CPU_THRESHOLD" ]    && log "ALERT: CPU at ${cpu}%"
        [[ "$im" =~ ^[0-9]+$ ]] && [ "$im" -ge "$MEM_THRESHOLD" ]    && log "ALERT: Memory at ${mem}%"
        [[ "$id" =~ ^[0-9]+$ ]] && [ "$id" -ge "$DISK_THRESHOLD" ]   && log "ALERT: Disk at ${disk}%"

        sleep "$MONITOR_INTERVAL"
    done
}

start_monitoring() {
    if [ -n "$MONITOR_PID" ] && kill -0 "$MONITOR_PID" 2>/dev/null; then
        echo "Monitoring is already running (PID $MONITOR_PID)."
        return
    fi

    local input
    read -rp "Check interval in seconds [${MONITOR_INTERVAL}]: " input
    if [ -n "$input" ]; then
        if validate_interval "$input"; then
            MONITOR_INTERVAL=$input
        else
            echo "Invalid interval — using ${MONITOR_INTERVAL}s."
        fi
    fi

    _monitor_loop &
    MONITOR_PID=$!
    echo "Monitoring started in background (PID $MONITOR_PID, every ${MONITOR_INTERVAL}s)."
    echo "Alerts are written to ${LOG_FILE}."
    log "Background monitoring started (PID $MONITOR_PID)."
}

stop_monitoring() {
    if [ -n "$MONITOR_PID" ] && kill -0 "$MONITOR_PID" 2>/dev/null; then
        kill "$MONITOR_PID"
        wait "$MONITOR_PID" 2>/dev/null
        log "Auto-monitor stopped (PID $MONITOR_PID)."
        MONITOR_PID=""
        echo "Monitoring stopped."
    else
        echo "Monitoring is not running."
    fi
}

# -------- Main Loop --------

main() {
    check_dependencies

    while true; do
        local status_str
        if [ -n "$MONITOR_PID" ] && kill -0 "$MONITOR_PID" 2>/dev/null; then
            status_str="${GREEN}running${NC} (PID $MONITOR_PID, every ${MONITOR_INTERVAL}s)"
        else
            status_str="${RED}stopped${NC}"
        fi

        echo ""
        echo -e "${BOLD}============================================${NC}"
        echo -e "${BOLD}       Linux Server Health Monitor${NC}"
        echo -e "${BOLD}============================================${NC}"
        echo -e "Auto-monitor : $status_str"
        echo    "Thresholds   : CPU=${CPU_THRESHOLD}%  Mem=${MEM_THRESHOLD}%  Disk=${DISK_THRESHOLD}%"
        echo    "Log file     : ${LOG_FILE}"
        echo    "--------------------------------------------"
        echo    "1. Show current system health"
        echo    "2. Configure alert thresholds"
        echo    "3. View activity log"
        echo    "4. Clear activity log"
        echo    "5. Start background monitoring"
        echo    "6. Stop background monitoring"
        echo    "7. Exit"
        echo    "--------------------------------------------"
        read -rp "Choice: " choice

        case "$choice" in
            1) show_health ;;
            2) configure_thresholds ;;
            3) view_logs ;;
            4) clear_logs ;;
            5) start_monitoring ;;
            6) stop_monitoring ;;
            7)
                stop_monitoring 2>/dev/null
                log "Script exited."
                echo "Goodbye."
                exit 0
                ;;
            "")
                ;;
            *)
                echo -e "${YELLOW}Invalid choice. Enter 1-7.${NC}"
                ;;
        esac
    done
}

main
