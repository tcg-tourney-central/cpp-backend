#include "match.h"

#include <algorithm>

#include "match-id.h"
#include "match-result.h"
#include "player.h"
#include "util.h"

namespace tcgtc {
namespace internal {

Match MatchImpl::CreateBye(Player p, MatchId id) {
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(p, std::nullopt, id)));
  m->Init();

  // Immediately commit the result of the bye back to the player's cache.
  // MTR states that a Bye is considered won 2-0 in games.
  m->CommitResult(MatchResult{id, p->id(), 2});
  return m;
}

Match MatchImpl::CreatePairing(Player a, Player b, MatchId id) {
  assert(a != b);

  // We do this so that we have a consistent order of lock acquisition when we
  // e.g. commit results back to players once confirmed.
  auto l = a < b ? a : b;
  auto r = a < b ? b : a;
  Match m(std::shared_ptr<MatchImpl>(new MatchImpl(l, r, id)));
  m->Init();
  return m;
}

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

}  // namespace internal
}  // namespace tcgtc
