workspace(name = "tcgtc")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# absl
http_archive(
  name = "com_google_absl",
  urls = ["https://github.com/abseil/abseil-cpp/archive/38db52adb2eabc0969195b33b30763e0a1285ef9.zip"],
  strip_prefix = "abseil-cpp-38db52adb2eabc0969195b33b30763e0a1285ef9",
)

# GoogleTest/GoogleMock framework. Used by most unit-tests.
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/011959aafddcd30611003de96cfd8d7a7685c700.zip"],  # 2020-05-14T00:36:05Z
    strip_prefix = "googletest-011959aafddcd30611003de96cfd8d7a7685c700",
    sha256 = "6a5d7d63cd6e0ad2a7130471105a3b83799a7a2b14ef7ec8d742b54f01a4833c",
)

# Google benchmark.
http_archive(
    name = "com_github_google_benchmark",
    urls = ["https://github.com/google/benchmark/archive/16703ff83c1ae6d53e5155df3bb3ab0bc96083be.zip"],
    strip_prefix = "benchmark-16703ff83c1ae6d53e5155df3bb3ab0bc96083be",
    sha256 = "59f918c8ccd4d74b6ac43484467b500f1d64b40cc1010daa055375b322a43ba3",
)

# C++ rules for Bazel.
http_archive(
    name = "rules_cc",
    sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
    strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip",
        "https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip",
    ],
)