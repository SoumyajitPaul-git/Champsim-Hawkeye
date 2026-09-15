#ifndef HAWKEYE_PREDICTOR_H
#define HAWKEYE_PREDICTOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

class HawkeyePredictor {
public:
    // num_entries: size of the PC-indexed table (paper: 8K entries)
    // counter_bits: width of the saturating counter (paper: 3 bits,
    // range [0, 2^counter_bits - 1])
    HawkeyePredictor(std::size_t num_entries = 8192,
                     int counter_bits = 3);

    // Trains the counter indexed by a hash of `pc`.
    void train(uint64_t pc, bool opt_hit);

    // Returns the predicted classification for `pc`.
    bool predict(uint64_t pc) const;

    // Debug-only accessor: raw counter value in
    // [0, 2^counter_bits - 1].
    int get_counter(uint64_t pc) const;

private:
    std::size_t num_entries_;
    int counter_bits_;
    int max_counter_;
    int threshold_;

    std::vector<int> counters_;

    std::size_t index(uint64_t pc) const;
};

#endif