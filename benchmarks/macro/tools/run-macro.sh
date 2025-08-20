    #!/usr/bin/env bash
    set -euo pipefail
    cd "$(dirname "$0")/.."
    SCEN="scenarios.json"
    RUNTIME=$(jq -r '.runner' "$SCEN")
    TIMEOUT=$(jq -r '.timeout_sec' "$SCEN")
    RESULTS="results-$(date +%Y%m%d-%H%M%S).csv"
    echo "case,ms,ok" > "$RESULTS"
    jq -c '.cases[]' "$SCEN" | while read -r case; do
      NAME=$(echo "$case" | jq -r '.name')
      FILE=$(echo "$case" | jq -r '.file')
      ARGS=$(echo "$case" | jq -r '.args | join(" ")')
      START=$(python3 - <<'PY'
import time; print(int(time.time()*1000))
PY
)
      set +e
      timeout "$TIMEOUT"s "$RUNTIME" run "$FILE" $ARGS 1>/dev/null 2>&1
      CODE=$?
      set -e
      END=$(python3 - <<'PY'
import time; print(int(time.time()*1000))
PY
)
      MS=$((END-START))
      OK=$([ "$CODE" -eq 0 ] && echo 1 || echo 0)
      echo "$NAME,$MS,$OK" | tee -a "$RESULTS"
    done
    echo "[macro] results in $RESULTS"
