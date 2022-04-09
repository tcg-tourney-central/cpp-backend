#include "impl/round.h"

#include <algorithm>

#include "absl/container/flat_hash_set.h"
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


namespace {
struct PerMatchPointPairing {
  PerMatchPointPairing() = default;
  PerMatchPointPairing(std::vector<Player> pairs, std::vector<Player> leftovers) {
    assert((pairs.size() & 1) == 0);
    pairings.reserve(pairs.size() >> 1);
    for (int idx = 0; idx + 1 < pairs.size(); idx += 2) {
      auto& left = pairs[idx];
      auto& right = pairs[idx+1];
      pairings.push_back(std::make_pair(std::move(left), std::move(right)));
    }
    remainders = std::move(leftovers);
  }
  std::vector<std::pair<Player, Player>> pairings;
  std::vector<Player> remainders;
};

bool ValidPairing(const Player& l, const Player& r) { 
  return !l->has_played_opp(r);
}
bool ValidPairing(const std::pair<Player, Player>& pair) { 
  return ValidPairing(pair.first, pair.second);
}

bool AttemptInPlaceRepair(std::pair<Player, Player>& bad, 
                    std::pair<Player, Player>& good) {
  auto& a = bad.first;
  auto& b = bad.second;
  auto& c = good.first;
  auto& d = good.second;
  if (ValidPairing(a, d) && ValidPairing(b, c)) {
    std::swap(a, c);
    return true;
  }
  if (ValidPairing(a, c) && ValidPairing(b, d)) {
    std::swap(a, d);
    return true;
  }
  return false;
}

void AttemptFixes(PerMatchPointPairing& out) {
  std::vector<Player> problem_players;
  problem_players.swap(out.remainders);

  absl::flat_hash_set<int> bad_indices;
  for (int idx = 0; idx + 1 < problem_players.size(); idx += 2) {
    auto bad_pair = std::make_pair(problem_players[idx],
                                    problem_players[idx + 1]);
    bool repair_success = false;
    for (auto& pair : out.pairings) {
      if ((repair_success = AttemptInPlaceRepair(bad_pair, pair))) {
        // bad_pair now contains a valid matching, as does the in-place updated
        // pair.
        out.pairings.push_back(std::move(bad_pair));
        break;
      }
    }

    // Couldn't repair these two despite inspecting every other match for
    // possible swaps. Probably terrible luck.
    if (!repair_success) {
      bad_indices.insert(idx);
      bad_indices.insert(idx+1);
    }
  }
  out.remainders.reserve(bad_indices.size());
  for (auto idx : bad_indices) out.remainders.push_back(problem_players[idx]);
}

// TODO: This is a pretty naive and probably bad pairing algo.
template <typename URBG>
PerMatchPointPairing Pair(std::vector<Player> players, URBG& urbg) {
  std::shuffle(players.begin(), players.end(), urbg);

  std::vector<Player> paired_players;
  std::vector<Player> problem_players;
  paired_players.reserve(players.size());
  for (int idx = 0; idx + 1 < players.size(); idx += 2) {
    auto& left = players[idx];
    auto& right = players[idx+1];

    // Leave this pairing "as is", for now.
    if (ValidPairing(left, right)) {
      paired_players.push_back(left);
      paired_players.push_back(right);
      continue;
    }
    problem_players.push_back(left);
    problem_players.push_back(right);
  }
  // Odd number of players, using bit-wise odd number check.
  if ((players.size() & 1) != 0) {
    problem_players.push_back(players.back());
  }

  // Initial pairing.
  PerMatchPointPairing out(paired_players, problem_players);

  // Attempt to mix any unpairable "pairs" into the existing valid pairings.
  while (out.remainders.size() > 1) {
    auto prev_size = out.remainders.size();
    AttemptFixes(out);

    // This accounts for the "draw bracket" case.
    // TODO: Is it possible that we have equal sizes and have to run again?
    if (out.remainders.size() == prev_size) break;
  }
  return out;
}
}  // namespace 

// TODO: Look into benchmarking this function eventually.
absl::Status RoundImpl::GenerateSwissPairings() {
  auto p = parent_.Lock();
  if (!p.ok()) return p.status();

  Tournament parent = *std::move(p);
  auto players = parent->ActivePlayers();

  std::vector<std::pair<Player, Player>> pairings;
  std::vector<Player> remainders;
  for (auto it = players.rbegin(); it != players.rend(); ++it) {
    auto& current = it->second;

    // Collect any unpaired players from the last attempt.
    for (auto& p : remainders) current.push_back(std::move(p));
    auto tmp = Pair(std::move(current), parent->rand());

    for (auto& pair : tmp.pairings) pairings.push_back(std::move(pair));
    remainders = std::move(tmp.remainders);
  }
  assert(std::all_of(pairings.begin(), pairings.end(), [](auto p){
     return ValidPairing(p);
  }));
  assert(remainders.size() <= 1);

  absl::MutexLock l(&mu_);
  for (uint32_t idx = 0; idx < pairings.size(); ++idx) {
    MatchId id{id_, idx + 1};
    auto& l = pairings[idx].first;
    auto& r = pairings[idx].second;
    outstanding_matches_.insert({id, Match::Impl::CreatePairing(l, r, id)});
  }

  return absl::OkStatus();
}

}  // namespace internal
}  // namespace tcgtc
