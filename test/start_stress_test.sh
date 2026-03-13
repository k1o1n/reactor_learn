#!/bin/bash
# 简单的启动脚本用于编译和运行压测.

# 1. 切换到脚本所在目录
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

# 2. 编译客户端
if [ ! -f "client.cpp" ]; then
    echo "Creating client.cpp..."
fi

echo "[INFO] Compiling..."
g++ -O2 -std=c++11 client.cpp -o client
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile client.cpp"
    exit 1
fi

g++ -O2 -std=c++11 stress_client.cpp -o stress_client
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile stress_client.cpp"
    exit 1
fi

# 3. 设置 ulimit
ulimit -n 65535 2>/dev/null
CURRENT_LIMIT=$(ulimit -n)
echo "[INFO] Current file descriptor limit: $CURRENT_LIMIT"

if [ "$CURRENT_LIMIT" -lt 10050 ]; then
    echo "[WARNING] ulimit -n is less than 10050. Stress test may fail or be capped."
    echo "[TIP] Please run 'ulimit -n 65535' manually if needed."
fi

# 4. 运行
SERVER_IP="127.0.0.1"
SERVER_PORT=12345
CONCURRENCY=10000

echo "[INFO] Starting stress test towards $SERVER_IP:$SERVER_PORT with $CONCURRENCY concurrent connections..."
echo "[INFO] Press Ctrl+C to stop."

./stress_client $SERVER_IP $SERVER_PORT $CONCURRENCY
