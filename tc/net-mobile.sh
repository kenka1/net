#!/usr/bin/env bash
DEV=${1:-eth0}

sudo tc qdisc del dev "$DEV" root 2>/dev/null || true

sudo tc qdisc add dev "$DEV" root netem \
  delay 80ms 30ms distribution normal \
  loss 1% \
  reorder 0.2% 50%
