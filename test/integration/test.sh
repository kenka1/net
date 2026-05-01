
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf "Usage: %s <ssh(login@host)\n" "$0"
    exit 1
fi

readonly SSH=$1

output=$(ssh "$SSH" "bash ~/net/test/integration/run_multiple_clients.sh 176.124.161.239 8989 1 10 10 10")
echo "$output" > log.txt
