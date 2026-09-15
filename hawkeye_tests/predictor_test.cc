// predictor_test.cc
#include "../replacement/hawkeye/predictor.h"
#include <iostream>
#include <vector>
#include <utility>
#include <cstdint>

int main() {
    HawkeyePredictor pred;

    // TEST_VECTOR_START
    // train_events: ordered (pc, opt_hit) pairs applied via pred.train(pc, opt_hit)
    std::vector<std::pair<uint64_t, bool>> train_events = {
    };

    // query_pcs: pcs to print counter/prediction for, after all training is applied
    std::vector<uint64_t> query_pcs = {
    };
    // TEST_VECTOR_END


    // SELF TEST Start

    // train_events = {{0x400,true},{0x400,true},{0x400,true},{0x400,true},{0x400,true}};
    // query_pcs = {0x400};
    // train_events = {{0x800,false},{0x800,false},{0x800,false},{0x800,false},{0x800,false}};
    // query_pcs = {0x800};
    // train_events = {};
    // query_pcs = {0xC00};
    // train_events = {{0x1000,false}};
    // query_pcs = {0x1000};
    train_events = {{0x111,true},{0x222,false},{0x111,true},{0x222,false},{0x111,true}};
    query_pcs = {0x111, 0x222, 0x333};

    // SELF TEST End



    for (auto& [pc, opt_hit] : train_events) {
        pred.train(pc, opt_hit);
    }

    for (uint64_t pc : query_pcs) {
        std::cout << std::hex << pc << std::dec
                   << ": counter=" << pred.get_counter(pc)
                   << " predict=" << pred.predict(pc) << "\n";
    }
}