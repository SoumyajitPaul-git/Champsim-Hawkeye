

#include "hawkeye.h"

#include "cache.h"
#include <cstddef>
#include <cstdint>

hawkeye::hawkeye(CACHE* cache)
    : hawkeye(cache, cache->NUM_SET, cache->NUM_WAY)
{
}

hawkeye::hawkeye(CACHE* cache, long sets, long ways)
    : replacement(cache),
      num_sets(static_cast<std::size_t>(sets)),
      num_ways(static_cast<std::size_t>(ways)),
      optgen(num_sets, num_ways),
      predictor(),
      rrpv(num_sets, std::vector<int>(num_ways, 0)),
      history(num_sets),
      counter_map(num_sets),
      current_counter(num_sets, 0)
{
    const std::size_t history_window = num_ways * 8;

    for (std::size_t set_idx = 0; set_idx < num_sets; ++set_idx) {
        history.at(set_idx).reserve(history_window);
        counter_map.at(set_idx).reserve(history_window);
    }
}

void hawkeye::process_access(
    std::size_t set_idx,
    uint64_t address,
    uint64_t pc)
{
    auto& address_map = history.at(set_idx);
    auto& counters = counter_map.at(set_idx);
    auto& counter = current_counter.at(set_idx);

    const std::size_t history_window = num_ways * 8;

    /*
     * Move to the next counter position FIRST.
     *
     * The counter cycles through:
     *
     * 0, 1, 2, ..., history_window - 1, 0, 1, ...
     */
    counter = (counter + 1) % history_window;

    /*
     * Find the previous occurrence of the current address
     * before changing any history mappings.
     */
    auto previous_it = address_map.find(address);

    /*
     * OPTgen processes the current access.
     */
    const bool opt_hit =
        optgen.access(set_idx, address);

    /*
     * If the current address appeared previously, train
     * the PC associated with that previous occurrence.
     */
    if (previous_it != address_map.end()) {
        predictor.train(
            previous_it->second.pc,
            opt_hit
        );
    }

    /*
     * The new counter position is now the slot that is
     * going to be overwritten.
     */
    auto old_slot_it = counters.find(counter);

    if (old_slot_it != counters.end()) {
        const uint64_t old_address = old_slot_it->second;

        /*
         * Check whether this counter position is still the
         * most recent occurrence recorded for old_address.
         *
         * If it is, remove old_address from the address map.
         *
         * If it is not, then the same address appeared again
         * at a newer counter position, so we must keep it.
         */
        auto old_address_it = address_map.find(old_address);

        if (old_address_it != address_map.end() &&
            old_address_it->second.counter == counter) {
            address_map.erase(old_address_it);
        }

        /*
         * Remove the old counter -> address mapping.
         */
        counters.erase(old_slot_it);
    }

    /*
     * Insert the current address into the newly selected
     * counter position.
     */
    counters[counter] = address;

    /*
     * Store the current PC and counter position for this address.
     */
    address_map[address] = {
        pc,
        counter
    };
}

long hawkeye::find_victim(
    uint32_t triggering_cpu,
    uint64_t instr_id,
    long set,
    const champsim::cache_block* current_set,
    champsim::address ip,
    champsim::address full_addr,
    access_type type)
{
    (void)triggering_cpu;
    (void)instr_id;
    (void)current_set;
    (void)ip;
    (void)full_addr;
    (void)type;

    const std::size_t set_idx =
        static_cast<std::size_t>(set);

    const std::size_t victim =
        ::find_victim(rrpv.at(set_idx));

    return static_cast<long>(victim);
}

void hawkeye::replacement_cache_fill(
    uint32_t triggering_cpu,
    long set,
    long way,
    champsim::address full_addr,
    champsim::address ip,
    champsim::address victim_addr,
    access_type type)
{
    (void)triggering_cpu;
    (void)full_addr;
    (void)victim_addr;
    (void)type;

    if (way < 0 ||
        way >= static_cast<long>(num_ways)) {
        return;
    }

    const std::size_t set_idx =
        static_cast<std::size_t>(set);

    const std::size_t way_idx =
        static_cast<std::size_t>(way);

    /*
     * Predict whether the new line is cache-friendly
     * or cache-averse.
     */
    const bool prediction =
        predictor.predict(ip.to<uint64_t>());

    const Classification classification =
        prediction
            ? Classification::CACHE_FRIENDLY
            : Classification::CACHE_AVERSE;

    /*
     * New line insertion -> is_hit = false.
     */
    update_rrpv(
        rrpv.at(set_idx),
        way_idx,
        classification,
        false
    );
}

void hawkeye::update_replacement_state(
    uint32_t triggering_cpu,
    long set,
    long way,
    champsim::address full_addr,
    champsim::address ip,
    champsim::address victim_addr,
    access_type type,
    uint8_t hit)
{
    (void)triggering_cpu;
    (void)victim_addr;
    (void)type;

    const std::size_t set_idx =
        static_cast<std::size_t>(set);

    /*
     * Convert byte address into cache-line address.
     */
    const uint64_t address =
        full_addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;

    /*
     * Get the PC of the instruction causing this access.
     */
    const uint64_t pc =
        ip.to<uint64_t>();

    /*
     * Process EVERY LLC access, including misses.
     */
    process_access(
        set_idx,
        address,
        pc
    );

    /*
     * On a miss, ChampSim does not give us a valid
     * cache way here.
     */
    if (way < 0 ||
        way >= static_cast<long>(num_ways)) {
        return;
    }

    const std::size_t way_idx =
        static_cast<std::size_t>(way);

    /*
     * Only update the accessed line's RRPV on a hit.
     */
    if (hit != 0) {

        const bool prediction =
            predictor.predict(pc);

        const Classification classification =
            prediction
                ? Classification::CACHE_FRIENDLY
                : Classification::CACHE_AVERSE;

        update_rrpv(
            rrpv.at(set_idx),
            way_idx,
            classification,
            true
        );
    }
}