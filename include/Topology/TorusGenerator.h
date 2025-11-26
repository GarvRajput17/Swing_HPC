#ifndef TORUS_GENERATOR_H
#define TORUS_GENERATOR_H

#include "Node.h"
#include "NetworkTopology.h"
#include <memory>
#include <vector>
#include <stdexcept>
#include <cmath>

namespace swing {

class TorusGenerator {
public:
    explicit TorusGenerator(const TorusConfig& config) : config_(config) {
        validateConfig();
    }

    // Generate the torus topology
    std::unique_ptr<NetworkTopology> generate() {
        auto topology = std::make_unique<NetworkTopology>(config_);

        int total_nodes = config_.getTotalNodes();

        // Create all nodes
        for (int i = 0; i < total_nodes; ++i) {
            auto coords = linearToCoordinates(i);
            auto node = std::make_shared<Node>(i, coords);
            topology->addNode(node);
        }

        // Connect neighbors in torus fashion
        connectTorusNeighbors(topology.get());

        return topology;
    }

    // Convert linear rank to multi-dimensional coordinates
    Node::Coordinate linearToCoordinates(int rank) const {
        Node::Coordinate coords(config_.getNumDimensions());
        int remaining = rank;

        for (int d = 0; d < config_.getNumDimensions(); ++d) {
            coords[d] = remaining % config_.dimensions[d];
            remaining /= config_.dimensions[d];
        }

        return coords;
    }

    // Convert coordinates to linear rank
    int coordinatesToLinear(const Node::Coordinate& coords) const {
        int rank = 0;
        int multiplier = 1;

        for (int d = 0; d < config_.getNumDimensions(); ++d) {
            rank += coords[d] * multiplier;
            multiplier *= config_.dimensions[d];
        }

        return rank;
    }

    // Get neighbor in specific dimension
    int getNeighborRank(int rank, int dimension, int offset) const {
        auto coords = linearToCoordinates(rank);
        coords[dimension] = (coords[dimension] + offset + config_.dimensions[dimension])
                            % config_.dimensions[dimension];
        return coordinatesToLinear(coords);
    }

    // Calculate distance between two nodes on torus (minimal routing)
    int getTorusDistance(int rank1, int rank2) const {
        auto coords1 = linearToCoordinates(rank1);
        auto coords2 = linearToCoordinates(rank2);

        int distance = 0;
        for (int d = 0; d < config_.getNumDimensions(); ++d) {
            int diff = std::abs(coords1[d] - coords2[d]);
            int wrap_diff = config_.dimensions[d] - diff;
            distance += std::min(diff, wrap_diff);
        }

        return distance;
    }

    const TorusConfig& getConfig() const { return config_; }

private:
    TorusConfig config_;

    void validateConfig() {
        if (config_.dimensions.empty()) {
            throw std::invalid_argument("Torus must have at least 1 dimension");
        }

        for (size_t i = 0; i < config_.dimensions.size(); ++i) {
            if (config_.dimensions[i] < 2) {
                throw std::invalid_argument(
                    "Dimension " + std::to_string(i) + " must be at least 2");
            }
        }
    }

    void connectTorusNeighbors(NetworkTopology* topology) {
        int total_nodes = config_.getTotalNodes();
        int num_dims = config_.getNumDimensions();

        for (int rank = 0; rank < total_nodes; ++rank) {
            auto node = topology->getNode(rank);

            // For each dimension, connect to neighbors in both directions
            for (int d = 0; d < num_dims; ++d) {
                // Negative direction neighbor
                int neg_neighbor = getNeighborRank(rank, d, -1);
                node->addNeighbor(neg_neighbor);

                // Positive direction neighbor
                int pos_neighbor = getNeighborRank(rank, d, +1);
                node->addNeighbor(pos_neighbor);
            }
        }
    }
};

} // namespace swing

#endif // TORUS_GENERATOR_H
