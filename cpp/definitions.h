#ifndef _TCGTC_DEFINITIONS_H_
#define _TCGTC_DEFINITIONS_H_

#include <cstdint>
#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "cpp/container-class.h"
#include "cpp/match-id.h"

namespace tcgtc {
namespace internal {
class MatchImpl;
class PlayerImpl;
class RoundImpl;
class TournamentImpl;
}  // namespace internal


class Player : public ContainerClass<internal::PlayerImpl> {
 public:
  using Impl = ::tcgtc::internal::PlayerImpl;
  using Id = uint64_t;

 private:
  explicit Player(std::shared_ptr<Impl> impl)
    : ContainerClass(std::move(impl)) {}

  friend class ::tcgtc::internal::PlayerImpl;
};


class Match : public ContainerClass<internal::MatchImpl> {
 public:
  using Impl = ::tcgtc::internal::MatchImpl;
 private:
  explicit Match(std::shared_ptr<Impl> impl)
    : ContainerClass(std::move(impl)) {}

  friend class ::tcgtc::internal::MatchImpl;
};


class Round : public ContainerClass<internal::RoundImpl> {
 public:
  using Impl = ::tcgtc::internal::RoundImpl;
  using Id = RoundId;

 private:
  explicit Round(std::shared_ptr<Impl> impl)
    : ContainerClass(std::move(impl)) {}

  friend class ::tcgtc::internal::RoundImpl;
};


class Tournament : public ContainerClass<internal::TournamentImpl> {
 public:
  using Impl = ::tcgtc::internal::TournamentImpl;
  class View : public ImplementationView<Impl> {
   public:
    View() = default;
    absl::StatusOr<Tournament> Lock() const {
      if (auto ptr = lock(); ptr != nullptr) return Tournament(ptr);
      return absl::FailedPreconditionError("Viewed Tournament destroyed.");
    }
   private:
    explicit View(std::weak_ptr<Impl> impl)
      : ImplementationView<Impl>(std::move(impl)) {}
    friend class Tournament;
  };

 private:
  static View CreateView(std::weak_ptr<Impl> impl) {
    return View(std::move(impl));
  }

  explicit Tournament(std::shared_ptr<Impl> impl)
    : ContainerClass(std::move(impl)) {}
  friend class View;
  friend class ::tcgtc::internal::TournamentImpl;
};

}  // namespace tcgtc

#endif // _TCGTC_DEFINITIONS_H_
