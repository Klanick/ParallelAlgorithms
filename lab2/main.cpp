#include <iostream>
#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

struct Graph {
    [[nodiscard]] virtual size_t get_size() const = 0;
    [[nodiscard]] virtual std::vector<size_t> next_list(size_t v) const = 0;
    [[nodiscard]] virtual size_t next_list_size(size_t v) const {
        return next_list(v).size();
    }
};

/*struct VectorGraph : Graph {
    std::vector<std::vector<size_t>> edges;
    explicit VectorGraph(const std::vector<std::vector<size_t>> & edges) {
        this->edges = edges;
    }

    [[nodiscard]] std::vector<size_t> next_list(size_t v) const override {
        return edges[v];
    }

    [[nodiscard]] size_t get_size() const override {
        return edges.size();
    }
};*/

struct CubeGraph : Graph {
    size_t a;
    explicit CubeGraph(size_t a) {
        this->a = a;
    }

    [[nodiscard]] size_t get_size() const override {
        return a*a*a;
    }

    [[nodiscard]] size_t get_v(size_t x, size_t y, size_t z) const {
        return z + (a * y) + (x * a * a);
    }

    [[nodiscard]] std::tuple<size_t, size_t, size_t> get_xyz(size_t v) const {
        return std::make_tuple((v / a) / a, (v / a) % a, v % a);
    }

    [[nodiscard]] std::vector<size_t> next_list(size_t v) const override {
        std::vector<size_t> res;
        size_t x, y, z;
        std::tie(x, y, z) = get_xyz(v);
        if (z + 1 < a) res.push_back(get_v(x, y, z + 1));
        if (y + 1 < a) res.push_back(get_v(x, y + 1, z));
        if (x + 1 < a) res.push_back(get_v(x + 1, y, z));
        return res;
    }

    [[nodiscard]] size_t next_list_size(size_t v) const override {
        return 3;
    }
};



std::vector<long long> seq_bfs(const Graph & graph /*const std::vector<std::vector<size_t>> & edges*/) {
    std::vector<long long> dist(graph.get_size());
    std::fill(dist.begin(), dist.end(), -1);
    std::vector<size_t> front(1);

    front[0] = 0;
    dist[0] = 0;

    while (!front.empty()) {
        std::vector<size_t> new_front;
        for (size_t v : front) {
            for (size_t u : graph.next_list(v)) {
                if (dist[u] == -1) {
                    dist[u] = dist[v] + 1;
                    new_front.push_back(u);
                }
            }
        }
        std::swap(front, new_front);
    }
    return dist;
}

std::vector<long long> par_bfs(const Graph & graph) {
    parlay::sequence<std::atomic<long long>> dist(graph.get_size());
    std::fill(dist.begin(), dist.end(), -1);
    parlay::sequence<long long> front(1);

    front[0] = 0;
    dist[0] = 0;

    while (!front.empty()) {
        auto front_edges_scan = parlay::scan(
                parlay::map(front,
                            [&](const size_t & v) { return graph.next_list_size(v); }));

        parlay::sequence<long long> new_front(front_edges_scan.second, -1);

        parlay::parallel_for(0, front.size(),[&] (size_t i) {
            size_t v = front[i];
            size_t j = 0;
            for (size_t u : graph.next_list(v)) {
                size_t new_front_id = front_edges_scan.first[i] + j;

                std::__atomic_val_t<long long> expected = -1;
                if (std::atomic_compare_exchange_strong(&dist[u], &expected, dist[v] + 1)) {
                    new_front[new_front_id] = (long long) u;
                }
                j++;
            }
        });

        front = //parlay::unique(
                parlay::filter(new_front,
                        [&](long long u) { return  u >= 0; });
                        //);
    }
    std::vector<long long> res(dist.begin(), dist.end());
    return res;
}

bool is_correct_cube_bfs(const CubeGraph & cubeGraph, const std::vector<long long> & dist) {
    for (size_t x = 0; x < cubeGraph.a; x++) {
        for (size_t y = 0; y < cubeGraph.a; y++) {
            for (size_t z = 0; z < cubeGraph.a; z++) {
                size_t v = cubeGraph.get_v(x, y, z);
                if (dist[v] != x + y + z) {
                    return false;
                }
            }
        }
    }
    return true;
}

int main() {
    ////// a = 500, m = 5;
    size_t a = 500, m = 5;

    /// initialize cube-graph
    CubeGraph cubeGraph(a);

    /*std::vector<std::vector<size_t>> edges(a*a*a);
    for (size_t x = 0; x < a; x++) {
        for (size_t y = 0; y < a; y++) {
            for (size_t z = 0; z < a; z++) {
                size_t v = cubeGraph.get_v(x, y, z);
                if (z + 1 < a) edges[v].push_back(cubeGraph.get_v(x, y, z + 1));
                if (y + 1 < a) edges[v].push_back(cubeGraph.get_v(x, y + 1, z));
                if (x + 1 < a) edges[v].push_back(cubeGraph.get_v(x + 1, y, z));
            }
        }
    }
    VectorGraph vectorGraph(edges);*/

    std::cout << "Tests is running:" << std::endl;

    long long par_summary = 0, seq_summary = 0;
    for (int i = 0; i < m; ++i) {
        /// parallel bfs
        auto start_par = std::chrono::steady_clock::now();
        auto dist_par = par_bfs(cubeGraph);
        auto end_par = std::chrono::steady_clock::now();

        long long par_micro_sec = std::chrono::duration_cast<std::chrono::microseconds>(end_par - start_par).count();
        std::cout << "ParTime(sec) #" << i + 1 << ": " << (long double) par_micro_sec / 1000000 << std::endl;
        par_summary += par_micro_sec;
        std::cout << "ParBFS is correct: " <<  is_correct_cube_bfs(cubeGraph, dist_par) << std::endl;

        /// sequential bfs
        auto start_seq = std::chrono::steady_clock::now();
        auto dist_seq = seq_bfs(cubeGraph);
        auto end_seq = std::chrono::steady_clock::now();

        long long seq_micro_sec = std::chrono::duration_cast<std::chrono::microseconds>(end_seq - start_seq).count();
        std::cout << "SeqTime(sec) #" << i + 1 << ": " << (long double) seq_micro_sec / 1000000 << std::endl;
        seq_summary += seq_micro_sec;

        std::cout << "SeqBFS is correct: " <<  is_correct_cube_bfs(cubeGraph, dist_seq) << std::endl;
    }

    std::cout << "___" << std::endl;
    std::cout << "TotalTime(sec) : " << (long double) (seq_summary + par_summary) / 1000000 << std::endl;
    std::cout << "Average K = " << (long double) seq_summary / par_summary << std::endl;
    return 0;
}