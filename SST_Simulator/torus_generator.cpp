/**
 * Standalone Torus Network Generator
 *
 * Generates D-dimensional torus topologies for network simulations
 * Based on the NSDI 2024 "Swing" paper implementation
 *
 * Usage:
 *   g++ -std=c++17 -O3 torus_generator.cpp -o torus_gen
 *   ./torus_gen <dim1> <dim2> [dim3] [dim4] ...
 *
 * Example:
 *   ./torus_gen 8 8              # 8x8 2D torus (64 nodes)
 *   ./torus_gen 4 4 4            # 4x4x4 3D torus (64 nodes)
 *   ./torus_gen 16 4             # 16x4 rectangular torus (64 nodes)
 *   ./torus_gen 64 64            # 64x64 2D torus (4096 nodes)
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstdlib>

// ============================================================================
// Node Structure
// ============================================================================

struct Node {
    int id;
    std::vector<int> coordinates;
    std::vector<int> neighbors;  // neighbor IDs

    Node(int id_, const std::vector<int>& coords)
        : id(id_), coordinates(coords) {}

    std::string toString() const {
        std::ostringstream oss;
        oss << "Node " << id << " [";
        for (size_t i = 0; i < coordinates.size(); ++i) {
            oss << coordinates[i];
            if (i < coordinates.size() - 1) oss << ",";
        }
        oss << "]";
        return oss.str();
    }
};

// ============================================================================
// Torus Generator Class
// ============================================================================

class TorusGenerator {
private:
    std::vector<int> dimensions;
    std::vector<Node> nodes;
    int total_nodes;

public:
    TorusGenerator(const std::vector<int>& dims) : dimensions(dims) {
        validate();
        total_nodes = calculateTotalNodes();
    }

    // Generate the torus topology
    void generate() {
        std::cout << "\n🔧 Generating torus topology...\n";
        createNodes();
        connectNeighbors();
        std::cout << "✅ Topology generated successfully!\n";
    }

    // Convert linear rank to multi-dimensional coordinates
    std::vector<int> rankToCoordinates(int rank) const {
        std::vector<int> coords(dimensions.size());
        int remaining = rank;

        for (size_t d = 0; d < dimensions.size(); ++d) {
            coords[d] = remaining % dimensions[d];
            remaining /= dimensions[d];
        }

        return coords;
    }

    // Convert coordinates to linear rank
    int coordinatesToRank(const std::vector<int>& coords) const {
        int rank = 0;
        int multiplier = 1;

        for (size_t d = 0; d < dimensions.size(); ++d) {
            rank += coords[d] * multiplier;
            multiplier *= dimensions[d];
        }

        return rank;
    }

    // Get neighbor in specific dimension and direction
    int getNeighbor(int rank, int dimension, int offset) const {
        auto coords = rankToCoordinates(rank);
        coords[dimension] = (coords[dimension] + offset + dimensions[dimension])
                            % dimensions[dimension];
        return coordinatesToRank(coords);
    }

    // Calculate minimal distance between two nodes on torus
    int getDistance(int rank1, int rank2) const {
        auto coords1 = rankToCoordinates(rank1);
        auto coords2 = rankToCoordinates(rank2);

        int distance = 0;
        for (size_t d = 0; d < dimensions.size(); ++d) {
            int diff = std::abs(coords1[d] - coords2[d]);
            int wrap_diff = dimensions[d] - diff;
            distance += std::min(diff, wrap_diff);
        }

        return distance;
    }

    // Print topology information
    void printInfo() const {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "                    TORUS TOPOLOGY INFO\n";
        std::cout << std::string(70, '=') << "\n";

        std::cout << "Dimensions:        [";
        for (size_t i = 0; i < dimensions.size(); ++i) {
            std::cout << dimensions[i];
            if (i < dimensions.size() - 1) std::cout << " x ";
        }
        std::cout << "]\n";

        std::cout << "D (# of dims):     " << dimensions.size() << "\n";
        std::cout << "Total Nodes:       " << total_nodes << "\n";
        std::cout << "Ports per Node:    " << (2 * dimensions.size()) << " (2D bidirectional)\n";
        std::cout << "Is Square:         " << (isSquare() ? "Yes" : "No") << "\n";
        std::cout << "Power of 2 dims:   " << (allPowerOfTwo() ? "Yes" : "No") << "\n";

        // Calculate some interesting metrics
        int max_distance = calculateMaxDistance();
        std::cout << "Max Distance:      " << max_distance << " hops\n";
        std::cout << "Network Diameter:  " << max_distance << "\n";

        std::cout << std::string(70, '=') << "\n";
    }

    // Print sample nodes
    void printSampleNodes(int count = 10) const {
        std::cout << "\n📍 Sample Nodes (showing first " << std::min(count, total_nodes) << "):\n";
        std::cout << std::string(70, '-') << "\n";

        for (int i = 0; i < std::min(count, total_nodes); ++i) {
            const Node& node = nodes[i];
            std::cout << node.toString() << " -> Neighbors: [";
            for (size_t j = 0; j < node.neighbors.size(); ++j) {
                std::cout << node.neighbors[j];
                if (j < node.neighbors.size() - 1) std::cout << ", ";
            }
            std::cout << "]\n";
        }
        std::cout << std::string(70, '-') << "\n";
    }

    // Export to DOT format for Graphviz visualization
    void exportToDot(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "❌ Failed to open file: " << filename << "\n";
            return;
        }

        out << "graph Torus {\n";
        out << "  layout=neato;\n";
        out << "  overlap=false;\n";
        out << "  splines=true;\n";
        out << "  node [shape=circle, style=filled, fillcolor=lightblue];\n";

        // For 2D torus, position nodes in a grid
        if (dimensions.size() == 2) {
            for (const auto& node : nodes) {
                const auto& coords = node.coordinates;
                out << "  " << node.id
                    << " [pos=\"" << coords[0] << "," << coords[1] << "!\", label=\"" << node.id << "\"];\n";
            }
        } else {
            // For higher dimensions, just add labels
            for (const auto& node : nodes) {
                out << "  " << node.id << " [label=\"" << node.id << "\"];\n";
            }
        }

        // Add edges (only in positive direction to avoid duplicates)
        for (const auto& node : nodes) {
            for (int neighbor : node.neighbors) {
                if (node.id < neighbor) {  // Only add each edge once
                    out << "  " << node.id << " -- " << neighbor << ";\n";
                }
            }
        }

        out << "}\n";
        out.close();

        std::cout << "\n✅ Topology exported to: " << filename << "\n";
        std::cout << "💡 Visualize with: neato -Tpng " << filename << " -o torus.png\n";
        if (dimensions.size() == 2 && total_nodes <= 100) {
            std::cout << "   Or use:          fdp -Tpng " << filename << " -o torus.png\n";
        }
    }

    // Export to adjacency list format
    void exportToAdjacencyList(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "❌ Failed to open file: " << filename << "\n";
            return;
        }

        out << "# Torus Adjacency List\n";
        out << "# Dimensions: ";
        for (size_t i = 0; i < dimensions.size(); ++i) {
            out << dimensions[i];
            if (i < dimensions.size() - 1) out << "x";
        }
        out << "\n";
        out << "# Total Nodes: " << total_nodes << "\n";
        out << "# Format: node_id: neighbor1 neighbor2 ...\n\n";

        for (const auto& node : nodes) {
            out << node.id << ":";
            for (int neighbor : node.neighbors) {
                out << " " << neighbor;
            }
            out << "\n";
        }

        out.close();
        std::cout << "✅ Adjacency list exported to: " << filename << "\n";
    }

    // Export node coordinates to CSV
    void exportToCSV(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "❌ Failed to open file: " << filename << "\n";
            return;
        }

        // Header
        out << "node_id";
        for (size_t d = 0; d < dimensions.size(); ++d) {
            out << ",dim" << d;
        }
        out << ",num_neighbors\n";

        // Data
        for (const auto& node : nodes) {
            out << node.id;
            for (int coord : node.coordinates) {
                out << "," << coord;
            }
            out << "," << node.neighbors.size() << "\n";
        }

        out.close();
        std::cout << "✅ Node coordinates exported to: " << filename << "\n";
    }

    // Calculate distance matrix
    void printDistanceMatrix(int max_nodes = 16) const {
        int n = std::min(max_nodes, total_nodes);

        std::cout << "\n📊 Distance Matrix (first " << n << "x" << n << " nodes):\n";
        std::cout << "    ";
        for (int j = 0; j < n; ++j) {
            std::cout << std::setw(3) << j;
        }
        std::cout << "\n";

        for (int i = 0; i < n; ++i) {
            std::cout << std::setw(3) << i << " ";
            for (int j = 0; j < n; ++j) {
                std::cout << std::setw(3) << getDistance(i, j);
            }
            std::cout << "\n";
        }
    }

    // Get statistics
    void printStatistics() const {
        std::cout << "\n📈 Topology Statistics:\n";
        std::cout << std::string(70, '-') << "\n";

        // Degree distribution (should be uniform for torus)
        int degree = nodes[0].neighbors.size();
        std::cout << "Node Degree:       " << degree << " (uniform)\n";

        // Total edges
        int total_edges = (total_nodes * degree) / 2;
        std::cout << "Total Edges:       " << total_edges << "\n";

        // Average distance
        double avg_distance = 0;
        int count = 0;
        for (int i = 0; i < std::min(100, total_nodes); ++i) {
            for (int j = i + 1; j < std::min(100, total_nodes); ++j) {
                avg_distance += getDistance(i, j);
                count++;
            }
        }
        if (count > 0) {
            avg_distance /= count;
            std::cout << "Avg Distance:      " << std::fixed << std::setprecision(2)
                      << avg_distance << " hops (sampled)\n";
        }

        // Bisection bandwidth (for square torus)
        if (isSquare() && dimensions.size() == 2) {
            int bisection = dimensions[0] * 2;  // Cut through middle
            std::cout << "Bisection Width:   " << bisection << " links\n";
        }

        std::cout << std::string(70, '-') << "\n";
    }

    const std::vector<Node>& getNodes() const { return nodes; }
    int getTotalNodes() const { return total_nodes; }
    const std::vector<int>& getDimensions() const { return dimensions; }

private:
    void validate() {
        if (dimensions.empty()) {
            throw std::invalid_argument("Must have at least 1 dimension");
        }

        for (size_t i = 0; i < dimensions.size(); ++i) {
            if (dimensions[i] < 2) {
                throw std::invalid_argument(
                    "Dimension " + std::to_string(i) + " must be at least 2 (got " +
                    std::to_string(dimensions[i]) + ")");
            }
        }
    }

    int calculateTotalNodes() const {
        int total = 1;
        for (int dim : dimensions) {
            total *= dim;
        }
        return total;
    }

    void createNodes() {
        nodes.clear();
        nodes.reserve(total_nodes);

        for (int i = 0; i < total_nodes; ++i) {
            auto coords = rankToCoordinates(i);
            nodes.emplace_back(i, coords);
        }
    }

    void connectNeighbors() {
        int num_dims = dimensions.size();

        for (auto& node : nodes) {
            // For each dimension, connect to neighbors in both directions
            for (int d = 0; d < num_dims; ++d) {
                // Negative direction neighbor
                int neg_neighbor = getNeighbor(node.id, d, -1);
                node.neighbors.push_back(neg_neighbor);

                // Positive direction neighbor
                int pos_neighbor = getNeighbor(node.id, d, +1);
                node.neighbors.push_back(pos_neighbor);
            }
        }
    }

    bool isSquare() const {
        if (dimensions.empty()) return false;
        int first = dimensions[0];
        for (int dim : dimensions) {
            if (dim != first) return false;
        }
        return true;
    }

    bool isPowerOfTwo(int n) const {
        return n > 0 && (n & (n - 1)) == 0;
    }

    bool allPowerOfTwo() const {
        for (int dim : dimensions) {
            if (!isPowerOfTwo(dim)) return false;
        }
        return true;
    }

    int calculateMaxDistance() const {
        int max_dist = 0;
        for (size_t d = 0; d < dimensions.size(); ++d) {
            max_dist += dimensions[d] / 2;
        }
        return max_dist;
    }
};

// ============================================================================
// Main Program
// ============================================================================

void printUsage(const char* program) {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║              TORUS NETWORK TOPOLOGY GENERATOR                 ║
╚═══════════════════════════════════════════════════════════════╝

Usage: )" << program << R"( <dim1> <dim2> [dim3] [dim4] ...

Arguments:
  dim1, dim2, ... : Size of each torus dimension (must be >= 2)

Examples:
  )" << program << R"( 8 8              # 8x8 2D torus (64 nodes)
  )" << program << R"( 4 4 4            # 4x4x4 3D torus (64 nodes)
  )" << program << R"( 16 4             # 16x4 rectangular 2D torus
  )" << program << R"( 64 64            # 64x64 2D torus (4096 nodes)
  )" << program << R"( 8 8 8 8          # 8x8x8x8 4D torus (4096 nodes)

Output Files:
  - torus_topology.dot  : GraphViz visualization format
  - torus_adj.txt       : Adjacency list format
  - torus_coords.csv    : Node coordinates in CSV format

Visualization:
  neato -Tpng torus_topology.dot -o torus.png
  fdp -Tpng torus_topology.dot -o torus.png

)";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    // Parse dimensions
    std::vector<int> dimensions;
    for (int i = 1; i < argc; ++i) {
        int dim = std::atoi(argv[i]);
        if (dim < 2) {
            std::cerr << "❌ Error: Dimension " << i << " must be >= 2 (got " << dim << ")\n";
            return 1;
        }
        dimensions.push_back(dim);
    }

    try {
        // Create and generate torus
        TorusGenerator torus(dimensions);
        torus.generate();

        // Print information
        torus.printInfo();
        torus.printStatistics();

        // Print sample nodes (more for small networks)
        int sample_size = torus.getTotalNodes() <= 64 ? 16 : 10;
        torus.printSampleNodes(sample_size);

        // Print distance matrix for small networks
        if (torus.getTotalNodes() <= 64) {
            torus.printDistanceMatrix(16);
        }

        // Export to various formats
        std::cout << "\n📤 Exporting topology...\n";
        torus.exportToDot("torus_topology.dot");
        torus.exportToAdjacencyList("torus_adj.txt");
        torus.exportToCSV("torus_coords.csv");

        std::cout << "\n✅ All done!\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}
