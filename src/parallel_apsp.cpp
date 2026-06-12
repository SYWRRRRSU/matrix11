#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

struct Edge {
    int source;
    int target;
    double distance;
};

struct Query {
    int source;
    int target;
};

std::string trim(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    return value.substr(first, last - first);
}

std::vector<std::string> split_fields(std::string line) {
    for (char& ch : line) {
        if (ch == ',' || ch == '\t') {
            ch = ' ';
        }
    }

    std::istringstream input(line);
    std::vector<std::string> fields;
    std::string field;
    while (input >> field) {
        fields.push_back(field);
    }
    return fields;
}

bool parse_int(const std::string& text, int& value) {
    char* end = nullptr;
    long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        return false;
    }
    if (parsed < 0 || parsed > std::numeric_limits<int>::max()) {
        throw std::runtime_error("vertex id is out of range: " + text);
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parse_double(const std::string& text, double& value) {
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end != text.c_str() && *end == '\0' && std::isfinite(value);
}

std::vector<Edge> read_edges(const std::string& path, int& max_vertex) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open graph file: " + path);
    }

    std::vector<Edge> edges;
    max_vertex = -1;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = split_fields(line);
        if (fields.size() < 3) {
            throw std::runtime_error("graph line " + std::to_string(line_number) +
                                     " must contain source,target,distance");
        }

        int source = 0;
        int target = 0;
        double distance = 0.0;
        const bool ok = parse_int(fields[0], source) && parse_int(fields[1], target) &&
                        parse_double(fields[2], distance);
        if (!ok) {
            if (edges.empty()) {
                continue;  // Header line, e.g. source,target,distance.
            }
            throw std::runtime_error("invalid graph data at line " + std::to_string(line_number));
        }
        if (distance < 0.0) {
            throw std::runtime_error("negative edge weights are not supported by Floyd-Warshall");
        }

        edges.push_back({source, target, distance});
        max_vertex = std::max(max_vertex, std::max(source, target));
    }

    if (edges.empty()) {
        throw std::runtime_error("graph file contains no edges: " + path);
    }
    return edges;
}

std::vector<Query> read_queries(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open query file: " + path);
    }

    std::vector<Query> queries;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = split_fields(line);
        if (fields.size() < 2) {
            throw std::runtime_error("query line " + std::to_string(line_number) +
                                     " must contain source,target");
        }

        int source = 0;
        int target = 0;
        const bool ok = parse_int(fields[0], source) && parse_int(fields[1], target);
        if (!ok) {
            if (queries.empty()) {
                continue;  // Header line, e.g. source,target.
            }
            throw std::runtime_error("invalid query data at line " + std::to_string(line_number));
        }
        queries.push_back({source, target});
    }

    if (queries.empty()) {
        throw std::runtime_error("query file contains no vertex pairs: " + path);
    }
    return queries;
}

std::vector<double> build_distance_matrix(const std::vector<Edge>& edges, int max_vertex) {
    const std::size_t n = static_cast<std::size_t>(max_vertex) + 1;
    std::vector<double> dist(n * n, kInf);
    for (std::size_t i = 0; i < n; ++i) {
        dist[i * n + i] = 0.0;
    }

    for (const Edge& edge : edges) {
        const std::size_t u = static_cast<std::size_t>(edge.source);
        const std::size_t v = static_cast<std::size_t>(edge.target);
        double& uv = dist[u * n + v];
        double& vu = dist[v * n + u];
        uv = std::min(uv, edge.distance);
        vu = std::min(vu, edge.distance);  // The lab requires ignoring edge direction.
    }
    return dist;
}

void floyd_warshall_openmp(std::vector<double>& dist, std::size_t n) {
    const long long n_ll = static_cast<long long>(n);
    for (std::size_t k = 0; k < n; ++k) {
        const double* row_k = &dist[k * n];
#pragma omp parallel for schedule(static)
        for (long long i_ll = 0; i_ll < n_ll; ++i_ll) {
            const std::size_t i = static_cast<std::size_t>(i_ll);
            double* row_i = &dist[i * n];
            const double via_k_prefix = row_i[k];
            if (!std::isfinite(via_k_prefix)) {
                continue;
            }

            for (std::size_t j = 0; j < n; ++j) {
                const double via_k = via_k_prefix + row_k[j];
                if (via_k < row_i[j]) {
                    row_i[j] = via_k;
                }
            }
        }
    }
}

void write_query_results(const std::string& path, const std::vector<Query>& queries,
                         const std::vector<double>& dist, std::size_t n) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to open output file: " + path);
    }

    output << "source,target,distance\n";
    output << std::setprecision(12);
    for (const Query& query : queries) {
        output << query.source << ',' << query.target << ',';
        if (query.source < 0 || query.target < 0 ||
            static_cast<std::size_t>(query.source) >= n ||
            static_cast<std::size_t>(query.target) >= n) {
            output << "INF\n";
            continue;
        }

        const double value = dist[static_cast<std::size_t>(query.source) * n +
                                  static_cast<std::size_t>(query.target)];
        if (std::isfinite(value)) {
            output << value << '\n';
        } else {
            output << "INF\n";
        }
    }
}

int parse_threads(int argc, char** argv) {
    int threads = 1;
#ifdef _OPENMP
    threads = omp_get_max_threads();
#endif
    if (argc >= 5) {
        int requested = 0;
        if (!parse_int(argv[4], requested) || requested <= 0) {
            throw std::runtime_error("threads must be a positive integer");
        }
        threads = requested;
    }
#ifdef _OPENMP
    omp_set_num_threads(threads);
#endif
    return threads;
}

void print_usage(const char* program) {
    std::cerr << "Usage: " << program
              << " <graph_csv> <query_csv> <output_csv> [threads]\n"
              << "  graph_csv: source,target,distance CSV adjacency list\n"
              << "  query_csv: source,target CSV vertex-pair test file\n"
              << "  output_csv: source,target,distance shortest-path results\n"
              << "  threads: OpenMP worker count, e.g. 1..16\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 2;
    }

    try {
        const std::string graph_path = argv[1];
        const std::string query_path = argv[2];
        const std::string output_path = argv[3];
        const int threads = parse_threads(argc, argv);

        int max_vertex = -1;
        const std::vector<Edge> edges = read_edges(graph_path, max_vertex);
        const std::vector<Query> queries = read_queries(query_path);
        std::vector<double> dist = build_distance_matrix(edges, max_vertex);
        const std::size_t n = static_cast<std::size_t>(max_vertex) + 1;

        const auto start = std::chrono::steady_clock::now();
        floyd_warshall_openmp(dist, n);
        const auto end = std::chrono::steady_clock::now();
        const double elapsed_seconds =
            std::chrono::duration<double>(end - start).count();

        write_query_results(output_path, queries, dist, n);

        std::cout << "nodes=" << n << '\n'
                  << "edges=" << edges.size() << '\n'
                  << "queries=" << queries.size() << '\n'
                  << "threads=" << threads << '\n'
                  << std::fixed << std::setprecision(6)
                  << "elapsed_seconds=" << elapsed_seconds << '\n';
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
