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

## The fork's changes, which are two

Both are marked `FORK CHANGE` in the source so a future sync can find them.

- **`board.h`: `N` is 13, not 19.** `N` is the compile-time MAXIMUM; the size
  actually played is `pos->size`, set at runtime. One build therefore serves
  both 9x9 and 13x13, and the arrays are sized for the larger.
- **`board_util.c`: `log_fmt_s` tolerates a null `flog`.** That FILE\* is opened
  by `ui.c`, which is not vendored, so null is the normal state here rather than
  an error. The other two log functions funnel through this one.

## Speed, measured

One build, N=13, 1,500 simulations a move, on a laptop:

| Board | Playouts a second | A move |
| --- | --- | --- |
| 9x9 | 19,800 | 57 ms |
| 13x13 | 11,150 | 115 ms |

A 9x9-only build is about 20% faster (47 ms) -- that is what the larger arrays
cost -- and it is not worth a second copy of the engine.
