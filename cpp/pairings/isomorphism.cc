#include "cpp/pairings/isomorphism.h"

#include <deque>

namespace tcgtc {
namespace internal {
namespace {
Matching InitialMatching(const std::vector<Node>& nodes) {
  Matching m;
  if (nodes.empty()) return m;

  // Build a chain of nodes s.t. for each pair of nodes adjacent in the chain,
  // they are adjacent in the graph.
  std::deque<Node> q = {nodes.front()};
  for (int i = 1; i < nodes.size(); ++i) {
    const Node& n = nodes[i];
    if (n->Adjacent(q.front())) {
      q.push_front(n);
      continue;
    }
    if (n->Adjacent(q.back())) {
      q.push_back(n);
    }
  }
  for (int i = 0; i + 1 < q.size(); i += 2) m.Insert(q[i], q[i+1]);
  return m;
}
}  // namespace

PartialPairing PairChunkInternal(std::vector<Player> players) {
  std::vector<CanonicalNode> nodes;
  absl::flat_hash_map<Node, Player> n2p;
  nodes.reserve(players.size());
  n2p.reserve(players.size());

  auto player = [&](const Node& n) -> Player {
    auto it = n2p.find(n);
    assert(it != n2p.end());
    return it->second;
  };

  for (const auto& p : players) {
    auto& canonical = nodes.emplace_back();
    n2p.insert({canonical.view(), p});
  }

  // Initialize the graph edges.
  for (int i = 0; i < nodes.size(); i++) {
    auto ni = n2p.find(nodes[i].view());
    for (int j = 0; j < i; j++) {
      auto nj = n2p.find(nodes[j].view());
      // Do not include edges for players that have played before.
      if (ni->second->has_played_opp(nj->second)) continue;
      ni->first->AddNeighbor(nj->first);
    }
  }

  PartialPairing ret;

  // Trim simple base cases of Nodes with degree 0 and 1 from our graph.
  std::vector<Node> graph_nodes;
  graph_nodes.reserve(players.size());
  absl::flat_hash_set<Node> paired;
  for (const auto& n : nodes) {
    const auto& nbhd = n->neighbors();
    switch (nbhd.size()) {
      case 0:  // No legal pairings in this chunk.
        ret.unpaired.push_back(player(n));
        break;
      case 1:  // One legal pairing in this chunk.
        if (paired.insert(n).second) {  // Only insert the pairing once.
          const Node& adj = *nbhd.begin();
          ret.paired.push_back({player(n), player(adj)});
          paired.insert(adj);
        }
        break;
      default:
        graph_nodes.push_back(n);
    }
  }

  // Fill in the rest of the partial pairing.
  Matching initial = InitialMatching(graph_nodes);
  Graph g(graph_nodes);

  // TODO: Define Blossom algo.
  Matching maximal; // = Blossom(g, initial);

  for (const Node& n : g.nodes()) {
    if (auto it = maximal.edges().find(n); it != maximal.edges().end()) {
      if (paired.insert(n).second) {  // Only insert the pairing once.
        const Node& adj = it->second;
        ret.paired.push_back({player(n), player(adj)});
        paired.insert(adj);
      }
    } else {
      ret.unpaired.push_back(player(n));
    }
  }

  return ret;
}

}  // namespace internal
}  // namespace tcgtc
