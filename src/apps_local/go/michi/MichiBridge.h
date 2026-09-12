#pragma once

// The whole of michi, as six plain-C functions over plain types.
//
// This boundary exists because michi's headers do NOT compile as C++: they do
// arithmetic on enums and return string literals as `char*`, both of which C
// accepts and C++ refuses. Patching four thousand lines of somebody else's
// engine to satisfy a compiler it was never written for is a sync nobody wants
// to do twice, so the line is drawn here instead: everything on the michi side
// of this header is C, everything on the other side is C++, and the only things
// that cross are integers and byte arrays.
//
// Boards are `size * size` bytes in row-major order, row 0 at the top:
// 0 empty, 1 black, 2 white -- which is `go::kEmpty`, `go::kBlack`, `go::kWhite`
// by construction. Moves are `row * size + col`, or -1 for a pass.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Once per boot. Builds the 3x3 pattern set and michi's board tables, and
// allocates its working position. Safe to call repeatedly.
void michi_bridge_init(void);

// The move michi wants, given the board as it stands.
//
// `nowMs` is a clock the caller lends; when it is null the search runs its full
// simulation count, which is what the host tests want so that a result does not
// depend on how fast the machine running them happens to be.
int michi_bridge_genmove(int size, const uint8_t *board, int toMove, int komiHalves, int simulations,
                         uint32_t budgetMs, uint32_t (*nowMs)(void));

// How many simulations the last genmove actually ran before its clock stopped
// it, and how long it took. For the log line that turns an estimate into a fact.
int michi_bridge_last_simulations(void);
uint32_t michi_bridge_last_ms(void);

// Dead-stone detection is deliberately NOT here. michi answers it with
// compute_all_status(), which segmentation faults on a nearly full board --
// reproduced in forty lines of plain C: five empty points is fine, three is a
// fault -- and a counting screen is always a nearly full board. GoEngine's
// owner-map estimator answers it instead. See the note at the foot of
// GoMichi.cpp.

#ifdef __cplusplus
}
#endif
