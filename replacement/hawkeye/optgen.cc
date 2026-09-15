#include "optgen.h"

#include <algorithm>
#include <stdexcept>

OPTgen::OPTgen(std::size_t num_sets,
               std::size_t associativity,
               std::size_t history_multiplier)
    : num_sets_(num_sets),
      associativity_(associativity),
      history_multiplier_(history_multiplier),
      history_length_(associativity * history_multiplier),
      sets_(num_sets)
{
    if (num_sets_ == 0) {
        throw std::invalid_argument("OPTgen: num_sets must be greater than 0");
    }

    if (associativity_ == 0) {
        throw std::invalid_argument(
            "OPTgen: associativity must be greater than 0");
    }

    if (history_multiplier_ == 0) {
        throw std::invalid_argument(
            "OPTgen: history_multiplier must be greater than 0");
    }

    for (auto& state : sets_) {
        state.occupancy.assign(history_length_, 0);
        state.timestamp = 0;
    }
}

int& OPTgen::occupancy_at(SetState& state, std::size_t timestamp)
{
    return state.occupancy[timestamp % history_length_];
}

const int& OPTgen::occupancy_at(const SetState& state,
                                std::size_t timestamp) const
{
    return state.occupancy[timestamp % history_length_];
}

void OPTgen::advance(SetState& state)
{
    ++state.timestamp;

    // The current timestamp corresponds to the newest entry.
    occupancy_at(state, state.timestamp) = 0;
}

bool OPTgen::access(std::size_t set_idx, uint64_t address)
{
    if (set_idx >= num_sets_) {
        throw std::out_of_range("OPTgen: set index out of range");
    }

    SetState& state = sets_[set_idx];

    /*
     * The current access occupies the newest position in the occupancy
     * vector. Start that position at zero, as specified by Section 3.1.
     */
    advance(state);

    const std::size_t current = state.timestamp;

    auto previous = state.last_access.find(address);

    /*
     * First-time references do not modify the occupancy vector.
     * OPTgen cannot determine their OPT decision until their next reuse.
     */
    if (previous == state.last_access.end()) {
        state.last_access[address] = current;
        return false;
    }

    const std::size_t previous_timestamp = previous->second;

    /*
     * If the reuse interval is larger than the retained history,
     * the complete interval cannot be represented. Treat it as
     * cache-averse/miss rather than using incomplete information.
     */
    if (current - previous_timestamp > history_length_) {
        state.last_access[address] = current;
        return false;
    }

    /*
     * Usage interval:
     *
     *     previous access ... current access
     *
     * The current access itself is excluded.
     *
     * Every occupancy entry in this interval must be below W for
     * OPT to retain the line.
     */
    bool can_cache = true;

    for (std::size_t t = previous_timestamp; t < current; ++t) {
        if (occupancy_at(state, t) >=
            static_cast<int>(associativity_)) {
            can_cache = false;
            break;
        }
    }

    if (can_cache) {
        /*
         * OPT would have kept this line.
         * Increment every occupancy entry belonging to its liveness
         * interval.
         */
        for (std::size_t t = previous_timestamp; t < current; ++t) {
            ++occupancy_at(state, t);
        }
    }

    /*
     * The decision for this reference is now finalized because its
     * next reuse has occurred.
     */
    state.last_access[address] = current;

    return can_cache;
}