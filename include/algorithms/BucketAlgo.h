#ifndef BUCKET_ALGORITHM_H
#define BUCKET_ALGORITHM_H

#include "AllreduceAlgorithm.h"
#include <cmath>

namespace swing {

class BucketAlgorithm : public AllreduceAlgorithm {
public:
    std::string getName() const override {
        return "Bucket Algorithm (Torus)";
    }

    Type getType() const override {
        return Type::BANDWIDTH_OPTIMAL;
    }

    Statistics execute(NetworkTopology* topology, size_t vector_size_bytes) override {
        Statistics stats;
        int p = topology->getNumNodes();
        const auto& config = topology->getConfig();
        int dims = config.getNumDimensions();
        
        if (p < 2) return stats;

        // Current size of data held by each node. Starts full.
        // As we Reduce-Scatter through dimensions, this shrinks.
        double current_data_size = (double)vector_size_bytes;

        // --- Phase 1: Dimensional Reduce-Scatter ---
        // We iterate through dimensions 0 -> D-1
        for (int d = 0; d < dims; ++d) {
            int dim_size = config.dimensions[d];
            
            // If dimension size is 1, no communication needed for this dim
            if (dim_size <= 1) continue;

            // In Bucket algorithm, we split data into 'dim_size' chunks
            // and perform a ring algorithm along this dimension.
            current_data_size /= dim_size;
            size_t step_bytes = (size_t)std::max(1.0, current_data_size);

            // A ring on dimension D of size K takes K-1 steps
            for (int step = 0; step < dim_size - 1; ++step) {
                Step s;
                s.step_number = stats.num_steps++;
                s.data_size_bytes = step_bytes;
                s.description = "Bucket RS Dim " + std::to_string(d) + " Step " + std::to_string(step);

                // Every node sends to its positive neighbor in dimension 'd'
                for (int r = 0; r < p; ++r) {
                    auto node = topology->getNode(r);
                    // Get neighbor in dimension 'd', positive direction (+1)
                    int dest = node->getNeighbor(d, true);
                    
                    s.communications.push_back({r, dest});
                    stats.total_bytes_sent += step_bytes;
                }

                // Cost Model:
                // Bucket only talks to immediate neighbors, so distance is ALWAYS 1.
                // Congestion is low because links align with dimensions.
                double link_bw_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_ns = step_bytes / link_bw_ns;
                double step_time_ns = config.link_latency_ns + transfer_ns + (1 * config.hop_latency_ns);

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(s);
            }
        }

        // --- Phase 2: Dimensional Allgather ---
        // We iterate dimensions in REVERSE: D-1 -> 0
        for (int d = dims - 1; d >= 0; --d) {
            int dim_size = config.dimensions[d];
            if (dim_size <= 1) continue;

            // Data size grows as we Allgather
            // Current size is what we send. After full AG on this dim, we have size * dim_size
            size_t step_bytes = (size_t)std::max(1.0, current_data_size);

            for (int step = 0; step < dim_size - 1; ++step) {
                Step s;
                s.step_number = stats.num_steps++;
                s.data_size_bytes = step_bytes;
                s.description = "Bucket AG Dim " + std::to_string(d) + " Step " + std::to_string(step);

                for (int r = 0; r < p; ++r) {
                    auto node = topology->getNode(r);
                    // Standard Bucket uses same ring direction (or reverse, doesn't matter for cost)
                    int dest = node->getNeighbor(d, true);
                    
                    s.communications.push_back({r, dest});
                    stats.total_bytes_sent += step_bytes;
                }

                double link_bw_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_ns = step_bytes / link_bw_ns;
                double step_time_ns = config.link_latency_ns + transfer_ns + (1 * config.hop_latency_ns);

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(s);
            }
            
            // Update data size for next dimension
            current_data_size *= dim_size;
        }

        // Goodput Calculation
        double time_s = stats.estimated_time_us / 1e6;
        double data_gb = (vector_size_bytes * 8.0) / 1e9;
        stats.goodput_gbps = (time_s > 0) ? (data_gb / time_s) : 0.0;

        return stats;
    }
};

} // namespace swing

#endif // BUCKET_ALGORITHM_H
