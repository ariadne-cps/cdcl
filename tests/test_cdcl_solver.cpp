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
        std::vector<Clause> clauses;
        CdclSolver solver(2u,clauses);
        auto status=solver.solve(nullptr,
            [](CdclAssignment const&) { return CdclCompletionStatus::REJECTED; });
        require(status==CdclSolveStatus::UNSAT);
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
