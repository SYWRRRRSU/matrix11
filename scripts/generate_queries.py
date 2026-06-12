#!/usr/bin/env python3
"""Generate random source,target query pairs for an adjacency-list CSV graph."""

from __future__ import annotations

import argparse
import csv
import random
from pathlib import Path


def read_vertices(graph_path: Path) -> list[int]:
    vertices: set[int] = set()
    with graph_path.open(newline="") as graph_file:
        reader = csv.reader(graph_file)
        for row in reader:
            if len(row) < 2:
                continue
            try:
                source = int(row[0].strip())
                target = int(row[1].strip())
            except ValueError:
                continue
            vertices.add(source)
            vertices.add(target)

    if not vertices:
        raise ValueError(f"no vertices found in {graph_path}")
    return sorted(vertices)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate random vertex-pair query CSV for APSP experiments."
    )
    parser.add_argument("graph_csv", type=Path, help="input graph CSV")
    parser.add_argument("output_csv", type=Path, help="output query CSV")
    parser.add_argument(
        "-n", "--num-queries", type=int, default=1000, help="number of pairs to write"
    )
    parser.add_argument("--seed", type=int, default=2026, help="random seed")
    args = parser.parse_args()

    if args.num_queries <= 0:
        raise ValueError("--num-queries must be positive")

    vertices = read_vertices(args.graph_csv)
    rng = random.Random(args.seed)
    args.output_csv.parent.mkdir(parents=True, exist_ok=True)

    with args.output_csv.open("w", newline="") as output_file:
        writer = csv.writer(output_file)
        writer.writerow(["source", "target"])
        for _ in range(args.num_queries):
            writer.writerow([rng.choice(vertices), rng.choice(vertices)])


if __name__ == "__main__":
    main()
