#ifndef _TCGTC_MATCH_H_
#define _TCGTC_MATCH_H_

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

namespace tcgtc {
namespace internal {

class MatchImpl : public MemoryManagedImplementation<MatchImpl> {
 public:
  static Match CreateBye(Player p, MatchId id);
  static Match CreatePairing(Player a, Player b, MatchId id);

  bool is_bye() const { return !b_.has_value(); }
  MatchId id() const { return id_; }

  // TODO: Consolidate these two.
  bool has_player(const Player& p) const;
  bool has_player(Player::Id p) const;
  absl::StatusOr<Player> opponent(const Player& p) const;

  // Only returns a value if both players have reported the same result.
  absl::StatusOr<MatchResult> confirmed_result() const;

  // Returns false if the reporter or reported result is invalid.
  absl::Status PlayerReportResult(Player reporter, MatchResult result);

  // Returns false if the result is invalid. If there is already a committed
  // result, handles diffing the game scores from the previous committed
  // values.
  absl::Status JudgeSetResult(MatchResult result);

 private:
  MatchImpl(Player a, std::optional<Player> b, MatchId id);

  // Init() and this_match() can only be called after the constructor. See
  // documentation on std::enable_shared_from_this.
  void Init();
  Match this_match() const { return Match(self_copy()); }

  // Commits the result back to the Player(s), updating their matches/games
  // played and match/game points.
  absl::Status CommitResult(const MatchResult& result)
    ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);

  absl::Status CheckResultValidity(const MatchResult& result) const;

  const MatchId id_;
  const Player a_;
  const std::optional<Player> b_;

  mutable absl::Mutex mu_;

  // Reported results, per player.
  std::optional<MatchResult> a_result_ ABSL_GUARDED_BY(mu_);
  std::optional<MatchResult> b_result_ ABSL_GUARDED_BY(mu_);

  // Set either by a judge, if the match is a bye, or when players agree on a
  std::optional<MatchResult> committed_result_ ABSL_GUARDED_BY(mu_);

  // TODO: Add a log of extensions, GRVs, etc.
};

}  // namespace internal
}  // namespace tcgtc

#endif // _TCGTC_MATCH_H_
