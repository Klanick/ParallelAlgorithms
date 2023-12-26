#include <iostream>
#include <parlay/parallel.h>
#include <parlay/sequence.h>
#include <random>

template<typename T>
T median(T a, T b, T c) {
    return std::max(
            std::max(
                    std::min(a, b),
                    std::min(b, c)),
            std::min(a, c));
}

auto dist_value(parlay::sequence<long long>& arr, size_t index) {
    return std::make_pair(arr[index], index);
}

/*long long dist_value(parlay::sequence<long long>& arr, size_t index) {
    return arr[index];
}*/

size_t partition(parlay::sequence<long long>& arr, size_t left, size_t right) {
    if (right - left <= 1) {
        return left;
    }
    size_t l = left, r = right - 1;
    auto value = //dist_value(arr, (l + r + 1) / 2);
    median(
            dist_value(arr, l),
            dist_value(arr, (l + r + 1) / 2),
            dist_value(arr, r)
    );

    while (true) {
        while (l < r && dist_value(arr, l) < value) {
            l++;
        }

        while (r > l && dist_value(arr, r) > value) {
            r--;
        }

        if (l >= r) {
            return l;
        }
        std::swap(arr[l], arr[r]);
    }
}

void seq_quick_sort(parlay::sequence<long long>& arr, size_t left, size_t right) {
    if (right - left <= 1) {
        //std::sort(arr.begin() + left, arr.begin() + right);
        return;
    }

    size_t m = partition(arr, left, right);
    seq_quick_sort(arr, left, m);
    seq_quick_sort(arr, m, right);
}

void seq_quick_sort(parlay::sequence<long long>& arr) {
    seq_quick_sort(arr, 0, arr.size());
}

const size_t BLOCK = 1000;

void par_quick_sort(parlay::sequence<long long>& arr, size_t left, size_t right) {
    if (right - left <= BLOCK) {
        seq_quick_sort(arr, left, right);
        return;
    }

    size_t m = partition(arr, left, right);
    parlay::par_do(
            [&]() { par_quick_sort(arr, left, m); },
            [&]() { par_quick_sort(arr, m, right); }
    );
}

void par_quick_sort(parlay::sequence<long long>& arr) {
    par_quick_sort(arr, 0, arr.size());
}

bool is_sort(parlay::sequence<long long>& arr) {
    long long prev = LLONG_MIN;
    for (long long x : arr) {
        if (x < prev) {
            return false;
        }
        prev = x;
    }
    return true;
}

int main() {
    ////// n = 100000000, m = 5;
    size_t n = 100000000, m = 5;
    std::mt19937_64 m_generator(time(nullptr));
    std::uniform_int_distribution<long long> range(LLONG_MIN, LLONG_MAX); //(0, n);

    std::cout << "Tests is running:" << std::endl;

    long long par_summary = 0, seq_summary = 0;
    for (int i = 0; i < m; ++i) {
        /// generate array
        parlay::sequence<long long> seq_arr(n);
        for (int j = 0; j < n; j++) {
            seq_arr[j] = range(m_generator);
        }
        parlay::sequence<long long> par_arr(seq_arr);


        /// parallel quick_sort
        auto start_par = std::chrono::steady_clock::now();
        par_quick_sort(par_arr);
        auto end_par = std::chrono::steady_clock::now();

        long long par_micro_sec = std::chrono::duration_cast<std::chrono::microseconds>(end_par - start_par).count();
        std::cout << "ParTime(sec) #" << i + 1 << ": " << (long double) par_micro_sec / 1000000 << std::endl;
        par_summary += par_micro_sec;

        std::cout << "ParQuickSort is correct: " <<  is_sort(par_arr) << std::endl;

        /// sequential quick_sort
        auto start_seq = std::chrono::steady_clock::now();
        seq_quick_sort(seq_arr);
        auto end_seq = std::chrono::steady_clock::now();

        long long seq_micro_sec = std::chrono::duration_cast<std::chrono::microseconds>(end_seq - start_seq).count();
        std::cout << "SeqTime(sec) #" << i + 1 << ": " << (long double) seq_micro_sec / 1000000 << std::endl;
        seq_summary += seq_micro_sec;

        std::cout << "SeqQuickSort is correct: " <<  is_sort(seq_arr) << std::endl;
    }

    std::cout << "___" << std::endl;
    std::cout << "TotalTime(sec) : " << (long double) (seq_summary + par_summary) / 1000000 << std::endl;
    std::cout << "Average K = " << (long double) seq_summary / par_summary << std::endl;
    return 0;
}