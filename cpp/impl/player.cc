#include "player.h"

#include <cstdint>

#include "cpp/impl/match.h"
#include "cpp/match-id.h"
#include "cpp/match-result.h"
#include "cpp/util.h"

namespace tcgtc {
namespace internal {
namespace {
std::string ComputeDisplayName(const PlayerImpl::Options& opts) {
  if (opts.last_name.empty() || opts.first_name.empty()) {
    assert(!opts.username.empty());
    return opts.username;
  }
  return absl::StrCat(opts.last_name, ",", opts.first_name);
}
}  // namespace

PlayerImpl::PlayerImpl(const Options& opts)
  : id_(opts.id), last_name_(opts.last_name), first_name_(opts.first_name),
    username_(opts.username), display_name_(ComputeDisplayName(opts)) {}

Player PlayerImpl::CreatePlayer(Options opts) { 
  Player p(std::shared_ptr<PlayerImpl>(new PlayerImpl(opts)));
  p->Init();
  return p;
}

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
    opponents_.insert(std::make_pair((*opp)->id(), *opp));
  }
  return absl::OkStatus();
}

bool PlayerImpl::has_played_opp(const Player& p) const {
  absl::MutexLock l(&mu_);
  return opponents_.find(p->id()) != opponents_.end();
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

bool operator==(const Player& l, const Player& r) {
  if (l->id() != r->id()) return false;

  assert(l.get() == r.get());
  return true;
}

bool operator!=(const Player& l, const Player& r)  { return !(l == r); }

bool operator<(const Player& l, const Player& r) {
  return reinterpret_cast<uintptr_t>(l.get()) < 
         reinterpret_cast<uintptr_t>(r.get());
}

}  // namespace internal
}  // namespace tcgtc
