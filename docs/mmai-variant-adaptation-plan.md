# mmai Variant Adaptation Plan — Poof-Pente, Boat-Pente, O-Pente, Connect6

Target engine: `/Users/waliedothman/mariposa/coding/pente.org-project/react_mmai/MMAIWASM/Ai.cpp` (+ `Ai.h`, `mmai.cpp`), compiled to WASM via `compile.sh`. Rules oracle: `/Users/waliedothman/mariposa/coding/pente.org-project/react_mmai/src/Classes/GameClass.js`. Canonical game IDs: server `GridStateFactory` (Pente=1, Keryo=3, Poof=11, Connect6=13, Boat=15, O-Pente=25; even = Speed twin, identical board rules).

---

## 0. Prerequisite refactor: shared variant config (RECOMMENDED FIRST)

The reports establish that variant behavior is smeared across the engine in a way that punishes per-variant forking:

- Variant switch today is `int game` → `Kgame` flag: `Move()` Ai.cpp:407-410 sets `Kgame=1` iff `game==2`, but `dmov()` tests `game==2` directly at 542. Any new flag must be threaded through **both**.
- Capture mechanics live in **three places that must change in lockstep**: `dmov()` 523-551 (real board), `Tree()` 982-1043 (search make-move), `Score()` g-codes 1342-1428 + c2..c5 accounting 1450-1481, 1591-1602.
- Capture-win formula `Kgame*5+9` (and near-threshold `+7`, `+6`) is duplicated at 7 sites: Score() 1442, 1446, 1478, 1594, 1600-1603.

**Refactor**: introduce a `VariantConfig` struct populated once in the `CAi` constructor from the canonical game ID:

```cpp
struct VariantConfig {
  bool capturePairs;      // Pente family: true; Connect6: false
  bool captureTriples;    // Keryo, O-Pente
  int  capWinThreshold;   // 10 (Pente/Poof/Boat), 15 (Keryo/O), INT_MAX (Connect6)
  bool poofPairs;         // Poof, O-Pente
  bool poofTriples;       // O-Pente only (GameClass #detectKeryoPoof is used ONLY by O-Pente)
  bool boatWin;           // Boat, O-Pente: five must survive one opponent reply
  int  winRowLength;      // 5; 6 for Connect6 (blocked on pattern tables, see §4)
  int  stonesPerTurn;     // 1; 2 for Connect6 (first turn 1)
  bool tournamentOpening; // Pente family true; Connect6 false
  bool useOpeningBook;    // false for Poof/Boat/O/Connect6 (book is plain-Pente lines)
};
```

Replace: `Kgame` reads in Tree/Score, the `game==2` in dmov 542, all `Kgame*5+9/+7/+6` occurrences (→ `cfg.capWinThreshold`, `-2`, `-3`), and the `hlim=4/5` window in Score() 1313-1314. `Kgame` can remain as a derived alias (`cfg.captureTriples`) to limit diff size in Score's ~20 conditionals.

Map incoming game IDs in `mmai.cpp`/constructor: accept **canonical server IDs** (1/2→Pente, 3/4→Keryo, 11/12→Poof, 13/14→Connect6, 15/16→Boat, 25/26→O-Pente). Keep legacy `game==2 → Keryo` accepted for backward compat with the current saga until the JS side is updated.

Effort: ~2-4 days incl. regression test that refactored engine plays byte-identical moves to current engine for Pente and Keryo (deterministic seed; note `arc4random_uniform` in opening abandonment at cmove 715 — pin `obfl=0` for the comparison runs).

This refactor is justified by the reports and pays for itself by variant 2; do it before Poof.

---

## 1. Poof-Pente (game 11/12)

### Rule delta vs current engine
Engine today implements plain Pente self-capture immunity: playing into `XO_X` to make `XOOX` is safe. Poof-Pente inverts this: the flanked own stones are removed ("poofed") and **credited to the opponent's capture count**. Capture win threshold stays 10. Per the JS oracle (`#addPoofPenteMove` GameClass L676-680), Poof-Pente = pair captures + **2-stone poof only** (placed stone + one adjacent own stone flanked at both ends; 8 direction patterns, GameClass L1333-1419). Multi-direction poofs in one move remove up to 3 stones (placed + 2 neighbors, per the L1416-1418 tally). Rules doc adds: captures and poofs resolve simultaneously, **then** 5-in-a-row is checked; game continues at 10-10 (no draws) until one side has an advantage.

