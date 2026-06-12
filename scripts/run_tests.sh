#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

make bin/parallel_apsp

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

GRAPH="$TMP_DIR/graph.csv"
QUERIES="$TMP_DIR/queries.csv"

cat > "$GRAPH" <<'CSV'
source,target,distance
0,1,2.0
1,2,3.0
0,2,10.0
2,3,1.5
4,5,7.0
CSV

cat > "$QUERIES" <<'CSV'
source,target
0,3
3,0
0,2
4,5
0,5
CSV

for THREADS in 1 4; do
  OUTPUT="$TMP_DIR/output_${THREADS}.csv"
  bin/parallel_apsp "$GRAPH" "$QUERIES" "$OUTPUT" "$THREADS"
  python3 - "$OUTPUT" <<'PY'
import csv
import math
import sys

expected = {
    (0, 3): 6.5,
    (3, 0): 6.5,
    (0, 2): 5.0,
    (4, 5): 7.0,
    (0, 5): math.inf,
}

with open(sys.argv[1], newline="") as result_file:
    reader = csv.DictReader(result_file)
    rows = list(reader)

if len(rows) != len(expected):
    raise SystemExit(f"expected {len(expected)} rows, got {len(rows)}")

for row in rows:
    key = (int(row["source"]), int(row["target"]))
    if key not in expected:
        raise SystemExit(f"unexpected query row: {key}")
    actual_text = row["distance"]
    wanted = expected[key]
    if math.isinf(wanted):
        if actual_text != "INF":
            raise SystemExit(f"{key}: expected INF, got {actual_text}")
    else:
        actual = float(actual_text)
        if abs(actual - wanted) > 1e-9:
            raise SystemExit(f"{key}: expected {wanted}, got {actual}")
PY
done

echo "All APSP correctness tests passed."
