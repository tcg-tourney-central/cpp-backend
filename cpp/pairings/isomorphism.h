// This file defines the transformation from a set of players who need pairing
// to a graph structure where each player is a node and there is an edge
// between two nodes IFF those two players could be validly paired this round.
//
// In order to approximate selecting a random maximal matching from among the
// set of maximal matchings, we shuffle the input vector.

#ifndef _TCGTC_PAIRINGS_ISOMORPHISM_H_
#define _TCGTC_PAIRINGS_ISOMORPHISM_H_

#include <algorithm>
#include <utility>
#include <vector>

#include "cpp/player-match.h"
#include "cpp/pairings/graph.h"

namespace tcgtc {

struct PartialPairing {
  std::vector<std::pair<Player, Player>> paired;
  std::vector<Player> unpaired;
};

namespace internal {
PartialPairing PairChunkInternal(std::vector<Player> players);
}  // namespace internal

template <typename URBG>
PartialPairing PairChunk(std::vector<Player> players, URBG& urbg) {
  std::shuffle(players.begin(), players.end(), urbg);
  return internal::PairChunkInternal(players);
}

}  // namespce tcgtc

#endif  // _TCGTC_PAIRINGS_ISOMORPHISM_H_
