# Ariadne CDCL

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Unix Status](https://github.com/ariadne-cps/cdcl/actions/workflows/unix.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/unix.yml)
[![Windows Status](https://github.com/ariadne-cps/cdcl/actions/workflows/win.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/win.yml)
[![Sanitizers Status](https://github.com/ariadne-cps/cdcl/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/sanitizers.yml)
[![Coverage Status](https://github.com/ariadne-cps/cdcl/actions/workflows/coverage.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/coverage.yml)
[![codecov](https://codecov.io/gh/ariadne-cps/cdcl/branch/main/graph/badge.svg)](https://codecov.io/gh/ariadne-cps/cdcl)

Ariadne CDCL is the standalone C++20 **conflict-driven clause-learning** engine
used by [Ariadne SAT](https://github.com/ariadne-cps/sat) and
[Ariadne SMT](https://github.com/ariadne-cps/smt). It provides Boolean literals,
CNF propagation using two watched literals, conflict analysis with 1-UIP
learning, nonchronological backjumping, and an optional theory-feedback contract.
It has no dependency on BDD or numerical SMT libraries.

## Layout

Public headers are under `include/cdcl`, with umbrella header
`include/ariadne-cdcl.hpp`. Implementations are under `src`, C++
regression tests under `tests`, performance benchmarks under `benchmarks`,
Python bindings and tests under `python`, independent examples under
`tutorials`, and the implementation journal under
[`doc/designs/cdcl_design.md`](doc/designs/cdcl_design.md).

## Solver and theory integration

`CdclSolver` solves propositional CNF with exact `SAT`/`UNSAT` results.
It is currently **single-shot**: construct a fresh solver for each search.
`BooleanLiteral(variable,positive)` uses zero-based variable indices;
`from_dimacs()` and `dimacs()` use signed one-based DIMACS literals.

For SMT integration, the optional `CdclTheory::check_partial()` returns
`CONSISTENT`, `CONFLICT` with a currently falsified explanation clause,
or `UNKNOWN`. Only a sound theory-entailment explanation permits learning.
`UNKNOWN` never becomes UNSAT. Complete assignments can be checked with
`CdclCompletionCallback`, which distinguishes certified witnesses, proved
rejection, and inconclusive results. Structural explanation validation does
not prove theory entailment; that remains the theory solver's responsibility.

The engine uses two-watched-literal propagation. Clause-database reduction
and solver restarts are not yet implemented.

## Dependencies

CDCL depends directly on:

- [ariadne-cps/utility](https://github.com/ariadne-cps/utility) (Git submodule).
- [ariadne-cps/configuration](https://github.com/ariadne-cps/configuration) (shared build configuration).
- [ariadne-cps/python-common](https://github.com/ariadne-cps/python-common) (optional Python binding infrastructure).

Utility supplies the lower Ariadne dependencies transitively.

## Build

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/cdcl.git
cd cdcl
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel --target everything
ctest --test-dir build --output-on-failure
```

For a C++-only build, configure with `-DBUILD_PYTHON_BINDINGS=OFF`.

## Python bindings

When a compatible Python development environment is available, the
standalone build exposes `BooleanLiteral`, theory-feedback statuses,
`CdclTheoryFeedback` and feedback validation in the combined
`pyariadne` extension. The autonomous `CdclSolver` search is currently
a C++ API, not a standalone Python solver binding.

## Contribution guidelines

If you would like to contribute to Ariadne CDCL, please contact the developer:

- Luca Geretti <luca.geretti@univr.it>

## License

Ariadne CDCL is released under the GNU General Public License v3.0.
