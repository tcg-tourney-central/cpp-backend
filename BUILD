load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")


# Libraries -- KEEP ALPHABETIZED

cc_library(
  name = "container-class",
  hdrs = ["container-class.h"],
  copts = ["/std:c++17"],
)

cc_library(
  name = "definitions",
  hdrs = ["definitions.h", "match.h", "player.h"],
  srcs = ["match.cc", "player.cc"],
  deps = [
    ":container-class",
    ":fraction",
    ":match-id",
    ":match-result",
    ":tiebreaker",
    ":util",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/container:flat_hash_map",
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/status:statusor",
    "@com_google_absl//absl/synchronization",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "fraction",
  hdrs = ["fraction.h"],
  srcs = ["fraction.cc"],
  copts = ["/std:c++17"],
)

cc_library(
  name = "match-id",
  hdrs = ["match-id.h"],
  deps = [
    "@com_google_absl//absl/hash",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "match-result",
  hdrs = ["match-result.h"],
  srcs = ["match-result.cc"],
  deps = [
    ":match-id",
    ":util",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "tiebreaker",
  hdrs = ["tiebreaker.h"],
  srcs = ["tiebreaker.cc"],
  deps = [
    ":fraction",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "tournament",
  hdrs = ["round.h", "tournament.h"],
  srcs = ["round.cc", "tournament.cc"],
  deps = [
    ":match-id",
    ":definitions",
    ":util",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/container:flat_hash_map",
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/status:statusor",
    "@com_google_absl//absl/synchronization",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "util",
  hdrs = ["util.h"],
  deps = [
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

