/***************************************************************************
 *            cdcl/cdcl_theory.cpp
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

/*! \file cdcl/cdcl_theory.cpp
 *  \brief Theory feedback validation and result handling.
 */

#include "cdcl/cdcl_theory.hpp"

#include <stdexcept>
#include <utility>

namespace Ariadne {

CdclTheoryFeedback::CdclTheoryFeedback(
    CdclTheoryStatus status, BooleanClause explanation)
    : _status(status), _explanation(std::move(explanation)) { }

CdclTheoryFeedback CdclTheoryFeedback::consistent() {
    return CdclTheoryFeedback(CdclTheoryStatus::CONSISTENT,{});
}
CdclTheoryFeedback CdclTheoryFeedback::unknown() {
    return CdclTheoryFeedback(CdclTheoryStatus::UNKNOWN,{});
}
CdclTheoryFeedback CdclTheoryFeedback::conflict(BooleanClause explanation) {
    return CdclTheoryFeedback(CdclTheoryStatus::CONFLICT,std::move(explanation));
}

Void validate_cdcl_theory_feedback(
    CdclTheoryFeedback const& feedback, CdclAssignment const& assignment)
{
    if(feedback.status()!=CdclTheoryStatus::CONFLICT) {
        if(not feedback.explanation().empty()) {
            throw std::invalid_argument("Non-conflict theory feedback cannot carry a clause");
        }
        return;
    }
    for(BooleanLiteral literal:feedback.explanation()) {
        if(literal.variable()>=assignment.size()) {
            throw std::out_of_range("Theory conflict variable exceeds assignment");
        }
        auto const& value=assignment[literal.variable()];
        if(not value.has_value() or *value==literal.positive()) {
            throw std::invalid_argument(
                "Theory conflict explanation must be falsified by the assignment");
        }
    }
}

} // namespace Ariadne