### Ai.cpp changes
1. **`dmov()` (real board)** — after the capture block (insert after Ai.cpp:551): scan 8 directions from `(sx[tn], sy[tn])` for pattern `ENEMY-ME(placed or neighbor)-ME-ENEMY` where the placed stone is one of the flanked pair; remove both own stones (set `-1` like captures do at 533-540) and add 2 to the **opponent's** counter `ccc[0][3-cp]` per direction (+1 net adjustment for the shared placed stone on multi-direction poofs, matching GameClass tallying).
2. **`Tree()` (search make-move)** — insert after the capture block (after 1043), **before** the win check at 1058:
   - Reuse the generic undo machinery: push poofed stones onto `capx/capy/capv[lvl]` with `capv` = own color; the three restore paths (1135-1147, 1154-1166, 1184-1199) write back `capv` values generically, so no restore changes needed for the *neighbor* stone.
   - The played cell: restore paths blindly reset it to `-1` (lines 1139, 1158, 1190). On poof, set the played cell to `-1` at make time — this is exactly what restore expects, so it composes. Do **not** push the played cell onto the capture stack.
   - **Hash**: the move-placement XOR at 977-980 must be reversed for the poofed placed stone, and the neighbor stone removed following the capture-hash pattern at 1007-1012. Fold the opponent-credited count into the capture-count hash term (1046-1047 uses `cc1+cc2`, so incrementing `cc[lvl][opponent]` handles it).
   - Update `cc[lvl][opponent] += poofed` (opponent gains captures).
3. **Win ordering**: with poof applied before line 1058, a poofing move can no longer trip `scr=12000-lvl` off a vanished line — but `Score()` runs at candidate-ordering time (`Eval` in the fl1 block 849-862) and will still *score* a suicidal move as if it built a tria/four. Fix in `Eval()`/`Score()` (below).
4. **Threshold edge case**: `Score()` 1603 declares capture win at `cc+cap1 > threshold-1`. In Poof, both sides can hit 10; win only on advantage. Change the terminal test to `cc[fr] >= 10 && cc[fr] > cc[3-fr]` under `cfg.poofPairs`. Low-frequency case; acceptable to defer.

