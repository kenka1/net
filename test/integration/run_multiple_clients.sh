#!/usr/bin/env bash

set -euo pipefail

validate_positive_int() {
    local value=$1
    local name=$2

    if ! [[ "$value" =~ ^[0-9]+$ ]] || [[ "$value" -le 0 ]]; then
        printf "Error: %s must be an integer >= 1\n" "$name"
        exit 1
    fi
}

if [[ $# -ne 6 ]]; then
    printf "Usage: %s <host> <port> <clients> <freq> <num> <size>\n" "$0"
    exit 1
fi

# FD
ulimit -n 65536

readonly HOST=$1
readonly PORT=$2
readonly CLIENTS=$3
readonly FREQ=$4
readonly NUM=$5
readonly SIZE=$6

validate_positive_int "$CLIENTS" "clients"
validate_positive_int "$FREQ" "freq"
validate_positive_int "$NUM" "num"
validate_positive_int "$SIZE" "size"

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly SINGLE_CLIENT="$SCRIPT_DIR/single_client.py"

if [[ ! -f "$SINGLE_CLIENT" ]]; then
    printf "%s not found\n" "$SINGLE_CLIENT"
    exit 1
fi

readonly TMP_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

printf "Starting: %s clients freq: %s msg/s num: %s msg size: %s bytes\n" "$CLIENTS" "$FREQ" "$NUM" "$SIZE"

readonly START_TIME=$(date +%s%3N)

declare -a pids=()
for ((i=1; i<=CLIENTS; i++)); do
    python3 "$SINGLE_CLIENT" \
        --host "$HOST" \
        --port "$PORT" \
        --freq "$FREQ" \
        --num "$NUM" \
        --size "$SIZE" \
        --log "${TMP_DIR}/latency_${i}.log" > "${TMP_DIR}/client_${i}.log" 2>&1 &

    pids+=("$!")
done

failures=0
for pid in "${pids[@]}"; do
    if ! wait "$pid"; then
        ((failures++))
    fi
done

readonly END_TIME=$(date +%s%3N)
readonly ELAPSED="$(awk -v a=$((END_TIME - START_TIME)) 'BEGIN{printf "%.3f", a/1000}')"

printf "done: %s failures: %s\n" "$CLIENTS" "$failures"

total_sent_ok=0
total_sent_err=0
total_recv_ok=0
total_recv_err=0
count=0

shopt -s nullglob

for file in "$TMP_DIR"/client_*.log; do
    sent_ok=$(awk '$1 == "sent_ok" {print $2; found=1} END {if (!found) print 0}' "$file")
    total_sent_ok=$((total_sent_ok + sent_ok))

    sent_err=$(awk '$1 == "sent_err" {print $2; found=1} END {if (!found) print 0}' "$file")
    total_sent_err=$((total_sent_err + sent_err))

    recv_ok=$(awk '$1 == "recv_ok" {print $2; found=1} END {if (!found) print 0}' "$file")
    total_recv_ok=$((total_recv_ok + recv_ok))

    recv_err=$(awk '$1 == "recv_err" {print $2; found=1} END {if (!found) print 0}' "$file")
    total_recv_err=$((total_recv_err + recv_err))

    count=$((count + 1))
done

if [[ "$count" -ne 0 ]]; then
    printf "avg_sent_ok %s msg\n" "$((total_sent_ok / count))"
    printf "total_sent_ok %s msg\n" "$total_sent_ok"
    printf "avg_sent_err %s msg\n" "$((total_sent_err / count))"
    printf "total_sent_err %s msg\n" "$total_sent_err"
    printf "avg_recv_ok %s msg\n" "$((total_recv_ok / count))"
    printf "total_recv_ok %s msg\n" "$total_recv_ok"
    printf "avg_recv_err %s msg\n"  "$((total_recv_err / count))"
    printf "total_recv_err %s msg\n" "$total_recv_err"
    printf "avg_rate %s msg/s\n" "$(awk -v a=${total_sent_ok} -v b=${ELAPSED} 'BEGIN{ if (b > 0.001) printf "%.3f", a/b; else printf "0.0" }')"
    printf "elapsed %s s\n" "$ELAPSED"
else
    printf "No clients samples\n"
fi

declare -a latency_logs=()
for file in "$TMP_DIR"/latency_*.log; do
    latency_logs+=("$file")
done

if [[ "${#latency_logs[@]}" -ne 0 ]]; then
    sort -n "${latency_logs[@]}" | awk '{
        arr[NR] = $0
    } END {
        if (NR == 0) exit

        printf "p50 %.3f ms\n", arr[int((NR - 1) * 0.50) + 1]
        printf "p95 %.3f ms\n", arr[int((NR - 1) * 0.95) + 1]
        printf "p99 %.3f ms\n", arr[int((NR - 1) * 0.99) + 1]
    }'
else
    printf "No latency samples\n"
fi
