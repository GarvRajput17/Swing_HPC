#ifndef NODE_H
#define NODE_H

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

namespace swing {

class Node {
public:
    using NodeId = uint32_t;
    using Coordinate = std::vector<int>;

    Node(NodeId id, const Coordinate& coords)
        : id_(id), rank_(id), coordinates_(coords) {}

    NodeId getId() const { return id_; }
    int getRank() const { return rank_; }
    const Coordinate& getCoordinates() const { return coordinates_; }

    // Get neighbors (for torus, 2*D neighbors)
    const std::vector<NodeId>& getNeighbors() const { return neighbors_; }
    void addNeighbor(NodeId neighbor) { neighbors_.push_back(neighbor); }

    // Get neighbor in specific dimension and direction
    NodeId getNeighbor(int dimension, bool positive) const {
        int idx = dimension * 2 + (positive ? 1 : 0);
        return neighbors_[idx];
    }

    // Data buffer for allreduce
    std::vector<float> data;
    std::vector<float> recv_buffer;

    std::string toString() const {
        std::string s = "Node " + std::to_string(id_) + " [";
        for (size_t i = 0; i < coordinates_.size(); ++i) {
            s += std::to_string(coordinates_[i]);
            if (i < coordinates_.size() - 1) s += ",";
        }
        s += "]";
        return s;
    }

private:
    NodeId id_;
    int rank_;
    Coordinate coordinates_;
    std::vector<NodeId> neighbors_;
};

} // namespace swing

#endif // NODE_H
