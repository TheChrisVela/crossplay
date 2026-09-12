#include "GoMichi.h"

#include "GoEngine.h"
#include "michi/MichiBridge.h"

namespace gomichi {
namespace {

// Our position as the bridge wants it: one byte a point, row major, and the
// values are already `go::kEmpty` / `go::kBlack` / `go::kWhite` by construction.
// The game holds its board two bits a point so that a thirteen by thirteen
// position fits a packet; michi wants bytes.
void flatten(const go::Game& game, uint8_t out[go::kMaxPoints]) {
  const int points = game.points();
  for (int i = 0; i < points; ++i) out[i] = game.at(i);
}

}  // namespace

Settings settingsFor(const go::Level level) {
  // Simulations only. The handicap, the komi and which colour the player takes
  // are three SEPARATE settings and none of them is strength: a level that
  // silently spotted you stones made "easy" mean two things at once.
  //
  // The counts are where michi-c2 was MEASURED, not guessed. The budgets are
  // what those counts are expected to cost on this chip, which is roughly
  // twenty-six times slower than the laptop they were measured on, and they
  // exist because that multiplier is an estimate and the five second ceiling is
  // not negotiable.
  switch (level) {
    case go::Level::Easy:
      return Settings{60, 1200};
    case go::Level::Medium:
      // Roughly where michi-c2 measures LEVEL with GNU Go 3.8 at level 10.
      return Settings{500, 2500};
    case go::Level::Hard:
      // And roughly where it measures 60% against that same opponent.
      return Settings{1500, 4000};
    case go::Level::Count_:
      break;
  }
  return Settings{500, 2500};
}

int chooseMove(const go::Game& game, const go::Level level, uint32_t& seed, const Clock clock) {
  (void)seed;

  // When to PASS is decided here and not by the search, and that is the one
  // piece of judgement this bridge keeps.
  //
  // michi will not pass while there is a point left to take, because under area
  // scoring a neutral point is worth one and filling it is not a mistake. To a
  // person it reads as the machine not knowing the game is over -- so this is
  // the Leela Zero rule instead: pass when the opponent has passed AND passing
  // wins the game as it stands with every stone alive.
  //
  // Both halves are load-bearing. Without the first, White passes on move two:
  // an almost empty board is all neutral, so Black has nothing, White has komi,
  // and "passing wins" is true before a stone is played.
  //
  // michi's own answer to this is is_better_to_pass(), which is not called: it
  // runs compute_all_status(), which faults on a nearly full board.
  if (game.passes >= 1 && goengine::passingWins(game, game.toMove)) return go::kPass;
  // Nothing left but one's own eyes. Filling them is how a won group dies, so
  // passing is the only move, whoever is ahead.
  if (!go::hasUsefulMove(game, game.toMove)) return go::kPass;

  const Settings settings = settingsFor(level);
  uint8_t board[go::kMaxPoints];
  flatten(game, board);

  // The search never returns a pass: the two cases above are the only two this
  // app passes in, and michi liking a pass at sixty simulations is not one of
  // them. The bridge hands back its best non-pass move instead.
  const int ko = game.ko < game.points() ? static_cast<int>(game.ko) : -1;
  const int lastMove = game.lastMove < game.points() ? static_cast<int>(game.lastMove) : -1;
  const int move =
      michi_bridge_genmove(game.size, board, game.toMove, game.komiHalves, ko, lastMove,
                           static_cast<int>(game.moveNumber), settings.simulations, settings.budgetMs, clock);

  // Whatever the search produced has to survive OUR rules, and when it does not
  // the answer is the search's NEXT choice rather than a pass. michi keeps its
  // own superko hash and this game keeps its own ring; a pass here threw away a
  // move in the middle of a fight and, if the human passed back, the game.
  if (move >= 0 && go::legal(game, move, game.toMove)) return move;
  int ranked[8] = {};
  const int count = michi_bridge_ranked(game.size, ranked, 8);
  for (int i = 0; i < count; ++i) {
    if (go::legal(game, ranked[i], game.toMove)) return ranked[i];
  }
  // Nothing the search looked at is playable. Take any move that is not filling
  // our own eye before considering a pass, because hasUsefulMove() above said
  // there is one.
  const int points = game.points();
  for (int point = 0; point < points; ++point) {
    if (go::legal(game, point, game.toMove) && !go::isEye(game, point, game.toMove)) return point;
  }
  return go::kPass;
}

Context lastContext(const int size) {
  int ko = -1;
  int lastMove = -1;
  int moveNumber = 0;
  michi_bridge_context(size, &ko, &lastMove, &moveNumber);
  Context out{};
  out.ko = ko >= 0 ? ko : go::kNoPoint;
  out.lastMove = lastMove == -2 ? go::kPass : (lastMove >= 0 ? lastMove : go::kNoPoint);
  out.moveNumber = moveNumber;
  return out;
}

void forget() { michi_bridge_forget(); }

int lastSimulations() { return michi_bridge_last_simulations(); }

uint32_t lastMs() { return michi_bridge_last_ms(); }

// Dead stones are NOT michi's job here, and that is a measurement rather than
// a preference.
//
// michi's own `compute_all_status` crashes on a nearly-full board. Reproduced
// in forty lines of pure C with no C++ anywhere near it: five empty points is
// fine, three is a segmentation fault. The counting screen is always a nearly
// full board -- that is what counting is -- so the one position this app would
// ask the question in is the one position michi cannot answer it in.
//
// Upstream never meets this because `genmove` passes out of a decided game
// before the board gets that full, and its own use of the routine is at the end
// of a GTP game where it evidently does not. Fixing somebody else's search is a
// bigger commitment than this app needs to make for a function it already has,
// tested, next door.
//
// So: michi chooses the moves, which is what it is here for and what it is two
// or three stones better at. The owner-map estimator in GoEngine keeps the
// counting screen, where it is pinned against three settled positions whose
// answer is not in doubt.

}  // namespace gomichi
