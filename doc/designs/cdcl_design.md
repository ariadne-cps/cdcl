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
