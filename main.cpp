#include "CachePolicy.h"
#include "LruCache.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

class Timer {
public:
    Timer() : _start(std::chrono::high_resolution_clock::now()) {}

    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                     _start)
            .count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};

void print_results(const std::string &testname, int capacity,
                   const std::vector<int> &get_operations,
                   const std::vector<int> &hits) {
    std::cout << "Cache capacity: " << capacity << std::endl;
    std::cout << "LRU - Hit rate:  " << std::fixed << std::setprecision(2)
              << (100.0 * hits[0] / get_operations[0]) << "%" << std::endl;
    std::cout << "LRU-K - Hit rate:  " << std::fixed << std::setprecision(2)
              << (100.0 * hits[1] / get_operations[1]) << "%" << std::endl;
    // std::cout << "ARC - Hit rate:  " << std::fixed << std::setprecision(2)
    //           << (100.0 * hits[2] / get_operations[2]) << "%" << std::endl;
}

void test_hotdata_acess() {
    std::cout << "\n=== Test1:Test hot data access ===" << std::endl;

    const int CAPACITY = 50;
    const int OPERATIONS = 500000; // Number of get operations
    const int HOT_KEYS = 20;       // Number of hot keys
    const int COLD_KEYS = 5000;

    MinCache::LruCache<int, std::string> lru(CAPACITY);
    MinCache::LruKCache<int, std::string> lruk(CAPACITY, 2 * CAPACITY, 2);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::array<MinCache::CachePolicy<int, std::string> *, 2> caches = {&lru,
                                                                       &lruk};
    std::vector<int> hits(2, 0);
    std::vector<int> get_operations(2, 0);

    for (int i = 0; i < caches.size(); i++) {
        for (int op = 0; op < OPERATIONS; op++) {
            int key;
            if (op % 100 < 70) {
                key = gen() % HOT_KEYS;
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }

        for (int get_op = 0; get_op < OPERATIONS; get_op++) {
            int key;
            if (get_op % 100 < 70) {
                key = gen() % HOT_KEYS;
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }
    print_results("Hot key access data", CAPACITY, get_operations, hits);
}

void test_loop_pattern() {
    std::cout << "\n=== Test2: Test loop pattern ===" << std::endl;

    const int CAPACITY = 50;
    const int OPERATIONS = 500000; // Number of get operations
    const int LOOP_SIZE = 500;

    MinCache::LruCache<int, std::string> lru(CAPACITY);
    MinCache::LruKCache<int, std::string> lruk(CAPACITY, 2 * CAPACITY, 2);

    std::array<MinCache::CachePolicy<int, std::string> *, 2> caches = {&lru,
                                                                       &lruk};
    std::vector<int> hits(2, 0);
    std::vector<int> get_operations(2, 0);

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < LOOP_SIZE; ++key) {
            std::string value = "loop" + std::to_string(key);
            caches[i]->put(key, value);
        }

        int current_pos = 0;
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 60) {
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {
                key = gen() % LOOP_SIZE;
            } else {
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }

            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }
    print_results("Hot key access data", CAPACITY, get_operations, hits);
}

int main(int argc, char *argv[]) {
    test_hotdata_acess();
    test_loop_pattern();
    return 0;
}
