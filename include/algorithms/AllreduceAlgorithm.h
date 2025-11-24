#ifndef ALLREDUCE_ALGORITHM_H
#define ALLREDUCE_ALGORITHM_H

#include "../topology/NetworkTopology.h"
#include <vector>
#include <string>
#include <limits>

namespace swing {

class AllreduceAlgorithm {
public:
    struct Step {
        int step_number;
        std::vector<std::pair<int, int>> communications;  // (src, dst) pairs
        size_t data_size_bytes;
        std::string description;
    };

    struct Statistics {
        int num_steps = 0;
        size_t total_bytes_sent = 0;
        double estimated_time_us = 0.0;
        double goodput_gbps = 0.0;
        int max_congestion = 0;  // Max messages on any link
        std::vector<Step> steps;
    };

    virtual ~AllreduceAlgorithm() = default;

    // Main execution method
    virtual Statistics execute(
        NetworkTopology* topology,
        size_t vector_size_bytes) = 0;

    // Get algorithm name
    virtual std::string getName() const = 0;

    // Get algorithm type (for classification)
    enum class Type {
        LATENCY_OPTIMAL,
        BANDWIDTH_OPTIMAL,
        BANDWIDTH_OPTIMIZED
    };

    virtual Type getType() const = 0;

protected:
    // Helper to calculate modulo for negative numbers
    int mod(int a, int b) const {
        return ((a % b) + b) % b;
    }

    // Check if number is power of 2
    bool isPowerOfTwo(int n) const {
        return n > 0 && (n & (n - 1)) == 0;
    }

    // Calculate log base 2
    int log2(int n) const {
        int log = 0;
        while (n >>= 1) ++log;
        return log;
    }
};

} // namespace swing

#endif // ALLREDUCE_ALGORITHM_H
