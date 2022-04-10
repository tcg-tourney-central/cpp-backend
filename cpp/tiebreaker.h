#ifndef _TCGTC_TIEBREAKER_H_
#define _TCGTC_TIEBREAKER_H_

#include <cstdint>

#include "fraction.h"

namespace tcgtc {

// The MTR defined ordering of players.
struct TieBreakInfo {
  uint16_t match_points = 0;
  Fraction opp_mwp;
  Fraction gwp;
  Fraction opp_gwp;
};
bool operator==(const TieBreakInfo& l, const TieBreakInfo& r);
bool operator!=(const TieBreakInfo& l, const TieBreakInfo& r);
bool operator<(const TieBreakInfo& l, const TieBreakInfo& r);

}  // namespace tcgtc

#endif // _TCGTC_TIEBREAKER_H_
