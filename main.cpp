#include "include/topology/TorusGenerator.h"
#include "include/topology/NetworkTopology.h"
#include "include/algorithms/SwingAlgorithm.h"
#include "include/algorithms/RecursiveDoublingAlgorithm.h"
#include "include/algorithms/RingAlgorithm.h"
#include "include/algorithms/BucketAlgorithm.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <fstream>

using namespace swing;

void printSeparator()
{
    std::cout << std::string(80, '=') << "\n";
}

// Modified function signature to accept the CSV file stream
void runComparison(NetworkTopology *topology, size_t vector_size_bytes, std::ofstream &csvFile)
{
    std::cout << "\nAllreduce Vector Size: " << vector_size_bytes << " bytes ";
    std::cout << "(" << (vector_size_bytes / 1024.0) << " KB)\n";
    printSeparator();

    // Create algorithms
    std::vector<std::unique_ptr<AllreduceAlgorithm>> algorithms;
    algorithms.push_back(std::make_unique<SwingAlgorithm>(SwingAlgorithm::Variant::BANDWIDTH_OPTIMAL));
    algorithms.push_back(std::make_unique<SwingAlgorithm>(SwingAlgorithm::Variant::LATENCY_OPTIMAL));
    algorithms.push_back(std::make_unique<RecursiveDoublingAlgorithm>(RecursiveDoublingAlgorithm::Variant::BANDWIDTH_OPTIMAL));
    algorithms.push_back(std::make_unique<RecursiveDoublingAlgorithm>(RecursiveDoublingAlgorithm::Variant::LATENCY_OPTIMAL));
    algorithms.push_back(std::make_unique<RingAlgorithm>());
    algorithms.push_back(std::make_unique<BucketAlgorithm>());

    // Run all algorithms and collect results
    struct Result
    {
        std::string name;
        AllreduceAlgorithm::Statistics stats;
    };

    std::vector<Result> results;

    for (auto &algo : algorithms)
    {
        auto stats = algo->execute(topology, vector_size_bytes);
        results.push_back({algo->getName(), stats});
    }

    // Print results table to Console
    std::cout << std::left;
    std::cout << std::setw(40) << "Algorithm"
              << std::setw(12) << "Steps"
              << std::setw(15) << "Time (μs)"
              << std::setw(15) << "Goodput (Gb/s)"
              << std::setw(15) << "Total Bytes\n";
    printSeparator();

    // Find best algorithm for comparison
    double best_time = std::numeric_limits<double>::max();
    for (const auto &result : results)
    {
        if (result.stats.estimated_time_us < best_time)
        {
            best_time = result.stats.estimated_time_us;
        }
    }

    for (const auto &result : results)
    {
        // 1. Print to Console
        std::cout << std::setw(40) << result.name
                  << std::setw(12) << result.stats.num_steps
                  << std::setw(15) << std::fixed << std::setprecision(2) << result.stats.estimated_time_us
                  << std::setw(15) << std::fixed << std::setprecision(2) << result.stats.goodput_gbps
                  << std::setw(15) << result.stats.total_bytes_sent;

        // Show speedup in console
        if (result.stats.estimated_time_us > best_time)
        {
            double slowdown = result.stats.estimated_time_us / best_time;
            std::cout << " (" << std::fixed << std::setprecision(2) << slowdown << "x slower)";
        }
        else
        {
            std::cout << " ⭐ FASTEST";
        }
        std::cout << "\n";

        // 2. Write to CSV File
        // Format: Size,Algorithm,Steps,Time,Goodput,TotalBytes
        csvFile << vector_size_bytes << ","
                << result.name << ","
                << result.stats.num_steps << ","
                << result.stats.estimated_time_us << ","
                << result.stats.goodput_gbps << ","
                << result.stats.total_bytes_sent << "\n";
    }

    std::cout << "\n";
}

int main(int argc, char **argv)
{
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                   SWING ALLREDUCE SIMULATOR                   ║
║          Short-cutting Rings for Higher Bandwidth             ║
╚═══════════════════════════════════════════════════════════════╝
)" << "\n";

    // Configure torus topology
    TorusConfig config;

    if (argc > 1)
    {
        // Parse command line arguments for dimensions
        std::cout << "Using custom dimensions: ";
        for (int i = 1; i < argc; ++i)
        {
            int dim = std::atoi(argv[i]);
            config.dimensions.push_back(dim);
            std::cout << dim;
            if (i < argc - 1)
                std::cout << " x ";
        }
        std::cout << "\n\n";
    }
    else
    {
        // Default: 8x8 2D torus (64 nodes) - good for testing
        config.dimensions = {8, 8};
        std::cout << "Using default 8x8 2D torus (64 nodes)\n\n";
    }

    config.link_bandwidth_gbps = 400.0;
    config.link_latency_ns = 100.0;
    config.hop_latency_ns = 300.0;

    try
    {
        // Generate torus topology
        std::cout << "Generating torus topology...\n";
        TorusGenerator generator(config);
        auto topology = generator.generate();

        topology->printInfo();

        // Export topology visualization
        topology->exportToDot("torus_topology.dot");

        // Test vector sizes (as in paper: 32B to 128MiB)
        std::vector<size_t> vector_sizes = {
            32,              // 32 B
            128,             // 128 B
            512,             // 512 B
            2 * 1024,        // 2 KiB
            8 * 1024,        // 8 KiB
            32 * 1024,       // 32 KiB
            128 * 1024,      // 128 KiB
            512 * 1024,      // 512 KiB
            2 * 1024 * 1024, // 2 MiB
            8 * 1024 * 1024  // 8 MiB
        };

        // --- SETUP CSV OUTPUT ---
        std::ofstream csvFile("benchmark_results.csv");
        if (!csvFile.is_open())
        {
            throw std::runtime_error("Could not open benchmark_results.csv for writing");
        }
        // Write CSV Header
        csvFile << "VectorSizeBytes,Algorithm,Steps,Time_us,Goodput_Gbps,TotalBytes\n";

        std::cout << "\nRunning allreduce comparisons...\n";

        for (size_t vector_size : vector_sizes)
        {
            // Pass csvFile to the function
            runComparison(topology.get(), vector_size, csvFile);
        }

        csvFile.close(); // Close file explicitly

        std::cout << "\nSimulation completed successfully!\n";
        std::cout << "\nTips:\n";
        std::cout << "   - Data saved to: benchmark_results.csv\n";
        std::cout << "   - Use Python to plot: python plot_performance.py\n";
        std::cout << "   - Visualize topology: neato -Tpng torus_topology.dot -o torus.png\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}