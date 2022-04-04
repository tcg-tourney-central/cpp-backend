#include "definitions.h"

namespace tcgtc {

// Player ----------------------------------------------------------------------
Player Player::CreatePlayer(Options opts) { 
  Player p(std::shared_ptr<PlayerImpl>(new PlayerImpl(opts)));
  p->Init();
  return p;
}

uint64_t Player::id() const { return player_->id(); }

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

uint16_t MatchResult::match_points(const Player& p) const {
  if (!winner.has_value()) return 1;
  return p == *winner ? 3 : 0;
}

uint16_t MatchResult::game_points(const Player& p) const {
  if (!winner.has_value() || p == *winner) {
    return 3 * winner_games_won + games_drawn;
  }
  return 3 * winner_games_lost + games_drawn;
}

uint16_t MatchResult::games_played() const {
  return winner_games_won + winner_games_lost + games_drawn;
}



// Match -----------------------------------------------------------------------
Match Match::CreateBye(Player p, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(p, std::nullopt, id)));
  m->Init();

  // Immediately commit the result of the bye back to the player's cache.
  // MTR states that a Bye is considered won 2-0 in games.
  m->CommitResult(MatchResult{id, p, 2});
  return m;
}

Match Match::CreatePairing(Player a, Player b, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(a, b, id)));
  m->Init();
  return m;
}



// PlayerImpl ------------------------------------------------------------------
// TODO: Fill this out.
PlayerImpl::PlayerImpl(const Player::Options& opts)
  : id_(opts.id), last_name_(opts.last_name), first_name_(opts.first_name),
    username_(opts.username) {}

absl::Status PlayerImpl::CommitResult(const MatchResult& result,
                              const std::optional<MatchResult>& prev) {
  absl::MutexLock l(&mu_);
  if (matches_.find(result.id) == matches_.end()) {
    return absl::InvalidArgumentError(
        "Trying to commit a result for a match Player hasn't played.");
  }

  auto myself = this_player();
  games_played_ += result.games_played();
  game_points_ += result.game_points(myself);
  match_points_ += result.match_points(myself);

  // Remove any previously committed result values for this match.
  if (prev.has_value()) {
    // This should never actually happen...
    if (prev->id != result.id) {
      return absl::InvalidArgumentError(
                    "Trying to update a Player's match result with a "
                    "previous result for a different match.");
    }
    games_played_ -= prev->games_played();
    game_points_ -= prev->game_points(myself);
    match_points_ -= prev->match_points(myself);
  }
  return absl::OkStatus();
}

absl::Status PlayerImpl::AddMatch(Match m) {
  absl::MutexLock l(&mu_);
  auto me = this_player();
  if (!m->has_player(me)) {
    return absl::InvalidArgumentError(
                 "Trying to add a Match in which Player is not a participant.");
  }
  matches_.insert(std::make_pair(m->id(), m));
  if (!m->is_bye()) {
    auto opp = m->opponent(me);
    if (!opp.ok()) return opp.status();
    opponents_.insert(std::make_pair(opp->id(), *opp));
  }
  return absl::OkStatus();
}

bool PlayerImpl::has_played_opp(const Player& p) const {
  absl::MutexLock l(&mu_);
  return opponents_.find(p.id()) != opponents_.end();
}

PlayerImpl::TieBreakInfo PlayerImpl::ComputeBreakers() const {
  absl::MutexLock l(&mu_);
  auto me = this_player();
  Fraction omwp_sum(0);
  Fraction ogwp_sum(0);
  uint16_t num_opps = 0;
  for (const auto& [id, m] : matches_) {
    auto opp = m->opponent(me);
    if (!opp.ok()) continue;  // This match was a bye.
    omwp_sum += (*opp)->mwp().ApplyMtrBound();
    ogwp_sum += (*opp)->gwp().ApplyMtrBound();
    ++num_opps;
  }
  Fraction divisor(num_opps);
  TieBreakInfo out;
  out.opp_mwp = omwp_sum / divisor;
  out.gwp = gwp();
  out.opp_gwp = ogwp_sum / divisor;
  return out;
}



