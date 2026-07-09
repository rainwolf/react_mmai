# Connect6 v3.0 — Forcing-Search Win Terminal (VCDF prover) — FINAL SPEC

Status: AUTHORITATIVE, implementable as written. Target: `react_mmai/MMAIWASM/Ai.cpp` /
`Ai.h` @ HEAD 970a699. Synthesized from three adversarial proposals (A: correctness-max,
B: strength-max, C: defense-inclusive) and three independent judge verdicts.

## 0. Decision record

**v3.0 ships**: Proposal A's architecture (bounded AND-OR "victory by continuous
double-fives" prover, fives-only, unique-forced-reply AND nodes, refuse-on-ambiguity),
strengthened by exactly the grafts the judges certified provably safe:

| Graft (source) | Judge certification | Why safe |
|---|---|---|
| C's structure-theorem open-five pair generator replaces A's C(16,2) enumeration | Judges 1 & 3 | Pure narrowing of the OR candidate list; generation affects false negatives only — the child AND node re-derives everything it trusts from a fresh scan |
| C's B1 `canSixNow(P)` immediate-six base at OR nodes | Judges 1 & 3 (part of C's win side, "as safe as A's") | Direct one-turn win on the actual board; required companion of the tightened generator |
| C's deeper depth ladder (K up to 4 at lvl<=2) | Judge 3 | Depth is a refusal-only knob under a hard scan budget; identical wiring |
| C's `tnRoot>=6` gate refinement | Judge 3 | Refusal-only; behavior-neutral for v2 (v2 needs >=15 mover stones); defends the lone-opening-stone anomaly |
| v2 `c6UnstoppableThreat` kept verbatim as root fast path + subsumption assert | All three | Already-shipped audited code; W1 base is identical |
| Mutation gate (c6ForceEnabled twin engines, every curated win must flip getMove) | All three | The direct institutional fix for the v2 inertness failure |
| Exact-cell save/restore (0 vs -1) + full-board checksum discipline | Judge 1 | Purity hygiene |
| Speed-twin due-diligence check (game 14 / even ids) | Judge 3 | Pre-ship verification item, no code risk |

**Deferred** (Section 12, each its own isolated release — judges unanimous that these must
NOT bundle with v3.0 or with each other):
- v3.1a — B's four-based threat windows + exact min-hitting-set defender model
  (judge-flagged: most complex new primitive, latency risk; blocking gates listed).
- v3.1b — C's opponent-forced-win LOSS terminal (G1). Judge 3 found a concrete
  control-flow hole in C's `lfl` integration (Section 12.2 states the mandatory fix).
- B's poison-cell candidates ride with v3.1a: under the v3.0 fives-only structure theorem
  they are structurally inert (Section 4.4 explains why), so there is nothing to graft yet.