### Eval implications
- **Detect poof inside `Score()`**: the g-code neighbor scan (1342-1428) already reads own-run lengths and flanking stones per direction; add a poof-detection branch. A poofing move must be scored as *losing* the vanished stones: mirror the captured-stones subtraction loop in `Eval()` 1232-1245 (subtract each vanished stone's `Score()` contribution with the same 10% damping), plus a capture-economy penalty analogous to `cap1*160` credited to the opponent, plus the ±1024/2048 urgency bonuses (1475-1481, 1599-1602) when the poof pushes the opponent near 10.
- **Defensive-fill assumption breaks**: the engine currently treats filling `XO_X` as a safe block, and Score's "stones newly protected" credit (`-c3[i]*25`, 1591-1597) assumes occupying a flank protects a pair. Under poof, filling into a flank is material loss — gate the c3 protection credit and re-check the "block four" forcing-reply filter (`minscr=3000`, 853-855) so a poofing "block" isn't the only candidate kept.
- The −12 own-vulnerable-pair penalty (1434-1437) becomes more important (vulnerable pairs are now also poof traps for your own follow-ups); consider raising it under poof after testing.

### WASM / React / rebuild / testing — see §5-§8.

### Effort: 4-6 days engine work after the §0 refactor (poof scan x2, hash/undo verification, Eval poof scoring), +2 days differential testing.

### Risks
- **Highest**: Score()/Eval() mis-scoring poof moves (engine happily "builds" lines that vanish, or refuses ever to poof even when strategically correct). Mitigation: differential self-play vs GameClass legality + fixed-position tactics suite.
- Hash desync from the three-stone removal (placed + neighbor + capture combos in the same move) → transposition-table corruption that is silent and only shows as weak play. Mitigation: debug build with full-board rehash assertion after make/unmake.
- The three duplicated restore paths must all stay consistent — verified by the rehash assertion.
- Do **not** replicate the JS oracle's known bugs (GameClass `#detectPenteOf` never counts row/col 0 stones, L1569+ `i > 0` guards).

---

## 2. Boat-Pente (game 15/16)

### Rule delta vs current engine
Mechanics identical to Pente (pair captures, threshold 10, tournament opening) **except the five-in-a-row win is provisional**: after a five is made, the opponent gets one move; if they can capture across the five (a capture removing stones from the line, breaking it), the game continues. The five wins only if it survives that reply. Capture win (10) is unchanged and immediate.

### Ai.cpp changes
The engine funnels **both** win types through `sco[fr] >= 10000`: five-patterns come out of the `pAt/pAs` table walk (loop exit at Score() 1588, clamp 1605), capture win is set explicitly at 1603, and `Eval()` (1222-1225) / `Tree()` (1058-1068) treat any `>=10000` as terminal. Boat requires splitting these signals:

1. **Separate the signals**: have `Score()` record *which* condition fired (row vs captures) — e.g., a member flag set alongside `sco[fr]=12000` at 1603 vs the pattern path. Capture wins stay terminal.
2. **Row-win becomes conditional** (under `cfg.boatWin`): when a five is detected, run a **static breakability check**: for each maximal own run of length ≥5 through the placed stone, test every 5-window; the five stands iff some window exists in which *no* stone is part of a capturable pair (pattern `_XX O` / `O XX_` with the flanking empty actually playable, i.e. board value 0 or −1). This reuses the vulnerable-pair geometry Score() already computes in the g-code section (1434-1437 detects exactly "own pair capturable"). Overline care: a 6-run with one capturable stone can still contain a clean 5-window — check windows, not the whole run.
   - Unbreakable five → win as today (12000−lvl).
   - Breakable five → do **not** score terminal; award a large-but-nonterminal score (e.g. 9000, below the 9500 cap at 1274-1277) so the search naturally explores the opponent's capture reply on the next ply — Tree() already searches ply-alternating replies, and the capture there removes the line, so the follow-up evaluation is organically correct.
3. **`Tree()` 1058-1068**: gate the `sc>=10000 → win` shortcut on the "unconditional" flag; the ply-1 30000 short-circuit (1063-1067) likewise.
4. `dmov()` needs **no change** (captures unchanged; engine does no real-board game-over detection — JS owns that).

### Eval implications
- The static breakability check *is* the eval change; beyond it, increase the own-vulnerable-pair penalty (1434-1437) for stones participating in fours/fives, so the engine prefers building non-capturable fives. Tune after baseline testing.
- Depth parity: a "win" may now be win-in-2 (five + survive). The `ciel[lvl][cp]` win-distance capping (960-961, 1110-1111, 1150-1151) still works because the win is only scored terminal when genuinely unbreakable.

### Testing caveat — no JS oracle in this repo
GameClass.js implements Boat as plain Pente (branch L539-546, no boat win logic; `isGameOver` ignores 15/16). The rules oracle for the boat win condition is the **server**: `pente.org/dsg_src/java/org/pente/game/BoatPenteState.java` (extends `SimplePenteState`). Port its five-survival semantics into the test fixtures; do not trust GameClass for boat terminal states.

### Effort: 3-5 days after §0 (signal split + breakability check + Tree gating), +2 days building fixtures from BoatPenteState.java.

### Risks
- **Highest**: edge cases in the breakability check — overline windows, a capture that removes a line stone via a pair only half-inside the line, double fives in one move, and a five made *by the capture-executing move itself*. Enumerate these as unit fixtures from BoatPenteState.java before writing the C++.
- Conflating the two `>=10000` signals anywhere (7+ touch points) silently reverts boat to instant-win Pente — grep-audit every `10000`/`12000`/`30000` site.

---

## 3. O-Pente (game 25/26)

### Rule delta vs current engine
Union of Keryo + Poof + Boat: pair **and** triple captures, threshold **15**; pair **and triple** poofs; boat provisional five. Per GameClass `#addOPenteMove` (L692-698) = `#detectPoof` + `#detectKeryoPoof` + `#detectPenteCapture` + `#detectKeryoPenteCapture`. `#detectKeryoPoof` (3-stone poof: placed stone as end of a flanked own triple — 8 patterns — or as its center — 4 patterns) is used **only** by O-Pente.

### Ai.cpp changes
Almost pure composition once §1 and §2 land on the §0 config:
- `cfg = {capturePairs:1, captureTriples:1, capWinThreshold:15, poofPairs:1, poofTriples:1, boatWin:1}`. Keryo capture code paths already exist and are gated correctly after §0.
- **New work: triple poof.** Extend the poof scan in `dmov()` and `Tree()` to length-3 own runs flanked at both ends, with the placed stone at either end or center. Implement as a symmetric line scan (per-axis: walk own run through placed stone, check both flankers, run length exactly 2 or 3 per `cfg.poofTriples`) rather than transliterating GameClass's 24 unrolled patterns — and **fix, don't port, the GameClass "down" bounds bug** (L1490 checks `j+2 < 19` but L1492 reads `[i][j+3]`; require `j+3 < 19`).
- Undo capacity: `capx[19][24]` (max 8 dirs × 3) — a worst-case capture+poof move can exceed prior totals; audit the 24-slot bound (max realistic: 4 poof axes × 3 + captures; verify against 24, enlarge if needed).
- Boat check under Keryo/O: "capture across the five" includes **triple** captures — the §2 breakability check must test triple-capturable patterns (`_XXX O` / `O XXX_`) too when `cfg.captureTriples`.
- Score(): the Keryo-specific heuristics (75-point triple threats 1444-1447, hlim=5) apply as-is; add triple-poof awareness to the §1 poof scoring (subtract 3 vanished stones).

### Effort: 2-4 days on top of Poof + Boat (triple poof x2 sites, breakability extension, capacity audit), +2 days combinatorial edge-case fixtures.

### Risks
- **Highest**: rule-interaction explosion — capture-and-poofed in the same move, poof that breaks your *own* provisional five, simultaneous 15/15 race with poof credits. GameClass covers legality for all but boat; build a randomized differential fuzzer (engine make/unmake vs GameClass `addMove`) and run millions of positions.
- Threshold-15 economics with poof credits may need retuning of the ±1024/2048 urgency bonuses; expect a tuning pass.

---

## 4. Connect6 (game 13/14) — new AI on the mmai base

### Rule delta vs current engine
No captures ever; win = **6+ in a row** (overlines win); Black opens with **one** center stone, thereafter **two stones per player per turn** (player-by-move-index: 1,2,2,1,1,2,2,… — GameClass L284-285/455/537: player 1 iff `moves.length % 4 ∈ {0,3}`); no opening restrictions, no opening book; draws possible on a full board.

### Work item 1 — pattern knowledge (the blocker)
All line knowledge (pairs/trias/tesseras/fives, open/closed shapes, fukumi geometry) is frozen in the binary tables `files/pente.tbl` (automaton, 943×4 shorts) and `files/pente.scs` (912×14 shorts). **There is no generator in the repo.** A 6-in-a-row variant cannot be reached by code edits alone. Two paths:
- **(a) Write a table generator**: reverse-engineer the automaton format (Score()'s walk at 1495-1530 documents the transition semantics: item 1=enemy/edge, 2=empty, 3=friend; terminals index `pAs` rows at 1531-1579) and emit 6-length tables. Window grows from `hlim=4/5` to 6+ (Score() scan bounds `cx<19` at 1323/1349/1505 tolerate this, but array sizes and the c2 loop need audit). Fukumi columns can be zeroed initially.
- **(b) Replace the table walk with direct line evaluation** for Connect6 only (count run lengths / open ends / broken-6 shapes in code). Simpler to get right, slower, and forfeits the tuned Pente scores — acceptable since none of those scores transfer to Connect6 anyway.
Recommendation: **(b) first** to get a playing engine, (a) later if strength demands it.

### Work item 2 — two stones per turn (search structure)
One ply == one stone == strict alternation is structural in Tree(). Two options from the ai-core analysis:

- **Option 1 (recommended): ply = one stone, fix the player function.** Replace every `cp = 2 - tn%2` (getMove 369, dmov 516, cmove 658) and the Tree rotation `fr = cp-1+lvl; while(fr>np) fr-=np` (830-833) with a shared `playerForMove(n)` implementing the 1,2,2,1,1,2,2,… sequence. Consequences that must be re-derived, not assumed:
  - `3-fr` opponent math (854, 1052, 1061, 1082-1094, 1236, 1264, 1278) stays valid per-node (still 2 players), but the *parity assumptions* behind the sibling cutoff `scr[lvl] <= scr[lvl-2]` (1133-1149) break — lvl-2 is no longer "same player two plies ago". Rework the cutoff to compare against the most recent same-player ancestor ply.
  - `mxvt/mxvf` VCT parity tables and the fukumi/VCT extension machinery (932-958, exfl/exel) encode Pente threat parity — **disable extensions entirely for v1** (`vct=0`).
  - Transposition depth semantics (`pHashD == lvl`, 1049): still coherent if lvl counts stones, but two board states reached with different intra-turn stone orders are transpositions — the Zobrist hash handles this natively (order-independent XOR); keep TT on.
  - Defense weight `sco[fr] - sco[3-fr]*4` (1278): the 4× was tuned for "opponent replies with one stone". In Connect6 the opponent replies with two — defense is *more* urgent; expect retuning (start at 4, sweep).
- **Option 2: ply = stone pair.** Joint pair generation over the top-k singles ≈ k(k+1)/2 children (k=20 root → 210); doubles all undo bookkeeping; halves effective TT depth granularity. Stronger move coordination but a much bigger rewrite. Defer.

**Branching/threat reality check**: in Connect6 a single four is not forcing (defender places two stones and can kill two separate threats per turn); forcing sequences require **double threats**. The engine's whole forcing-move economy (open-four filter `minscr=3000` at 853-855, fukumi 4-3 logic) is Pente-shaped. v1 ships as plain fixed-depth search with the option-1 player function and no extensions; a real Connect6 threat-space search (Wu-style double-threat counting) is a v2 project.

### Work item 3 — interface (two moves out)
`getAIMove` returns one int. Options: (a) **packed return** `m1*361 + m2` (fits int; 361² < 2³¹) with `m2 = 361` sentinel for the single-stone first turn; (b) two WASM calls (engine is stateless and replays anyway) — but stone 1 chosen without knowledge of stone 2 is tactically weak; the search should pick the stone-pair line and the second call would redo the work. **Recommend (a)**: search to even stone-depth, return the first two stones of the PV packed. `mmai.cpp` gains the packing; keep the old return shape for 1-stone games.

### Work item 4 — remove Pente scaffolding
Under `cfg`: skip dmov 523-551 and Tree 982-1043 capture blocks and all Score capture scoring (g-codes, c2..c5, cap1*160); capture hash fold (1046-1047) becomes inert (cc always 0); disable turn-1/2/3 forced openings **except** keep turn-1 center (matches pente.org Connect6), kill `om2/op2/om3` and set `obfl=0`; tournament ring off.

### Fork vs shared core
Adopt the §0 shared core for the Pente family regardless. For Connect6: **start as a branch/fork of the refactored core**, because work items 1-2 touch structural code (pattern walk, ply/player logic, cutoff parity) that the Pente variants must not inherit risk from. If the option-1 player-function abstraction and eval-plug (table walk vs direct scan) prove clean, merge back behind `cfg.stonesPerTurn`/`cfg.winRowLength`; otherwise keep `AiC6.cpp` as a sibling sharing `VariantConfig`, the board/undo/TT plumbing, and `mmai.cpp`.

### Effort: 3-6 weeks (eval rewrite 1-2w, turn/search surgery 1-2w, interface + plumbing 2-3d, tuning/testing 1w+). Treat as a separate project after the Pente-family variants.

### Risks
- **Highest**: playing strength. Correct-but-weak is the likely v1 outcome (no threat-space search, untuned eval); set expectations that Connect6 launches at lower effective level than Pente.
- Hidden parity assumptions beyond the enumerated sites — the sibling cutoff and ciel logic need line-by-line audit under the new player function.
- Eval rewrite discards all tuned pattern scores; every constant in §7-magic-number territory (10000/12000, caps, 4× defense) is up for grabs.

---

## 5. WASM interface changes (all variants)

File: `MMAIWASM/mmai.cpp` (sole export `getAIMove(game, level, openingBook, *moves, numMoves)`).

- **Widen `game` to canonical GridStateFactory IDs** (map even Speed IDs to their odd base inside the engine). Constructor (`CAi::CAi`, Ai.cpp:16-23) builds `VariantConfig` from it. Keep accepting legacy `2 = Keryo` during transition.
- Connect6: packed two-move return (§4 item 3). No signature change needed; document the encoding in mmai.cpp and the saga.
- No new exports required; `_malloc`/`_free`/`ccall`/`HEAPU8` unchanged.

## 6. React plumbing changes

- **`src/redux_saga/sagas.js:35-36`** — delete the variant collapse (`g=1; if (game.game===3) g=2;`); pass `game.game` straight through. For Connect6, after `ccall`, unpack `m1/m2` and dispatch two sequential `ADD_MOVE`s (GameClass `currentPlayer` already implements the %4 turn pattern, so `isMyTurn()` gating in the saga keeps working; verify the saga's re-entry no-op still holds between the AI's two stones — it won't be the user's turn until both land, so guard the "AI should move" branch with a move-count check).
- **`src/redux_reducers/rootReducer.js:68-79`** — replace the 1↔3 `CHANGE_GAME` toggle with a variant selector payload (1, 3, 11, 13, 15, 25); UI in `src/Pages/GameInfoPanel.js:43`.
- **GameClass.js gaps to close (it's the client-side referee)**:
  - `isGameOver` (L145-169) implements capture-win only for games 1 and 3 — add 11 (10), 15 (10), 25 (15).
  - Row-win (`#detectPenteOf`) is only wired for games <5 (addMove L500-513) — wire it for 11/15/25, with the boat survival rule for 15/25 (port from `BoatPenteState.java`); Connect6 needs a 6-in-a-row detector (none exists).
  - Fix the two known GameClass bugs before using it as an oracle: `#detectKeryoPoof` down-direction bounds (L1490/1492) and `#detectPenteOf` row/col-0 exclusion (L1569+ strict `> 0` guards).
- Opening: `START_GAME` saga dispatches `ADD_MOVE 180` — correct for all four variants (Connect6 also opens center, one stone).

## 7. Rebuild + multi-platform propagation

1. Edit `MMAIWASM/Ai.cpp`/`Ai.h`/`mmai.cpp` → run `MMAIWASM/compile.sh` (emcc, embeds `files/`) → **manually copy** `ai.js` + `ai.wasm` to `react_mmai/public/` (no automated copy step exists; md5-verify the pairs).
2. Native fast-iteration loop: `test.cpp` builds a CLI binary — extend it to take game ID + move list for fixture testing without the WASM round-trip.
3. Other copies (do **not** blind-copy — all three `Ai.cpp` md5s differ; server↔wasm diff is 882 lines):
   - `pentelive-android/app/src/main/jni/Ai.cpp` (NDK rebuild) — apply the logical patch.
   - `pente.org/dsg_src/mmai/Ai.cpp` (JNI lib rebuild) — apply the logical patch.
   - `penteLive-iOS/test1/MMAI.m` — full **manual Objective-C port** (independent reimplementation, currently has zero keryo/poof/boat logic). Budget separately; the §0 VariantConfig should be mirrored as a config object there.
   - Sequence: land + validate on WASM first; propagate per-platform afterward.

## 8. Testing approach

1. **Differential legality fuzzer (primary)**: random/self-play games where every engine move (and every simulated make in a debug replay) is re-applied through GameClass.js (`addMove`) and board + `captures[]` are compared cell-for-cell. Covers Poof (games 11), O-Pente legality (25), Connect6 turn order (13). Node harness calling the WASM build + GameClass directly.
2. **Boat exception**: GameClass has no boat win logic — build terminal-state fixtures from `org/pente/game/BoatPenteState.java` semantics (five + breakable → continue; five survives reply → win; capture win immediate).
3. **Make/unmake integrity**: debug build with a full-board Zobrist rehash + board-snapshot assertion after every Tree() make and after each of the three restore paths (1135-1147, 1154-1166, 1184-1199). Non-negotiable for Poof/O (multi-stone removals).
4. **Pente/Keryo regression**: post-§0-refactor engine must reproduce current engine's moves on a fixed suite (obfl=0 to remove randomness).
5. **Tactics fixtures per variant**: poof-as-blunder, poof-as-sacrifice, boat five-breaking capture, O-Pente capture+poof same move, Connect6 double-threat defense.
6. **Strength**: engine-vs-engine ladders (new variant level N vs level N-1) to sanity-check that level scaling still behaves.

## 9. Effort + order summary

| Step | Effort | Depends on |
|---|---|---|
| §0 VariantConfig refactor + regression | 2-4 d | — |
| Poof-Pente | 4-6 d engine + 2 d test | §0 |
| Boat-Pente | 3-5 d + 2 d fixtures | §0 |
| O-Pente | 2-4 d + 2 d fixtures | Poof + Boat |
| Connect6 | 3-6 wk | §0; independent of the others |
| React/GameClass plumbing (all) | 2-3 d | parallel |
| Per-platform propagation (Android/server/iOS) | 1-2 d each; iOS ~1 wk (manual port) | after WASM validation |

Recommended order (matches user priority and dependency graph): **§0 → Poof → Boat → O-Pente → Connect6**.
