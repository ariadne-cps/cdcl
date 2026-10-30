# Ariadne CDCL design and implementation journal

## Initial architecture (2026-10-27)

Extracted `BooleanLiteral`, `CdclStatistics` and `CdclState` from
`ariadne-cps/smt` after its successful regressions. Kept original generic
unit propagation, reason-based implication trail, resolution and 1-UIP
logic without adding heuristics. The protected-state core is a reusable
base but is not a complete SAT solver yet.

The accompanying theory interface is deliberately minimal:
`check_partial(assignment)` returns `CONSISTENT`, `CONFLICT` with an
explanation clause, or `UNKNOWN`. Only a sound, theory-entailed and currently
falsified conflict can enter the learned clause database. UNKNOWN cannot
be converted to Boolean conflict; a provisional consistent result cannot
be taken as a numerical model certificate.

`validate_cdcl_theory_feedback` performs structural checks, not a proof.
A theory-specific complete-model certification and witness stays in SMT.
`CdclState` owns an immutable reference to the original clause database:
the caller must keep that database alive until the state is destroyed.

Dependencies: configuration + utility, optional python-common.
Unit tests exercise propagation, backtrack, learned clauses, typed literals
and theory-feedback validation. Benchmarks and independent installed C++/Python
tutorials are provided. All CI workflows follow the current Ariadne matrix.

## Complete Boolean search moved out of SMT (2026-10-27)

`CdclSolver` now owns the recursive decision search, theory conflict
integration, first-UIP learning, nonchronological backjumping and full-model
completion. It reuses the unchanged `CdclState` primitives. A caller may
supply a `CdclTheory` and a complete-assignment callback; pure SAT requires
neither. The completion callback returns `CERTIFIED` (valid witness),
`REJECTED` (proven infeasible) or `UNKNOWN` (inconclusive).
An UNKNOWN partial/complete check never produces a learned clause and cannot
be reported as UNSAT if the search finishes without a certified model.

The solver is single-shot: a second call raises `logic_error`; the original
clauses remain a borrowed, immutable reference and must outlive the solver.
New tests check SAT, UNSAT, zero-variable formulas, typed assignments,
explained conflicts, unknown results and completion callbacks.

### Complete assignments must consult the theory oracle (2026-10-27)

The initial common solver short-circuited at a complete Boolean assignment
before checking the attached theory oracle. This is incorrect for a general
CDCL(T) user because a theory conflict may be present even when every
Boolean variable has been assigned. The theory callback is now invoked
before the completion callback. A certified theory conflict can therefore
learn at the root level or trigger a backjump; UNKNOWN still cannot learn.
SMT's adapter can explicitly skip the duplicate complete theory-consistency
check because its completion callback performs full numeric certification.

### 2026-10-27 - Include the solver test in the build target

The new `test_cdcl_solver` executable was registered with CTest but omitted
from the `tests` custom target dependencies. The sanitizer workflow builds
only `tests`, so CTest found a registered test without a compiled executable.
The dependency list now includes all three C++ regression executables.
This is a build-graph correction, not a solver behavior change.

### 2026-10-27 - Fix nested backtracking and preserve root propagation

ThreadSanitizer exposed an unbounded recursive search in `test_cdcl_solver`
after the missing test target had been added to the build. The cause was an
architectural backtracking error in `CdclState::_backtrack_to_level`: a
decision-level marker records the *beginning* of that level, not the end.
Using `markers[level]` when returning to `level` also erased that level's
decision and propagations, so an ancestor could repeatedly choose the same
variable and eventually overflow the stack.

The rollback boundary is now `markers[level+1]` when moving from a higher
level to the requested lower level. This preserves all assignments at and below
the target level, including root unit propagations. Backtracking to the
current level is a no-op, and backtracking to a future level is rejected.

Targeted regressions cover nested levels, root propagation retention, and
exhaustive completion-callback rejection with exact leaf counts, rather than
merely accepting the absence of a crash. No recursion-depth limit, sanitizer
suppression or test weakening was introduced.

### 2026-10-27 - Normalize source headers

All CDCL C++ sources and headers now use the established full Ariadne GPL
license notice and Doxygen file banners, matching SMT's canonical header.
This formatting-only change does not affect solver semantics or test coverage.

### 2026-10-27 - Two-watched-literal event propagation

The initial fixed-point propagator re-scanned all original and learned
clauses for every new unit. It is replaced by two watched positions per
non-unit clause, with literal-indexed watchlists and an event cursor into
the implication trail. A newly falsified watched literal triggers only
its attached clauses; watchers move to a different non-false literal when
possible, otherwise detect a unit or report a conflict. Stable numeric
clause indices preserve implication reasons and 1-UIP resolution across
vector reallocations, and watchers need no modification on backtrack.

The initial clause database is checked once. A separate pending-clause
queue handles new Boolean and theory-learned clauses *after backjump*, when
a clause may already be unit under assignments that were propagated before
its insertion. The pending head is deliberately retained after a conflict
so an unprocessed clause cannot be lost. Root-level unit assignments,
conflict statistics, learned/theory reason statistics and UNKNOWN semantics
remain unchanged. The learned clause reduction mechanism is still only
candidate selection, so no watch-list removal or physical clause deletion
has been introduced.

Targeted tests verify watched-literal relocation, implication reasons,
learned theory-clause propagation after backjump, repeated literals,
tautologies and empty-clause conflicts. An additional independent
256-instance truth-table oracle checks generated small CNFs, statuses and
total SAT witnesses. The previous CDCL solver regressions remain intact.

The watch registration is deferred until the pending clause is checked
against its post-backjump assignment. This avoids watching two already
false literals of a freshly learned clause while its asserting literal is
unassigned. A regression verifies that a ternary conflict learned under a
fully falsifying assignment becomes unit after backjump, retaining the
correct reason and watching the newly available literal.

### 2026-10-27 - Fix repeated local identifier in watched-literal regression

The new `test_cdcl_state` scenario used `learned` for a second clause
index inside the same `main()` scope. Clang (with warnings as errors)
reported a redefinition before tests could execute. Renamed only the
new scenario's local index to `delayed_learned` and updated its assertions;
the test scenario and production propagation algorithm are unchanged.

### 2026-10-30 - Remove work with no effect from deferred clause reduction

Clause reduction remains deferred: exceeding the configured limit records
a reduction attempt but retains all learned clauses. The previous code
selected and sorted a local candidate vector and then discarded it without
changing any state or returning the candidates. That computation has been
removed; activity/generation tracking, protected queries, statistics, watches
and reason locks are preserved. A regression checks retention of old long
clauses as well as their continued propagation. No deletion policy has been
introduced as part of coverage work.

Two unreachable checks were also removed after establishing their invariants:
the state constructor already validates original literals before the solver
constructor body runs; and pending-clause conflict detection returns before
watch installation if every literal is false. Non-conflict theory feedback
has an empty explanation by construction through its private constructor and
public factories. Range errors and single-shot solve errors remain checked.
