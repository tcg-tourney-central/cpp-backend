#ifndef _TCGTC_UTIL_H_
#define _TCGTC_UTIL_H_

#include <cassert>
#include <utility>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

namespace tcgtc {

template <typename... Args>
absl::Status Err(Args&&... args) {
  return absl::InvalidArgumentError(absl::StrCat(std::forward<Args>(args)...));
}

}  // namespace tcgtc

#endif // _TCGTC_UTIL_H_
