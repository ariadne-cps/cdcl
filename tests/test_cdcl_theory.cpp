#include <stdexcept>
#include "cdcl/cdcl_theory.hpp"

using namespace Ariadne;

namespace {
Void require(Bool ok) {
    if(not ok) { throw std::runtime_error("CDCL theory test failed"); }
}
class MockTheory : public CdclTheory {
  public:
    CdclTheoryFeedback check_partial(CdclAssignment const& assignment) override {
        if(assignment[0].has_value() and *assignment[0]) {
            return CdclTheoryFeedback::conflict({BooleanLiteral(0,false)});
        }
        if(not assignment[0].has_value()) {
            return CdclTheoryFeedback::unknown();
        }
        return CdclTheoryFeedback::consistent();
    }
};
}
int main() {
    MockTheory theory;
    CdclAssignment partial(2u);
    auto u=theory.check_partial(partial);
    require(u.status()==CdclTheoryStatus::UNKNOWN);
    validate_cdcl_theory_feedback(u,partial);

    partial[0]=false;
    auto c=theory.check_partial(partial);
    require(c.status()==CdclTheoryStatus::CONSISTENT);
    validate_cdcl_theory_feedback(c,partial);

    partial[0]=true;
    auto f=theory.check_partial(partial);
    require(f.status()==CdclTheoryStatus::CONFLICT);
    require(f.explanation().size()==1u);
    validate_cdcl_theory_feedback(f,partial);

    Bool threw=false;
    try {
        validate_cdcl_theory_feedback(
            CdclTheoryFeedback::conflict({BooleanLiteral(0)}),partial);
    } catch(std::invalid_argument const&) { threw=true; }
    require(threw);

    threw=false;
    try {
        validate_cdcl_theory_feedback(
            CdclTheoryFeedback::conflict({BooleanLiteral(1)}),partial);
    } catch(std::invalid_argument const&) { threw=true; }
    require(threw);

    // Empty clause is permitted as an explicit global contradiction.
    validate_cdcl_theory_feedback(CdclTheoryFeedback::conflict({}),partial);
    return 0;
}
