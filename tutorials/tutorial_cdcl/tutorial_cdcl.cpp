#include <iostream>
#include "ariadne-cdcl.hpp"

using namespace Ariadne;

int main() {
    BooleanLiteral x(0u);
    BooleanLiteral not_x=x.negation();
    if(x.dimacs()!=1 or not_x.dimacs()!=-1) { return 1; }

    CdclAssignment assignment(1u);
    assignment[0]=true;
    auto conflict=CdclTheoryFeedback::conflict({not_x});
    validate_cdcl_theory_feedback(conflict,assignment);
    std::cout<<"Theory conflict explanation validated\n";
    assignment[0].reset();
    auto unknown=CdclTheoryFeedback::unknown();
    validate_cdcl_theory_feedback(unknown,assignment);
    std::cout<<"UNKNOWN is not a Boolean conflict\n";
    std::vector<BooleanClause> clauses{{BooleanLiteral(0u)}};
    CdclSolver solver(1u,clauses);
    if(solver.solve()!=CdclSolveStatus::SAT) { return 1; }
    std::cout<<"A satisfiable CNF was solved\n";
    return 0;
}
