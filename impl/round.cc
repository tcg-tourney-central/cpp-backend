#include "impl/round.h"

#include "match-id.h"
#include "match-result.h"
#include "player-match.h"
#include "impl/tournament.h"
#include "util.h"

namespace tcgtc {
namespace internal {

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
