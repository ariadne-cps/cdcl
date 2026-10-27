# Ariadne CDCL

Reusable C++20 propositional CDCL state and a theory-solver feedback contract,
extracted from the validated Boolean engine in [ariadne-cps/smt](https://github.com/ariadne-cps/smt).
The repository is independent of numerical SMT and BDD implementation details.

**Status:** complete basic recursive CDCL search with unit propagation,
1-UIP learning, nonchronological backjumping, and optional theory feedback. Clause database
reduction, watched literals, and restarting are not implemented.

## Solver

`CdclSolver` solves propositional CNF without a theory oracle and returns
`SAT` or `UNSAT`. The optional `CdclTheory` partial-check callback and
`CdclCompletionCallback` allow SMT to distinguish certified witnesses,
proved rejection and inconclusive numerical checks.

## Interface

`BooleanLiteral(variable,positive)` uses zero-based logical variable indices.
DIMACS import/export is explicitly one-based via `from_dimacs()` / `dimacs()`.

`CdclTheory::check_partial(CdclAssignment const&)` returns:
- `CONSISTENT`: partial theory constraints not disproved, **not** a witness;
- `CONFLICT`: a theory-entailment explanation clause, falsified by the
  current Boolean assignment; an empty clause is allowed only for a global
  contradiction;
- `UNKNOWN`: inconclusive, **never** usable for clause learning or UNSAT.

`validate_cdcl_theory_feedback()` checks structural consistency and that
every conflict literal is assigned false. It does **not** establish logical
validity of the explanation, which is the theory solver's responsibility.

## Dependencies

Direct Git submodules: `configuration`, `utility`, and (for optional
Python bindings) `python-common`. No dependence on `smt`, `sat`,
`bdd`, or numerical libraries.

## Build

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/cdcl.git
cmake -S cdcl -B cdcl/build -DCMAKE_BUILD_TYPE=Debug
cmake --build cdcl/build --target everything --parallel
ctest --test-dir cdcl/build --output-on-failure
```

The repository follows the Ariadne Unix/Windows/coverage/sanitizer CI matrix.
Installed C++ and Python tutorials test packaging. Python 3.11+ is optional;
set `-DBUILD_PYTHON_BINDINGS=OFF` for a C++-only build.
See [design journal](doc/designs/cdcl_design.md).