// PlayerImpl::TieBreakInfo ----------------------------------------------------
namespace {
using RefTup = std::tuple<const Fraction&, const Fraction&, const Fraction&>;
}  // namespace
bool operator==(const PlayerImpl::TieBreakInfo& l, 
                const PlayerImpl::TieBreakInfo& r) {
  RefTup lhs(l.opp_mwp, l.gwp, l.opp_gwp);
  RefTup rhs(r.opp_mwp, r.gwp, r.opp_gwp);
  return lhs == rhs;
}
bool operator!=(const PlayerImpl::TieBreakInfo& l,
                const PlayerImpl::TieBreakInfo& r) {
  return !(l == r);
}
bool operator<(const PlayerImpl::TieBreakInfo& l,
               const PlayerImpl::TieBreakInfo& r) {
  RefTup lhs(l.opp_mwp, l.gwp, l.opp_gwp);
  RefTup rhs(r.opp_mwp, r.gwp, r.opp_gwp);
  return lhs < rhs;
}


// MatchImpl -------------------------------------------------------------------
MatchImpl::MatchImpl(Player a, std::optional<Player> b, MatchId id)
  : id_(id), a_(a), b_(b) {}

void MatchImpl::Init() {
  // Initialize our ability to hand out Matches that are equivalent to ourselves.
  self_ptr_ = weak_from_this();

  // Add this match to the participating players as well.
  assert(a_->AddMatch(this_match()).ok());
  if (b_.has_value()) {
    assert((*b_)->AddMatch(this_match()).ok());
  }
}

absl::StatusOr<MatchResult> MatchImpl::confirmed_result() const {
  absl::MutexLock l(&mu_);
  // Result set by a judge, or if the match is a bye. Use it.
  if (committed_result_.has_value()) return *committed_result_;

  // TODO: Include names, match numbers, etc.
  if (!a_result_.has_value()) {
    return absl::FailedPreconditionError("Player A has not reported.");
  }
  if (!b_result_.has_value()) {
    return absl::FailedPreconditionError("Player B has not reported.");
  }

  if (*a_result_ != *b_result_) {
    return absl::FailedPreconditionError("Players reported different results.");
  }
  return *a_result_;
}

absl::StatusOr<Player> MatchImpl::opponent(const Player& p) const {
  if (!has_player(p)) {
    return absl::InvalidArgumentError("Player not in this match.");
  }
  if (is_bye()) return absl::InvalidArgumentError("This match is a Bye.");
  return p == a_ ? *b_ : a_;
}

absl::Status MatchImpl::PlayerReportResult(Player reporter, 
                                           MatchResult result) {
  // This shouldn't happen. Bye results are already committed.
  if (is_bye()) {
    return absl::InvalidArgumentError(
                  "Trying to report a match result for a Bye.");
  }

  // Only players can report for their matches.
  if (!has_player(reporter)) {
    return absl::InvalidArgumentError("Reporting player is not in this match.");
  }

  // Run other validity checks on the result.
  if (auto out = CheckResultValidity(result); !out.ok()) return out;

  absl::MutexLock l(&mu_);
  if (reporter == a_) {
    a_result_ = result;
  } else {
    b_result_ = result;
  }

  // If this report has confirmed the result, commit it back to the players.
  if (auto conf = confirmed_result(); conf.ok()) return CommitResult(*conf);

  // The report was successful even though we didn't confirm+commit.
  return absl::OkStatus();
}

absl::Status MatchImpl::JudgeSetResult(MatchResult result) {
  if (auto out = CheckResultValidity(result); !out.ok()) return out;
  return CommitResult(result);
}

// TODO: This validation should perhaps exist on parse, rather than here.
absl::Status MatchImpl::CheckResultValidity(const MatchResult& result) const {
  // Reported for the wrong match id.
  // TODO: Consider removing this?
  if (result.id != id_) {
    return absl::InvalidArgumentError("Reported MatchId is invalid.");
  }

  // Check draw validity.
  if (!result.winner.has_value()) {
    // No winner implies match was drawn. Check that the wins align.
    if (result.winner_games_won == result.winner_games_lost) {
      return absl::OkStatus();
    }
    return absl::InvalidArgumentError(
                  "Reported drawn match does not have equal game wins.");
  }

  // Check win validity.
  if (!has_player(*result.winner)) {
    return absl::InvalidArgumentError(
                  "Match report has winner not in this match.");
  }
  if (result.winner_games_won <= result.winner_games_lost) {
    // Match has a winner, but the match result doesn't align with that.
    return absl::InvalidArgumentError(
              "Match report has a winner but reported game score is invalid.");
  }
  return absl::OkStatus();
}

absl::Status MatchImpl::CommitResult(const MatchResult& result) {
  auto out = a_->CommitResult(result, committed_result_);
  if (!out.ok()) return out;
  if (b_.has_value()) {
    out = (*b_)->CommitResult(result, committed_result_);
    if (!out.ok()) return out;
  }
  committed_result_ = result;
  return absl::OkStatus();
}

}  // namespace tcgtc
