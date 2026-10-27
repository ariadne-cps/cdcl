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
