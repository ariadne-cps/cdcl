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
