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
#include "macro/testing.hpp"
#include <vector>
#include <cstdint>
#include "cdcl/cdcl_solver.hpp"

using namespace Ariadne;

namespace {
using Clause=BooleanClause;
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
// The theory requires x0 OR x2, independently of x1. A conflict at
// level three must jump past x1 and assert x2 at level one.
class DisjunctionTheory : public CdclTheory {
  public:
    CdclTheoryFeedback check_partial(CdclAssignment const& assignment) override {
        if(assignment[0]==std::optional<Bool>(false)
                and assignment[2]==std::optional<Bool>(false)) {
            return CdclTheoryFeedback::conflict({BooleanLiteral(0u),BooleanLiteral(2u)});
        }
        return CdclTheoryFeedback::consistent();
    }
};
}
Void test_cdcl_solver() {
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Root propagation and single-shot solve");
        std::vector<Clause> clauses{{BooleanLiteral(0)}, {
            BooleanLiteral(0,false),BooleanLiteral(1)}};
        CdclSolver solver(2u,clauses);
        ARIADNE_TEST_ASSERT(solver.solve()==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        ARIADNE_TEST_ASSERT(model[0].value() and model[1].value());
        ARIADNE_TEST_ASSERT(solver.statistics().boolean_propagations==2u);
        ARIADNE_TEST_THROWS(solver.solve(),std::logic_error);
        ARIADNE_TEST_ASSERT(solver.assignment()==model);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Contradictory root units");
        std::vector<Clause> clauses{
            {BooleanLiteral(0)}, {BooleanLiteral(0,false)}};
        CdclSolver solver(1u,clauses);
        ARIADNE_TEST_ASSERT(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Empty clause is UNSAT");
        std::vector<Clause> clauses{{}};
        CdclSolver solver(0u,clauses);
        ARIADNE_TEST_ASSERT(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Empty zero-variable formula is SAT");
        std::vector<Clause> clauses;
        CdclSolver solver(0u,clauses);
        ARIADNE_TEST_ASSERT(solver.solve()==CdclSolveStatus::SAT);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Theory rejects a root assignment");
        std::vector<Clause> clauses{{BooleanLiteral(0)}};
        ExclusionTheory theory;
        CdclSolver solver(2u,clauses);
        ARIADNE_TEST_ASSERT(solver.solve(&theory)==CdclSolveStatus::UNSAT);
        ARIADNE_TEST_ASSERT(solver.statistics().theory_learned_clauses>=1u);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Inconclusive theory and completion");
        std::vector<Clause> clauses;
        UnknownTheory theory;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(&theory,
            [](CdclAssignment const&) { return CdclCompletionStatus::UNKNOWN; });
        ARIADNE_TEST_ASSERT(status==CdclSolveStatus::UNKNOWN);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Completion certifies the second branch");
        std::vector<Clause> clauses;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(nullptr,
            [](CdclAssignment const& model) {
                return model[0].value()
                    ? CdclCompletionStatus::CERTIFIED
                    : CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TEST_ASSERT(status==CdclSolveStatus::SAT);
        ARIADNE_TEST_ASSERT(solver.assignment()[0].value());
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Exhaustive completion rejection visits every leaf");
        // All four leaves must be visited. A broken nested backtrack can
        // erase the parent decision and recurse indefinitely here.
        std::vector<Clause> clauses;
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [&completed_models](CdclAssignment const& assignment) {
                ARIADNE_TEST_ASSERT(assignment[0].has_value());
                ARIADNE_TEST_ASSERT(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TEST_ASSERT(status==CdclSolveStatus::UNSAT);
        ARIADNE_TEST_ASSERT(completed_models==4u);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Root propagation survives exhaustive completion rejection");
        // Root propagation is kept after exploring both branches of the
        // unrelated decision variable. Only two completions are possible.
        std::vector<Clause> clauses{{BooleanLiteral(0u)}};
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [&completed_models](CdclAssignment const& assignment) {
                ARIADNE_TEST_ASSERT(assignment[0]==std::optional<Bool>(true));
                ARIADNE_TEST_ASSERT(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TEST_ASSERT(status==CdclSolveStatus::UNSAT);
        ARIADNE_TEST_ASSERT(completed_models==2u);
        ARIADNE_TEST_ASSERT(solver.statistics().boolean_propagations==1u);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Truth-table differential oracle");
        // Differential oracle against complete truth-table enumeration:
        // every generated instance is checked for both status and witness.
        // Include empty, repeated, tautological, and ternary clauses.
        for(SizeType seed=0u;seed<256u;++seed) {
            ARIADNE_PRINT_TEST_CASE_TITLE("Truth-table CNF seed " << seed);
            std::vector<Clause> clauses;
            for(SizeType j=0u;j<7u;++j) {
                SizeType code=(seed*29u+j*17u+seed*j*3u)%343u;
                Clause clause;
                SizeType length=code%4u;
                code/=4u;
                for(SizeType k=0u;k<length;++k) {
                    SizeType v=(code+2u*k)%3u;
                    Bool positive=((code>>(k+1u))&1u)!=0u;
                    clause.emplace_back(v,positive);
                }
                clauses.push_back(clause);
            }
            Bool expected_sat=false;
            for(SizeType bits=0u;bits<8u;++bits) {
                Bool all_clauses=true;
                for(Clause const& clause:clauses) {
                    Bool clause_true=false;
                    for(BooleanLiteral literal:clause) {
                        Bool value=((bits>>literal.variable())&1u)!=0u;
                        clause_true=clause_true or value==literal.positive();
                    }
                    all_clauses=all_clauses and clause_true;
                }
                expected_sat=expected_sat or all_clauses;
            }
            CdclSolver solver(3u,clauses);
            auto status=solver.solve();
            ARIADNE_TEST_ASSERT((status==CdclSolveStatus::SAT)==expected_sat);
            if(status==CdclSolveStatus::SAT) {
                CdclAssignment assignment=solver.assignment();
                for(Clause const& clause:clauses) {
                    Bool clause_true=false;
                    for(BooleanLiteral literal:clause) {
                        ARIADNE_TEST_ASSERT(assignment[literal.variable()].has_value());
                        clause_true=clause_true or
                            (*assignment[literal.variable()]==literal.positive());
                    }
                    ARIADNE_TEST_ASSERT(clause_true);
                }
            }
        }
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Theory conflict backjumps past an unrelated decision");
        std::vector<Clause> clauses;
        DisjunctionTheory theory;
        CdclSolver solver(3u,clauses);
        ARIADNE_TEST_ASSERT(solver.assignment()==CdclAssignment(3u));
        ARIADNE_TEST_ASSERT(solver.solve(&theory)==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        ARIADNE_TEST_ASSERT(model==CdclAssignment({false,false,true}));
        auto const& stats=solver.statistics();
        ARIADNE_TEST_ASSERT(stats.boolean_conflicts_analyzed==1u);
        ARIADNE_TEST_ASSERT(stats.nonchronological_backjumps==1u);
        ARIADNE_TEST_ASSERT(stats.last_backjump_level==1u);
        ARIADNE_TEST_ASSERT(stats.last_learned_current_level_literals==1u);
        ARIADNE_TEST_ASSERT(stats.last_learned_clause_literals==2u);
        ARIADNE_TEST_ASSERT(stats.theory_learned_clauses==1u);
        ARIADNE_TEST_ASSERT(stats.theory_learned_clause_propagations==1u);
    }
    {
        ARIADNE_PRINT_TEST_CASE_TITLE("Undeclared original variable is rejected");
        std::vector<Clause> clauses{{BooleanLiteral(1)}};
        ARIADNE_TEST_THROWS(CdclSolver(1u,clauses),std::out_of_range);
    }
}

int main() {
    ARIADNE_TEST_CALL(test_cdcl_solver());
    return ARIADNE_TEST_FAILURES;
}
