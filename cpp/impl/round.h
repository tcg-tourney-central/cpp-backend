#ifndef _TCGTC_ROUND_H_
#define _TCGTC_ROUND_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "cpp/container-class.h"
#include "cpp/definitions.h"
#include "cpp/fraction.h"
#include "cpp/match-id.h"
#include "cpp/tiebreaker.h"
#include "cpp/util.h"

namespace tcgtc {
namespace internal {

class RoundImpl : public MemoryManagedImplementation<RoundImpl> {
 public:
  struct Options {
    Round::Id id;
    Tournament::View parent;
  };
  static Round CreateRound(const Options& opts);

  // Initializes this round, including generating pairings.
  absl::Status Init();

  std::string ErrorStringId() const;

  absl::Status CommitMatchResult(Match m);
  absl::Status JudgeSetResult(Match m);

  bool RoundComplete() const {
    absl::MutexLock l(&mu_);
    return outstanding_matches_.empty();
  }
   
 private:
  explicit RoundImpl(const Options& opts);
  Round this_round() const { return Round(self_copy()); }

  absl::Status GenerateSwissPairings();

  const Round::Id id_;
  const Tournament::View parent_;

  mutable absl::Mutex mu_;

  absl::flat_hash_map<MatchId, Match> outstanding_matches_ ABSL_GUARDED_BY(mu_);
  absl::flat_hash_map<MatchId, Match> reported_matches_ ABSL_GUARDED_BY(mu_);
};

}  // namespace internal
}  // namespace tcgtc

#endif // _TCGTC_ROUND_H_
