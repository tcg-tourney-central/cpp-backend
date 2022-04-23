#ifndef _TCGTC_PAIRINGS_BLOSSOM_H_
#define _TCGTC_PAIRINGS_BLOSSOM_H_

#include "cpp/pairings/graph.h"

namespace tcgtc {

// Creates a maximal matching on the graph g from some initial matching m.
Matching Blossom(const Graph& g, const Matching& m);

}  // namespce tcgtc

#endif  // _TCGTC_PAIRINGS_BLOSSOM_H_
