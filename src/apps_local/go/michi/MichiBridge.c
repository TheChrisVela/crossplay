// See MichiBridge.h for why this file exists at all.

#include "MichiBridge.h"

#include <string.h>

#include "michi.h"

// nsims() lives in control.c and michi.h does not declare it.
int nsims(Game *game);

static int      gReady = 0;
static Position *gPos;
static Game     *gGame;
static int      *gOwnerMap;
static int      *gScoreCount;
static TreeNode *gTree;
static int       gLastSimulations;
static uint32_t  gLastMs;

// A point of ours, as a point of michi's.
//
// michi lays the board out as a bordered array indexed `row * (N + 1) + col`,
// with row counted from the top and col one-based, and it plays a board of
// `pos->size` inside an array sized for the compile-time maximum N. So a 9x9
// game on an N=13 build sits in the bottom-left of the larger array, which is
// what the `N - size` term is.
static Point michi_point(int row, int col, int size)
{
    return (Point)((N - size + 1 + row) * (N + 1) + col + 1);
}

static int our_point(Point pt, int size, int *row, int *col)
{
    int r = (int)pt / (N + 1) - (N - size + 1);
    int c = (int)pt % (N + 1) - 1;
    if (r < 0 || r >= size || c < 0 || c >= size) return 0;
    *row = r; *col = c;
    return 1;
}

void michi_bridge_init(void)
{
    if (gReady) return;
    // michi logs through a FILE* that ui.c opens, and ui.c is not vendored.
    // board_util.c is patched to accept a null sink; this says so out loud
    // rather than leaving a null global looking like an oversight.
    flog = NULL;

    make_pat3set();
    // The LARGE pattern board, which upstream initialises inside
    // init_large_patterns() -- the function that loads patterns.prob and
    // patterns.spat from disk. Those two files are several megabytes and are
    // not vendored, so large-pattern matching is off here. expand() still calls
    // copy_to_large_board() unconditionally, and with the coordinate map left
    // zeroed that copy writes every point to large_board[0] and trips its own
    // assert. One line here is cheaper than a patch to somebody else's board.
    init_large_board();
    already_suggested = michi_calloc(1, sizeof(Mark));
    board_init();
    gOwnerMap   = michi_calloc(BOARDSIZE, sizeof(int));
    gScoreCount = michi_calloc(2 * N * N + 1, sizeof(int));
    gPos  = new_position();
    gGame = michi_calloc(1, sizeof(Game));
    gGame->pos = gPos;
    // time_init 0 means "use the simulation count", not "use a clock". michi's
    // own time management reads clock(), whose meaning on this chip is not
    // something to build a move budget on; the budget is applied below instead.
    gGame->time_init = 0;

    // The tree has to EXIST before the first genmove, because genmove's first
    // act is to free the one it was given and free_tree dereferences its
    // argument without checking it. Upstream's ui.c creates it once before the
    // GTP loop; there is no ui.c here, so this is that line.
    gTree = new_tree_node();

    // Play the game out to the end, or stop once it is decided? michi-c2's own
    // CGOS configuration says 1, and CGOS is a machine playing machines. A
    // person watching a finished board being filled in one point at a time
    // reads it as the machine not knowing the game is over.
    play_until_the_end = 0;

    // Never resign. michi returns RESIGN_MOVE once its win rate drops below
    // this, and the bridge would have to report that as a pass -- which hands
    // the opponent a free move every turn for the rest of a lost game. Losing
    // games are played out here; the app decides when a game is over.
    RESIGN_THRES = 0.0;

    // Quiet. michi is a command-line program and says so: at verbosity 2 it
    // dumps its whole tree to stderr after every search, and REPORT_PERIOD
    // prints a progress line inside one. Neither has a reader here -- the
    // device has no stderr and the host suite's output is the suite's.
    verbosity = 0;
    REPORT_PERIOD = 1000000000;

    gReady = 1;
}

static void adopt(int size, const uint8_t *board, int toMove, int komiHalves)
{
    board_set_size(gPos, size);
    empty_position(gPos);
    board_set_komi(gPos, (float)komiHalves / 2.0f);

    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            uint8_t here = board[row * size + col];
            if (here == 0) continue;
            // PLACED, not played: play_move would apply captures and ko, and
            // the position handed in is already the result of both.
            board_place_stone(gPos, michi_point(row, col, size), here == 1 ? BLACK : WHITE);
        }
    }
    board_set_color_to_play(gPos, toMove == 1 ? BLACK : WHITE);

    slist_clear(allpoints);
    FORALL_POINTS(gPos, pt)
        if (point_color(gPos, pt) == EMPTY) slist_push(allpoints, pt);
    gGame->komi = board_komi(gPos);
}

