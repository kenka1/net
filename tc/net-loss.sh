#!/usr/bin/env bash
DEV=${1:-eth0}

sudo tc qdisc del dev "$DEV" root 2>/dev/null || true
sudo tc qdisc add dev "$DEV" root netem loss 2%
