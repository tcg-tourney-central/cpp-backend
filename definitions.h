#ifndef _TCGTC_DEFINITIONS_H_
#define _TCGTC_DEFINITIONS_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "container-class.h"
#include "fraction.h"
#include "match-id.h"
#include "util.h"

namespace tcgtc {
namespace internal {
class PlayerImpl;
class MatchImpl;
}  // namespace internal

// Thin wrapper around a shared_ptr, so the Player class is copyable, with a
// canonical copy shared by e.g. Tournaments, Matches, etc.
class Player : public ContainerClass<internal::PlayerImpl> {
  friend class ::tcgtc::internal::PlayerImpl;
  using PlayerImpl = ::tcgtc::internal::PlayerImpl;
 public:
  using Id = uint64_t;

  struct Options {
    Id id;
    std::string first_name;
    std::string last_name;
    std::string username;
  };
  static Player CreatePlayer(Options opts);

  // TODO: Remove this in favor of indirection
  Id id() const;

 private:
  explicit Player(std::shared_ptr<PlayerImpl> impl)
    : ContainerClass(std::move(impl)) {}
};
bool operator==(const Player& l, const Player& r);
bool operator<(const Player& l, const Player& r);


struct MatchResult {
  MatchId id;
  // May be empty if the match was drawn.
  std::optional<Player> winner;
  uint16_t winner_games_won = 0;
  uint16_t winner_games_lost = 0;
  uint16_t games_drawn = 0;

  uint16_t match_points(const Player& p) const;
  uint16_t game_points(const Player& p) const;
  uint16_t games_played() const;
};
bool operator==(const MatchResult& l, const MatchResult& r);
bool operator!=(const MatchResult& l, const MatchResult& r);


// Thin wrapper around a shared_ptr, so the Player class is copyable, with a
// canonical copy shared by e.g. Tournaments, Players, etc.
//
// TODO: Generalize this so we can take teams and not just players.
class Match : public ContainerClass<internal::MatchImpl> {
  friend class ::tcgtc::internal::MatchImpl;
  using MatchImpl = ::tcgtc::internal::MatchImpl;
 public:
  static Match CreateBye(Player p, MatchId id);
  static Match CreatePairing(Player a, Player b, MatchId id);

 private:
  explicit Match(std::shared_ptr<MatchImpl> impl)
    : ContainerClass(std::move(impl)) {}
};


namespace internal {
// IMPLEMENTATION --------------------------------------------------------------



class PlayerImpl : public MemoryManagedImplementation<PlayerImpl> {
 public:
  // Cannot be called during the constructor as `weak_from_this()` is not
  // available, so we called it immediately after.
  void Init() { InitSelfPtr(); } 

  // TODO: Probably determined by the player's persistent stored ID, but can be
  // per-tournament.
  Player::Id id() const { return id_; }
  const std::string& last_name() const { return last_name_; }
  const std::string& first_name() const { return first_name_; }
  const std::string& username() const { return username_; }

  std::string ErrorStringId() const {
    return absl::StrCat("Player (", display_name_, ")");
  }

  bool has_played_opp(const Player& p) const;

  uint16_t match_points() const { return match_points_; }
  Fraction mwp() const { 
    return Fraction(match_points_, 3 * matches_played()).ApplyMtrBound();
  }
  Fraction gwp() const { 
    return Fraction(game_points_, 3 * games_played_).ApplyMtrBound();
  }

  struct TieBreakInfo {
    Fraction opp_mwp;
    Fraction gwp;
    Fraction opp_gwp;
  };
  TieBreakInfo ComputeBreakers() const ABSL_LOCKS_EXCLUDED(mu_);

  // Commit a result, and if there is a previous result for that match, erase
  // that from the cache.
  absl::Status CommitResult(const MatchResult& result,
                    const std::optional<MatchResult>& prev)
    ABSL_LOCKS_EXCLUDED(mu_);

  absl::Status AddMatch(Match m) ABSL_LOCKS_EXCLUDED(mu_);

 private:
  // We want these implementations created only by the container classes.
  friend class Player;
  explicit PlayerImpl(const Player::Options& opts);
  Player this_player() const { return Player(self_copy()); }

  uint16_t matches_played() const { return matches_.size(); }

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

  std::map<Player::Id, Player> opponents_ ABSL_GUARDED_BY(mu_);
  std::map<MatchId, Match> matches_ ABSL_GUARDED_BY(mu_);

  // TODO: Add a log of GRVs, warnings, etc.
};
bool operator==(const PlayerImpl::TieBreakInfo& l, 
                const PlayerImpl::TieBreakInfo& r);
bool operator!=(const PlayerImpl::TieBreakInfo& l,
                const PlayerImpl::TieBreakInfo& r);
bool operator<(const PlayerImpl::TieBreakInfo& l,
               const PlayerImpl::TieBreakInfo& r);



class MatchImpl : public MemoryManagedImplementation<MatchImpl> {
 public:
  // Cannot be called during the constructor as `weak_from_this()` is not
  // available, so we called it immediately after.
  void Init();

  bool is_bye() const { return !b_.has_value(); }
  MatchId id() const { return id_; }
  bool has_player(const Player& p) const {
    return a_ == p || (b_.has_value() && *b_ == p);
  }
  absl::StatusOr<Player> opponent(const Player& p) const;

  // Only returns a value if both players have reported the same result.
  //
  // TODO: Use absl::StatusOr<MatchResult>
  absl::StatusOr<MatchResult> confirmed_result() const;

  // Returns false if the reporter or reported result is invalid.
  absl::Status PlayerReportResult(Player reporter, MatchResult result);

  // Returns false if the result is invalid. If there is already a committed
  // result, handles diffing the game scores from the previous committed
  // values.
  absl::Status JudgeSetResult(MatchResult result);

 private:
  // We want these implementations created only by the container classes.
  friend class Match;
  MatchImpl(Player a, std::optional<Player> b, MatchId id);
  Match this_match() const { return Match(self_copy()); }

  // Commits the result back to the Player(s), updating their matches/games
  // played and match/game points.
  absl::Status CommitResult(const MatchResult& result)
    ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_);

  absl::Status CheckResultValidity(const MatchResult& result) const;

  const MatchId id_;
  const Player a_;
  const std::optional<Player> b_;

  // Can't store a std::shared_ptr to ourselves or this would be a memory leak,
  // used to allow `this_match()` to be const-qualified. Cannot be initialized
  // in the constructor, as `weak_from_this()` is unavailable, but otherwise is
  // effectively const.
  std::weak_ptr<MatchImpl> self_ptr_;

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

#endif // _TCGTC_DEFINITIONS_H_
