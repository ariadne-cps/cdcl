/***************************************************************************
 *            tests/test_cdcl_theory.cpp
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

/** @file tests/test_cdcl_theory.cpp
 *  @brief Ariadne CDCL regression tests.
 */

#include <memory>
#include <stdexcept>

#include "cdcl/cdcl_theory.hpp"

#include "ariadne-testing.hpp"

using namespace Ariadne;

namespace {
class MockTheory : public CdclTheory {
  public:
    CdclTheoryFeedback check_partial(const CdclAssignment& assignment) override {
        if (assignment[0].has_value() and *assignment[0]) {
            return CdclTheoryFeedback::conflict({BooleanLiteral(0, false)});
        }
        if (not assignment[0].has_value()) {
            return CdclTheoryFeedback::unknown();
        }
        return CdclTheoryFeedback::consistent();
    }
};
class OwnedTheory : public MockTheory {
  public:
    explicit OwnedTheory(Bool& destroyed) : _destroyed(destroyed) {}
    ~OwnedTheory() override { _destroyed = true; }

  private:
    Bool& _destroyed;
};

class TestCdclTheory : public Ariadne::Testing::ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    Void test_feedback_statuses() {
        MockTheory theory;
        CdclAssignment partial(2u);
        auto u = theory.check_partial(partial);
        ARIADNE_TESTING_PROVE(u.status() == CdclTheoryStatus::UNKNOWN);
        ARIADNE_TESTING_EXPECT_NO_THROW(validate_cdcl_theory_feedback(u, partial));

        partial[0] = false;
        auto c = theory.check_partial(partial);
        ARIADNE_TESTING_PROVE(c.status() == CdclTheoryStatus::CONSISTENT);
        ARIADNE_TESTING_EXPECT_NO_THROW(validate_cdcl_theory_feedback(c, partial));

        partial[0] = true;
        auto f = theory.check_partial(partial);
        ARIADNE_TESTING_PROVE(f.status() == CdclTheoryStatus::CONFLICT);
        ARIADNE_TESTING_EXPECT_EQUAL(f.explanation().size(), 1u);
        ARIADNE_TESTING_EXPECT_NO_THROW(validate_cdcl_theory_feedback(f, partial));
    }

    Void test_feedback_validation() {
        CdclAssignment partial{true, std::nullopt};
        ARIADNE_TESTING_NOTE("A satisfied conflict literal is invalid");
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, validate_cdcl_theory_feedback(
                                                                 CdclTheoryFeedback::conflict({BooleanLiteral(0)}), partial));

        ARIADNE_TESTING_NOTE("An unassigned conflict literal is invalid");
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, validate_cdcl_theory_feedback(
                                                                 CdclTheoryFeedback::conflict({BooleanLiteral(1)}), partial));

        ARIADNE_TESTING_NOTE("An empty clause represents a global contradiction");
        ARIADNE_TESTING_EXPECT_NO_THROW(validate_cdcl_theory_feedback(CdclTheoryFeedback::conflict({}), partial));

        ARIADNE_TESTING_NOTE("An undeclared conflict variable is invalid");
        ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, validate_cdcl_theory_feedback(
                                                             CdclTheoryFeedback::conflict({BooleanLiteral(partial.size())}), partial));
    }

    Void test_theory_ownership() {
        CdclAssignment partial{true, std::nullopt};
        Bool destroyed = false;
        std::unique_ptr<CdclTheory> owned = std::make_unique<OwnedTheory>(destroyed);
        ARIADNE_TESTING_PROVE(owned->check_partial(partial).status() == CdclTheoryStatus::CONFLICT);
        owned.reset();
        ARIADNE_TESTING_PROVE(destroyed);
    }
};
} // namespace

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("test_cdcl_theory");
    ARIADNE_TESTING_ADD_TEST(TestCdclTheory, test_feedback_statuses);
    ARIADNE_TESTING_ADD_TEST(TestCdclTheory, test_feedback_validation);
    ARIADNE_TESTING_ADD_TEST(TestCdclTheory, test_theory_ownership);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}
