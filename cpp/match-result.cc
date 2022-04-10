#include "match-result.h"

namespace tcgtc {

// MatchResult -----------------------------------------------------------------
bool operator==(const MatchResult& l, const MatchResult& r) {
  return l.id == r.id &&
         l.winner == r.winner &&
         l.winner_games_won == r.winner_games_won &&
         l.winner_games_lost == r.winner_games_lost &&
         l.games_drawn == r.games_drawn;
}

bool operator!=(const MatchResult& l, const MatchResult& r) {
  return !(l == r);
}

uint16_t MatchResult::match_points(PlayerId p) const {
  if (!winner.has_value()) return 1;
  return p == *winner ? 3 : 0;
}

uint16_t MatchResult::game_points(PlayerId p) const {
  if (!winner.has_value() || p == *winner) {
    return 3 * winner_games_won + games_drawn;
  }
  return 3 * winner_games_lost + games_drawn;
}

uint16_t MatchResult::games_played() const {
  return winner_games_won + winner_games_lost + games_drawn;
}

}  // namespace tgctc
