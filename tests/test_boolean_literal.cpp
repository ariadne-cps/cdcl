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
#include "macro/testing.hpp"

using namespace Ariadne;

namespace {
Void test_index_and_negation() {
    BooleanLiteral first(0u);
    ARIADNE_TEST_EQUALS(first.variable(),0u);
    ARIADNE_TEST_ASSERT(first.positive());
    ARIADNE_TEST_EQUALS(first.dimacs(),1);
    ARIADNE_TEST_ASSERT(first.negation()==BooleanLiteral(0u,false));
    ARIADNE_TEST_EQUALS((-first).dimacs(),-1);
    ARIADNE_TEST_ASSERT(-(-first)==first);
    ARIADNE_TEST_ASSERT(first!= -first);
    ARIADNE_TEST_ASSERT(not (first!=first));
}

Void test_dimacs_round_trip_and_boundaries() {
    Int const largest=std::numeric_limits<Int>::max();
    SizeType const last_variable=static_cast<SizeType>(largest)-1u;
    for(Int value: {Int(1),Int(-1),largest,-largest}) {
        ARIADNE_PRINT_TEST_CASE_TITLE("DIMACS round trip for " << value);
        BooleanLiteral literal=BooleanLiteral::from_dimacs(value);
        ARIADNE_TEST_EQUALS(literal.dimacs(),value);
        ARIADNE_TEST_EQUALS(literal.positive(),value>0);
        ARIADNE_TEST_EQUALS(literal.variable(),(value==1 or value==-1 ? 0u : last_variable));
        ARIADNE_TEST_EQUALS(literal.negation().dimacs(),-value);
    }
    ARIADNE_TEST_EQUALS(BooleanLiteral(last_variable).dimacs(),largest);
    ARIADNE_TEST_EQUALS(BooleanLiteral(last_variable,false).dimacs(),-largest);
}

Void test_invalid_ranges() {
    ARIADNE_TEST_THROWS(BooleanLiteral::from_dimacs(0),std::out_of_range);
    ARIADNE_TEST_THROWS(BooleanLiteral::from_dimacs(std::numeric_limits<Int>::min()),std::out_of_range);
    ARIADNE_TEST_THROWS((void)BooleanLiteral(static_cast<SizeType>(std::numeric_limits<Int>::max())),
                        std::out_of_range);
    ARIADNE_TEST_THROWS((void)BooleanLiteral(std::numeric_limits<SizeType>::max()),std::out_of_range);
}
}

int main() {
    ARIADNE_TEST_CALL(test_index_and_negation());
    ARIADNE_TEST_CALL(test_dimacs_round_trip_and_boundaries());
    ARIADNE_TEST_CALL(test_invalid_ranges());
    return ARIADNE_TEST_FAILURES;
}
