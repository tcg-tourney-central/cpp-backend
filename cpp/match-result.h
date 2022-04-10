#ifndef _TCGTC_MATCH_RESULT_H_
#define _TCGTC_MATCH_RESULT_H_

#include <cstdint>
#include <optional>

#include "match-id.h"

namespace tcgtc {

using PlayerId = uint64_t;

struct MatchResult {
  MatchId id;
  // May be empty if the match was drawn.
  std::optional<PlayerId> winner;
  uint16_t winner_games_won = 0;
  uint16_t winner_games_lost = 0;
  uint16_t games_drawn = 0;

  // TODO: Add droppers.

  uint16_t match_points(PlayerId p) const;
  uint16_t game_points(PlayerId p) const;
  uint16_t games_played() const;
};
bool operator==(const MatchResult& l, const MatchResult& r);
bool operator!=(const MatchResult& l, const MatchResult& r);

}  // namespace tcgtc

#endif  // _TCGTC_MATCH_RESULT_H_
