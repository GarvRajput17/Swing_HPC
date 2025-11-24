#ifndef NETWORK_TOPOLOGY_H
#define NETWORK_TOPOLOGY_H

#include "Node.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <set>

namespace swing {

// Forward declaration
class TorusGenerator;

// TorusConfig definition - shared between classes
struct TorusConfig {
    std::vector<int> dimensions;
    double link_bandwidth_gbps = 400.0;
    double link_latency_ns = 100.0;
    double hop_latency_ns = 300.0;

    int getNumDimensions() const { return dimensions.size(); }
    int getTotalNodes() const {
        int total = 1;
        for (int d : dimensions) total *= d;
        return total;
    }

    bool isSquare() const {
        if (dimensions.empty()) return false;
        int first = dimensions[0];
        for (int d : dimensions) {
            if (d != first) return false;
        }
        return true;
    }

    bool isPowerOfTwo(int n) const {
        return n > 0 && (n & (n - 1)) == 0;
    }

    bool allDimensionsPowerOfTwo() const {
        for (int d : dimensions) {
            if (!isPowerOfTwo(d)) return false;
        }
        return true;
    }
};

class NetworkTopology {
public:
    using NodePtr = std::shared_ptr<Node>;

    explicit NetworkTopology(const TorusConfig& config)
        : config_(config) {}

    void addNode(NodePtr node) {
        nodes_.push_back(node);
        node_map_[node->getId()] = node;
    }

    NodePtr getNode(Node::NodeId id) const {
        auto it = node_map_.find(id);
        if (it == node_map_.end()) {
            throw std::out_of_range("Node not found: " + std::to_string(id));
        }
        return it->second;
    }

    const std::vector<NodePtr>& getAllNodes() const { return nodes_; }
    size_t getNumNodes() const { return nodes_.size(); }

    const TorusConfig& getConfig() const { return config_; }

    // Export topology to DOT format for visualization
    void exportToDot(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        out << "graph Torus {\n";
        out << "  layout=neato;\n";
        out << "  overlap=false;\n";

        // For 2D torus, position nodes in grid
        if (config_.getNumDimensions() == 2) {
            for (const auto& node : nodes_) {
                const auto& coords = node->getCoordinates();
                out << "  " << node->getId()
                    << " [pos=\"" << coords[0] << "," << coords[1] << "!\"]\n";
            }
        }

        // Add edges (only in positive direction to avoid duplicates)
        for (const auto& node : nodes_) {
            Node::NodeId id = node->getId();
            for (auto neighbor : node->getNeighbors()) {
                if (id < neighbor) {  // Only add edge once
                    out << "  " << id << " -- " << neighbor << ";\n";
                }
            }
        }

        out << "}\n";
        out.close();

        std::cout << "Exported topology to " << filename << std::endl;
        std::cout << "Visualize with: neato -Tpng " << filename << " -o torus.png\n";
    }

    // Print topology information
    void printInfo() const {
        std::cout << "\n=== Torus Topology Information ===\n";
        std::cout << "Dimensions: [";
        for (size_t i = 0; i < config_.dimensions.size(); ++i) {
            std::cout << config_.dimensions[i];
            if (i < config_.dimensions.size() - 1) std::cout << " x ";
        }
        std::cout << "]\n";
        std::cout << "Total Nodes: " << getNumNodes() << "\n";
        std::cout << "Is Square: " << (config_.isSquare() ? "Yes" : "No") << "\n";
        std::cout << "All Power of Two: " << (config_.allDimensionsPowerOfTwo() ? "Yes" : "No") << "\n";
        std::cout << "Link Bandwidth: " << config_.link_bandwidth_gbps << " Gb/s\n";
        std::cout << "Link Latency: " << config_.link_latency_ns << " ns\n";
        std::cout << "==================================\n\n";
    }

private:
    TorusConfig config_;
    std::vector<NodePtr> nodes_;
    std::unordered_map<Node::NodeId, NodePtr> node_map_;
};

} // namespace swing

#endif // NETWORK_TOPOLOGY_H
