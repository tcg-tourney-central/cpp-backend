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
  uint16_t lhsmp = l->match_points();
  uint16_t rhsmp = r->match_points();
  return lhsmp < rhsmp;

  /*
  if (lhsmp != rhsmp) return lhsmp < rhsmp;

  // Tie-break within equal match points.
  auto lhstb = std::make_tuple(lhs->opp_mwp(), lhs->gwp(), lhs->opp_gwp());
  auto rhstb = std::make_tuple(rhs->opp_mwp(), rhs->gwp(), rhs->opp_gwp());
  return lhstb < rhstb;
  */
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
PlayerImpl::PlayerImpl(const Player::Options& opts) {}

bool PlayerImpl::CommitResult(const MatchResult& result,
                              const std::optional<MatchResult>& prev) {
  if (matches_.find(result.id) == matches_.end()) return false;

  auto myself = this_player();
  games_played_ += result.games_played();
  game_points_ += result.game_points(myself);
  match_points_ += result.match_points(myself);

  // Remove any previously committed result values for this match.
  if (prev.has_value()) {
    // This should never actually happen...
    if (prev->id != result.id) return false;
    games_played_ -= prev->games_played();
    game_points_ -= prev->game_points(myself);
    match_points_ -= prev->match_points(myself);
  }
  return true;
}

void PlayerImpl::AddMatch(Match m) {
  matches_.insert(std::make_pair(m->id(), m));
  if (!m->is_bye()) {
    auto opp = m->opponent(this_player());
    opponents_.insert(std::make_pair(opp->id(), *opp));
  }
}

bool PlayerImpl::has_played_opp(const Player& p) const {
  return opponents_.find(p.id()) != opponents_.end();
}

Fraction PlayerImpl::opp_mwp() const {
  auto myself = this_player();
  Fraction sum(0);
  uint16_t num_opps = 0;
  for (const auto& [id, m] : matches_) {
    auto opp = m->opponent(myself);
    if (!opp.has_value()) continue;
    sum = sum + (*opp)->mwp().ApplyMtrBound();
    ++num_opps;
  }
  Fraction divisor(num_opps);
  return sum / divisor;
}

Fraction PlayerImpl::opp_gwp() const {
  auto myself = this_player();
  Fraction sum(0);
  uint16_t num_opps = 0;
  for (const auto& [id, m] : matches_) {
    auto opp = m->opponent(myself);
    if (!opp.has_value()) continue;
    sum = sum + (*opp)->gwp().ApplyMtrBound();
    ++num_opps;
  }
  Fraction divisor(num_opps);
  return sum / divisor;
}



// MatchImpl -------------------------------------------------------------------
MatchImpl::MatchImpl(Player a, std::optional<Player> b, MatchId id)
  : id_(id), a_(a), b_(b) {}

void MatchImpl::Init() {
  // Initialize our ability to hand out Matches that are equivalent to ourselves.
  self_ptr_ = weak_from_this();

  // Add this match to the participating players as well.
  a_->AddMatch(this_match());
  if (b_.has_value()) (*b_)->AddMatch(this_match());
}

std::optional<MatchResult> MatchImpl::confirmed_result() const {
  // Result set by a judge, or if the match is a bye. Use it.
  if (committed_result_.has_value()) return committed_result_;

  // Result has not been confirmed.
  if (!a_result_.has_value() || !b_result_.has_value()) return std::nullopt;

  if (*a_result_ == *b_result_) return *a_result_;

  // TODO: Add an error message.
  return std::nullopt;
}

std::optional<Player> MatchImpl::opponent(const Player& p) const {
  // TODO: Consider a validity check that p is in this match.
  if (is_bye()) return std::nullopt;
  return p == a_ ? *b_ : a_;
}

bool MatchImpl::PlayerReportResult(Player reporter, MatchResult result) {
  // This shouldn't happen. Bye results are already committed.
  if (is_bye()) return false;

  // Only players can report for their matches.
  if (!has_player(reporter)) return false;

  // Run other validity checks on the result.
  if (!CheckResultValidity(result)) return false;

  if (reporter == a_) {
    a_result_ = result;
  } else {
    b_result_ = result;
  }

  // If this report has confirmed the result, commit it back to the players.
  if (auto conf = confirmed_result(); conf.has_value()) {
    return CommitResult(*conf);
  }

  // The report was successful whether or not we confirmed/committed.
  return true;
}

bool MatchImpl::JudgeSetResult(MatchResult result) {
  return CheckResultValidity(result) && CommitResult(result);
}

// TODO: This validation should perhaps exist on parse, rather than here.
bool MatchImpl::CheckResultValidity(const MatchResult& result) const {
  // Reported for the wrong match id.
  // TODO: Consider removing this?
  if (result.id != id_) return false;

  // Check draw validity.
  if (!result.winner.has_value()) {
    // No winner implies match was drawn. Check that the wins align.
    return result.winner_games_won == result.winner_games_lost;
  }

  // Check win validity.
  if (!has_player(*result.winner)) return false;
  if (result.winner_games_won <= result.winner_games_lost) {
    // Match has a winner, but the match result doesn't align with that.
    return false;
  }
  return true;
}

bool MatchImpl::CommitResult(const MatchResult& result) {
  if (!a_->CommitResult(result, committed_result_)) return false;
  if (b_.has_value()) {
    if (!(*b_)->CommitResult(result, committed_result_)) return false;
  }
  committed_result_ = result;
  return true;
}

}  // namespace tcgtc
