#ifndef SWING_ALGORITHM_H
#define SWING_ALGORITHM_H

#include "AllreduceAlgorithm.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace swing
{

    class SwingAlgorithm : public AllreduceAlgorithm
    {
    public:
        enum class Variant
        {
            LATENCY_OPTIMAL,  // Algorithm from Section 3.1.2
            BANDWIDTH_OPTIMAL // Algorithm from Section 3.1.1
        };

        explicit SwingAlgorithm(Variant variant = Variant::BANDWIDTH_OPTIMAL)
            : variant_(variant) {}

        std::string getName() const override
        {
            return variant_ == Variant::LATENCY_OPTIMAL
                       ? "Swing (Latency-Optimal)"
                       : "Swing (Bandwidth-Optimal)";
        }

        Type getType() const override
        {
            return variant_ == Variant::LATENCY_OPTIMAL
                       ? Type::LATENCY_OPTIMAL
                       : Type::BANDWIDTH_OPTIMAL;
        }

        Statistics execute(NetworkTopology *topology, size_t vector_size_bytes) override
        {
            Statistics stats;
            int p = topology->getNumNodes();
            const auto &config = topology->getConfig();
            int D = config.getNumDimensions();

            if (variant_ == Variant::BANDWIDTH_OPTIMAL)
            {
                stats = executeBandwidthOptimal(topology, vector_size_bytes);
            }
            else
            {
                stats = executeLatencyOptimal(topology, vector_size_bytes);
            }

            return stats;
        }

    private:
        Variant variant_;

        // ρ(s) = Σ(i=0 to s) -2^i = (1-(-2)^(s+1))/3
        int rho(int s) const
        {
            int result = 0;
            for (int i = 0; i <= s; ++i)
            {
                result += (i % 2 == 0) ? (1 << i) : -(1 << i);
            }
            return result;
        }

        // δ(s) = |ρ(s)| = (2^(s+1) - (-1)^(s+1)) / 3
        int delta(int s) const
        {
            return std::abs(rho(s));
        }

        // π(r,s) - compute communication peer at step s for rank r
        int pi(int r, int s, int p) const
        {
            int rho_s = rho(s);
            if (r % 2 == 0)
            {
                return mod(r + rho_s, p);
            }
            else
            {
                return mod(r - rho_s, p);
            }
        }

        // Bandwidth-Optimal Swing (Reduce-Scatter + Allgather)
        Statistics executeBandwidthOptimal(NetworkTopology *topology, size_t vector_size_bytes)
        {
            Statistics stats;
            int p = topology->getNumNodes();
            const auto &config = topology->getConfig();
            int D = config.getNumDimensions();

            if (p < 2)
                return stats;

            int num_steps = isPowerOfTwo(p) ? log2(p) : (log2(p) + 1);

            // Reduce-Scatter phase
            auto rs_stats = executeReduceScatter(topology, vector_size_bytes, num_steps);
            stats.num_steps += rs_stats.num_steps;
            stats.total_bytes_sent += rs_stats.total_bytes_sent;
            stats.estimated_time_us += rs_stats.estimated_time_us;
            stats.steps.insert(stats.steps.end(), rs_stats.steps.begin(), rs_stats.steps.end());

            // Allgather phase (reverse communication pattern)
            auto ag_stats = executeAllgather(topology, vector_size_bytes, num_steps);
            stats.num_steps += ag_stats.num_steps;
            stats.total_bytes_sent += ag_stats.total_bytes_sent;
            stats.estimated_time_us += ag_stats.estimated_time_us;
            stats.steps.insert(stats.steps.end(), ag_stats.steps.begin(), ag_stats.steps.end());

            // Calculate goodput
            double time_s = stats.estimated_time_us / 1e6;
            double data_gb = (vector_size_bytes * 8.0) / 1e9;
            stats.goodput_gbps = data_gb / time_s;

            return stats;
        }

        Statistics executeReduceScatter(NetworkTopology *topology, size_t vector_size_bytes, int num_steps)
        {
            Statistics stats;
            int p = topology->getNumNodes();
            const auto &config = topology->getConfig();
            int D = config.getNumDimensions();

            for (int s = 0; s < num_steps; ++s)
            {
                Step step;
                step.step_number = s;
                step.data_size_bytes = vector_size_bytes / (1 << (s + 1)); // Halves each step
                step.description = "Reduce-Scatter step " + std::to_string(s);

                // Determine dimension to communicate on
                int dim = s % D;
                int sigma_s = s / D; // Step within dimension

                for (int r = 0; r < p; ++r)
                {
                    int peer = pi(r, s, p);
                    step.communications.push_back({r, peer});
                    stats.total_bytes_sent += step.data_size_bytes;
                }

                // Estimate time for this step
                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      delta(sigma_s) * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }

            stats.num_steps = num_steps;
            return stats;
        }

        Statistics executeAllgather(NetworkTopology *topology, size_t vector_size_bytes, int num_steps)
        {
            Statistics stats;
            int p = topology->getNumNodes();
            const auto &config = topology->getConfig();
            int D = config.getNumDimensions();

            // Allgather: reverse order of reduce-scatter
            for (int s = num_steps - 1; s >= 0; --s)
            {
                Step step;
                step.step_number = num_steps + (num_steps - 1 - s);
                step.data_size_bytes = vector_size_bytes / (1 << (s + 1));
                step.description = "Allgather step " + std::to_string(s);

                int dim = s % D;
                int sigma_s = s / D;

                for (int r = 0; r < p; ++r)
                {
                    int peer = pi(r, s, p);
                    step.communications.push_back({r, peer});
                    stats.total_bytes_sent += step.data_size_bytes;
                }

                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      delta(sigma_s) * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }

            stats.num_steps = num_steps;
            return stats;
        }

        // Latency-Optimal Swing (Full vector exchange)
        Statistics executeLatencyOptimal(NetworkTopology *topology, size_t vector_size_bytes)
        {
            Statistics stats;
            int p = topology->getNumNodes();
            const auto &config = topology->getConfig();
            int D = config.getNumDimensions();

            if (p < 2)
                return stats;

            int num_steps = isPowerOfTwo(p) ? log2(p) : (log2(p) + 1);
            stats.num_steps = num_steps;

            for (int s = 0; s < num_steps; ++s)
            {
                Step step;
                step.step_number = s;
                step.data_size_bytes = vector_size_bytes; // Full vector each step
                step.description = "Latency-Optimal step " + std::to_string(s);

                int dim = s % D;
                int sigma_s = s / D;

                for (int r = 0; r < p; ++r)
                {
                    int peer = pi(r, s, p);
                    step.communications.push_back({r, peer});
                    stats.total_bytes_sent += step.data_size_bytes;
                }

                double link_bandwidth_bytes_per_ns = (config.link_bandwidth_gbps * 1e9 / 8.0) / 1e9;
                double transfer_time_ns = step.data_size_bytes / link_bandwidth_bytes_per_ns;
                double step_time_ns = config.link_latency_ns + transfer_time_ns +
                                      delta(sigma_s) * config.hop_latency_ns;

                stats.estimated_time_us += step_time_ns / 1000.0;
                stats.steps.push_back(step);
            }

            // Calculate goodput
            double time_s = stats.estimated_time_us / 1e6;
            double data_gb = (vector_size_bytes * 8.0) / 1e9;
            stats.goodput_gbps = data_gb / time_s;

            return stats;
        }
    };

} // namespace swing

#endif // SWING_ALGORITHM_H
