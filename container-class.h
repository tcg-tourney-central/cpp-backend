// Defines a CRTP for a copyable container class for an implementation class.

#ifndef _TCGTC_CONTAINER_CLASS_H_
#define _TCGTC_CONTAINER_CLASS_H_

#include <cassert>
#include <memory>
#include <utility>

namespace tcgtc {

template <typename Impl>
class ImplementationView {
 public:
  ImplementationView() = default;
 protected:
  explicit ImplementationView(std::weak_ptr<Impl> impl)
    : impl_(std::move(impl)) {}

  // Must be checked for nullness.
  std::shared_ptr<Impl> lock() const { return impl_.lock(); }

 private:
  mutable std::weak_ptr<Impl> impl_;
};

template <typename Impl>
class ContainerClass {
 public:
  Impl* get() const { return impl_.get(); }
  Impl* operator->() const { return get(); }
  Impl& operator*() const { return *get(); }
 protected:
  Impl& me() const { return *get(); }
  std::weak_ptr<Impl> get_weak() const { return impl_; }

  ContainerClass() = delete;
  explicit ContainerClass(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {
    assert(impl != nullptr);
  }
 private:
  std::shared_ptr<Impl> impl_;
};

template <typename Derived>
class MemoryManagedImplementation : 
  public std::enable_shared_from_this<Derived> {
 protected:
  // Cannot be called during the constructor of Derived, per the C++ standard,
  // re: enable_shared_from_this.
  void InitSelfPtr() { self_ptr_ = weak_from_this(); }

  std::shared_ptr<Derived> self_copy() const { return self_ptr_.lock(); }
  std::weak_ptr<Derived> self_ref() const { return self_ptr_; }

 private:
  // A weak reference to ourselves, so that we can generate (with const-ness),
  // shared_ptr-s to ourselves for use by the container classes which hold
  // shared_ptr to the implementation classes.
  //
  // N.B. We *cannot* have a shared_ptr here, or we would leak that memory,
  // since the memory managed by the shared_ptr would hold a shared handle on
  // itself forever.
  std::weak_ptr<Derived> self_ptr_;
};

}  // namespace tcgtc

#endif  // _TCGTC_CONTAINER_CLASS_H_
