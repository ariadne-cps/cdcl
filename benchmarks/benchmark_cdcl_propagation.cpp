/***************************************************************************
 *            benchmark_cdcl_propagation.cpp
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

/*! \file benchmark_cdcl_propagation.cpp
 *  \brief Ariadne CDCL benchmark.
 */

#include <chrono>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "cdcl/cdcl_state.hpp"

using namespace Ariadne;
using Clause=std::vector<BooleanLiteral>;

namespace {
class Probe : public CdclState {
  public:
    Probe(SizeType variables,std::vector<Clause> const& clauses)
        : CdclState(variables,clauses,0u) {}
    using CdclState::_unit_propagate;
    using CdclState::_next_unassigned_variable;
};
}
int main() {
    for(SizeType n: {10u,100u,500u}) {
        std::vector<Clause> cnf;
        cnf.push_back({BooleanLiteral(0u)});
        for(SizeType i=1u;i<n;++i) {
            cnf.push_back({BooleanLiteral(i-1u,false),BooleanLiteral(i)});
        }
        auto start=std::chrono::steady_clock::now();
        Probe probe(n,cnf);
        if(not probe._unit_propagate() or probe._next_unassigned_variable()!=n) {
            throw std::runtime_error("Propagation chain benchmark failed");
        }
        auto end=std::chrono::steady_clock::now();
        std::cout<<n<<" "<<std::chrono::duration<double,std::milli>(end-start).count()<<" ms\n";
    }
}
