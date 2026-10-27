/***************************************************************************
 *            tutorial_cdcl.cpp
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

/*! \file tutorial_cdcl.cpp
 *  \brief Ariadne CDCL tutorial.
 */

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
