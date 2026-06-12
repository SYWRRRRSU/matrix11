# 并行多源最短路径实验

本仓库实现实验文档“8 - 并行多源最短路径搜索”的代码与测试脚本。程序使用
OpenMP 并行 Floyd-Warshall 算法计算无向图所有顶点对之间的最短路径距离。

## 数据格式

图文件为邻接表 CSV，每行包含两个整型顶点 ID 和一个浮点型边权：

```csv
source,target,distance
0,1,1.094099325
2,3,1.107530956
```

实验要求忽略边方向，因此程序会把每条输入边 `(u, v, w)` 同时作为 `u -> v`
和 `v -> u`。邻接表中不存在的边距离为无穷大。

测试文件为顶点对 CSV：

```csv
source,target
0,3
3,0
```

输出文件会保留测试文件的两列，并追加最短路径距离：

```csv
source,target,distance
0,3,6.5
3,0,6.5
```

如果两个顶点不可达，距离输出为 `INF`。

## 编译

需要支持 OpenMP 的 C++17 编译器，例如 `g++`：

```bash
make
```

生成的可执行文件为：

```text
bin/parallel_apsp
```

## 运行

```bash
bin/parallel_apsp <graph_csv> <query_csv> <output_csv> [threads]
```

示例：

```bash
python3 scripts/generate_queries.py updated_mouse.csv results/mouse_queries.csv -n 1000
bin/parallel_apsp updated_mouse.csv results/mouse_queries.csv results/mouse_output.csv 4
```

程序会在标准输出打印节点数、边数、查询数、线程数和 Floyd-Warshall 计算耗时：

```text
nodes=525
edges=14691
queries=1000
threads=4
elapsed_seconds=0.123456
```

## 测试脚本

运行小图正确性测试：

```bash
make test
```

该脚本会构造一个小型无向加权图，分别用 1 线程和 4 线程运行程序，并检查可达、
不可达和无向边路径结果。

## 基准脚本

运行两个给定数据集在不同线程数下的实验：

```bash
make benchmark
```

默认会为每个数据集生成 1000 条随机查询，并测试 `1 2 4 8 16` 线程。可以通过
环境变量调整：

```bash
QUERY_COUNT=5000 THREADS="1 2 4 8" make benchmark
```

汇总结果写入：

```text
results/benchmark.csv
```
