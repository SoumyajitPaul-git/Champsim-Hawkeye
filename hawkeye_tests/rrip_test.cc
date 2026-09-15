// rrip_test.cc
#include "../replacement/hawkeye/rrip.h"
#include <iostream>
#include <vector>
#include <cstddef>

int main()
{
    // --- Case 1: insertion policy ---
    std::vector<int> rrpv = {
        // left empty; the grading script substitutes initial per-way RRPV state
    };

    // SELF TEST Start

    rrpv = {3, 3, 3, 3};
// averse insert at way 0, friendly insert at way 1

    // SELF TEST End



    update_rrpv(rrpv, 0, Classification::CACHE_AVERSE, /*is_hit=*/false);
    update_rrpv(rrpv, 1, Classification::CACHE_FRIENDLY, /*is_hit=*/false);
    for (int v : rrpv)
        std::cout << v << " ";
    std::cout << "\n";
    std::cout << "victim: " << find_victim(rrpv) << "\n";

    // --- Case 2: find a victim ---
    std::vector<int> rrpv2 = {
        // left empty; the grading script substitutes initial per-way RRPV state
    };

     // SELF TEST Start

    rrpv2 = {2, 3, 1, 5};

    // SELF TEST End



    std::size_t v = find_victim(rrpv2);
    for (int x : rrpv2)
        std::cout << x << " ";
    std::cout << "\n";
    std::cout << "victim: " << v << "\n";
}