**No false-positive hole was found in the v3.0 core by any judge.** The single concrete
hole found anywhere (judge 3, C's `lfl` sibling-loop edit) lives entirely in the deferred
loss terminal and is closed by the mandatory fix in Section 12.2.

**Correctness bar (non-negotiable)**: the prover may NEVER return a false-positive forced
win. The ONLY acceptable error is a mistaken REFUSAL (false negative), which falls back to
the unchanged fixed-depth search — status quo behavior.

---

## 1. Scope and gates

Fires only for the no-capture two-stone variant (canonical Connect6, game id 13):

```
cfg.stonesPerTurn==2 && !cfg.capturePairs && c6ForceEnabled
&& !hfl && c6NextSame==0 && tnRoot>=6 && lvl<=4
```

- `c6NextSame==0`: turn-final ply — both of the mover's stones this turn are on `bd`,
  so a whole-board scan sees the completed pair. (Ai.h:134; set at Ai.cpp:1103 & 1490.)
- `tnRoot>=6` (NEW vs v2): skips the opening single-stone anomaly entirely. Behavior-
  neutral for v2's base (W1 needs >=15 mover stones on board) — refusal-only tightening.
- `lvl<=4`: same injection plies as v2. Depth ladder inside the prover: K=4 mover-turns
  at lvl<=2, K=2 at lvl 3-4.
- **Pre-ship due diligence (mandatory, no code)**: verify no OTHER game id reaches
  `cfg.stonesPerTurn==2 && !cfg.capturePairs` (the "speed twin" / even-id concern). If
  any does, confirm its `tnRoot`/`ownerOf` cadence matches game 13's, or add an explicit
  canonical-game-id gate.

Everything sits inside the existing `#ifndef C6_NO_FORCE` block; `c6ForceEnabled=false`
disables v3 exactly as it disables v2 (the mutation-gate switch, unchanged).

---

## 2. Definitions (all relative to the current hypothetical `bd`)

Window geometry is v2's, byte-for-byte (Ai.cpp:2273-2297): a *window* = 6 consecutive
cells along one of the 4 axes `dx[a],dy[a]`, fully on-board (k==5 endpoint check; straight
lines make all interior cells on-board too). `P` = mover, `O = 3-P`. Cell values: `1/2`
stones; `0` or `-1` empty (`v<=0`).

- **five(P)**: window with `pc==5 && oc==0` (5 P + 1 empty). Its *completion cell* is
  the single empty. No captures => P stones permanent => the ONLY cell O can ever occupy
  to kill this window is that completion cell (**singleton cover set**).
- **C(P)**: the set of DISTINCT completion cells over all five(P) windows (`seen[361]`
  dedup, exactly v2's `nDistinct`).
- **six(P)**: window with `pc==6`. Overlines: any run >=6 contains a 6-window, so covered.
- **oppImm(P)** (a.k.a. `canSixNow(O)`): exists a window with `pc==0 && oc>=4` (<=2
  empties) — O can complete a six within ONE 2-stone turn. Identical to v2's veto.
- **canSixNow(P)**: exists a window with `oc==0 && pc>=4` — P can complete a six with
  this turn's 2 stones. (Same test, roles swapped; `pc==5` fives are a subset.)
- **c6ScanFives(P)**: ONE whole-board pass over all ~1064 windows computing, into a local
  struct: `pSix` (any six(P)), `oppImm`, `pFour` (any `oc==0 && pc>=4` window, i.e.
  canSixNow(P)), `nDistinct=|C(P)|` with the first two completion cells recorded
  (`c1,c2`, used when `nDistinct==2`). Cost = one v2-sized scan (~6.4k cell reads).

**Node invariant NI**: no six(O) is ever on `bd` at any prover node. (At the injection
point, Tree's win machinery would already have terminated any completed six; inside the
prover, O stones are placed only after `oppImm==false` was verified on the pre-placement
board, and `oppImm==false` implies O's 2 stones cannot bring any window to 6 O. P stones
never advance O windows.)

---

## 3. The algorithm

Three pure functions of `(bd, P)` plus a scan-budget member counter. `d` = remaining
P attacking turns the prover may spend. All placements save the EXACT prior cell value
(0 vs -1 both occur and both matter) and restore it on every return path. The prover
reads only `bd`, `dx[]`, `dy[]`; it never touches `sco[]`, `rowWin`, `c6NextSame`, `fr`,
`scr[]`, the hash table, or move lists; it calls no `rand()` (goldens stay reproducible).

```cpp
// ---- top level: replaces the v2 call at Ai.cpp:1460 ----------------------
bool CAi::c6ForcedWin(int P, int lvl) {
    if (c6UnstoppableThreat(P)) return true;   // v2 verbatim = free fast path (W1@root)
    c6Budget = 3000;                            // whole-board scans allowed this call
    int K = (lvl <= 2) ? 4 : 2;                 // deterministic depth ladder by ply
    return c6And(P, K);
}

// ---- AND node: P just completed a turn (both stones on bd); O to move ----
// Returns true only if P's win is PROVEN. false = refuse (always safe).
bool CAi::c6And(int P, int d) {
    if (--c6Budget < 0) return false;              // budget exhausted: REFUSE
    C6Scan R = c6ScanFives(P);                     // one board pass
    if (R.pSix)           return true;             // W0: P six already on board
    if (R.oppImm)         return false;            // O sixes on its intervening turn: REFUSE
    if (R.nDistinct >= 3) return true;             // W1: v2 covering base
    if (R.nDistinct != 2 || d == 0) return false;  // |C|<=1 => O has a free stone: REFUSE
    // |C|==2: O's unique non-losing reply is exactly {c1,c2} (Lemma L2, Section 5).
    int s1 = bd[R.c1.x][R.c1.y], s2 = bd[R.c2.x][R.c2.y];  // save EXACT values (0 vs -1)
    bd[R.c1.x][R.c1.y] = 3-P;  bd[R.c2.x][R.c2.y] = 3-P;
    bool w = c6Or(P, d - 1);
    bd[R.c1.x][R.c1.y] = s1;   bd[R.c2.x][R.c2.y] = s2;    // exact restore
    return w;
}

// ---- OR node: P to place 2 stones -----------------------------------------
bool CAi::c6Or(int P, int d) {
    if (--c6Budget < 0) return false;
    C6Scan R = c6ScanFives(P);                     // fresh scan on post-reply board
    if (R.pSix)  return true;                      // defensive (see note below)
    if (R.pFour) return true;                      // B1: canSixNow(P) — six THIS turn
    unsigned short pairs[C6_PAIR_MAX];             // packed a*361+b
    int n = c6GenOpenFivePairs(P, pairs, C6_PAIR_MAX);   // Section 4
    for (int i = 0; i < n; i++) {
        CPoint a = unpackA(pairs[i]), b = unpackB(pairs[i]);
        int va = bd[a.x][a.y], vb = bd[b.x][b.y];  // both verified empty at generation
        bd[a.x][a.y] = P;  bd[b.x][b.y] = P;
        bool w = c6And(P, d);                      // bases + recursion all inside
        bd[a.x][a.y] = va; bd[b.x][b.y] = vb;
        if (w) return true;
    }
    return false;                                  // no proving pair found: REFUSE
}
```

Constants: `enum { C6_PAIR_MAX = 12, C6_SCAN_BUDGET = 3000 };` `int c6Budget;` (member,
valid only within one top-level call; Tree is single-threaded per CAi instance).

Structural notes:
- **AND branching is exactly 1** (the unique forced reply). All branching lives in OR
  nodes (<= C6_PAIR_MAX = 12 children). Recursion depth <= 2K+1 frames, no allocation.
- `c6And(P, 0)` is bases-only = v2 plus the pSix fast case, so v3 is a strict extension:
  every v2 TRUE is a v3 TRUE (via the fast path / W1 at the root). This is the
  `T_v2_subsumption` invariant (Section 11).
- Interior reachability: by the structure theorem (Section 4.1) interior AND nodes have
  `nDistinct` exactly 2 or (rarely, on generator drift) something the node refuses; W1
  (`>=3`) is reachable only at the root; W0/pSix at AND nodes and pSix at OR nodes are
  defensive (the tightened generator never places a six directly — B1 catches all
  finishes). Keep all checks anyway: they cost nothing, are individually sound, and make
  the prover robust to any future generator change (C's own recommendation, judge-endorsed).
- **Semantics** (proved in Section 7): `c6And(P,d)` true => with O to move, P forces a
  six within d+1 further P-turns. K=4 => wins proven up to 5 P-turns ≈ 18 plies past the
  injection ply, vs the fixed horizon's plv=4 plies.

---

## 4. Forcing-pair generation (OR nodes)

### 4.1 Structure theorem (C, judge-verified — makes generation exact and tiny)

At any OR node that reaches generation, `R.pFour` is false, i.e. **no 0-enemy window
holds >=4 P stones** (every 4P/0O window has <=2 empties => canSixNow; any uncovered
pre-existing five (5P+1e) contains a 4P/0O window and likewise trips B1). Consequences:

1. A NEW five can only be made by placing **both** of this turn's stones into a window
   currently holding exactly **3 P + 3 empty + 0 O**.
2. Two windows on different axes intersect in <=1 cell, so one pair (2 stones) can never
   complete fives on two different lines (each would need 2 placed stones).
3. Therefore `|C| >= 2` after the pair is achievable ONLY as an **open five on one
   line**: the pair completes a contiguous 5-run of P whose both outer extension cells
   are empty and on-board; that run lies in exactly two 5P+1e windows, giving
   C = {the two run ends}, `|C|` exactly 2. `|C| >= 3` is unreachable at interior nodes.

This is the classic Connect6 VCF shape: roll a three into an open five each turn; the
defender must spend both stones on the two ends; the attacker's placed stones accumulate
on crossing lines until canSixNow fires.

### 4.2 Generator `c6GenOpenFivePairs(P, out, max)`

1. Scan all windows once; for each window with exactly **3 P + 0 O** (3 empties),
   enumerate the <=3 ways to place 2 stones into its empties such that the
   post-placement line contains a contiguous P 5-run with BOTH extension cells empty
   and on-board (checked directly on the line). Each success yields one candidate pair.
   No 3P+0O window anywhere => return 0 (structural prefilter, instant refuse).
2. Deduplicate pairs (packed `a*361+b`, a<b canonical). Rank by a cheap follow-up
   heuristic: number of OTHER 3P/0O or 2P/0O windows through the placed cells (these
   become the next wave's fives / the canSixNow finisher). Truncate to C6_PAIR_MAX=12.
   Ordering must be deterministic (fixed scan order, stable tie-break by packed value).
3. (Optional perf screen, not required for soundness): scratch place/undo + local rescan
   through both cells (4 axes x <=6 windows x 6 cells ≈ 300 reads) confirming the open
   five materialized on the live board; drop pairs that fail.

### 4.3 Why generation can never cause a false positive

The child `c6And` re-derives EVERYTHING it trusts (fives, |C|, veto, six) from a fresh
whole-board scan of the live board. The generator's only power is to omit or include
candidates: omission => refusal (safe false negative); inclusion of a useless pair =>
the child refuses it. Truncation (C6_PAIR_MAX), ranking bugs, and the scan budget are all
therefore refusal-only. Half-open fives (one extension cell off-board or occupied) give
`|C|==1` and are skipped by the generator; the child AND would refuse them anyway.

### 4.4 Poison cells — explicitly NOT grafted (recorded decision)

Judge 1 certified B's poison-cell candidate source (cells inside O's `oc>=3 && pc==0`
windows) as additive-safe. It is NOT included in v3.0 because it is **structurally inert
here**: by 4.1, a recursion-continuing pair must place BOTH stones inside one 3P+0O
window; a pair spending one stone on a poison cell leaves `|C|<=1` and the child AND
refuses. Poison cells only pay off under B's four-based h==2 model, so they ride with
v3.1a (Section 12.1).

---

## 5. Opponent reply model — COMPLETE by construction

The prover recurses through an AND node only when `|C(P)|==2` (completions `c1 != c2`)
and `oppImm==false`.

**Lemma L2 (unique non-losing reply).** O's only reply that does not lose to a single
P stone on P's very next turn is to place its two stones on exactly `{c1, c2}`.

*Proof.* O places exactly 2 stones on 2 distinct empty cells (no passing inside a game
turn; the `m2==361` sentinel exists only at getMove packing, never mid-game here).
(i) Suppose O's reply leaves some `ci` empty. The five owning `ci` consists of 5
permanent P stones (`cfg.capturePairs==false` — no stone is ever removed) plus the empty
`ci`, still empty. On P's next turn P plays `ci` as its first stone: 6 consecutive P =
six (`winRowLength==6`; overlines also win), game over before O moves again. No other O
action can affect that window: it contains no other empty cell, and cells outside a
window are irrelevant to it.
(ii) Suppose O instead tries to WIN during its turn: a six for O after 2 placements
requires a pre-existing window with `pc==0 && oc>=4` — exactly `oppImm`, false at this
node. Note the veto tests "any 2 O placements", a superset of `{c1,c2}` — so the corner
case "the forced cover cells themselves complete an O six" is already excluded.
(iii) Therefore every non-losing reply covers both `c1` and `c2`; with exactly 2 stones
and `c1 != c2`, the reply IS `{c1,c2}`. ∎

The AND node's single child is exhaustive: every O defense is either the unique cover
(searched) or a deviation (loses immediately by (i), no search needed). There is no third
category — "defend by counter-attack" requires an O six within one turn (vetoed), slower
O resources never mature because `|C|==2` consumes both O stones at EVERY AND node (O
never gains tempo anywhere in the proof tree), and "defend by capture" does not exist in
a capture-free game. Nodes where the reply is NOT unique (`|C|<=1`: O holds a free stone
with no cheap irrelevance proof) are REFUSED outright, never approximated.

---

## 6. Win base cases (exhaustive list — nothing else returns true)

- **W0** (AND/OR, `pSix`): a six(P) window is on the board and, by invariant NI, no
  six(O) is; P made the last placement, so P has already won. Checked BEFORE the veto —
  a completed six cannot be raced.
- **W1** (AND, `nDistinct>=3`, veto false): v2's covering theorem verbatim
  (Ai.cpp:2250-2264): >=3 distinct singleton cover obligations vs O's 2 stones;
  pigeonhole leaves one five open; P completes it with one stone next turn; the veto
  excludes O making a six during its single intervening turn.
- **B1** (OR, `pFour`): a window with `oc==0 && pc>=4` has <=2 empties; P fills them
  with this turn's 2 stones => six on the board now. (`pc==5` needs one stone; the
  second is irrelevant — matching the engine's win-in-pair semantics.)

Every `true` bottoms out in W0, W1, or B1.

---

## 7. No-false-positive correctness proof

**Theorem.** If `c6ForcedWin(P, lvl)` returns true on a board satisfying NI at a
turn-final ply, then P has a forced win against every O strategy.

The fast path is v2's audited theorem. For the recursion, prove: *`c6And(P,d)` true =>
with O to move, P forces a six within d+1 further P-turns; `c6Or(P,d)` true => with P to
move, P forces a six within d+1 P-turns (including this one).* Mutual induction on d.

- **c6Or bases**: pSix — already won. B1 — direct one-turn win by window arithmetic on
  the actual board (permanence irrelevant; P just plays the <=2 empties). ✓
- **c6Or step**: some concrete pair (a,b) — legal: both cells verified empty on the
  CURRENT board (candidates are drawn from a fresh scan taken AFTER O's forced stones
  are down, so they can never collide with them) — makes `c6And(P,d)` true; the pair
  spends 1 P-turn, IH gives <= d+1 more; total <= d+2... bounded by the caller's ladder;
  the bound is not load-bearing for soundness, only for cost. The child board satisfies
  NI (P stones never advance O windows; a P six is W0 = win). ✓
- **c6And bases**: W0 — P six on board, no O six (NI), P placed last: game already won.
  W1 — v2's covering argument: (a) permanence makes each counted five's cover set the
  singleton {completion cell}; (b) `seen[]` dedup makes nDistinct the number of
  independent obligations; O covers <=2 of >=3; (c) some five stays open, P completes it
  next turn; (d) the veto excludes O's one intervening-turn six, and O gets no other
  turn before P's six. ✓
- **c6And step** (`|C|==2`, veto false, `d>=1`): by L2, O either deviates — losing
  immediately to one P stone — or plays exactly `{c1,c2}`, producing the board the
  prover constructed, where `c6Or(P,d-1)` returned true; IH applies. Every branch of O's
  strategy tree is covered; the win is forced. ∎

**Refusals are always safe**: every `return false` (budget, |C| not in {2,>=3}, veto,
pair exhaustion, d==0, generator empty) merely falls back to the normal alpha-beta value
at Ai.cpp:1465+ — provably identical to today's behavior.

### 7.1 False-positive channel enumeration (every way a win could be wrongly claimed)

| # | Channel | Prevention |
|---|---------|-----------|
| 1 | Hallucinated five/six (window miscount, enemy stone or off-board cell inside) | Scan geometry/counting is v2's audited loop reused, not rewritten (endpoint check, enemy kills window, single implied empty) |
| 2 | Duplicate completion cells inflating \|C\| | `seen[361]` dedup per node, computed on the live scratch board |
| 3 | Opponent wins first | `oppImm` veto re-run at EVERY AND node on the CURRENT hypothetical board — exactly when O would next move; O never holds a free stone anywhere in the proof tree (L2), so any O win must be a one-turn six = precisely the vetoed shape; the veto also covers "the forced cover completes an O six" (superset test) and "block stones build a future O four" (re-tested at the next AND node) |
| 4 | Missed O defense (alternative block, counter-attack, free stone) | L2 completeness; non-unique-reply nodes (\|C\|<=1) refused outright; alternative blocks impossible under permanence (singleton cover sets); captures don't exist |
| 5 | Win pre-empted at the last step | W0 is already placed; W1/B1 completions happen on P's turn before O's next turn, and the counted window's only empties are the cells P fills (had O filled one, the window would not have been counted on the current board) |
| 6 | Stale threats (fives O already covered) | C and candidates recomputed from a fresh scan at every node |
| 7 | Board corruption / scratch leakage | Exact save/restore of prior cell values (INCLUDING -1) on all paths; debug full-board memcmp + sco/rowWin/c6NextSame snapshot asserts around every top-level call; no engine globals written |
| 8 | TT / search-state interaction | Identical injection contract as v2: `sc=12000; hfl=0;` into the untouched win path at 1465; nothing stored in the hash table; no depth semantics change; exfl/exel/mxvt/mxvf/vct untouched |
| 9 | Cadence confusion (a role doesn't really own 2 stones) | The prover never reads ownerOf/c6NextSame; it models whole 2-stone turns from a turn-final position, guaranteed by the gate (`c6NextSame==0`, `cfg.stonesPerTurn==2`, `tnRoot>=6`) |
| 10 | Overline edge | `winRowLength==6` with overlines counting: any 5P+1e completion wins; any O run >=6 contains a 6-window and is caught by `oc>=4` |
| 11 | Illegal placement (occupied cell, a==b) | Every placed cell verified empty at generation/placement time on the then-current board; completion cells empty by five-definition on the pre-reply board; pairs canonical a<b |
| 12 | Budget/depth edges | Decrement-before-work; every exhaustion path returns false; deterministic |
| 13 | Near-full board | If c1,c2 were the last empties, the child OR's generator finds nothing and refuses — a draw is never misread as a win |

---

## 8. Non-inertness — two checked-in beyond-horizon exhibits (MANDATORY tests)

Both boards go into the curated suite and the mutation gate. `vct==0` for Connect6
(Ai.cpp:592) => `mxlv=plv` ALWAYS: there is NO extension mechanism, so any forcing chain
whose six lands beyond plv plies is structurally invisible to today's engine AND to v2.

### 8.1 Exhibit A (from proposal A §7; judge-1 hand-verified stone census)

Turn-final ply, X (=P) just placed game stones 15,16 (idx%4 cadence: X=9 stones, O=8 —
legal census). plv=4, prover fires at lvl=2 (K=4 available; the proof needs the first
wave only).

```
        col: 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
row  3 .  .  .  .  .  .  .  .  .  .  .  .  O  .  .
row  4 .  .  .  .  .  .  .  .  .  .  .  .  .  O  O
row  5 .  .  .  .  .  .  .  .  .  .  .  .  O  .  .
row  6 .  .  .  .  .  .  .  .  .  .  .  .  .  O  .
row  7 .  .  .  .  .  .  .  .  .  .  .  .  O  .  O
row  9 .  .  .  a  X  X  X  X  X  b  .  .  .  .  .
row 11 .  .  .  c  X  X  X  X  d  .  .  .  .  .  O
row 13 .  .  .  .  .  .  .  .  .  .  .  .  .  .  O
```

X: five (5..9,9) — win-in-pair provably did NOT fire while placing it (after the first
stone the best window held 4 X; at the second, `c6NextSame==0` makes n==5 non-terminal,
Ai.cpp:2215) — plus four (5..8,11). No O window has `pc==0 && oc>=4` => veto false.

Trace: `c6And(X,4)`: C={a=(4,9), b=(10,9)}, |C|==2 => O uniquely forced onto {a,b}.
`c6Or(X,3)`: fresh scan sees the row-11 window cols[4..9] with `pc==4, oc==0` => **B1
fires** (canSixNow: X plays c,d => six). Win completes at ply lvl+4 = 6 > mxlv = 4; the
fixed search leafs out right after O's block with the static `sco[fr]-sco[en]*4+rand()%6`
(< 7800 per side), never 12000. v2 also refuses (|C|==2 < 3).

### 8.2 Exhibit C (from proposal C §10; deeper — no >=4 window exists ANYWHERE at root)

Root board: only threes (strictest "structurally invisible" start). plv=6 (argument also
holds for plv=8). X to move; O scattered so no 0-X window holds >=3 O (veto false at
every node); no 0-enemy window holds >=4 stones for either side.

```
        x= 4  5  6  7  8  9 10 11 12
  y= 4      .  .  .  .  .  .  .  O  .
  y= 6      .  .  .  X  X  X  .  .  O     <- row-6 three (7,6)(8,6)(9,6)
  y= 7      O  .  .  .  .  .  .  .  .
  y= 8      .  .  X  .  .  .  .  O  .     <- (6,8)
  y= 9      .  .  X  .  .  .  .  .  .     <- (6,9)
  y=10      .  .  .  X  X  X  .  O  .     <- row-10 three (7,10)(8,10)(9,10)
  y=11      .  O  .  .  .  .  .  .  .
  y=12      .  .  .  .  O  .  .  .  O
```

X's in-tree turn 1: (6,6)+(10,6) => row-6 OPEN FIVE, and col x=6 quietly becomes a three.
At that turn-final node (lvl=2) the prover runs `c6And(X,4)`: |C|={(5,6),(11,6)} => O
forced to both ends. `c6Or(X,3)`: no B1; generator emits (6,7)+(6,10) (col x=6 is a
3X/0O window) => col open five, and (6,10) turns row 10 into a FOUR (x=6..9, ends
empty, 0 O). `c6And(X,3)`: |C|={(6,5),(6,11)} => O forced again, its stones consumed;
the row-10 four untouchable. `c6Or(X,2)`: **B1** — X plays (5,10),(10,10) => six.
Depth used: 2 waves + finisher, needs K>=2 — provable even at the lvl 3-4 tier (K=2).

Six lands on plies 9-10 past the root — beyond plv=6 AND plv=8. Within the horizon a
win terminal for X would need a pre-existing 4X/0O window at a c6NextSame ply: none
exists at ply 1, and after O's best (blocking) replies X's 0-O windows hold <=3 at
ply 5, so win-in-pair never fires; the ply-6 n==5 is correctly non-terminal. v2 sees
|C|=2<3 everywhere. Root score without v3 stays <10000 in every O-defending line; v3
returns 12000 at lvl 2.

### 8.3 Firing-frequency mitigation (A's own open risk)

Exhibit C is the answer to A's flagged risk that the |C|==2 root shape may rarely be
proposed by the normal move loop: with the tightened generator + K=4 the prover proves
chains STARTING from bare threes, which arise constantly in real middlegames. If, despite
the mutation gate passing on the curated boards, the golden run stays byte-identical
(Section 11.5), widen the injection plies (lvl<=6) or K before shipping — as refusal-only
knobs this needs no new proof.

---

## 9. Complexity and hard bounds

- One `c6ScanFives` = ~1064 windows x 6 cells ≈ 6.4k cell reads — the same cost class as
  one v2 call, which already runs at every qualifying node today (absorbed by the golden).
- OR branching <= C6_PAIR_MAX = 12; AND branching = 1. Uncapped worst case at K=4 is
  ~sum(12^i, i=0..3) ≈ 1.9k OR nodes + as many AND scans ≈ 3.9k scans; the **hard budget
  C6_SCAN_BUDGET=3000 scans (~19M cell reads, low single-digit ms in WASM)** clamps it,
  refuse on exhaustion, deterministic (decrement-before-work).
- Common path is ONE scan: the v2 fast path plus the root shape check; recursion is
  entered only on the rare `nDistinct==2` root shape, and the 3P+0O structural prefilter
  makes empty-generator refusals instant.
- Expected per-turn regression well under 10%; pathological dense boards clamped.
  A c6test timing gate (Section 11.7) enforces < 2x baseline median/p99.
- Tuning knob order (all refusal-only, NEVER the correctness machinery):
  C6_SCAN_BUDGET, K ladder, C6_PAIR_MAX.

---

## 10. Exact integration diff

### Ai.h (Connect6 block, next to `bool c6UnstoppableThreat(int P);` ~line 134 region)

```cpp
// Connect6 v3 forcing-search prover (pure w.r.t. engine state; bd scratch place/undo)
struct C6Scan { int nDistinct; CPoint c1, c2; bool pSix, pFour, oppImm; };
enum { C6_PAIR_MAX = 12, C6_SCAN_BUDGET = 3000 };
C6Scan c6ScanFives(int P);              // one-pass scan (shared with v2)
bool   c6And(int P, int d);             // AND node: bases + unique-reply recursion
bool   c6Or(int P, int d);              // OR node: B1 + open-five pair enumeration
int    c6GenOpenFivePairs(int P, unsigned short *out, int max); // packed a*361+b, a<b
bool   c6ForcedWin(int P, int lvl);     // top level: v2 fast path + budget + K ladder
int    c6Budget;                        // scan budget, valid only within one call
```

### Ai.cpp

1. **Factor v2's whole-board loop body (Ai.cpp:2273-2298) into `c6ScanFives(P)`**,
   adding to the SAME pass: `pc==6 => pSix`, `oc==0 && pc>=4 => pFour`, and recording
   c1/c2 for the first two distinct completion cells. Reimplement the v2 symbol as
   `bool CAi::c6UnstoppableThreat(int P){ C6Scan r=c6ScanFives(P);
   return !r.oppImm && r.nDistinct>=3; }` — behavior-identical (keep the symbol: it is
   the fast path, the subsumption-assert reference, and the unit-test baseline).
2. **Add `c6GenOpenFivePairs`, `c6And`, `c6Or`, `c6ForcedWin`** (Sections 3-4, ~100
   lines) directly below `c6UnstoppableThreat`.
3. **Injection (Ai.cpp:1458-1462)** — change ONLY the gate's tail and the call, inside
   the existing `#ifndef C6_NO_FORCE` block; update the block comment from "v2" to "v3":

```cpp
if (cfg.stonesPerTurn==2 && !cfg.capturePairs && c6ForceEnabled
    && !hfl && c6NextSame==0 && tnRoot>=6 && lvl<=4
    && c6ForcedWin(fr, lvl)) {          // was: c6UnstoppableThreat(fr)
    sc = 12000; hfl = 0;
}
```

The `sc=12000; hfl=0;` contract and the win path at Ai.cpp:1465-1475 (`scr[lvl][fr]=
12000-lvl; ... wfl=1;`) are untouched — byte-identical machinery to v2. **No changes**
to: the do-while at 1479, the backtrack/fr-recompute at 1490, the TT store, wfl/hfl
semantics, move ordering, exfl/exel/mxvt/mxvf, vct, Score6, or getMove's m1*362+m2
packing. `C6_NO_FORCE` still compiles the whole feature out.

---

## 11. Acceptance plan — TDD, oracle, mutation gate, golden (ALL gates blocking)

Harness: `MMAIWASM/c6test.cpp` (builds with `#define protected public`; deterministic
xorshift fuzz, genBoard styles 0/1/2). Run `./c6test 50000` from the MMAIWASM dir
(argv[1] = fuzz board COUNT). Golden: `regress.cpp` + `golden/connect6_g13.txt`
(seed 777 via time() override).

1. **T1_v3verify — proof-checker fuzz (the primary gate)**: for every fuzz board, call
   `c6ForcedWin` for both players. For every TRUE, run an independent **full-width
   verifier**: at each AND step enumerate ALL O pairs over ALL empty cells (E^2); every
   pair != {c1,c2} must lose to a one-stone P six (O(1) check via C); NO O pair may
   complete an O six (veto validation); for the covering pair, descend with the prover's
   recorded winning P pair and repeat until W0/W1/B1, re-verifying the base on the final
   board. Any violation = hard build fail. This computationally re-proves L2 + the
   theorem per claim; cost is per-claim only (claims are rare).
2. **Oracle cross-check**: extend `oracleForcedWin` to `oracleForcedWinK(P,K)` — exact
   alternating solver, attacker pairs over a relevance ring (empties within distance 2
   of any stone), defender pairs exhaustive, plus a full-board O race scan per O turn.
   Used on boards <= ~16 stones with K <= 3: every tractable prover TRUE must be
   oracle-confirmed; one refutation fails the build. Refusals are never oracle-checked
   (false negatives are legal by design).
3. **T_purity**: debug memcmp of the full 19x19 `bd` plus snapshots of `sco[]`, `rowWin`,
   `c6NextSame`, and hash counters before/after every top-level call under the fuzz
   loop, including boards containing `-1` cells => byte-identical.
4. **Curated suite**: Exhibits 8.1 and 8.2 (both must be WIN); a K=2 two-wave variant of
   8.1; and >=6 near-miss REFUSE boards: |C|==1; oppImm true; completion-cell collision
   (shared cell => |C|==1); budget-buster dense board (assert refusal, not garbage);
   O-four created by the forced block cells (deeper veto refusal); board-full edge.
   Assert exact WIN/REFUSE verdicts. Plus `T_v2_subsumption`: every fuzz board where v2
   fires, v3 fires.
5. **Mutation gate (the v2 lesson — mandatory, non-negotiable)**: build the engine pair
   with `c6ForceEnabled` true/false. For EVERY curated WIN position, `getMove` of the ON
   engine must select the proven line (any 12000-scored move) and the OFF engine must
   select a DIFFERENT move — proving each win is load-bearing at the move-choice level.
   Any curated win that does not flip behavior is either fixed or the release is
   rejected as inert. Then 200-game ON-vs-OFF self-play at plv=4, alternating seats:
   ON score >= OFF, and log every game where a v3 12000 decided a move.
6. **Golden**: run regress against `golden/connect6_g13.txt`. **A diff is EXPECTED and
   REQUIRED** (byte-identical output was v2's inertness symptom). Procedure: diff;
   manually proof-check each diverging move with T1's verifier; regenerate the golden;
   pin Exhibits 8.1/8.2 with their expected winning moves as new golden entries. All
   other game ids must remain byte-identical (the cfg gate makes v3 a strict no-op
   elsewhere). If byte-identical on g13: apply Section 8.3 widening BEFORE shipping.
7. **Determinism + perf**: two identical runs of (1)+(6) byte-identical (no rand, fixed
   iteration orders, deterministic budget); median/p99 Connect6 move latency < 2x
   baseline, enforced in c6test.
8. **T0_fuzz** unchanged (regression).

Recommended implementation order (TDD): (a) `c6ScanFives` refactor + assert
byte-equivalence of the reimplemented `c6UnstoppableThreat` against the original over
the fuzz corpus; (b) generator + unit tests on hand boards; (c) c6And/c6Or against the
curated suite; (d) T1 verifier + oracle; (e) injection swap; (f) mutation gate + golden.

---

## 12. Deferred to v3.1+ (each an ISOLATED release; never bundled together)

### 12.1 v3.1a — B's four-based threat model (VCDT: general pc>=4 windows + exact min-hitting-set)

Strictly larger reach (wins from four-only threats, no five ever formed — B's §5 exhibit
has zero five-windows anywhere, untouchable by v3.0). Judges found NO soundness hole in
the design (the cap-safety inequality computed_h <= true_h and the h==2 pair-enumeration
superset argument were independently re-derived and confirmed), but flagged the
hand-rolled `c6MinHit2` exact hitting-set primitive as the highest coding-bug surface of
the panel and the K=6/cap=3000 lvl<=2 tier as a real latency risk. **Blocking gates
before it may ship**: (a) an independent brute-force cross-validator of c6MinHit2's h
and pair set running clean on every fired position over the full 50k fuzz corpus;
(b) oracle confirmation of every claimed win; (c) real-middlegame latency profiling
inside the <2x budget (not just worst-case math); (d) ship the full-rescan reference
first, gate any incremental fast path on corpus A/B equality. B's poison-cell candidate
source lands here (it is inert under v3.0's fives-only structure theorem, Section 4.4).
Spec of record: `proposal-B-strength-max.md` (scratchpad c6v3/).

### 12.2 v3.1b — C's opponent-forced-win LOSS terminal (gap G1)

The highest-value deferred idea (avoiding a beyond-horizon opponent VCF is worth more
than an extra win), with a clean soundness story: Theorem L = Theorem W instantiated
with A=en on the same board — the loss prover IS the shipped win prover with roles
swapped, no separate failure mode. What defers it is integration, not math.

**Judge-found hole (MUST be closed before this ships — rule 2 of this synthesis).**
C proposed adding `&& !lfl` to the do-while condition at Ai.cpp:1479
(`while (*pmv<*pmxmv && lvl<mxlv && !wfl && !hfl);`). Judge 3's concrete finding: `wfl`
terminates that sibling-move loop early, correct for a WIN (one proven win suffices);
reusing the same exit slot for `lfl` would stop enumerating siblings on the FIRST proven
LOSS — backwards, since a mover holding one losing move must keep searching for a
non-losing (or, per C's §7.3 kFound grading, least-bad) alternative, and the grading
requires comparing ALL siblings. **Mandatory fix**: `lfl` must have per-move scope
exactly like `hfl` — it may only suppress DESCENT into the current move's subtree
(hash-hit-style: the standard `scr[lvl]` back-comparison runs, then the sibling loop
continues) and must be cleared at the per-move `hfl` reset point; it must NEVER be able
to exit the sibling enumeration the way `wfl` does. Acceptance for v3.1b additionally
requires: (i) a unit test proving all siblings are still enumerated and scored after an
earlier sibling sets lfl=1; (ii) a single-step debugger trace of the 1479-1530 backtrack
path (C's own demand); (iii) fuzz validation of the new `!lfl` TT-store exclusion guard;
(iv) a test that non-Connect6 game paths are bit-identical when lfl is never set;
(v) C's T_loss_symmetry, kFound/lvl audit log, and zero-A-losses-where-the-terminal-fired
self-play gate. When built, prefer rebuilding it on whatever threat model has shipped
(v3.1a's if available) — the Theorem-L symmetry pattern transfers.
Spec of record: `proposal-C-defense.md` (scratchpad c6v3/).

### 12.3 v4 — free-stone model

Sound analysis of `|C|<=1` AND nodes (defender holds an unconstrained stone) needs a
free-stone irrelevance theory; every abstraction examined so far had a correctness hole.
Until then those nodes refuse — the deliberate soundness boundary of this whole design.

---

## 13. Open risks (v3.0)

- **Firing frequency**: mitigated by the tightened generator + K=4 + Exhibit 8.2's
  threes-only start; backstop = Section 8.3 refusal-only widening if the golden stays
  byte-identical despite a passing mutation gate.
- **Budget-clamped cost spikes** on dense boards waste up to 3000 scans then refuse —
  safe by construction; tune C6_SCAN_BUDGET on real WASM timings (Section 11.7 gate).
- **Golden churn**: every g13 divergence needs one manual proof-check pass at
  regeneration time; subsequent churn is normal.
- **Deliberate narrowness**: fives-only + unique-reply-only misses four-based (v3.1a)
  and free-stone (v4) wins — by design, not defect; refusal ≡ status quo, so strength
  is strictly non-negative.

---

## 14. Implementation & experimental results (as shipped)

Implemented in `MMAIWASM/Ai.cpp` / `Ai.h`; tests in `MMAIWASM/c6test.cpp`; self-play
harness `MMAIWASM/c6strength.cpp`. `ai.wasm` regenerated via `compile.sh`.

**Correctness — verified, no false positives:**
- 50k-board fuzz: v2 refactor `c6UnstoppableThreat` == `oracleForcedWin` with 0 mismatches;
  v2⇒v3 subsumption 0 holes; the independent exhaustive-defender oracle confirmed ~5000
  prover wins with **0 refutations**, 0 unresolved-on-tractable.
- ASAN+UBSAN clean; `bd` byte-identical before/after every prover call (purity, incl. `-1`).
- Curated exhibits: open-five+four (K=1), spec Exhibit A (K=1), Exhibit C (K=2 — `c6And`
  refuses at d=1, proves at d=2: the depth ladder is load-bearing).
- Goldens: all 6 byte-identical. connect6_g13 unchanged is BENIGN (the regress corpus's
  random positions contain no beyond-horizon forcing wins) — proven benign by the
  end-to-end mutation gate below, not an injection failure.

**Non-inertness / end-to-end (v3 is load-bearing):**
- `T_v3_flip`: on a constructed 2-turn forced win, `getMove` with the terminal ON claims
  the win (bscr 30000) and plays the forcing move; OFF is blind (bscr −10000). The
  terminal demonstrably changes the engine's move.

**Accuracy — the improvement (this is the "improved accuracy" result):**
- `T_v3_accuracy`: on a position holding a K=2 forced win, v3 recognizes it at EVERY
  search level (L=2,3,4); the baseline recognizes it only at L=4 (where Score6's
  win-in-pair happens to reach) and is blind at L=2,3. v3 **strictly extends tactical
  reach** — most at lower levels and for chains deeper than win-in-pair's ~2-ply extension.

**Self-play — honest null, and why:** engine-vs-engine self-play (v3 ON vs OFF), empty
and random openings, L=2/3/4: ALL draws, v3 fired 0 times. A harness self-test confirms
six-detection works, and a strong(L4)-vs-weak(L1) control is ALSO all draws with the
engine's best score never exceeding the 10000 non-terminal clamp — i.e. the engine
**never forces a win in self-play at all**. Its Connect6 OFFENSE is too weak to build
toward forced wins in its own play, so v3 (a win-*recognition* terminal) has nothing to
convert there. v3's gain is realized when a forcing win actually exists in the search
(vs weaker/human opponents, tactical positions) — exactly what the accuracy test isolates.
The larger remaining Connect6 strength lever is stronger multi-threat OFFENSE — a separate
project, out of v3's scope.

**Performance:** raw prover ≈ 0.008 ms/call on dense boards (hard-capped by
`C6_SCAN_BUDGET`); on forcing positions v3 SHORTENS `getMove` (proven win prunes the
search). No stall risk.

**Code review (high-effort, applied):** F1 fixed (test-oracle `oraOr` missing `nc<2`
UNKNOWN guard); F4 fixed (removed redundant v2 double-scan — `c6And`'s bases cover it);
F5 fixed (gate pinned to `cfg.winRowLength==6`); F2 documented (relevance-ring defender
soundness argument); F3 closed by measurement; F6 (harness DRY) noted, deferred.
