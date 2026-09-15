#include "predictor.h"

#include <stdexcept>

HawkeyePredictor::HawkeyePredictor(std::size_t num_entries,
                                   int counter_bits)
    : num_entries_(num_entries),
      counter_bits_(counter_bits),
      max_counter_(0),
      threshold_(0),
      counters_(num_entries, 0)
{
    if (num_entries_ == 0) {
        throw std::invalid_argument(
            "HawkeyePredictor: num_entries must be greater than 0");
    }

    if (counter_bits_ <= 0 || counter_bits_ > 30) {
        throw std::invalid_argument(
            "HawkeyePredictor: invalid counter_bits");
    }

    /*
     * The assignment allows us to assume num_entries is a non-zero
     * power of two.
     */

    max_counter_ = (1 << counter_bits_) - 1;

    /*
     * For a 3-bit counter:
     *
     *   0,1,2,3 -> CACHE_AVERSE
     *   4,5,6,7 -> CACHE_FRIENDLY
     *
     * An untrained PC starts at 4, so it defaults to friendly.
     */
    threshold_ = 1 << (counter_bits_ - 1);

    /*
     * Initialize every predictor entry to the midpoint.
     *
     * For the required 3-bit predictor this is 4.
     */
    std::fill(counters_.begin(), counters_.end(), threshold_);
}

std::size_t HawkeyePredictor::index(uint64_t pc) const
{
    /*
     * Required assignment hash:
     *
     *     pc ^ (pc >> 12)
     *
     * The predictor is indexed using the lower log2(num_entries)
     * bits of the hashed PC.
     */
    const uint64_t hashed_pc = pc ^ (pc >> 12);

    /*
     * Since num_entries is guaranteed to be a power of two,
     * num_entries - 1 is the appropriate low-bit mask.
     */
    const uint64_t mask =
        static_cast<uint64_t>(num_entries_ - 1);

    return static_cast<std::size_t>(hashed_pc & mask);
}

void HawkeyePredictor::train(uint64_t pc, bool opt_hit)
{
    const std::size_t idx = index(pc);

    if (opt_hit) {
        /*
         * OPT hit -> train toward cache-friendly.
         */
        if (counters_[idx] < max_counter_) {
            ++counters_[idx];
        }
    } else {
        /*
         * OPT miss -> train toward cache-averse.
         */
        if (counters_[idx] > 0) {
            --counters_[idx];
        }
    }
}

bool HawkeyePredictor::predict(uint64_t pc) const
{
    /*
     * The high-order bit determines the classification.
     */
    return counters_[index(pc)] >= threshold_;
}

int HawkeyePredictor::get_counter(uint64_t pc) const
{
    return counters_[index(pc)];
}