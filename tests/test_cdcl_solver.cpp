/***************************************************************************
 *            tests/test_cdcl_solver.cpp
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne CDCL. If not, see <https://www.gnu.org/licenses/>.
 */

/** @file tests/test_cdcl_solver.cpp
 *  @brief Ariadne CDCL regression tests.
 */

#include <stdexcept>
#include "ariadne-testing.hpp"
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
class TestCdclSolver : public Ariadne::Testing::ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    Void test_cdcl_solver() {
    {
        ARIADNE_TESTING_NOTE("Root propagation and single-shot solve");
        std::vector<Clause> clauses{{BooleanLiteral(0)}, {
            BooleanLiteral(0,false),BooleanLiteral(1)}};
        CdclSolver solver(2u,clauses);
        ARIADNE_TESTING_PROVE(solver.solve()==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        ARIADNE_TESTING_PROVE(model[0].value() and model[1].value());
        ARIADNE_TESTING_PROVE(solver.statistics().boolean_propagations==2u);
        ARIADNE_TESTING_EXPECT_THROWS(std::logic_error, solver.solve());
        ARIADNE_TESTING_PROVE(solver.assignment()==model);
    }
    {
        ARIADNE_TESTING_NOTE("Contradictory root units");
        std::vector<Clause> clauses{
            {BooleanLiteral(0)}, {BooleanLiteral(0,false)}};
        CdclSolver solver(1u,clauses);
        ARIADNE_TESTING_PROVE(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        ARIADNE_TESTING_NOTE("Empty clause is UNSAT");
        std::vector<Clause> clauses{{}};
        CdclSolver solver(0u,clauses);
        ARIADNE_TESTING_PROVE(solver.solve()==CdclSolveStatus::UNSAT);
    }
    {
        ARIADNE_TESTING_NOTE("Empty zero-variable formula is SAT");
        std::vector<Clause> clauses;
        CdclSolver solver(0u,clauses);
        ARIADNE_TESTING_PROVE(solver.solve()==CdclSolveStatus::SAT);
    }
    {
        ARIADNE_TESTING_NOTE("Theory rejects a root assignment");
        std::vector<Clause> clauses{{BooleanLiteral(0)}};
        ExclusionTheory theory;
        CdclSolver solver(2u,clauses);
        ARIADNE_TESTING_PROVE(solver.solve(&theory)==CdclSolveStatus::UNSAT);
        ARIADNE_TESTING_PROVE(solver.statistics().theory_learned_clauses>=1u);
    }
    {
        ARIADNE_TESTING_NOTE("Inconclusive theory and completion");
        std::vector<Clause> clauses;
        UnknownTheory theory;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(&theory,
            [](CdclAssignment const&) { return CdclCompletionStatus::UNKNOWN; });
        ARIADNE_TESTING_PROVE(status==CdclSolveStatus::UNKNOWN);
    }
    {
        ARIADNE_TESTING_NOTE("Completion certifies the second branch");
        std::vector<Clause> clauses;
        CdclSolver solver(1u,clauses);
        auto status=solver.solve(nullptr,
            [](CdclAssignment const& model) {
                return model[0].value()
                    ? CdclCompletionStatus::CERTIFIED
                    : CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TESTING_PROVE(status==CdclSolveStatus::SAT);
        // assignment() returns by value. Keep the model alive while the
        // testing check captures the value() result by reference.
        CdclAssignment const model=solver.assignment();
        ARIADNE_TESTING_PROVE(model[0].value());
    }
    {
        ARIADNE_TESTING_NOTE("Exhaustive completion rejection visits every leaf");
        // All four leaves must be visited. A broken nested backtrack can
        // erase the parent decision and recurse indefinitely here.
        std::vector<Clause> clauses;
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [this, &completed_models](CdclAssignment const& assignment) {
                ARIADNE_TESTING_PROVE(assignment[0].has_value());
                ARIADNE_TESTING_PROVE(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TESTING_PROVE(status==CdclSolveStatus::UNSAT);
        ARIADNE_TESTING_PROVE(completed_models==4u);
    }
    {
        ARIADNE_TESTING_NOTE("Root propagation survives exhaustive completion rejection");
        // Root propagation is kept after exploring both branches of the
        // unrelated decision variable. Only two completions are possible.
        std::vector<Clause> clauses{{BooleanLiteral(0u)}};
        CdclSolver solver(2u,clauses);
        SizeType completed_models=0u;
        auto status=solver.solve(nullptr,
            [this, &completed_models](CdclAssignment const& assignment) {
                ARIADNE_TESTING_PROVE(assignment[0]==std::optional<Bool>(true));
                ARIADNE_TESTING_PROVE(assignment[1].has_value());
                ++completed_models;
                return CdclCompletionStatus::REJECTED;
            });
        ARIADNE_TESTING_PROVE(status==CdclSolveStatus::UNSAT);
        ARIADNE_TESTING_PROVE(completed_models==2u);
        ARIADNE_TESTING_PROVE(solver.statistics().boolean_propagations==1u);
    }
    {
        ARIADNE_TESTING_NOTE("Truth-table differential oracle");
        // Differential oracle against complete truth-table enumeration:
        // every generated instance is checked for both status and witness.
        // Include empty, repeated, tautological, and ternary clauses.
        for(SizeType seed=0u;seed<256u;++seed) {
            ARIADNE_TESTING_NOTE("Truth-table CNF seed " << seed);
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
            ARIADNE_TESTING_PROVE((status==CdclSolveStatus::SAT)==expected_sat);
            if(status==CdclSolveStatus::SAT) {
                CdclAssignment assignment=solver.assignment();
                for(Clause const& clause:clauses) {
                    Bool clause_true=false;
                    for(BooleanLiteral literal:clause) {
                        ARIADNE_TESTING_PROVE(assignment[literal.variable()].has_value());
                        clause_true=clause_true or
                            (*assignment[literal.variable()]==literal.positive());
                    }
                    ARIADNE_TESTING_PROVE(clause_true);
                }
            }
        }
    }
    {
        ARIADNE_TESTING_NOTE("Theory conflict backjumps past an unrelated decision");
        std::vector<Clause> clauses;
        DisjunctionTheory theory;
        CdclSolver solver(3u,clauses);
        ARIADNE_TESTING_PROVE(solver.assignment()==CdclAssignment(3u));
        ARIADNE_TESTING_PROVE(solver.solve(&theory)==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        ARIADNE_TESTING_PROVE(model==CdclAssignment({false,false,true}));
        auto const& stats=solver.statistics();
        ARIADNE_TESTING_PROVE(stats.boolean_conflicts_analyzed==1u);
        ARIADNE_TESTING_PROVE(stats.nonchronological_backjumps==1u);
        ARIADNE_TESTING_PROVE(stats.last_backjump_level==1u);
        ARIADNE_TESTING_PROVE(stats.last_learned_current_level_literals==1u);
        ARIADNE_TESTING_PROVE(stats.last_learned_clause_literals==2u);
        ARIADNE_TESTING_PROVE(stats.theory_learned_clauses==1u);
        ARIADNE_TESTING_PROVE(stats.theory_learned_clause_propagations==1u);
    }
    {
        ARIADNE_TESTING_NOTE("Undeclared original variable is rejected");
        std::vector<Clause> clauses{{BooleanLiteral(1)}};
        ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, CdclSolver(1u,clauses));
    }
}

};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("test_cdcl_solver");
    ARIADNE_TESTING_ADD_TEST(TestCdclSolver, test_cdcl_solver);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}
