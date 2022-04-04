load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")


# Libraries -- KEEP ALPHABETIZED

cc_library(
  name = "definitions",
  hdrs = ["definitions.h"],
  srcs = ["definitions.cc"],
  deps = [
    ":fraction",
    ":match-id",
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
)
