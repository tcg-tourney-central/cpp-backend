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
  PlayerImpl* operator->() { return get(); }
  PlayerImpl& operator*() { return *get(); }
  PlayerImpl* operator->() const { return get(); }
  PlayerImpl& operator*() const { return *get(); }

 private:
  std::shared_ptr<PlayerImpl> player_;
 public:
  // DO NOT USE, PLEASE. FIGHTING WITH COMPILER.
  explicit Player(std::shared_ptr<PlayerImpl> player);

  Player() = delete;
  Player(const Player& other) = default;
  Player& operator=(const Player& other) = default;
  Player(Player&& other) = default;
  Player& operator=(Player&& other) = default;
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
  MatchImpl* operator->() { return get(); }
  MatchImpl& operator*() { return *get(); }
  MatchImpl* operator->() const { return get(); }
  MatchImpl& operator*() const { return *get(); }

 private:
  std::shared_ptr<MatchImpl> match_;
 public:
  // DO NOT USE, PLEASE. FIGHTING WITH COMPILER.
  explicit Match(std::shared_ptr<MatchImpl> match);

  Match() = delete;
  Match(const Match& other) = default;
  Match& operator=(const Match& other) = default;
  Match(Match&& other) = default;
  Match& operator=(Match&& other) = default;
};



// IMPLEMENTATION --------------------------------------------------------------

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class PlayerImpl : public std::enable_shared_from_this<PlayerImpl> {
 public:
  // TODO: Probably determined by the player's persistent stored ID, but can be
  // per-tournament.
  Player::Id id() { return id_; }
  std::string& last_name() { return last_name_; }
  std::string& first_name() { return first_name_; }
  std::string& username() { return username_; }

  uint16_t match_points() { return match_points_; }
  Fraction mwp() { return Fraction(match_points_, 3 * matches_played()); }
  Fraction gwp() { return Fraction(game_points_, 3 * games_played_); }

  // This player's averaged Opponent Match Win %
  Fraction opp_mwp();
  // This player's averaged Opponent Game Win %
  Fraction opp_gwp();

  // Commit a result, and if there is a previous result for that match, erase
  // that from the cache.
  bool CommitResult(const MatchResult& result,
                    const std::optional<MatchResult>& prev);

  bool has_played_opp(const Player& p);

  void AddMatch(Match m);

 private:
  explicit PlayerImpl(const Player::Options& opts);
  Player this_player() { return Player(shared_from_this()); }

  uint16_t matches_played() { return matches_.size(); }

  Player::Id id_;
  std::string last_name_;
  std::string first_name_;
  std::string username_;  // e.g. for online tournaments.

  // Local cache of results. Modified by the matches when a result is committed.
  uint16_t game_points_ = 0;
  uint16_t games_played_ = 0;
  uint16_t match_points_ = 0;

  std::map<Player::Id, Player> opponents_;
  std::map<MatchId, Match> matches_;

  // TODO: Add a log of GRVs, warnings, etc.

  // We want implementations created only by the outer container classes, so we
  // hide the constructor.
  friend class Player;
};

// TODO: Add synchronization, this will be important for performant
// match-reporting, so we don't hammer a "global" (per-tournament) Mutex.
class MatchImpl : public std::enable_shared_from_this<MatchImpl> {
 public:
  // Cannot be called during the constructor as `shared_from_this()` is not
  // available.
  void AddMatchToPlayers();

  bool is_bye() { return !b_.has_value(); }
  MatchId id() { return id_; }

  bool has_player(const Player& p) {
    return a_ == p || (b_.has_value() && *b_ == p);
  }

  std::optional<Player> opponent(const Player& p);

  // Only returns a value if both players have reported the same result.
  //
  // TODO: Use absl::StatusOr<MatchResult>
  std::optional<MatchResult> confirmed_result();

  // Returns false if the reporter or reported result is invalid.
  bool PlayerReportResult(Player reporter, MatchResult result);

  // Returns false if the result is invalid. If there is already a committed
  // result, handles diffing the game scores from the previous committed
  // values.
  bool JudgeSetResult(MatchResult result);

 private:
  MatchImpl(Player a, std::optional<Player> b, MatchId id);
  Match this_match() { return Match(shared_from_this()); }

  // Commits the result back to the Player(s), updating their matches/games
  // played and match/game points.
  bool CommitResult(const MatchResult& result);

  // TODO: Migrate to absl::Status
  bool CheckResultValidity(const MatchResult& result);

  MatchId id_;

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

}  // namespace tcgtc

#endif // _TCGTC_DEFINITIONS_H_
