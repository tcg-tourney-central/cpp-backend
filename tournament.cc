#include "tournament.h"

// TODO: Create a single header for these.
#include "match.h"
#include "player.h"
#include "round.h"

namespace tcgtc {
namespace internal {

// Tournament ------------------------------------------------------------------
TournamentImpl::TournamentImpl(const Options& opts) : opts_(opts) {}

absl::StatusOr<Player> TournamentImpl::GetPlayer(Player::Id player) const {
  absl::MutexLock l(&mu_);
  return GetPlayerLocked(player);
}
absl::StatusOr<Player> TournamentImpl::GetPlayerLocked(Player::Id player) const {
  if (auto it = players_.find(player); it != players_.end()) return it->second;

  return Err("No Player in this tournament for the reporting player ID (",
             player,").");

}

absl::StatusOr<Match> TournamentImpl::GetMatch(MatchId id) const {
  absl::MutexLock l(&mu_);
  return GetMatchLocked(id);
}
absl::StatusOr<Match> TournamentImpl::GetMatchLocked(MatchId id) const {
  if (auto it = matches_.find(id); it != matches_.end()) return it->second;

  return Err("No Match in this tournament for id ", id.ErrorStringId());
}

absl::StatusOr<Round> TournamentImpl::CurrentRound() const {
  absl::MutexLock l(&mu_);
  return CurrentRoundLocked();
}

// Returns an error status if no rounds have started.
absl::StatusOr<Round> TournamentImpl::CurrentRoundLocked() const {
  if (rounds_.empty()) return Err("Round 1 has not yet started!");
  return rounds_.rbegin()->second;
}

absl::Status TournamentImpl::AddPlayer(const Player::Impl::Options& info) {
  absl::MutexLock l(&mu_);
  return AddPlayerLocked(info);
}
absl::Status
TournamentImpl::AddPlayerLocked(const Player::Impl::Options& info) {
  // TODO: Fill this in.
  return absl::OkStatus();
}

absl::Status TournamentImpl::DropPlayer(Player::Id player) {
  absl::MutexLock l(&mu_);
  return DropPlayerLocked(player);
}
absl::Status TournamentImpl::DropPlayerLocked(Player::Id player) {
  // TODO: Fill this in.
  return absl::OkStatus();
}

std::map<uint32_t, std::vector<Player>> TournamentImpl::ActivePlayers() const {
  absl::MutexLock l(&mu_);
  std::map<uint32_t, std::vector<Player>> out;
  for (const auto& [id, p] : active_players_) {
    out[p->match_points()].push_back(p);
  }
  return out;
}

// Returns an error status if the result is for a round that is not current.
absl::Status TournamentImpl::ReportResult(Player::Id player, 
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

absl::Status TournamentImpl::JudgeSetResult(const MatchResult& result) {
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
absl::StatusOr<Round> TournamentImpl::PairNextRound() {
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
  opts.parent = self_view();
  Round next = internal::RoundImpl::CreateRound(opts);
  rounds_.insert(std::make_pair(round_num, next));
  l.Release();

  if (auto out = next->Init(); !out.ok()) return out;
  return next;
}

}  // namespace internal
}  // namespace tcgtc
