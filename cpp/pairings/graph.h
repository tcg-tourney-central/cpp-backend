#ifndef _TCGTC_PAIRINGS_GRAPH_H_
#define _TCGTC_PAIRINGS_GRAPH_H_

#include "absl/hash/hash.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "cpp/container-class.h"

// This is an implementation of the Blossom Algorithm for maximum matchings on
// general graphs. The implementation is my own but the algorithm comes directly
// from https://en.wikipedia.org/wiki/Blossom_algorithm

namespace tcgtc {
namespace internal {
class NodeImpl;
}  // namespace internal

class CanonicalNode;

class Node : public RawView<internal::NodeImpl> {
 public:
  // Allow implicit conversion from the RawContainer.
  Node(const CanonicalNode& node);

  using Impl = ::tcgtc::internal::NodeImpl;
  using Id = uint64_t;

  // TODO: Do we want this method?
  bool Adjacent(const Node& n) const;

  bool operator<(const Node& n) const;
  bool operator==(const Node& n) const;
  bool operator!=(const Node& n) const;

  template <typename H>
  friend H AbslHashValue(H h, const Node& n) {
    return H::combine(std::move(h), n.iptr());
  }

 private:
  explicit Node(Impl* impl) : RawView(impl) {}

  uintptr_t iptr() const;

  friend class ::tcgtc::internal::NodeImpl;
};

// Holds the memory for a given node.
class CanonicalNode : public RawContainer<internal::NodeImpl> {
 public:
  CanonicalNode();
  Node view() const { return Node(*this); }
};

class Edge {
 public:
  Edge(Node a, Node b);

  Node a() const { return a_; }
  Node b() const { return b_; }

  bool operator==(const Edge& e) const;
  bool operator!=(const Edge& e) const;

  template <typename H>
  friend H AbslHashValue(H h, const Edge& e) {
    return H::combine(std::move(h), e.a(), e.b());
  }

 private:
  Node a_;
  Node b_;
};

class Matching {
 public:
  bool HasVertex(const Node& n);
  bool HasEdge(const Edge& e);

  // Returns true if both of the vertices are in the matching, but the edge is
  // not.
  bool AugmentingEdge(const Edge& e);
 private:
  void insert(const Edge& e);

  // For all Node A in edges_,
  // edges_[edges_[A]] == A
  absl::flat_hash_map<Node, Node> edges_;
};

class Graph {
 public:
  struct Options {
    std::vector<CanonicalNode> owned_nodes;
    std::vector<Node> nodes;
  };

  void AddEdge(const Node& a, const Node& b);

  const absl::flat_hash_set<Node>& nodes() const { return all_nodes_; }
  const absl::flat_hash_set<Edge>& edges() const { return edges_; }

 private:
  std::vector<CanonicalNode> owned_nodes_;
  absl::flat_hash_set<Node> all_nodes_;
  absl::flat_hash_set<Edge> edges_;
};

namespace internal {

class NodeImpl {
 public:
  NodeImpl() = default;

  bool Adjacent(const Node& node) const;
  void AddNeighbor(const Node& node);
  void RemoveNeighbor(const Node& node);  
  int degree() const { return neighbors_.size(); }

  const absl::flat_hash_set<Node>& neighbors() const { return neighbors_; }

 private:
  Node self() const { return Node(const_cast<NodeImpl*>(this)); }
  absl::flat_hash_set<Node> neighbors_;
};

}  // namespace internal
}  // namespace tcgtc

#endif  // _TCGTC_PAIRINGS_GRAPH_H_
