#include "rrip.h"

#include <stdexcept>

namespace {
constexpr int MAX_RRPV = 7;
constexpr int MIN_RRPV = 0;
}

void update_rrpv(std::vector<int>& rrpv,
                 std::size_t way,
                 Classification cls,
                 bool is_hit)
{
    if (way >= rrpv.size()) {
        throw std::out_of_range("update_rrpv: way out of range");
    }

    /*
     * The assignment requires the Table 1 update behavior.
     *
     * Cache-friendly:
     *     RRPV = 0
     *
     * Cache-averse:
     *     RRPV = 7
     *
     * This applies when the line is accessed on a hit as well as
     * when its replacement/insertion state is established.
     */
    (void)is_hit;

    if (cls == Classification::CACHE_FRIENDLY) {
        rrpv[way] = MIN_RRPV;
    } else {
        rrpv[way] = MAX_RRPV;
    }
}

std::size_t find_victim(std::vector<int>& rrpv)
{
    if (rrpv.empty()) {
        throw std::invalid_argument(
            "find_victim: RRPV vector must not be empty");
    }

    /*
     * First look for an RRPV of exactly 7.
     */
    for (std::size_t way = 0; way < rrpv.size(); ++way) {
        if (rrpv[way] == MAX_RRPV) {
            return way;
        }
    }

    /*
     * Assignment clarification:
     *
     * If no line has RRPV == 7, age the set until one reaches 7.
     */
    while (true) {
        for (int& value : rrpv) {
            if (value < MAX_RRPV) {
                ++value;
            }
        }

        for (std::size_t way = 0; way < rrpv.size(); ++way) {
            if (rrpv[way] == MAX_RRPV) {
                return way;
            }
        }
    }
}