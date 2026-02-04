#!/usr/bin/env bash
DEV=${1:-eth0}

sudo tc qdisc del dev "$DEV" root 2>/dev/null || true
sudo tc qdisc add dev "$DEV" root tbf rate 2mbit burst 32kbit latency 400ms
