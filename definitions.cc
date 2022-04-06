#include "definitions.h"

#include "match.h"
#include "match-id.h"
#include "match-result.h"
#include "player.h"
#include "util.h"

namespace tcgtc {

// Player ----------------------------------------------------------------------
Player Player::CreatePlayer(Options opts) { 
  Player p(std::shared_ptr<PlayerImpl>(new PlayerImpl(opts)));
  p->Init();
  return p;
}

uint64_t Player::id() const { return me().id(); }

bool operator==(const Player& l, const Player& r) {
  if (l.id() != r.id()) return false;

  // TODO: Add an invariant check that `l.player_ == r.player_`
  return true;
}

bool operator<(const Player& l, const Player& r) {
  // Skip the OMWP computations when possible.
  uint16_t lhsmp = l->match_points();
  uint16_t rhsmp = r->match_points();
  if (lhsmp != rhsmp) return lhsmp < rhsmp;

  // Tie-break within equal match points.
  return l->ComputeBreakers() < r->ComputeBreakers();
}



// Match -----------------------------------------------------------------------
Match Match::CreateBye(Player p, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(p, std::nullopt, id)));
  m->Init();

  // Immediately commit the result of the bye back to the player's cache.
  // MTR states that a Bye is considered won 2-0 in games.
  m->CommitResult(MatchResult{id, p->id(), 2});
  return m;
}

Match Match::CreatePairing(Player a, Player b, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(a, b, id)));
  m->Init();
  return m;
}

absl::StatusOr<Tournament> Tournament::View::Lock() {
  if (auto ptr = lock(); ptr != nullptr) return Tournament(ptr);
  return Err("Viewed Tournament has been torn down.");
}

}  // namespace tcgtc
