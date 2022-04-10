#ifndef _TCGTC_PLAYER_H_
#define _TCGTC_PLAYER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "container-class.h"
#include "definitions.h"
#include "fraction.h"
#include "match-id.h"
#include "match-result.h"
#include "tiebreaker.h"
#include "util.h"

namespace tcgtc {
namespace internal {

class PlayerImpl : public MemoryManagedImplementation<PlayerImpl> {
 public:

  struct Options {
    Player::Id id;
    std::string first_name;
    std::string last_name;
    std::string username;
  };
  static Player CreatePlayer(Options opts);

  // The persistent ID in the DB schema.
  Player::Id id() const { return id_; }
  const std::string& last_name() const { return last_name_; }
  const std::string& first_name() const { return first_name_; }
  const std::string& username() const { return username_; }

  std::string ErrorStringId() const {
    return absl::StrCat("Player (", display_name_, ")");
  }

  bool has_played_opp(const Player& p) const  ABSL_LOCKS_EXCLUDED(mu_);

  uint16_t match_points() const { return match_points_; }
  Fraction mwp() const { 
    return Fraction(match_points_, 3 * matches_.size()).ApplyMtrBound();
  }
  Fraction gwp() const { 
    return Fraction(game_points_, 3 * games_played_).ApplyMtrBound();
  }

  TieBreakInfo ComputeBreakers() const ABSL_LOCKS_EXCLUDED(mu_);

  // Commit a result, and if there is a previous result for that match, erase
  // that from the cache.
  absl::Status CommitResult(const MatchResult& result,
                            const std::optional<MatchResult>& prev)
    ABSL_LOCKS_EXCLUDED(mu_);

  absl::Status AddMatch(Match m) ABSL_LOCKS_EXCLUDED(mu_);

 private:
  explicit PlayerImpl(const Options& opts);

  // Init() and this_player() can only be called after the constructor. See
  // documentation on std::enable_shared_from_this.
  void Init() { InitSelfPtr(); } 
  Player this_player() const { return Player(self_copy()); }

  const Player::Id id_;
  const std::string last_name_;
  const std::string first_name_;
  const std::string username_;  // e.g. for online tournaments.
  const std::string display_name_;  // Used for Match Slips, standings, etc.

  mutable absl::Mutex mu_;

  // Local cache of results. Modified by the matches when a result is committed.
  uint16_t game_points_ ABSL_GUARDED_BY(mu_)= 0;
  uint16_t games_played_ ABSL_GUARDED_BY(mu_) = 0;
  uint16_t match_points_ ABSL_GUARDED_BY(mu_) = 0;

  absl::flat_hash_map<Player::Id, Player> opponents_ ABSL_GUARDED_BY(mu_);
  std::map<MatchId, Match> matches_ ABSL_GUARDED_BY(mu_);

  // TODO: Add a log of GRVs, warnings, etc.
};

bool operator==(const Player& l, const Player& r);
bool operator!=(const Player& l, const Player& r);
bool operator<(const Player& l, const Player& r);

}  // namespace internal
}  // namespace tcgtc

#endif // _TCGTC_PLAYER_H_
