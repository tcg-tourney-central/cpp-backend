load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")


# Libraries -- KEEP ALPHABETIZED

cc_library(
  name = "definitions",
  hdrs = ["definitions.h", "container-class.h"],
  srcs = ["definitions.cc"],
  deps = [
    ":fraction",
    ":match-id",
    ":util",
    "@com_google_absl//absl/base",
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
)

cc_library(
  name = "match-id",
  hdrs = ["match-id.h"],
  deps = [
    "@com_google_absl//absl/hash",
  ],
)

cc_library(
  name = "tournament",
  hdrs = ["tournament.h"],
  srcs = ["tournament.cc"],
  deps = [
    ":definitions",
    ":match-id",
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
)

