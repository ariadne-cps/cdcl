/***************************************************************************
 *            test_boolean_literal.cpp
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

/*! \file test_boolean_literal.cpp
 *  \brief Boolean literal conversion and range regression tests.
 */

#include <limits>
#include <stdexcept>
#include "cdcl/boolean_literal.hpp"
#include "ariadne-testing.hpp"

using namespace Ariadne;

namespace {
class TestBooleanLiteral : public Ariadne::Testing::ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    Void test_index_and_negation() {
    BooleanLiteral first(0u);
    ARIADNE_TESTING_EXPECT_EQUAL(first.variable(),0u);
    ARIADNE_TESTING_PROVE(first.positive());
    ARIADNE_TESTING_EXPECT_EQUAL(first.dimacs(),1);
    ARIADNE_TESTING_PROVE(first.negation()==BooleanLiteral(0u,false));
    ARIADNE_TESTING_EXPECT_EQUAL((-first).dimacs(),-1);
    ARIADNE_TESTING_PROVE(-(-first)==first);
    ARIADNE_TESTING_PROVE(first!= -first);
    ARIADNE_TESTING_PROVE(not (first!=first));
}

Void test_dimacs_round_trip_and_boundaries() {
    Int const largest=std::numeric_limits<Int>::max();
    SizeType const last_variable=static_cast<SizeType>(largest)-1u;
    for(Int value: {Int(1),Int(-1),largest,-largest}) {
        ARIADNE_TESTING_NOTE("DIMACS round trip for " << value);
        BooleanLiteral literal=BooleanLiteral::from_dimacs(value);
        ARIADNE_TESTING_EXPECT_EQUAL(literal.dimacs(),value);
        ARIADNE_TESTING_EXPECT_EQUAL(literal.positive(),value>0);
        ARIADNE_TESTING_EXPECT_EQUAL(literal.variable(),(value==1 or value==-1 ? 0u : last_variable));
        ARIADNE_TESTING_EXPECT_EQUAL(literal.negation().dimacs(),-value);
    }
    ARIADNE_TESTING_EXPECT_EQUAL(BooleanLiteral(last_variable).dimacs(),largest);
    ARIADNE_TESTING_EXPECT_EQUAL(BooleanLiteral(last_variable,false).dimacs(),-largest);
}

Void test_invalid_ranges() {
    ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, BooleanLiteral::from_dimacs(0));
    ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, BooleanLiteral::from_dimacs(std::numeric_limits<Int>::min()));
    ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, (void)BooleanLiteral(static_cast<SizeType>(std::numeric_limits<Int>::max())));
    ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, (void)BooleanLiteral(std::numeric_limits<SizeType>::max()));
}
};
}

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("test_boolean_literal");
    ARIADNE_TESTING_ADD_TEST(TestBooleanLiteral, test_index_and_negation);
    ARIADNE_TESTING_ADD_TEST(TestBooleanLiteral, test_dimacs_round_trip_and_boundaries);
    ARIADNE_TESTING_ADD_TEST(TestBooleanLiteral, test_invalid_ranges);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}
