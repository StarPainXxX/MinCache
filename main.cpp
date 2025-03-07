#include "LruCache.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    MinCache::LruCache<int, std::string> cache(5);
    cache.put(1, "z");
    cache.put(2, "y");
    cache.put(3, "d");
    cache.put(4, "i");
    cache.put(5, "m");
    cache.get(1);
    cache.get(2);
    cache.get(3);
    cache.put(6, "a");
    cache.put(7, "b");

    std::string res1 = cache.get(4);
    if (res1.empty()) {
        std::cout << "res1 is empty" << std::endl;
    }
    std::string res2 = cache.get(5);
    if (res2.empty()) {
        std::cout << "res2 is empty" << std::endl;
    }
    std::string res3 = cache.get(1);
    std::cout << "res: " << res3 << std::endl;
    std::string res4 = cache.get(2);
    std::cout << "res: " << res4 << std::endl;
    std::string res5 = cache.get(3);
    std::cout << "res: " << res5 << std::endl;
    return 0;
}
