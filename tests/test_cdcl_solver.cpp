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
#include <cstdint>
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
int main() {
    {
        std::vector<Clause> clauses{{BooleanLiteral(0)}, {
            BooleanLiteral(0,false),BooleanLiteral(1)}};
        CdclSolver solver(2u,clauses);
        require(solver.solve()==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        require(model[0].value() and model[1].value());
        require(solver.statistics().boolean_propagations==2u);
        Bool rejected_repeat=false;
        try { (void)solver.solve(); }
        catch(std::logic_error const&) { rejected_repeat=true; }
        if(not rejected_repeat) { throw std::runtime_error("A second solve must be rejected"); }
        require(solver.assignment()==model);
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
        // Differential oracle against complete truth-table enumeration:
        // every generated instance is checked for both status and witness.
        // Include empty, repeated, tautological, and ternary clauses.
        for(SizeType seed=0u;seed<256u;++seed) {
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
            auto result=solver.solve();
            require((result==CdclSolveStatus::SAT)==expected_sat);
            if(result==CdclSolveStatus::SAT) {
                CdclAssignment assignment=solver.assignment();
                for(Clause const& clause:clauses) {
                    Bool clause_true=false;
                    for(BooleanLiteral literal:clause) {
                        require(assignment[literal.variable()].has_value());
                        clause_true=clause_true or
                            (*assignment[literal.variable()]==literal.positive());
                    }
                    require(clause_true);
                }
            }
        }
    }
    {
        std::vector<Clause> clauses;
        DisjunctionTheory theory;
        CdclSolver solver(3u,clauses);
        require(solver.assignment()==CdclAssignment(3u));
        require(solver.solve(&theory)==CdclSolveStatus::SAT);
        auto model=solver.assignment();
        if(model!=CdclAssignment({false,false,true})) {
            throw std::runtime_error("Theory backjump must retain x0 and assert x2");
        }
        auto const& stats=solver.statistics();
        require(stats.boolean_conflicts_analyzed==1u);
        require(stats.nonchronological_backjumps==1u);
        require(stats.last_backjump_level==1u);
        require(stats.last_learned_current_level_literals==1u);
        require(stats.last_learned_clause_literals==2u);
        require(stats.theory_learned_clauses==1u);
        require(stats.theory_learned_clause_propagations==1u);
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
