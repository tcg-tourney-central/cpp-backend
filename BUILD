load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")


# Libraries -- KEEP ALPHABETIZED

cc_library(
  name = "container-class",
  hdrs = ["cpp/container-class.h"],
  copts = ["/std:c++17"],
)

cc_library(
  name = "definitions",
  hdrs = ["cpp/definitions.h"],
  deps = [
    ":container-class",
    ":match-id",
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/status:statusor",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "fraction",
  hdrs = ["cpp/fraction.h"],
  srcs = ["cpp/fraction.cc"],
  copts = ["/std:c++17"],
)

cc_library(
  name = "graph",
  hdrs = ["cpp/pairings/graph.h"],
  srcs = ["cpp/pairings/graph.cc"],
  deps = [
    ":container-class",
    "@com_google_absl//absl/hash",
    "@com_google_absl//absl/container:flat_hash_map",
    "@com_google_absl//absl/container:flat_hash_set",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "isomorphism",
  hdrs = ["cpp/pairings/isomorphism.h"],
  srcs = ["cpp/pairings/isomorphism.cc"],
  deps = [
    ":container-class",
    ":graph",
    ":player-match",
    "@com_google_absl//absl/hash",
    "@com_google_absl//absl/container:flat_hash_map",
    "@com_google_absl//absl/container:flat_hash_set",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "match-id",
  hdrs = ["cpp/match-id.h"],
  deps = [
    "@com_google_absl//absl/hash",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "match-result",
  hdrs = ["cpp/match-result.h"],
  srcs = ["cpp/match-result.cc"],
  deps = [
    ":match-id",
    ":util",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "player-match",
  hdrs = ["cpp/player-match.h"],
  srcs = [
    "cpp/impl/match.h", 
    "cpp/impl/match.cc",
    "cpp/impl/player.h",
    "cpp/impl/player.cc"
  ],
  deps = [
    ":container-class",
    ":definitions",
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
  name = "tiebreaker",
  hdrs = ["cpp/tiebreaker.h"],
  srcs = ["cpp/tiebreaker.cc"],
  deps = [
    ":fraction",
  ],
  copts = ["/std:c++17"],
)

cc_library(
  name = "tournament",
  hdrs = ["cpp/impl/round.h", "cpp/impl/tournament.h"],
  srcs = ["cpp/impl/round.cc", "cpp/impl/tournament.cc"],
  deps = [
    ":definitions",
    ":match-id",
    ":player-match",
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
  hdrs = ["cpp/util.h"],
  deps = [
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/strings",
  ],
  copts = ["/std:c++17"],
)

