#ifndef _TCGTC_DEFINITIONS_H_
#define _TCGTC_DEFINITIONS_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "fraction.h"
#include "match-id.h"

namespace tcgtc {

// Forward declaration for internal implementations of tightly linked types.
// Matches have players, Players have Matches, Matches have Results, Players
// have Results, 
class PlayerImpl;
class MatchImpl;

// Thin wrapper around a shared_ptr, so the Player class is copyable, with a
// canonical copy shared by e.g. Tournaments, Matches, etc.
class Player {
 public:
  using Id = uint64_t;

  struct Options {
    Id id;
    std::string first_name;
    std::string last_name;
    std::string username;
  };
  static Player CreatePlayer(Options opts);

  // TODO: external (forwarding) API.
  Id id() const;

  PlayerImpl* get() const { return player_.get(); }
  PlayerImpl* operator->() const { return get(); }
  PlayerImpl& operator*() const { return *get(); }

 private:
  std::shared_ptr<PlayerImpl> player_;
 public:
  // DO NOT USE, FIGHTING WITH COMPILER FRIEND DECLARATIONS.
  explicit Player(std::shared_ptr<PlayerImpl> player) : player_(player) {}
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
class Match {
 public:
  static Match CreateBye(Player p, MatchId id);
  static Match CreatePairing(Player a, Player b, MatchId id);

  MatchImpl* get() const { return match_.get(); }
  MatchImpl* operator->() const { return get(); }
  MatchImpl& operator*() const { return *get(); }

 private:
  std::shared_ptr<MatchImpl> match_;
 public:
  // DO NOT USE, FIGHTING WITH COMPILER FRIEND DECLARATIONS.
  explicit Match(std::shared_ptr<MatchImpl> match) : match_(match) {}
};



// IMPLEMENTATION --------------------------------------------------------------

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class PlayerImpl : public std::enable_shared_from_this<PlayerImpl> {
 public:
  void Init() { self_ptr_ = weak_from_this(); } 

  // TODO: Probably determined by the player's persistent stored ID, but can be
  // per-tournament.
  Player::Id id() { return id_; }
  const std::string& last_name() { return last_name_; }
  const std::string& first_name() { return first_name_; }
  const std::string& username() { return username_; }

  uint16_t match_points() const { return match_points_; }
  Fraction mwp() const { return Fraction(match_points_, 3 * matches_played()); }
  Fraction gwp() const { return Fraction(game_points_, 3 * games_played_); }

  // This player's averaged Opponent Match Win %
  Fraction opp_mwp() const;
  // This player's averaged Opponent Game Win %
  Fraction opp_gwp() const;

  bool has_played_opp(const Player& p) const;

  // Commit a result, and if there is a previous result for that match, erase
  // that from the cache.
  bool CommitResult(const MatchResult& result,
                    const std::optional<MatchResult>& prev);

  void AddMatch(Match m);

 private:
  // We want these implementations created only by the container classes.
  friend class Player;
  explicit PlayerImpl(const Player::Options& opts);

  // Neither Init or this_player can be called within the constructor as they
  // rely on std::shared_from_this
  Player this_player() const { return Player(self_ptr_.lock()); }

  uint16_t matches_played() const { return matches_.size(); }

  Player::Id id_;
  std::string last_name_;
  std::string first_name_;
  std::string username_;  // e.g. for online tournaments.

  // Can't store a std::shared_ptr to ourselves or this would be a memory leak.
  std::weak_ptr<PlayerImpl> self_ptr_;

  // Local cache of results. Modified by the matches when a result is committed.
  uint16_t game_points_ = 0;
  uint16_t games_played_ = 0;
  uint16_t match_points_ = 0;

  std::map<Player::Id, Player> opponents_;
  std::map<MatchId, Match> matches_;

  // TODO: Add a log of GRVs, warnings, etc.
};

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class MatchImpl : public std::enable_shared_from_this<MatchImpl> {
 public:
  // Cannot be called during the constructor as `shared_from_this()` is not
  // available, so we called it immediately after.
  void Init();

  bool is_bye() const { return !b_.has_value(); }
  MatchId id() const { return id_; }
  bool has_player(const Player& p) const {
    return a_ == p || (b_.has_value() && *b_ == p);
  }
  std::optional<Player> opponent(const Player& p) const;

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
  // We want these implementations created only by the container classes.
  friend class Match;
  MatchImpl(Player a, std::optional<Player> b, MatchId id);
  Match this_match() const { return Match(self_ptr_.lock()); }

  // Commits the result back to the Player(s), updating their matches/games
  // played and match/game points.
  bool CommitResult(const MatchResult& result);

  // TODO: Migrate to absl::Status
  bool CheckResultValidity(const MatchResult& result) const;

  const MatchId id_;

  const Player a_;
  const std::optional<Player> b_;

  // Can't store a std::shared_ptr to ourselves or this would be a memory leak,
  // used to allows `this_match()` to be const-qualified.
  std::weak_ptr<MatchImpl> self_ptr_;

  // Reported results, per player.
  std::optional<MatchResult> a_result_;
  std::optional<MatchResult> b_result_;

  // Set either by a judge, if the match is a bye, or when players agree on a
  std::optional<MatchResult> committed_result_;

  // TODO: Add a log of extensions, GRVs, etc.
};

}  // namespace tcgtc

#endif // _TCGTC_DEFINITIONS_H_
