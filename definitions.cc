#include "definitions.h"

#include "match.h"
#include "match-id.h"
#include "match-result.h"
#include "player.h"
#include "round.h"
#include "util.h"

namespace tcgtc {

// Player ----------------------------------------------------------------------
Player Player::CreatePlayer(Options opts) { 
  Player p(std::shared_ptr<PlayerImpl>(new PlayerImpl(opts)));
  p->Init();
  return p;
}

uint64_t Player::id() const { return me().id(); }

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



// Match -----------------------------------------------------------------------
Match Match::CreateBye(Player p, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(p, std::nullopt, id)));
  m->Init();

  // Immediately commit the result of the bye back to the player's cache.
  // MTR states that a Bye is considered won 2-0 in games.
  m->CommitResult(MatchResult{id, p->id(), 2});
  return m;
}

Match Match::CreatePairing(Player a, Player b, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(a, b, id)));
  m->Init();
  return m;
}

absl::StatusOr<Tournament> Tournament::View::Lock() {
  if (auto ptr = lock(); ptr != nullptr) return Tournament(ptr);
  return Err("Viewed Tournament has been torn down.");
}


namespace internal {
namespace {
std::string ComputeDisplayName(const Player::Options& opts) {
  if (opts.last_name.empty() || opts.first_name.empty()) {
    assert(!opts.username.empty());
    return opts.username;
  }
  return absl::StrCat(opts.last_name, ",", opts.first_name);
}
}  // namespace


// PlayerImpl ------------------------------------------------------------------
PlayerImpl::PlayerImpl(const Player::Options& opts)
  : id_(opts.id), last_name_(opts.last_name), first_name_(opts.first_name),
    username_(opts.username), display_name_(ComputeDisplayName(opts)) {}

absl::Status PlayerImpl::CommitResult(const MatchResult& result,
                              const std::optional<MatchResult>& prev) {
  absl::MutexLock l(&mu_);
  if (matches_.find(result.id) == matches_.end()) {
    return Err("Trying to commit result for ", result.id.ErrorStringId(),
               " ", ErrorStringId()," hasn't played.");
  }
  if (prev.has_value() && prev->id != result.id) {
    return Err("Trying to update ", ErrorStringId(), " ", 
               prev->id.ErrorStringId() ," result with result for a different ",
               result.id.ErrorStringId());
  }

  games_played_ += result.games_played();
  game_points_ += result.game_points(id_);
  match_points_ += result.match_points(id_);

  // Remove any previously committed result values for this match.
  if (prev.has_value()) {
    games_played_ -= prev->games_played();
    game_points_ -= prev->game_points(id_);
    match_points_ -= prev->match_points(id_);
  }
  return absl::OkStatus();
}

absl::Status PlayerImpl::AddMatch(Match m) {
  absl::MutexLock l(&mu_);
  auto me = this_player();
  if (!m->has_player(me)) {
    return Err("Trying to add ", m->id().ErrorStringId(), " in which ",
               ErrorStringId(), " is not a participant.");
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

TieBreakInfo PlayerImpl::ComputeBreakers() const {
  absl::MutexLock l(&mu_);
  auto me = this_player();
  Fraction omwp_sum(0);
  Fraction ogwp_sum(0);
  uint16_t num_opps = 0;
  for (const auto& [id, m] : matches_) {
    assert(id == m->id());

    // Elimination rounds are not part of tie-breakers.
    if (id.bracket_match()) continue;

    // Have an opponent iff. this match isn't a bye, so include it in tie-break.
    if (auto opp = m->opponent(me); opp.ok()) {
      // mwp() and gwp() already apply the MTR Bound of 1/3 (33.3%) minimum.
      omwp_sum += (*opp)->mwp();
      ogwp_sum += (*opp)->gwp();
      ++num_opps;
    }
  }
  // Return a default value of 1.
  // TODO: Make sure this aligns with MTR. Probably just an R1/2 corner case.
  TieBreakInfo out;
  out.match_points = match_points_;
  if (num_opps == 0) {
    static const Fraction kOne(1);
    out.opp_mwp = kOne;
    out.gwp = kOne;
    out.opp_gwp = kOne;
  } else {
    Fraction divisor(num_opps);
    out.opp_mwp = omwp_sum / divisor;
    out.gwp = gwp();
    out.opp_gwp = ogwp_sum / divisor;
  }
  return out;
}



// MatchImpl -------------------------------------------------------------------
MatchImpl::MatchImpl(Player a, std::optional<Player> b, MatchId id)
  : id_(id), a_(a), b_(b) {}

void MatchImpl::Init() {
  InitSelfPtr();

  // Add this match to the participating players as well.
  assert(a_->AddMatch(this_match()).ok());
  if (b_.has_value()) {
    assert((*b_)->AddMatch(this_match()).ok());
  }
}

absl::StatusOr<MatchResult> MatchImpl::confirmed_result() const {
  absl::MutexLock l(&mu_);
  if (committed_result_.has_value()) return *committed_result_;

  // TODO: Include names, match numbers, etc.
  if (!a_result_.has_value()) {
    return Err(a_->ErrorStringId(), " has not reported for ",
               id_.ErrorStringId());
  }
  if (!b_result_.has_value()) {
    return Err((*b_)->ErrorStringId(), " has not reported for ",
               id_.ErrorStringId());
  }

  if (*a_result_ != *b_result_) {
    return Err(a_->ErrorStringId(), " and ", (*b_)->ErrorStringId(),
               " reported different results.");
  }
  return *a_result_;
}

// TODO: Consolidate these two.
bool MatchImpl::has_player(const Player& p) const {
  return a_ == p || (b_.has_value() && *b_ == p);
}
bool MatchImpl::has_player(Player::Id p) const {
  return a_->id() == p || (b_.has_value() && (*b_)->id() == p);
}

absl::StatusOr<Player> MatchImpl::opponent(const Player& p) const {
  if (!has_player(p)) {
    return Err(p->ErrorStringId(), " is not in this match.");
  }
  if (is_bye()) return Err("This match is a Bye.");
  return p == a_ ? *b_ : a_;
}

absl::Status MatchImpl::PlayerReportResult(Player reporter, 
                                           MatchResult result) {
  // This shouldn't happen. Bye results are already committed.
  if (is_bye()) {
    return Err("Trying to report a Match result for a Bye for ", 
                a_->ErrorStringId());
  }

  // Only players can report for their matches.
  if (!has_player(reporter)) {
    return Err("Reporting ", reporter->ErrorStringId(), " is not in ",
               id_.ErrorStringId());
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
  if (result.id != id_) {
    return Err("Reported ", result.id.ErrorStringId(), " does not equal ", 
               id_.ErrorStringId());
  }

  // Check draw validity.
  if (!result.winner.has_value()) {
    // No winner implies match was drawn. Check that the wins align.
    if (result.winner_games_won == result.winner_games_lost) {
      return absl::OkStatus();
    }
    return Err("Reported draw ", id_.ErrorStringId(),
               " does not have equal game wins between ", a_->ErrorStringId(),
               " and ", (*b_)->ErrorStringId());
  }

  // Check win validity.
  auto& winner = *result.winner;
  if (!has_player(winner)) {
    return Err(id_.ErrorStringId(), " report has winner ",
               // TODO: This generic int string is not very useful.
               winner, " not in this match.");
  }
  if (result.winner_games_won <= result.winner_games_lost) {
    // Match has a winner, but the match result doesn't align with that.
    //
    // TODO: This generic int string is not very useful.
    return Err(id_.ErrorStringId(), " report has a winner ", winner,
               " but reported games score is invalid for a won match.");
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



// RoundImpl -------------------------------------------------------------------
RoundImpl::RoundImpl(const Options& opts)
  : id_(opts.id), parent_(opts.parent) {}

Round RoundImpl::CreateRound(const Options& opts) {
  return Round(std::shared_ptr<RoundImpl>(new RoundImpl(opts)));
}

// Initializes this round, including generating pairings.
absl::Status RoundImpl::Init() {
  InitSelfPtr();

  return GeneratePairings();
}

std::string RoundImpl::ErrorStringId() const {
  return absl::StrCat("Round ", (id_ & kRoundMask));
}

absl::Status RoundImpl::CommitMatchResult(Match m) {
  // TODO: Fill this out.
  return absl::OkStatus();
}

absl::Status RoundImpl::JudgeSetResult(Match m) {
  // TODO: Fill this out.
  return absl::OkStatus();
}

absl::Status RoundImpl::GeneratePairings() {
  /*
  auto p = parent_.Lock();
  if (!p.ok()) return p.status();

  Tournament parent = *std::move(p);
  auto players = parent->ActivePlayers();
  */

  // TODO: Shuffle the vectors and generate pairings.

  return absl::OkStatus();
}

}  // namespace internal
}  // namespace tcgtc
