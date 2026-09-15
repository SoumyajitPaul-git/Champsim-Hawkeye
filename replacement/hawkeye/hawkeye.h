

#ifndef HAWKEYE_H
#define HAWKEYE_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "modules.h"

#include "optgen.h"
#include "predictor.h"
#include "rrip.h"

struct hawkeye : public champsim::modules::replacement {
    std::size_t num_sets;
    std::size_t num_ways;

    OPTgen optgen;
    HawkeyePredictor predictor;

    std::vector<std::vector<int>> rrpv;

    struct HistoryEntry {
        uint64_t pc;
        std::size_t counter;
    };

    // For each set:
    //
    // address -> {most recent PC, counter position}
    std::vector<std::unordered_map<uint64_t, HistoryEntry>> history;

    // For each set:
    //
    // counter position -> address
    std::vector<std::unordered_map<std::size_t, uint64_t>> counter_map;

    // Current counter position for each set.
    std::vector<std::size_t> current_counter;

    explicit hawkeye(CACHE* cache);
    hawkeye(CACHE* cache, long sets, long ways);

    long find_victim(
        uint32_t triggering_cpu,
        uint64_t instr_id,
        long set,
        const champsim::cache_block* current_set,
        champsim::address ip,
        champsim::address full_addr,
        access_type type
    );

    void replacement_cache_fill(
        uint32_t triggering_cpu,
        long set,
        long way,
        champsim::address full_addr,
        champsim::address ip,
        champsim::address victim_addr,
        access_type type
    );

    void update_replacement_state(
        uint32_t triggering_cpu,
        long set,
        long way,
        champsim::address full_addr,
        champsim::address ip,
        champsim::address victim_addr,
        access_type type,
        uint8_t hit
    );

private:
    void process_access(
        std::size_t set_idx,
        uint64_t address,
        uint64_t pc
    );
};

#endif
