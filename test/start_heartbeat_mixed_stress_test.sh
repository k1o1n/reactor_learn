#!/bin/bash

set -u

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

SERVER_IP="${1:-127.0.0.1}"
SERVER_PORT="${2:-12345}"
TOTAL_CLIENTS="${TOTAL_CLIENTS:-10000}"
ACTIVE_CLIENTS="${ACTIVE_CLIENTS:-2000}"
SEND_INTERVAL_MS="${SEND_INTERVAL_MS:-3000}"
ACTIVE_PHASE_MS="${ACTIVE_PHASE_MS:-25000}"
IDLE_MIN_MS="${IDLE_MIN_MS:-8000}"
IDLE_MAX_MS="${IDLE_MAX_MS:-15000}"
BIN_NAME="heartbeat_mixed_stress_client"

echo "[INFO] Compiling ${BIN_NAME}.cpp ..."
g++ -O2 -std=c++17 -Wall -Wextra heartbeat_mixed_stress_client.cpp -o "$BIN_NAME"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile heartbeat_mixed_stress_client.cpp"
    exit 1
fi

ulimit -n 65535 2>/dev/null
CURRENT_LIMIT=$(ulimit -n)
echo "[INFO] Current file descriptor limit: $CURRENT_LIMIT"

NEEDED_LIMIT=$((TOTAL_CLIENTS + 2048))
if [ "$CURRENT_LIMIT" -lt "$NEEDED_LIMIT" ]; then
    echo "[WARNING] ulimit -n is less than $NEEDED_LIMIT. Connection count may be capped."
    echo "[TIP] Try: ulimit -n 65535"
fi

echo "[INFO] Target server: ${SERVER_IP}:${SERVER_PORT}"
echo "[INFO] total=${TOTAL_CLIENTS} active=${ACTIVE_CLIENTS} silent=$((TOTAL_CLIENTS - ACTIVE_CLIENTS))"
echo "[INFO] active clients send every ${SEND_INTERVAL_MS}ms for ${ACTIVE_PHASE_MS}ms"
echo "[INFO] expected idle close window: ${IDLE_MIN_MS}ms - ${IDLE_MAX_MS}ms"
LAST_ACTIVE_SEND_MS=$((ACTIVE_PHASE_MS - SEND_INTERVAL_MS))
if [ "$LAST_ACTIVE_SEND_MS" -lt 0 ]; then
    LAST_ACTIVE_SEND_MS=0
fi
EARLIEST_FULL_RESULT_MS=$((LAST_ACTIVE_SEND_MS + IDLE_MIN_MS))
LATEST_FULL_RESULT_MS=$((ACTIVE_PHASE_MS + IDLE_MAX_MS))
echo "[INFO] around ${IDLE_MIN_MS}-${IDLE_MAX_MS}ms you should first see whether silent clients are timing out"
echo "[INFO] full mixed verdict usually appears around ${EARLIEST_FULL_RESULT_MS}-${LATEST_FULL_RESULT_MS}ms from start"

./"$BIN_NAME" \
    "$SERVER_IP" \
    "$SERVER_PORT" \
    "$TOTAL_CLIENTS" \
    "$ACTIVE_CLIENTS" \
    "$SEND_INTERVAL_MS" \
    "$ACTIVE_PHASE_MS" \
    "$IDLE_MIN_MS" \
    "$IDLE_MAX_MS"