/***************************************************************************
 *            test_cdcl_solver.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne CDCL.
 *
 *  Ariadne CDCL is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne CDCL is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne CDCL.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file test_cdcl_solver.cpp
 *  \brief Ariadne CDCL regression tests.
 */

#include <stdexcept>
#include <vector>
#include "cdcl/cdcl_solver.hpp"

using namespace Ariadne;

namespace {
using Clause=BooleanClause;
Void require(Bool condition) {
    if(not condition) { throw std::runtime_error("CDCL solver regression"); }
}
class ExclusionTheory : public CdclTheory {
  public:
    CdclTheoryFeedback check_partial(CdclAssignment const& assignment) override {
        if(assignment[0]==std::optional<Bool>(true)) {
            return CdclTheoryFeedback::conflict({BooleanLiteral(0,false)});
        }
        return CdclTheoryFeedback::consistent();
    }
};
class UnknownTheory : public CdclTheory {
  public:
    CdclTheoryFeedback check_partial(CdclAssignment const&) override {
        return CdclTheoryFeedback::unknown();
    }
};
}
int main() {
    {
        std::vector<Clause> clauses{{BooleanLiteral(0)}, {
            BooleanLiteral(0,false),BooleanLiteral(1)}};
        CdclSolver solver(2u,clauses);
        require(solver.solve()==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        require(model[0].value() and model[1].value());
        require(solver.statistics().boolean_propagations==2u);
    }
    {
        std::vector<Clause> clauses{
            {BooleanLiteral(0)}, {BooleanLiteral(0,false)}};
        CdclSolver solver(1u,clauses);
        require(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        std::vector<Clause> clauses{{}};
        CdclSolver solver(0u,clauses);
        require(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        std::vector<Clause> clauses;
        CdclSolver solver(0u,clauses);
        require(solver.solve()==CdclSolveStatus::SAT);
    }
    {
        std::vector<Clause> clauses{{BooleanLiteral(0)}};
        ExclusionTheory theory;
        CdclSolver solver(2u,clauses);
        require(solver.solve(&theory)==CdclSolveStatus::UNSAT);
        require(solver.statistics().theory_learned_clauses>=1u);
    }
    {
        std::vector<Clause> clauses;
        UnknownTheory theory;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(&theory,
            [](CdclAssignment const&) { return CdclCompletionStatus::UNKNOWN; });
        require(status==CdclSolveStatus::UNKNOWN);
    }
    {
        std::vector<Clause> clauses;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(nullptr,
            [](CdclAssignment const& model) {
                return model[0].value()
                    ? CdclCompletionStatus::CERTIFIED
                    : CdclCompletionStatus::REJECTED;
            });
        require(status==CdclSolveStatus::SAT);
        require(solver.assignment()[0].value());
    }
    {
        // All four leaves must be visited. A broken nested backtrack can
        // erase the parent decision and recurse indefinitely here.
        std::vector<Clause> clauses;
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [&completed_models](CdclAssignment const& assignment) {
                require(assignment[0].has_value());
                require(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        require(status==CdclSolveStatus::UNSAT);
        require(completed_models==4u);
    }
    {
        // Root propagation is kept after exploring both branches of the
        // unrelated decision variable. Only two completions are possible.
        std::vector<Clause> clauses{{BooleanLiteral(0u)}};
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [&completed_models](CdclAssignment const& assignment) {
                require(assignment[0]==std::optional<Bool>(true));
                require(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        require(status==CdclSolveStatus::UNSAT);
        require(completed_models==2u);
        require(solver.statistics().boolean_propagations==1u);
    }
    {
        std::vector<Clause> clauses{{BooleanLiteral(1)}};
        Bool threw=false;
        try { CdclSolver solver(1u,clauses); }
        catch(std::out_of_range const&) { threw=true; }
        require(threw);
    }
    return 0;
}
