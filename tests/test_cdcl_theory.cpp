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
class OwnedTheory : public MockTheory {
  public:
    explicit OwnedTheory(Bool& destroyed) : _destroyed(destroyed) { }
    ~OwnedTheory() override { _destroyed=true; }
  private:
    Bool& _destroyed;
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

    threw=false;
    try {
        validate_cdcl_theory_feedback(
            CdclTheoryFeedback::conflict({BooleanLiteral(partial.size())}),partial);
    } catch(std::out_of_range const&) { threw=true; }
    if(not threw) { throw std::runtime_error("Out-of-range theory variable must be rejected"); }

    // Theory adapters can be owned and destroyed through the interface.
    Bool destroyed=false;
    std::unique_ptr<CdclTheory> owned=std::make_unique<OwnedTheory>(destroyed);
    require(owned->check_partial(partial).status()==CdclTheoryStatus::CONFLICT);
    owned.reset();
    if(not destroyed) { throw std::runtime_error("Theory interface must destroy its derived adapter"); }
    return 0;
}
