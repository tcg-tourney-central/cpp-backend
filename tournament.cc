#include "tournament.h"

namespace tcgtc {

// Tournament ------------------------------------------------------------------
Tournament::Tournament(const Options& opts) : opts_(opts) {}

absl::StatusOr<Player> Tournament::GetPlayer(Player::Id player) const {
  absl::MutexLock l(&mu_);
  return GetPlayerLocked(player);
}

absl::StatusOr<Player> Tournament::GetPlayerLocked(Player::Id player) const {
  if (auto it = players_.find(player); it != players_.end()) return it->second;

  return Err("No Player in this tournament for the reporting player ID (",
             player,").");

}

absl::StatusOr<Match> Tournament::GetMatch(MatchId id) const {
  absl::MutexLock l(&mu_);
  return GetMatchLocked(id);
}

absl::StatusOr<Match> Tournament::GetMatchLocked(MatchId id) const {
  if (auto it = matches_.find(id); it != matches_.end()) return it->second;

  return Err("No Match in this tournament for id ", id.ErrorStringId());
}

absl::StatusOr<Round> Tournament::CurrentRound() const {
  absl::MutexLock l(&mu_);
  return CurrentRoundLocked();
}

// Returns an error status if no rounds have started.
absl::StatusOr<Round> Tournament::CurrentRoundLocked() const {
  if (rounds_.empty()) return Err("Round 1 has not yet started!");
  return rounds_.rbegin()->second;
}

absl::Status Tournament::AddPlayer(const Player::Options& info) {
  absl::MutexLock l(&mu_);
  return AddPlayerLocked(info);
}
absl::Status Tournament::AddPlayerLocked(const Player::Options& info) {
  // TODO: Fill this in.
  return absl::OkStatus();
}

absl::Status Tournament::DropPlayer(Player::Id player) {
  absl::MutexLock l(&mu_);
  return DropPlayerLocked(player);
}
absl::Status Tournament::DropPlayerLocked(Player::Id player) {
  // TODO: Fill this in.
  return absl::OkStatus();
}

// Returns an error status if the result is for a round that is not current.
absl::Status Tournament::ReportResult(Player::Id player, 
                                      const MatchResult& result) {
  absl::ReleasableMutexLock l(&mu_);
  auto p = GetPlayerLocked(player);
  if (!p.ok()) return p.status();
  auto m = GetMatchLocked(result.id);
  if (!m.ok()) return m.status();
  auto r = GetRoundLocked(result.id.round);
  if (!r.ok()) return r.status();
  l.Release();

  Match match = *std::move(m);
  if (auto out = match->PlayerReportResult(*p, result); !out.ok()) return out;

  // If the report confirmed a result, try to commit it to the round.
  if (match->confirmed_result().ok()) return (*r)->CommitMatchResult(match);
  
  // If the report was valid but the first one received, we can't commit but
  // the report is still okay.
  return absl::OkStatus();
}

absl::Status Tournament::JudgeSetResult(const MatchResult& result) {
  absl::ReleasableMutexLock l(&mu_);
  auto m = GetMatchLocked(result.id);
  if (!m.ok()) return m.status();
  auto r = GetRoundLocked(result.id.round);
  if (!r.ok()) return r.status();
  l.Release();

  Match match = *std::move(m);
  if (auto out = match->JudgeSetResult(result); !out.ok()) return out;
  return (*r)->JudgeSetResult(match);
}

// For now, require a request to pair the next round, but we can maybe
// consider doing this automagically when all pairings are received in the
// previous round.
absl::StatusOr<Round> Tournament::PairNextRound() {
  absl::ReleasableMutexLock l(&mu_);

  // Next round number.
  uint8_t round_num = rounds_.size() + 1;
  if (round_num > opts_.swiss_rounds) round_num |= kBracketBit;
  if (!rounds_.empty()) {
    Round prev = rounds_.rbegin()->second;
    if (!prev->RoundComplete()) {
      return Err(prev->ErrorStringId(), " is not complete!");
    }
  }

  internal::RoundImpl::Options opts;
  opts.id = round_num;
  opts.parent = this;
  Round next = internal::RoundImpl::CreateRound(opts);
  rounds_.insert(std::make_pair(round_num, next));
  l.Release();

  if (auto out = next->Init(); !out.ok()) return out;
  return next;
}



namespace internal {


// RoundImpl -------------------------------------------------------------------
RoundImpl::RoundImpl(const Options& opts)
  : id_(opts.id), parent_(opts.parent) {}

Round RoundImpl::CreateRound(const Options& opts) {
  return Round(std::shared_ptr<RoundImpl>(new RoundImpl(opts)));
}

// Initializes this round, including generating pairings.
absl::Status RoundImpl::Init() {
  self_ptr_ = weak_from_this();

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
  // TODO: Fill this out.
  return absl::OkStatus();
}



}  // namespace internal
}  // namespace tcgtc
