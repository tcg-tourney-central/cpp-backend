#ifndef _TCGTC_TOURNAMENT_H_
#define _TCGTC_TOURNAMENT_H_

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "definitions.h"
#include "match-id.h"
#include "util.h"

namespace tcgtc {
namespace internal {
class RoundImpl;
}  // namespace internal

// Currently only support the following bracket sizes.
enum class BracketSize : uint8_t {
  kNoBracket = 0,
  kTop2 = 2,
  kTop4 = 4,
  kTop6 = 6,
  kTop8 = 8,
};

class Round {
  friend class ::tcgtc::internal::RoundImpl;
  using RoundImpl = ::tcgtc::internal::RoundImpl;
 public:
  // TODO: Consolidate this with MatchId.round;
  using Id = uint8_t;

  RoundImpl* get() const { return round_.get(); }
  RoundImpl* operator->() const { return get(); }
  RoundImpl& operator*() const { return *get(); }

 private:
  explicit Round(std::shared_ptr<RoundImpl> round) : round_(round) {}

  std::shared_ptr<RoundImpl> round_;
};

class Tournament {
 public:
  struct Options {
    uint8_t swiss_rounds = 0;
    BracketSize bracket = BracketSize::kNoBracket;

    // First table number to use for the tournament.
    uint32_t table_one = 1;
  };
  explicit Tournament(const Options& opts);

  // Interact with this tournament ---------------------------------------------
  //
  // TODO: For "weird" requests, like adding a player in Round 5, or setting a
  // result after it has been confirmed by the players, add a "pending/verify"
  // mechanism. We will hand out tokens for these requests, and if they confirm
  // (for that token) we will commit it later.
  absl::Status AddPlayer(const Player::Options& info);
  absl::Status DropPlayer(Player::Id player);

  // Returns an error status if the result is for a round that is not current.
  absl::Status ReportResult(Player::Id player, const MatchResult& result);
  absl::Status JudgeSetResult(const MatchResult& result);


  // For now, require a request to pair the next round, but we can maybe
  // consider doing this automagically when all pairings are received in the
  // previous round.
  absl::StatusOr<Round> PairNextRound();


  // Accessors for information about the running tournament.
  absl::StatusOr<Player> GetPlayer(Player::Id player) const
      ABSL_LOCKS_EXCLUDED(mu_);
  absl::StatusOr<Match> GetMatch(MatchId player) const
      ABSL_LOCKS_EXCLUDED(mu_);
  absl::StatusOr<Round> CurrentRound() const  // Error if tournament unstarted.
      ABSL_LOCKS_EXCLUDED(mu_);

  /*
  // Active (pairable) players, by match points.
  std::map<uint32_t, std::vector<Player>> ActivePlayers() const
      ABSL_LOCKS_EXCLUDED(mu_);
  */

 private:
  absl::Status AddPlayerLocked(const Player::Options& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);
  absl::StatusOr<Player> GetPlayerLocked(Player::Id player) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);
  absl::StatusOr<Match> GetMatchLocked(MatchId player) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);
  absl::StatusOr<Round> GetRoundLocked(Round::Id id) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);
  absl::StatusOr<Round> CurrentRoundLocked() const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);

  const Options opts_;

  mutable absl::Mutex mu_;
  // Canonical store of player information for all players in the tournament.
  absl::flat_hash_map<Player::Id, Player> players_ ABSL_GUARDED_BY(mu_);
  absl::flat_hash_map<Player::Id, Player> active_players_ ABSL_GUARDED_BY(mu_);
  absl::flat_hash_map<Player::Id, Player> dropped_players_ ABSL_GUARDED_BY(mu_);
  absl::flat_hash_map<MatchId, Match> matches_ ABSL_GUARDED_BY(mu_);
  std::map<Round::Id, Round> rounds_ ABSL_GUARDED_BY(mu_);
};

namespace internal {

class RoundImpl : public std::enable_shared_from_this<RoundImpl> {
 public:
  struct Options {
    Round::Id id;
    Tournament* parent;
  };
  static Round CreateRound(const Options& opts);

  // Initializes this round, including generating pairings.
  absl::Status Init();

  std::string ErrorStringId() const;

  absl::Status CommitMatchResult(Match m);
  absl::Status JudgeSetResult(Match m);

  bool RoundComplete() const {
    absl::MutexLock l(&mu_);
    return outstanding_matches_.empty();
  }
   
 private:
  explicit RoundImpl(const Options& opts);
  Round this_round() const { return Round(self_ptr_.lock()); }

  absl::Status GeneratePairings();

  const Round::Id id_;
  Tournament* const parent_;

  // Can't store a std::shared_ptr to ourselves or this would be a memory leak,
  // used to allow `this_round()` to be const-qualified. Cannot be initialized
  // in the constructor, as `weak_from_this()` is unavailable, but otherwise is
  // effectively const.
  std::weak_ptr<RoundImpl> self_ptr_;

  mutable absl::Mutex mu_;

  absl::flat_hash_map<MatchId, Match> outstanding_matches_ ABSL_GUARDED_BY(mu_);
  absl::flat_hash_map<MatchId, Match> reported_matches_ ABSL_GUARDED_BY(mu_);
};

}  // namespace internal
}  // namespace tcgtc

#endif // _TCGTC_TOURNAMENT_H_