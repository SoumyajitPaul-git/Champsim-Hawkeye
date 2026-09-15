#ifndef HAWKEYE_OPTGEN_H
#define HAWKEYE_OPTGEN_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class OPTgen {
public:
    // num_sets: number of cache sets tracked independently
    // associativity: W, the cache associativity (occupancy vector cap)
    // history_multiplier: length of tracked history, in units of the set's
    // capacity (paper uses 8x; see Figure 2).
    OPTgen(std::size_t num_sets,
           std::size_t associativity,
           std::size_t history_multiplier = 8);

    // Processes one access to `address`, mapped to set `set_idx`, per
    // Section 3.1.
    bool access(std::size_t set_idx, uint64_t address);

private:
    struct SetState {
        std::vector<int> occupancy;
        std::unordered_map<uint64_t, std::size_t> last_access;
        std::size_t timestamp = 0;
    };

    std::size_t num_sets_;
    std::size_t associativity_;
    std::size_t history_multiplier_;
    std::size_t history_length_;

    std::vector<SetState> sets_;

    int& occupancy_at(SetState& state, std::size_t timestamp);
    const int& occupancy_at(const SetState& state,
                            std::size_t timestamp) const;

    void advance(SetState& state);
};

#endif