#!/bin/bash

set -u

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

THREAD_COUNT="${THREAD_COUNT:-8}"
DURATION_SECONDS="${DURATION_SECONDS:-30}"
PAYLOAD_BYTES="${PAYLOAD_BYTES:-128}"
LOG_PATH="${LOG_PATH:-logger_stress.log}"
BIN_NAME="logger_stress"

echo "[INFO] Compiling ${BIN_NAME}.cpp ..."
g++ -O2 -std=c++17 -Wall -Wextra -pthread \
    -I../include \
    logger_stress.cpp ../src/logger.cpp \
    -o "$BIN_NAME"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile logger_stress.cpp"
    exit 1
fi

echo "[INFO] thread_count=${THREAD_COUNT} duration_seconds=${DURATION_SECONDS} payload_bytes=${PAYLOAD_BYTES}"
echo "[INFO] macro logger output will be renamed to ${LOG_PATH} after run"

rm -f "$LOG_PATH"
./"$BIN_NAME" "$THREAD_COUNT" "$DURATION_SECONDS" "$PAYLOAD_BYTES"

RUNTIME_LOG=$(find . -maxdepth 1 -type f -name 'pid*.log' -printf '%T@ %p\n' | sort -nr | head -n 1 | awk '{print $2}')
if [ -z "$RUNTIME_LOG" ]; then
    echo "[ERROR] logger runtime output pid*.log not found"
    exit 1
fi

if [ "$RUNTIME_LOG" != "./$LOG_PATH" ] && [ "$RUNTIME_LOG" != "$LOG_PATH" ]; then
    mv "$RUNTIME_LOG" "$LOG_PATH"
fi

echo "[INFO] final log file=${LOG_PATH}"