#include "cpp/impl/round.h"

#include <algorithm>

#include "cpp/match-id.h"
#include "cpp/match-result.h"
#include "cpp/player-match.h"
#include "cpp/impl/tournament.h"
#include "cpp/pairings/isomorphism.h"
#include "cpp/util.h"

namespace tcgtc {
namespace internal {
namespace {
bool ValidPairing(const std::pair<Player, Player>& p) {
  return !p.first->has_played_opp(p.second);
}
}  // namespace

RoundImpl::RoundImpl(const Options& opts)
  : id_(opts.id), parent_(opts.parent) {}

Round RoundImpl::CreateRound(const Options& opts) {
  return Round(std::shared_ptr<RoundImpl>(new RoundImpl(opts)));
}

// Initializes this round, including generating pairings.
absl::Status RoundImpl::Init() {
  InitSelfPtr();

  if (MatchId::IsSwiss(id_)) return GenerateSwissPairings();

  // TODO: Provide the ability to pair brackets correctly.
  return absl::OkStatus();
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

// TODO: Look into benchmarking this function eventually.
absl::Status RoundImpl::GenerateSwissPairings() {
  auto p = parent_.Lock();
  if (!p.ok()) return p.status();

  Tournament parent = *std::move(p);
  auto players = parent->ActivePlayers();

  PartialPairing final;
  for (auto it = players.rbegin(); it != players.rend(); ++it) {
    auto& current = it->second;

    // Collect any unpaired players from the last attempt.
    for (auto& p : final.unpaired) current.push_back(std::move(p));
    auto tmp = PairChunk(current, parent->rand());

    // Collect the pairings for this chunk.
    for (auto& pair : tmp.paired) final.paired.push_back(std::move(pair));
    final.unpaired.swap(tmp.unpaired);
  }
  assert(std::all_of(final.paired.begin(), final.paired.end(), [](auto p){
     return ValidPairing(p);
  }));
  assert(final.unpaired.size() <= 1);

  absl::MutexLock l(&mu_);
  IdGen gen(id_);
  for (const auto& p : final.paired) {
    MatchId id = gen.next();
    const auto& l = p.first;
    const auto& r = p.second;
    outstanding_matches_.insert({id, Match::Impl::CreatePairing(l, r, id)});
  }
  for (auto& p :  final.unpaired) {
    MatchId id = gen.next();
    reported_matches_.insert({id, Match::Impl::CreateBye(p, id)});
  }

  return absl::OkStatus();
}

}  // namespace internal
}  // namespace tcgtc
