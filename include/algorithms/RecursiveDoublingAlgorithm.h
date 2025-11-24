#ifndef RECURSIVE_DOUBLING_ALGORITHM_H
#define RECURSIVE_DOUBLING_ALGORITHM_H

#include "AllreduceAlgorithm.h"

namespace swing {

class RecursiveDoublingAlgorithm : public AllreduceAlgorithm {
public:
    enum class Variant {
        LATENCY_OPTIMAL,
        BANDWIDTH_OPTIMAL
    };

    explicit RecursiveDoublingAlgorithm(Variant variant = Variant::BANDWIDTH_OPTIMAL)
        : variant_(variant) {}

    std::string getName() const override {
        return variant_ == Variant::LATENCY_OPTIMAL
            ? "Recursive Doubling (Latency-Optimal)"
            : "Recursive Doubling (Bandwidth-Optimal)";
    }

    Type getType() const override {
        return variant_ == Variant::LATENCY_OPTIMAL
            ? Type::LATENCY_OPTIMAL
            : Type::BANDWIDTH_OPTIMAL;
    }

    Statistics execute(NetworkTopology* topology, size_t vector_size_bytes) override {
        Statistics stats;
        int p = topology->getNumNodes();
        const auto& config = topology->getConfig();

        if (p < 2) return stats;

        int num_steps = isPowerOfTwo(p) ? log2(p) : (log2(p) + 1);

        if (variant_ == Variant::BANDWIDTH_OPTIMAL) {
            // Reduce-Scatter + Allgather
            stats.num_steps = 2 * num_steps;

            // Reduce-Scatter
            for (int s = 0; s < num_steps; ++s) {
                Step step;
                step.step_number = s;
                step.data_size_bytes = vector_size_bytes / (1 << (s + 1));
                step.description = "RD Reduce-Scatter step " + std::to_string(s);

                for (int r = 0; r < p; ++r) {
                    int peer = r ^ (1 << s);  // XOR for recursive doubling
                    if (peer < p) {
                        step.communications.push_back({r, peer});
                        stats.total_bytes_sent += step.data_size_bytes;
                    }
                }

                // Distance doubles each step
                int distance = 1 << s;
                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      distance * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }

            // Allgather (similar but reverse)
            for (int s = num_steps - 1; s >= 0; --s) {
                Step step;
                step.step_number = 2 * num_steps - 1 - s;
                step.data_size_bytes = vector_size_bytes / (1 << (s + 1));
                step.description = "RD Allgather step " + std::to_string(s);

                for (int r = 0; r < p; ++r) {
                    int peer = r ^ (1 << s);
                    if (peer < p) {
                        step.communications.push_back({r, peer});
                        stats.total_bytes_sent += step.data_size_bytes;
                    }
                }

                int distance = 1 << s;
                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      distance * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }
        } else {
            // Latency-Optimal: full vector exchange
            stats.num_steps = num_steps;

            for (int s = 0; s < num_steps; ++s) {
                Step step;
                step.step_number = s;
                step.data_size_bytes = vector_size_bytes;
                step.description = "RD Latency-Optimal step " + std::to_string(s);

                for (int r = 0; r < p; ++r) {
                    int peer = r ^ (1 << s);
                    if (peer < p) {
                        step.communications.push_back({r, peer});
                        stats.total_bytes_sent += step.data_size_bytes;
                    }
                }

                int distance = 1 << s;
                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      distance * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }
        }

        // Calculate goodput
        double time_s = stats.estimated_time_us / 1e6;
        double data_gb = (vector_size_bytes * 8.0) / 1e9;
        stats.goodput_gbps = data_gb / time_s;

        return stats;
    }

private:
    Variant variant_;
};

} // namespace swing

#endif // RECURSIVE_DOUBLING_ALGORITHM_H
