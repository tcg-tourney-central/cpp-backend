#include "cpp/pairings/graph.h"

namespace tcgtc {

// Node ------------------------------------------------------------------------
Node::Node(const CanonicalNode& n) : Node(n.get()) {}

bool Node::Adjacent(const Node& n) const {
  return get()->Adjacent(n);
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

// CanonicalNode ---------------------------------------------------------------
CanonicalNode::CanonicalNode() : RawContainer<internal::NodeImpl>() {}


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

// Graph -----------------------------------------------------------------------
void Graph::AddEdge(const Node& a, const Node& b) {
  assert(all_nodes_.contains(a) && all_nodes_.contains(b));
  edges_.insert(Edge(a,b));

  // AddNeighbor is symmetric.
  a->AddNeighbor(b);
}


namespace internal {

void NodeImpl::Adjacent(const Node& n) const {
  bool ret = neighbors_.contains(node);
  // Adjacency should be symmetric.
  assert(n.get()->neighbors_.contains(self()) == ret);
  return ret;
}

void NodeImpl::AddNeighbor(const Node& n) {
  assert(n != self());
  neighbors_.insert(n);
  n.get()->neighbors_.insert(self());
}

void NodeImpl::RemoveNeighbor(const Node& n) {
  neighbors_.erase(node); 
  n.get()->neighbors_.erase(self());
}

}  // namespace internal
}  // namespace tcgtc
