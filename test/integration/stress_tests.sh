#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 6 ]]; then
    printf "Usage: %s <server> <script> <ssh(login@host) <host> <port> <max_clients> \n" "$0"
    exit 1
fi

readonly SERVER=$1
readonly SCRIPT=$2
readonly SSH=$3
readonly HOST=$4
readonly PORT=$5
readonly MAX_CLIENTS=$6
readonly FREQ=1000
readonly NUMBER=20000
readonly SIZE=256

if ! [[ "$MAX_CLIENTS" =~ ^[0-9]+$ ]] || [[ "$MAX_CLIENTS" -lt 1 ]]; then
    printf "Error: <max_clients> must be an integer >= 1\n"
    exit 1
fi

if ! [[ -f "$SERVER" ]]; then
    printf "Error: server %s not found\n" "$SERVER"
    exit 1
fi

# FD
ulimit -n 65536

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly LOG_ROOT="${SCRIPT_DIR}/log"
readonly RUN_ID="$(date +%Y%m%d_%H%M%S)"
readonly OUT_DIR="${LOG_ROOT}/run_${RUN_ID}"
mkdir -p "$OUT_DIR"

readonly TMP_DIR="${OUT_DIR}/raw"
mkdir -p "$TMP_DIR"

printf "Log directory: %s\n" "$OUT_DIR"
printf "Client steps: 1, 2, 4, 8, ... up to %s\n" "$MAX_CLIENTS"

server_pid=""
perf_pid=""

cleanup_procs() {
    [[ -n "${perf_pid}" ]] && kill -INT "${perf_pid}" 2>/dev/null || true
    [[ -n "${perf_pid}" ]] && wait "${perf_pid}" 2>/dev/null || true
    [[ -n "${server_pid}" ]] && kill -INT "${server_pid}" 2>/dev/null || true
    [[ -n "${server_pid}" ]] && wait "${server_pid}" 2>/dev/null || true
}
trap cleanup_procs EXIT INT TERM

for ((i = 1; i <= MAX_CLIENTS; i *= 2)); do
    # 1. START SERVER
    "$SERVER" "0::0" "$PORT"> "${TMP_DIR}/server_${i}.log" 2>&1 &
    server_pid=$!
    printf "Server started (PID: %s) for %s clients\n" "$server_pid" "$i"
    sleep 5

    # 2. START PERF
    perf stat -d -p "$server_pid" -o "${TMP_DIR}/perf_${i}.log" &
    perf_pid=$!

    # 3. START TESTS
    printf "Running tests for %s clients...\n" "$i"
    output=$(ssh "$SSH" "${SCRIPT} ${HOST} ${PORT} ${i} ${FREQ} ${NUMBER} ${SIZE}")
    echo "$output" > "${TMP_DIR}/clients_${i}.log" 2>&1

    # 4. TESTS DONE, STOP PERF (ignore errors: perf exits non-zero on SIGINT)
    kill -INT "$perf_pid" 2>/dev/null || true
    wait "$perf_pid" 2>/dev/null || true
    perf_pid=""

    # 5. STOP SERVER
    printf "Stopping server (PID: %s)\n" "$server_pid"
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
    server_pid=""

    printf "Finished iteration for %s clients\n" "$i"
    sleep 15
done

readonly PERF_CON="${OUT_DIR}/perf_con.log"
readonly PERF_INSTR="${OUT_DIR}/perf_instr.log"
readonly PERF_MIG="${OUT_DIR}/perf_mig.log"
readonly PERF_ELAPSED="${OUT_DIR}/perf_elapsed.log"

readonly CLI_RATE="${OUT_DIR}/cli_rate.log"
readonly CLI_P50="${OUT_DIR}/cli_p50.log"
readonly CLI_P95="${OUT_DIR}/cli_p95.log"
readonly CLI_P99="${OUT_DIR}/cli_p99.log"

echo "clients; context_switches; context_switches_per_s" > "$PERF_CON"
echo "clients; instructions; instructions_per_s"         > "$PERF_INSTR"
echo "clients; cpu_migrations; cpu_migrations_per_s"     > "$PERF_MIG"
echo "clients; elapsed_s"                                > "$PERF_ELAPSED"

echo "clients; rate" > "$CLI_RATE"
echo "clients; p50"  > "$CLI_P50"
echo "clients; p95"  > "$CLI_P95"
echo "clients; p99"  > "$CLI_P99"

for ((i = 1; i <= MAX_CLIENTS; i *= 2)); do
    PERF_FILE="${TMP_DIR}/perf_${i}.log"
    CLI_FILE="${TMP_DIR}/clients_${i}.log"

    if [[ -f "$PERF_FILE" ]]; then
        awk -v i="$i" -v perf_con="$PERF_CON" -v perf_instr="$PERF_INSTR" \
            -v perf_mig="$PERF_MIG" -v perf_elapsed="$PERF_ELAPSED" '
            function num(s) {
                gsub(/,/, "", s)
                return s + 0
            }
            BEGIN { has_ctx = 0; has_inst = 0; has_mig = 0; elapsed = 0 }
            $2 == "context-switches" { ctx = num($1); has_ctx = 1 }
            $2 == "instructions"     { inst = num($1); has_inst = 1 }
            $2 == "cpu-migrations"   { mig = num($1); has_mig = 1 }
            /seconds time elapsed/   { elapsed = num($1) }
            END {
                if (elapsed > 0) {
                    printf "%d; %.3f\n", i, elapsed >> perf_elapsed
                } else {
                    printf "%d;\n", i >> perf_elapsed
                }
                if (has_ctx) {
                    if (elapsed > 0) {
                        printf "%d; %.0f; %.3f\n", i, ctx, ctx / elapsed >> perf_con
                    } else {
                        printf "%d; %.0f;\n", i, ctx >> perf_con
                    }
                }
                if (has_inst) {
                    if (elapsed > 0) {
                        printf "%d; %.0f; %.0f\n", i, inst, inst / elapsed >> perf_instr
                    } else {
                        printf "%d; %.0f;\n", i, inst >> perf_instr
                    }
                }
                if (has_mig) {
                    if (elapsed > 0) {
                        printf "%d; %.0f; %.3f\n", i, mig, mig / elapsed >> perf_mig
                    } else {
                        printf "%d; %.0f;\n", i, mig >> perf_mig
                    }
                }
            }
        ' "$PERF_FILE"
    fi

    if [[ -f "$CLI_FILE" ]]; then
        awk -v i="$i" -v cli_rate="$CLI_RATE" -v cli_p50="$CLI_P50" \
            -v cli_p95="$CLI_P95" -v cli_p99="$CLI_P99" '
            $1 == "avg_rate" {
                printf "%d; %.3f\n", i, $2 >> cli_rate
            }
            $1 == "p50" {
                printf "%d; %.3f\n", i, $2 >> cli_p50
            }
            $1 == "p95" {
                printf "%d; %.3f\n", i, $2 >> cli_p95
            }
            $1 == "p99" {
                printf "%d; %.3f\n", i, $2 >> cli_p99
            }
        ' "$CLI_FILE"
    fi
done

printf "Done. Summaries under: %s\n" "$OUT_DIR"
ln -sfn "${OUT_DIR}" "${LOG_ROOT}/latest" 
