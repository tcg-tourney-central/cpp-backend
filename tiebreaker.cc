#include "tiebreaker.h"

#include <tuple>

namespace tcgtc {
namespace {

// Use a packed tuple of references for lex-sorting OMWP, GWP, OGWP.
using RefTup = std::tuple<const uint16_t&, const Fraction&,
                          const Fraction&, const Fraction&>;

RefTup FromTieBreakInfo(const TieBreakInfo& info) {
  return RefTup(info.match_points, info.opp_mwp, info.gwp, info.opp_gwp);
}

}  // namespace

bool operator==(const TieBreakInfo& l, const TieBreakInfo& r) {
  return FromTieBreakInfo(l) == FromTieBreakInfo(r);
}

bool operator!=(const TieBreakInfo& l, const TieBreakInfo& r) {
  return !(l == r);
}

bool operator<(const TieBreakInfo& l, const TieBreakInfo& r) {
  return FromTieBreakInfo(l) < FromTieBreakInfo(r);
}

}  // namespace tgctc
