/***************************************************************************
 *            test_cdcl_theory.cpp
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

/*! \file test_cdcl_theory.cpp
 *  \brief Ariadne CDCL regression tests.
 */

#include <stdexcept>
#include <memory>
#include "cdcl/cdcl_theory.hpp"
#include "macro/testing.hpp"

using namespace Ariadne;

namespace {
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
class OwnedTheory : public MockTheory {
  public:
    explicit OwnedTheory(Bool& destroyed) : _destroyed(destroyed) { }
    ~OwnedTheory() override { _destroyed=true; }
  private:
    Bool& _destroyed;
};

Void test_feedback_statuses() {
    MockTheory theory;
    CdclAssignment partial(2u);
    auto u=theory.check_partial(partial);
    ARIADNE_TEST_ASSERT(u.status()==CdclTheoryStatus::UNKNOWN);
    ARIADNE_TEST_TRY(validate_cdcl_theory_feedback(u,partial));

    partial[0]=false;
    auto c=theory.check_partial(partial);
    ARIADNE_TEST_ASSERT(c.status()==CdclTheoryStatus::CONSISTENT);
    ARIADNE_TEST_TRY(validate_cdcl_theory_feedback(c,partial));

    partial[0]=true;
    auto f=theory.check_partial(partial);
    ARIADNE_TEST_ASSERT(f.status()==CdclTheoryStatus::CONFLICT);
    ARIADNE_TEST_EQUALS(f.explanation().size(),1u);
    ARIADNE_TEST_TRY(validate_cdcl_theory_feedback(f,partial));
}

Void test_feedback_validation() {
    CdclAssignment partial{true,std::nullopt};
    ARIADNE_PRINT_TEST_CASE_TITLE("A satisfied conflict literal is invalid");
    ARIADNE_TEST_THROWS(validate_cdcl_theory_feedback(
        CdclTheoryFeedback::conflict({BooleanLiteral(0)}),partial),std::invalid_argument);

    ARIADNE_PRINT_TEST_CASE_TITLE("An unassigned conflict literal is invalid");
    ARIADNE_TEST_THROWS(validate_cdcl_theory_feedback(
        CdclTheoryFeedback::conflict({BooleanLiteral(1)}),partial),std::invalid_argument);

    ARIADNE_PRINT_TEST_CASE_TITLE("An empty clause represents a global contradiction");
    ARIADNE_TEST_TRY(validate_cdcl_theory_feedback(CdclTheoryFeedback::conflict({}),partial));

    ARIADNE_PRINT_TEST_CASE_TITLE("An undeclared conflict variable is invalid");
    ARIADNE_TEST_THROWS(validate_cdcl_theory_feedback(
        CdclTheoryFeedback::conflict({BooleanLiteral(partial.size())}),partial),std::out_of_range);
}

Void test_theory_ownership() {
    CdclAssignment partial{true,std::nullopt};
    Bool destroyed=false;
    std::unique_ptr<CdclTheory> owned=std::make_unique<OwnedTheory>(destroyed);
    ARIADNE_TEST_ASSERT(owned->check_partial(partial).status()==CdclTheoryStatus::CONFLICT);
    owned.reset();
    ARIADNE_TEST_ASSERT(destroyed);
}
}

int main() {
    ARIADNE_TEST_CALL(test_feedback_statuses());
    ARIADNE_TEST_CALL(test_feedback_validation());
    ARIADNE_TEST_CALL(test_theory_ownership());
    return ARIADNE_TEST_FAILURES;
}
