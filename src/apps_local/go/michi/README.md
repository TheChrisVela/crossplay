# michi-c2, vendored

The Go engine. **michi-c2** by Denis Blumstein, a C recoding of Petr Baudis's
`michi.py`, which is itself a compact expression of the MCTS+RAVE design from
the MoGo line of papers. MIT, stated in `UPSTREAM-README.md`:

> Michi-c2 is distributed under the MIT licence. Now go forth, hack and peruse!

There is no `LICENSE` file in any repository in the Michi family and most source
files carry no header, so the grant above is the grant, and the attribution in
`THIRD-PARTY-NOTICES.md` had to be written by hand rather than copied.

## Why this and not the engine we had

Measured, not asserted, over about 1,200 9x9 games:

| | vs GNU Go 3.8 level 10 |
| --- | --- |
| michi-c2 at 500 simulations | level |
| michi-c2 at 1,500 | 60% |
| the engine this replaced, at 8,000 playouts | 8% |

**michi-c2 at 500 simulations beats the old engine at 8,000.** Both run at about
the same raw playout rate; the difference is entirely what each playout knows.
That is two or three stones, and it is the reason this is a port rather than
another round of tuning.

## What was taken, and what was not

Six files of the nine in the project: `board.c/h`, `board_util.c`, `michi.c/h`,
`patterns.c`, `params.c`, `control.c`, plus `non_portable.h`. **Nothing** from
`ui.c`, `sgf.c`, `debug.c` or `main.c` -- and that is a measurement rather than
a hope: linking the core alone leaves exactly two undefined symbols, both
supplied by `MichiShim.c`.

The large pattern files (`patterns.prob`, `patterns.spat`) are **not** used and
must not be reached for. They are megabytes, they derive from a commercial game
database, their download URL is dead and the Wayback Machine never archived one
of them. The numbers above are all without them.

## The fork's changes, which are eight

Every one is marked `FORK CHANGE` in the source so a future sync can find them.
Three are ports. **Five are bugs**, and each is one only a build like this one
reaches: a maximum N larger than the board being played, a device with no
`stderr` and no x86, and an app that plays on where upstream would have passed.

**The three ports:**

- **`board.h`: `N` is 13, not 19.** `N` is the compile-time MAXIMUM; the size
  actually played is `pos->size`, set at runtime. One build therefore serves
  both 9x9 and 13x13, and the arrays are sized for the larger.
- **`board_util.c`: `log_fmt_s` tolerates a null `flog`.** That FILE\* is opened
  by `ui.c`, which is not vendored, so null is the normal state here rather than
  an error. The other two log functions funnel through this one.
- **`board_util.c`: `michi_malloc`/`michi_calloc` allocate from PSRAM** on
  ESP32, falling back to internal RAM. The search tree is hundreds of kilobytes
  and internal SRAM does not have it to spare.

**The five bugs:**

- **`michi.c`: `expand()` allocates `slist_size(moves)+2` child slots.** The
  array is NULL-terminated and `free_tree()` walks to the NULL, but the block
  that adds a PASS child when a node has two or fewer children writes it into
  the terminator's slot whenever every candidate was legal. The walk then runs
  off the end and frees whatever it reads. Upstream cannot reach it because
  `genmove()` passes out of a decided game before the board is down to two
  points; this fork decides passing itself and plays on, so it is the last two
  moves of nearly every game.
- **`board.c`: `line_height()` subtracts `N - size`.** `empty_position()` lays
  a board of `size` out at array rows `N-size+1..N`, not `1..size`. With N
  equal to the board being played the two agree, which is why upstream never
  sees it; with N=13 and a 9x9 game it returned the first-line penalty for
  tengen and no penalty at all for the real first line.
- **`michi.c`: `mark1`, `mark2` and `buf` are `extern` here.** Upstream defines
  each of them twice -- `michi.c` and `board.c`, `michi.c` and `control.c` --
  which links only under `-fcommon`. GCC 10 turned that off.
- **`non_portable.h`: no `<x86intrin.h>`.** It is included on any GCC that is
  not Apple's and it is vestigial: the two functions under it are compiler
  builtins. The Xtensa cross-compiler has no such header.
- **`board_util.c`: `print_board()` returns on a null `FILE*`.** The only
  caller that reaches it with `flog` unset is `michi_assert`'s failure path,
  and a crash inside the code that reports a crash reports nothing.

## Speed, measured

One build, N=13, 1,500 simulations a move, on a laptop:

| Board | Playouts a second | A move |
| --- | --- | --- |
| 9x9 | 19,800 | 57 ms |
| 13x13 | 11,150 | 115 ms |

A 9x9-only build is about 20% faster (47 ms) -- that is what the larger arrays
cost -- and it is not worth a second copy of the engine.
