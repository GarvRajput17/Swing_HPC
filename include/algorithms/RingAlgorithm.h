#ifndef RING_ALGORITHM_H
#define RING_ALGORITHM_H

#include "AllreduceAlgorithm.h"
#include <cmath>

namespace swing {

class RingAlgorithm : public AllreduceAlgorithm {
public:
    std::string getName() const override {
        return "Ring Algorithm";
    }

    Type getType() const override {
        return Type::BANDWIDTH_OPTIMAL;
    }

    Statistics execute(NetworkTopology* topology, size_t vector_size_bytes) override {
        Statistics stats;
        int p = topology->getNumNodes();
        const auto& config = topology->getConfig();
        
        if (p < 2) return stats;

        // Ring splits data into 'p' chunks
        size_t chunk_size = vector_size_bytes / p;
        if (chunk_size == 0) chunk_size = 1; // Prevent zero-size messages

        // --- Phase 1: Reduce-Scatter (p-1 steps) ---
        for (int step = 0; step < p - 1; ++step) {
            Step s;
            s.step_number = step;
            s.data_size_bytes = chunk_size;
            s.description = "Ring Reduce-Scatter Step " + std::to_string(step);

            double max_step_latency = 0;

            for (int r = 0; r < p; ++r) {
                // Logical Ring: r sends to (r+1)%p
                int dest = (r + 1) % p;
                
                s.communications.push_back({r, dest});
                stats.total_bytes_sent += chunk_size;

                // Calculate Physical Distance Cost
                // We need coordinates to calc distance. 
                // Since we don't have the Generator here, we rely on the fact 
                // that we can calculate distance via coordinates stored in Node.
                auto node_src = topology->getNode(r);
                auto node_dst = topology->getNode(dest);
                
                int distance = 0;
                const auto& c1 = node_src->getCoordinates();
                const auto& c2 = node_dst->getCoordinates();
                
                // Calculate Manhattan distance on Torus manually
                for(size_t d=0; d<c1.size(); ++d) {
                    int dim_size = config.dimensions[d];
                    int diff = std::abs(c1[d] - c2[d]);
                    int wrap = dim_size - diff;
                    distance += std::min(diff, wrap);
                }

                // Latency Model: Alpha + Beta*n + Hops*HopLatency
                double link_bw_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_ns = chunk_size / link_bw_ns;
                double latency = config.link_latency_ns + transfer_ns + (distance * config.hop_latency_ns);
                
                if (latency > max_step_latency) max_step_latency = latency;
            }
            stats.estimated_time_us += max_step_latency / 1000.0;
            stats.steps.push_back(s);
        }

        // --- Phase 2: Allgather (p-1 steps) ---
        // Exactly the same logic, just different logical data chunks being moved
        for (int step = 0; step < p - 1; ++step) {
            Step s;
            s.step_number = (p - 1) + step;
            s.data_size_bytes = chunk_size;
            s.description = "Ring Allgather Step " + std::to_string(step);

            double max_step_latency = 0;

            for (int r = 0; r < p; ++r) {
                int dest = (r + 1) % p;
                s.communications.push_back({r, dest});
                stats.total_bytes_sent += chunk_size;

                // Recalculate distance (same as above)
                auto node_src = topology->getNode(r);
                auto node_dst = topology->getNode(dest);
                int distance = 0;
                const auto& c1 = node_src->getCoordinates();
                const auto& c2 = node_dst->getCoordinates();
                for(size_t d=0; d<c1.size(); ++d) {
                    int dim_size = config.dimensions[d];
                    int diff = std::abs(c1[d] - c2[d]);
                    int wrap = dim_size - diff;
                    distance += std::min(diff, wrap);
                }

                double link_bw_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_ns = chunk_size / link_bw_ns;
                double latency = config.link_latency_ns + transfer_ns + (distance * config.hop_latency_ns);
                
                if (latency > max_step_latency) max_step_latency = latency;
            }
            stats.estimated_time_us += max_step_latency / 1000.0;
            stats.steps.push_back(s);
        }

        stats.num_steps = 2 * (p - 1);
        
        // Goodput Calculation
        double time_s = stats.estimated_time_us / 1e6;
        double data_gb = (vector_size_bytes * 8.0) / 1e9;
        stats.goodput_gbps = (time_s > 0) ? (data_gb / time_s) : 0.0;

        return stats;
    }
};

} // namespace swing

#endif // RING_ALGORITHM_H
