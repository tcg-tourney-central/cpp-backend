#include "definitions.h"

namespace tcgtc {

// Player ----------------------------------------------------------------------
Player Player::CreatePlayer(Options opts) { 
  return Player(std::make_shared<internal::PlayerImpl>(opts));
}

uint64_t Player::id() const { return player_->id(); }

bool Player::operator==(const Player& other) {
  if (this->id() != other->id()) return false;

  // TODO: Add an invariant check that `this->player_ == other.player_`
  return true;
}

bool Player::operator<(const Player& other) {
  auto* lhs = this->player_.get();
  auto* rhs = other.player_.get();
  uint16_t lhsmp = lhs->match_points();
  uint16_t rhsmp = rhs->match_points();
  if (lhsmp != rhsmp) return lhsmp < rhsmp;

  // Tie-break within equal match points.
  auto lhstb = std::make_tuple(lhs->opp_mwp(), lhs->gwp(), lhs->opp_gwp());
  auto rhstb = std::make_tuple(rhs->opp_mwp(), rhs->gwp(), rhs->opp_gwp());
  return lhstb < rhstb;
}



// MatchResult -----------------------------------------------------------------
bool MatchResult::operator==(const MatchResult& other) {
  return this->id == other.id &&
         this->winner == other.winner &&
         this->winner_games_won == other.winner_games_won &&
         this->winner_games_lost == other.winner_games_lost &&
         this->games_drawn == other.games_drawn;
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
  Match m(std::make_shared<internal::MatchImpl>(p, std::nullopt, id));
  m->AddMatchToPlayers();

  // Immediately commit the result of the bye back to the player's cache.
  m->CommitResult(MatchResult{.id = id,
                                   .winner = p,
                                   // MTR dictates that a Bye is considered won
                                   // 2-0.
                                   .winner_games_won = 2});
  return m;
}

Match Match::CreatePairing(Player a, Player b, MatchId id) {
  Match m(std::make_shared<internal::MatchImpl>(a, b, id));
  m->AddMatchToPlayers();
  return m;
}



namespace internal {  // -------------------------------------------------------

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

  // Remove any previously commited result values for this match.
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
  matches_.insert({m.id(), m});
  if (!m->is_bye()) {
    auto opp = match->opponent(this_player());
    opponents_[opp->id()] = *opp;
  }
}

bool has_played_opp(const Player& p) const {
  return opponents_.find(p.id()) != opponents.end();
}

Fraction PlayerImpl::opp_mwp() const {
  auto myself = this_player();
  Fraction sum(0);
  uint16_t num_opps = 0;
  for (const auto& [id, m] : matches_) {
    auto opp = m->opponent(myself);
    if (!opp.has_value()) continue;
    sum += (*opp)->mwp().ApplyMtrBound();
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
    sum += (*opp)->gwp().ApplyMtrBound();
    ++num_opps;
  }
  Fraction divisor(num_opps);
  return sum / divisor;
}



// MatchImpl -------------------------------------------------------------------
MatchImpl::MatchImpl(Player a, std::optional<Player> b, MatchId id)
  : id_(id), a_(a), b_(b) {}

void MatchImpl::AddMatchToPlayers() const {
  a_->AddMatch(this_match());
  if (b_.has_value()) b_->AddMatch(this_match());
}

std::optional<MatchResult> MatchImpl::confirmed_result() const {
  // Result set by a judge, or if the match is a bye. Use it.
  if (comitted_result_.has_value()) return committed_result_;

  // Result has not been confirmed.
  if (!a_result_.has_value() || !b_result_.has_value()) return std::nullopt;

  if (*a_result_ == *b_result_) return *a_result_;

  // TODO: Add an error message.
  return std::nullopt;
}

std::optional<Player> MatchImpl::opponent(const Player& p) {
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

void MatchImpl::CommitResult(const MatchResult& result) {
  if (!a_->CommitResult(result, commited_result_)) return false;
  if (b_.has_value()) {
    if (!(*b_)->CommitResult(result, commited_result_)) return false;
  }
  commited_result_ = result;
  return true;
}

}  // namespace internal
}  // namespace tcgtc
