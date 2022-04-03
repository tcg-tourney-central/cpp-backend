#ifndef _TCGTC_MATCH_ID_H_
#define _TCGTC_MATCH_ID_H_

#include <cstdint>

namespace tcgtc {

// A round and match number (within that round) which together uniquely identify
// a match within a tournament.
struct MatchId {
  // Top-bit is set for elimination (vs. Swiss) rounds. This gives us a maximum
  // of 128 Swiss rounds. Seems fine.
  uint8_t round;
  // The match number per-round. This allows ~16 million matches in a single
  // round. Seems fine.
  uint32_t number : 24;

  // Pack the round and the number into an integer, for unique ids in e.g. maps.
  uint32_t id() const { return (number << 8) | round; }

  static MatchId FromInt(uint32_t id) {
    return MatchId{.round = id & 0xFF, .number = id >> 8; };
  }

  bool operator==(const MatchId& other) { return this->id() == other.id(); }
};

}  // namespace tcgtc

#endif // _TCGTC_MATCH_ID_H_
