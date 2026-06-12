#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

make bin/parallel_apsp

QUERY_COUNT="${QUERY_COUNT:-1000}"
THREADS="${THREADS:-1 2 4 8 16}"
RESULT_DIR="${RESULT_DIR:-results}"
mkdir -p "$RESULT_DIR/queries" "$RESULT_DIR/outputs"

SUMMARY="$RESULT_DIR/benchmark.csv"
echo "dataset,nodes,edges,queries,threads,elapsed_seconds,output_csv" > "$SUMMARY"

for DATASET in updated_mouse.csv updated_flower.csv; do
  NAME="${DATASET%.csv}"
  QUERY_FILE="$RESULT_DIR/queries/${NAME}_queries.csv"
  python3 scripts/generate_queries.py "$DATASET" "$QUERY_FILE" \
    --num-queries "$QUERY_COUNT" --seed 2026

  for THREAD_COUNT in $THREADS; do
    OUTPUT_FILE="$RESULT_DIR/outputs/${NAME}_${THREAD_COUNT}t.csv"
    LOG_FILE="$RESULT_DIR/${NAME}_${THREAD_COUNT}t.log"
    bin/parallel_apsp "$DATASET" "$QUERY_FILE" "$OUTPUT_FILE" "$THREAD_COUNT" \
      | tee "$LOG_FILE"

    python3 - "$DATASET" "$THREAD_COUNT" "$OUTPUT_FILE" "$LOG_FILE" "$SUMMARY" <<'PY'
import sys

dataset, threads, output_csv, log_file, summary_file = sys.argv[1:]
stats = {}
with open(log_file, encoding="utf-8") as input_file:
    for line in input_file:
        line = line.strip()
        if "=" in line:
            key, value = line.split("=", 1)
            stats[key] = value

with open(summary_file, "a", encoding="utf-8") as output_file:
    output_file.write(
        ",".join(
            [
                dataset,
                stats["nodes"],
                stats["edges"],
                stats["queries"],
                threads,
                stats["elapsed_seconds"],
                output_csv,
            ]
        )
        + "\n"
    )
PY
  done
done

echo "Benchmark summary written to $SUMMARY"
