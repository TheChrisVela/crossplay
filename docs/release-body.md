[Install it](https://crossplay.ma-r-s.com/#get) · [which file, and by hand](https://github.com/ma-r-s/crossplay/blob/xteink/docs/install.md) · [earlier releases](https://github.com/ma-r-s/crossplay/blob/xteink/docs/release-notes.md)

### What is new in 1.13.0

- Go is the twenty-first game on the shelf. Play someone sitting next to you on the same device, someone else in the room on their own device, or the machine. Nine by nine, or thirteen by thirteen when you want a longer game.
- The opponent is michi-c2, a real Go engine rather than one written for this device. On MEDIUM it plays level with GNU Go 3.8 at its strongest setting, which is two or three stones above the engine this app shipped with first. It thinks for at most 1.2, 2.5 or 4 seconds a move depending on the level, and that is a ceiling rather than an average: the search is stopped against a clock, so a move never runs longer than the level promises however full the board gets.
- EASY, MEDIUM and HARD now say how hard the machine thinks, and nothing else. How many stones you are spotted, which colour you take and which board you play on are three settings of their own. EASY used to hand you two free stones as well, and there was no way to have one without the other.
- A handicap is yours to set, 2 to 5 stones, at the komi a handicap game is properly played at.
- The front door draws the game you are in the middle of, with the move number under it. It falls back to your last finished game, and to nothing at all before your first.
- RESUME has a bin on the end of it. The row picks your game back up; the square throws it away. It is only there when there is a game to throw away.
- The machine knows when the game is over. It passes once you have passed and passing wins it, instead of filling in neutral points you were taught are worthless.
- A game survives being put down. The board, whose turn it is, the stones you agreed were dead and the rule that stops a ko repeating all come back exactly as you left them.

