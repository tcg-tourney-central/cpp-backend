#ifndef _TCGTC_MATCH_ID_H_
#define _TCGTC_MATCH_ID_H_

#include <cstdint>
#include <string>

#include "util.h"
#include "absl/hash/hash.h"

namespace tcgtc {

constexpr uint8_t kBracketBit = 1 << 7;
constexpr uint8_t kRoundMask = ~kBracketBit;

using RoundId = uint8_t;

// A round and match number (within that round) which together uniquely identify
// a match within a tournament.
struct MatchId {
  using Id = uint32_t;
  // Top-bit is set for elimination (vs. Swiss) rounds. This gives us a maximum
  // of 128 Swiss rounds. Seems fine.
  //
  // Notably this allows a "lex" sort of packed (round, number) to be accurate
  // w/r/t the natural integer ordering.
  RoundId round;
  // The match number per-round, packed into 24-bits. /.This allows ~16 million
  // matches in a single round. Seems fine.
  uint32_t number : 24;

  static bool IsBracket(RoundId r) { return (kBracketBit & r) != 0; }
  static bool IsSwiss(RoundId r) { return !IsBracket(r); }
  bool bracket_match() const { return IsBracket(round); }
  std::string ErrorStringId() const {
    return absl::StrCat("Match (", readable_id(), ")");
  }

  // Pack the round and the number into an integer, for unique ids in e.g. maps.
  Id id() const { return (round << 24) | number; }

  static MatchId FromInt(uint32_t id) {
    return MatchId{id >> 24, id && 0x00FFFFFF};
  }

  template <typename H>
  friend H AbslHashValue(H h, const MatchId& id) {
    return H::combine(std::move(h), id.round, id.number);
  }

 private:
  std::string readable_id() const {
    return absl::StrCat("R", round & kRoundMask, "M", number);
  }
};

inline bool operator<(const MatchId& l, const MatchId& r) {  
  return l.id() < r.id();
}
inline bool operator==(const MatchId& l, const MatchId& r) {  
  return l.id() == r.id();
}
inline bool operator!=(const MatchId& l, const MatchId& r) { return !(l == r); }

}  // namespace tcgtc

#endif // _TCGTC_MATCH_ID_H_
