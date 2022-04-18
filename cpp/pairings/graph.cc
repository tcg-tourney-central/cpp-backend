#include "cpp/pairings/graph.h"

namespace tcgtc {

// Node ------------------------------------------------------------------------
bool Node::Adjacent(const Node& n) const {
  return me().Adjacent(n);
}

bool Node::operator<(const Node& n) const {
  return this->iptr() < n.iptr();
}

bool Node::operator==(const Node& n) const {
  return this->get() == n.get();
}

bool Node::operator!=(const Node& n) const {
  return !(*this == n);
}

uintptr_t Node::iptr() const { return reinterpret_cast<uintptr_t>(get()); }

// Node::View ------------------------------------------------------------------
absl::StatusOr<Node> Node::View::Lock() const {
  if (auto ptr = lock(); ptr != nullptr) return Node(ptr);
  return absl::FailedPreconditionError("Viewed Node destroyed.");
}

Node::View::View(std::weak_ptr<Node::Impl> impl)
  : ImplementationView<Impl>(std::move(impl)) {}


// Edge ------------------------------------------------------------------------
Edge::Edge(Node a, Node b) : a_(a < b ? a : b), b_(a < b ? b : a) {
  assert(a_ != b);
}

bool operator==(const Edge& e) const {
  return this->a_ == e.a_ && this->b_ == e.b_;
}

bool operator!=(const Edge& e) const {
  return !(*this == e);
}


// Matching --------------------------------------------------------------------
bool Matching::HasVertex(const Node& n) {
  return edges_.contains(n);
}

bool Matching::HasEdge(const Edge& e) {
  auto it = edges_.find(e.a());
  return it != edges_.end() && it->second == e.b();
}

bool Matching::AugmentingEdge(const Edge& e) {
  auto it = edges_.find(e.a());
  return it != edges_.end() && it->second != e.b() && HasVertex(b);
}

void Matching::insert(const Edge& e) {
  assert(!HasVertex(e.a()) && !HasVertex(e.b()));
  edges_.insert({e.a(), e.b()});
  edges_.insert({e.b(), e.a()});
}


namespace internal {
class NodeImpl;
}  // namespace internal
}  // namespace tcgtc