// The best move that is NOT a pass, read off the tree the search just built.
//
// Passing is the APP's decision, not the engine's (see the note in
// GoMichi.cpp), so when the search likes a pass this asks it for its next
// choice instead. michi's own best_move() takes a list of nodes to skip, which
// is exactly this question; without it a search that liked a pass at sixty
// simulations would end a game the app was still winning.
static Point best_non_pass(void)
{
    if (gTree == NULL || gTree->children == NULL) return PASS_MOVE;
    TreeNode *except[2] = {NULL, NULL};
    for (TreeNode **child = gTree->children ; *child != NULL ; child++) {
        if ((*child)->move == PASS_MOVE) { except[0] = *child; break; }
    }
    TreeNode *best = best_move(gTree, except[0] != NULL ? except : NULL);
    if (best == NULL) return PASS_MOVE;
    return best->move;
}

int michi_bridge_genmove(int size, const uint8_t *board, int toMove, int komiHalves, int simulations,
                         uint32_t budgetMs, uint32_t (*nowMs)(void))
{
    michi_bridge_init();
    adopt(size, board, toMove, komiHalves);

    // The search in CHUNKS, against a wall clock.
    //
    // Upstream's genmove() runs the whole simulation count in one call and
    // there is no way in or out of it. That is fine for a program with a GTP
    // time control and wrong for a panel somebody is holding: the count that
    // costs two seconds on nine by nine costs five on thirteen, and "under five
    // seconds" is the requirement the levels are built to.
    //
    // tree_search() accumulates into the tree it is given -- michi itself calls
    // it twice on one tree when it wants to think harder -- so the search can be
    // stopped between chunks without being restarted. What is NOT safe is
    // reimplementing genmove's preamble: an earlier version of this function
    // did, missed part of it, and produced a tree in which PASS won every
    // playout and every real move lost every one. So the preamble below is
    // genmove's, line for line, and only the loop is ours.
    //
    // is_better_to_pass() is deliberately not called. It runs
    // compute_all_status(), which segmentation faults on a nearly full board,
    // and it can only answer yes when the opponent has just passed -- which the
    // position handed to us never records, because the stones are PLACED rather
    // than played. Passing is the app's decision and it is taken in GoEngine.
    N_SIMS = simulations;
    gGame->time_init = 0;

    free_tree(gTree);
    gTree = new_tree_node();
    memset(gOwnerMap, 0, BOARDSIZE * sizeof(int));
    memset(gScoreCount, 0, (2 * N * N + 1) * sizeof(int));
    nplayouts_real = 0;

    uint32_t began = nowMs != NULL ? nowMs() : 0;
    Point pt = PASS_MOVE;
    int done = 0;
    // The first chunk is small because nothing is known yet about how long a
    // simulation costs on this chip at this board size. Every chunk after it is
    // sized from the rate actually measured, which is why the budget holds
    // across both boards without a constant per board.
    int chunk = 8;
    while (done < simulations) {
        int want = simulations - done;
        if (want > chunk) want = chunk;
        pt = tree_search(gPos, gTree, want, gOwnerMap, gScoreCount, 0);
        done += want;
        if (nowMs == NULL) { chunk = 256; continue; }

        uint32_t spent = nowMs() - began;
        if (spent >= budgetMs) break;
        if (spent == 0) spent = 1;
        // Sixty percent of what is left, so a chunk that runs slower than the
        // measured rate still lands inside the budget rather than through it.
        double perMs = (double)done / (double)spent;
        double afford = perMs * (double)(budgetMs - spent) * 0.6;
        if (afford < 1.0) break;
        chunk = (int)afford;
        if (chunk > 512) chunk = 512;
    }

    gLastMs = nowMs != NULL ? nowMs() - began : 0;
    gLastSimulations = nplayouts_real;

    if (pt == PASS_MOVE || pt == RESIGN_MOVE) pt = best_non_pass();
    if (pt == PASS_MOVE || pt == RESIGN_MOVE) return -1;
    int row, col;
    if (!our_point(pt, size, &row, &col)) return -1;
    return row * size + col;
}

int michi_bridge_last_simulations(void) { return gLastSimulations; }
uint32_t michi_bridge_last_ms(void) { return gLastMs; }
