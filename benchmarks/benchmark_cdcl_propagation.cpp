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
