#ifndef _TCGTC_PAIRINGS_GRAPH_H_
#define _TCGTC_PAIRINGS_GRAPH_H_

#include "cpp/container-class.h"

// This is an implementation of the Blossom Algorithm for maximum matchings on
// general graphs. The implementation is my own but the algorithm comes directly
// from https://en.wikipedia.org/wiki/Blossom_algorithm

namespace tcgtc {
namespace internal {
class NodeImpl;
}  // namespace internal

class Node : public ContainerClass<internal::NodeImpl> {
 public:
  using Impl = ::tcgtc::internal::NodeImpl;
  using Id = uint64_t;

  class View : public ImplementationView<Impl> {
   public:
    View() = default;
    absl::StatusOr<Node> Lock() const {
      if (auto ptr = lock(); ptr != nullptr) return Node(ptr);
      return absl::FailedPreconditionError("Viewed Node destroyed.");
    }
   private:
    explicit View(std::weak_ptr<Impl> impl)
      : ImplementationView<Impl>(std::move(impl)) {}
    friend class Node;
  };

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
  explicit Node(std::shared_ptr<Impl> impl)
    : ContainerClass(std::move(impl)) {}

  uintptr_t iptr() const; { return reinterpret_cast<uintptr_t>(get()); }

  friend class ::tcgtc::internal::NodeImpl;
};

class Edge {
 public:
  Edge(Node a, Node b) : a_(a < b ? a : b), b_(a < b ? b : a) {
    assert(a_ != b);
  }

  const Node& a() const { return a_; }
  const Node& b() const { return b_; }

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
  bool HasVertex(const Node& n) {
    return edges_.contains(n);
  }
  bool HasEdge(const Edge& e) {
    auto it = edges_.find(e.a());
    return it != edges_.end() && it->second == e.b();
  }
  // Returns true if both of the vertices are in the matching, but the edge is
  // not.
  bool AugmentingEdge(const Edge& e) {
    auto it = edges_.find(e.a());
    return it != edges_.end() && it->second != e.b() && HasVertex(b);
  }
 private:
  void insert(const Edge& e);

  // For all Node A in edges_,
  // edges_[edges_[A]] == A
  absl::flat_hash_map<Node, Node> edges_;
};
  
}  // namespace tcgtc

#endif  // _TCGTC_PAIRINGS_GRAPH_H_
