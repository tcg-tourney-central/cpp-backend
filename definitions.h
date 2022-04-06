#ifndef _TCGTC_DEFINITIONS_H_
#define _TCGTC_DEFINITIONS_H_

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
#include "container-class.h"
#include "fraction.h"
#include "match-id.h"
#include "match-result.h"
#include "tiebreaker.h"
#include "util.h"

namespace tcgtc {
namespace internal {
class MatchImpl;
class PlayerImpl;
class RoundImpl;
class TournamentImpl;
}  // namespace internal


class Player : public ContainerClass<internal::PlayerImpl> {
  friend class ::tcgtc::internal::PlayerImpl;
  using PlayerImpl = ::tcgtc::internal::PlayerImpl;
 public:
  using Id = uint64_t;

  struct Options {
    Id id;
    std::string first_name;
    std::string last_name;
    std::string username;
  };
  static Player CreatePlayer(Options opts);

  // TODO: Remove this in favor of indirection
  Id id() const;

 private:
  explicit Player(std::shared_ptr<PlayerImpl> impl)
    : ContainerClass(std::move(impl)) {}
};
bool operator==(const Player& l, const Player& r);
bool operator<(const Player& l, const Player& r);


class Match : public ContainerClass<internal::MatchImpl> {
  friend class ::tcgtc::internal::MatchImpl;
  using MatchImpl = ::tcgtc::internal::MatchImpl;
 public:
  static Match CreateBye(Player p, MatchId id);
  static Match CreatePairing(Player a, Player b, MatchId id);

 private:
  explicit Match(std::shared_ptr<MatchImpl> impl)
    : ContainerClass(std::move(impl)) {}
};


class Round : public ContainerClass<internal::RoundImpl> {
  friend class ::tcgtc::internal::RoundImpl;
  using RoundImpl = ::tcgtc::internal::RoundImpl;
 public:
  // TODO: Consolidate this with MatchId.round;
  using Id = uint8_t;

 private:
  explicit Round(std::shared_ptr<RoundImpl> impl)
    : ContainerClass(std::move(impl)) {}
};


class Tournament : public ContainerClass<internal::TournamentImpl> {
  friend class ::tcgtc::internal::TournamentImpl;
  friend class View;
  using TournamentImpl = ::tcgtc::internal::TournamentImpl;
 public:
  class View : public ImplementationView<TournamentImpl> {
   public:
    View() = default;
    explicit View(std::weak_ptr<TournamentImpl> impl)
      : ImplementationView<TournamentImpl>(std::move(impl)) {}

    absl::StatusOr<Tournament> Lock();
  };

  View tournament_view() const { return View(get_weak()); }

 private:
  explicit Tournament(std::shared_ptr<TournamentImpl> impl)
    : ContainerClass(std::move(impl)) {}
};

}  // namespace tcgtc

#endif // _TCGTC_DEFINITIONS_H_
