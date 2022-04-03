#ifndef _TCGTC_DEFINITIONS_H_
#define _TCGTC_DEFINITIONS_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "fraction.h"
#include "match-id.h"

namespace tcgtc {

// Forward declaration for internal implementations of tightly linked types.
// Matches have players, Players have Matches, Matches have Results, Players
// have Results, 
namespace internal {
class PlayerImpl;
class MatchImpl;
}  // namespace internal

// Thin wrapper around a shared_ptr, so the Player class is copyable, with a
// canonical copy shared by e.g. Tournaments, Matches, etc.
class Player {
 public:
  struct Options {
    // Use designated initializers, must be provided.
    const uint64_t id;
    const std::string first_name;
    const std::string last_name;
    std::string username;
  };
  static Player CreatePlayer(Options opts);

  // TODO: external (forwarding) API.
  uint64_t id() const;

  bool operator==(const Player& other) const;
  bool operator<(const Player& other) const;  // MTR standings order.

 private:
  explicit Player(std::shared_ptr<internal::PlayerImpl> player)
    : player_(player) {}

  std::shared_ptr<internal::PlayerImpl> player_;

  // Allow Matches to acces the internal Impl pointer.
  friend class Match;
  friend class internal::PlayerImpl;
};

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

  bool operator==(const MatchResult& other) const;
};

// Thin wrapper around a shared_ptr, so the Player class is copyable, with a
// canonical copy shared by e.g. Tournaments, Players, etc.
//
// TODO: Generalize this so we can take teams and not just players.
class Match {
 public:
  static Match CreateBye(Player p, MatchId id);
  static Match CreatePairing(Player a, Player b, MatchId id);

  // TODO: external (forwarded) API.
 private:
  // TODO: Ensure invariant that match is never null.
  explicit Match(std::shared_ptr<internal::MatchImpl> match) : match_(match) {}

  std::shared_ptr<internal::MatchImpl> match_;
};

namespace internal {

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class PlayerImpl : public std::enable_shared_from_this<PlayerImpl> {
 public:
  // TODO: Probably determined by the player's persistent stored ID, but can be
  // per-tournament.
  uint64_t id() const { return id_; }
  const std::string& last_name() const { return last_name_; }
  const std::string& last_name() const { return first_name_; }
  const std::string& username() const { return username_; }

  uint16_t match_points() const { return match_points_; }
  Fraction mwp() const { return Fraction(match_points_, 3 * matches_played()); }
  Fraction gwp() const { return Fraction(game_points_, 3 * games_played_); }

  // This player's averaged Opponent Match Win %
  Fraction opp_mwp() const;
  // This player's averaged Opponent Game Win %
  Fraction opp_gwp() const;

  // Commit a result, and if there is a previous result for that match, erase
  // that from the cache.
  bool CommitResult(const MatchResult& result,
                    const std::optional<MatchResult>& prev);

 private:
  explicit PlayerImpl(const Player::Options& opts);

  uint16_t matches_played() const { return matches_.size(); }

  Player this_player() const { return Player(this->shared_from_this()); }

  const uint64_t id_;
  const std::string last_name_;
  const std::string first_name_;
  const std::string username_;  // e.g. for online tournaments.

  // Local cache of results. Modified by the matches when a result is committed.
  uint16_t game_points_ = 0;
  uint16_t games_played_ = 0;
  uint16_t match_points_ = 0;

  std::vector<Match> matches_;

  // TODO: Add a log of GRVs, warnings, etc.

  // We want implementations created only by the outer container classes, so we
  // hide the constructor.
  friend class Player;
};

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class MatchImpl {
 public:
  bool is_bye() const { return !b_.has_value(); }
  MatchId id() const { return id_; }

  bool has_player(const Player& p) const {
    return a_ == p || (b_.has_value() && *b_ == p);
  }

  std::optional<Player> opponent(const Player& p);

  // Only returns a value if both players have reported the same result.
  //
  // TODO: Use absl::StatusOr<MatchResult>
  std::optional<MatchResult> confirmed_result() const;

  // Returns false if the reporter or reported result is invalid.
  bool PlayerReportResult(Player reporter, MatchResult result);

  // Returns false if the result is invalid. If there is already a committed
  // result, handles diffing the game scores from the previous committed
  // values.
  bool JudgeSetResult(MatchResult result);

 private:
  MatchImpl(Player a, std::optional<Player> b, MatchId id);

  // Commits the result back to the Player(s), updating their matches/games
  // played and match/game points.
  void CommitResult(const MatchResult& result);

  // TODO: Migrate to absl::Status
  bool CheckResultValidity(const MatchResult& result) const;

  const MatchId id_;

  Player a_;
  std::optional<Player> b_;

  // Reported results, per player.
  std::optional<MatchResult> a_result_;
  std::optional<MatchResult> b_result_;

  // Set either by a judge, if the match is a bye, or when players agree on a
  std::optional<MatchResult> committed_result_;

  // TODO: Add a log of extensions, GRVs, etc.

  // We want implementations created only by the outer container classes, so we
  // hide the constructor.
  friend class Match;
};

}  // namespace internal
}  // namespace tcgtc

#endif // _TCGTC_DEFINITIONS_H_
