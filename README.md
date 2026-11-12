# Ariadne CDCL

[![Format](https://github.com/ariadne-cps/cdcl/actions/workflows/format.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/format.yml)
[![macOS Status](https://github.com/ariadne-cps/cdcl/actions/workflows/macos.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/macos.yml)
[![Ubuntu Status](https://github.com/ariadne-cps/cdcl/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/ariadne-cps/cdcl/actions/workflows/ubuntu.yml)
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
Although independently maintained and usable, this repository is also part of the [Ariadne repository structure](https://github.com/ariadne-cps/ariadne/blob/main/doc/structure.md).

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
- [ariadne-cps/build](https://github.com/ariadne-cps/build) (shared build configuration).
- [ariadne-cps/threading](https://github.com/ariadne-cps/threading) (threading and logging support).
- [ariadne-cps/benchmarking](https://github.com/ariadne-cps/benchmarking) (standalone benchmarks only; never linked into the production library).
- [ariadne-cps/python-common](https://github.com/ariadne-cps/python-common) (optional Python binding infrastructure).

Utility supplies the lower Ariadne dependencies transitively.

## Build

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/cdcl.git
cd cdcl
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel --target ariadne-cdcl pyariadne
cmake --build build --parallel --target benchmarks
ctest --test-dir build --output-on-failure
```

For a C++-only build, configure with `-DBUILD_PYTHON_BINDINGS=OFF` and
build `ariadne-cdcl` without the `pyariadne` target.

The Release CI builds `benchmarks` after the library and Python bindings.
The `benchmark_cdcl_propagation` benchmark uses the shared Benchmarking
framework, with CNF construction outside the timed interval. Each sample
times fresh CDCL state construction and unit propagation; correctness checks
are outside timing. By default, it runs five samples for each of 10, 100 and
500 variables. Use `--variables`, `--samples`, `--warmups` and `--summary`
to customize the experiment and summary.

## Python bindings

When a compatible Python development environment is available, the
standalone build exposes `BooleanLiteral`, theory-feedback statuses,
`CdclTheoryFeedback` and feedback validation in the combined
`pyariadne_cdcl` extension. The autonomous `CdclSolver` search is currently
a C++ API, not a standalone Python solver binding.

## Contribution guidelines

If you would like to contribute to Ariadne CDCL, please contact the developer:

- Luca Geretti <luca.geretti@univr.it>
