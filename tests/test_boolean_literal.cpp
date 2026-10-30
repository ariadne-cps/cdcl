#include <limits>
#include <stdexcept>
#include "cdcl/boolean_literal.hpp"

using namespace Ariadne;

namespace {
Void require(Bool condition, char const* message) {
    if(not condition) { throw std::runtime_error(message); }
}
template<class F> Void require_out_of_range(F const& operation, char const* message) {
    try { operation(); }
    catch(std::out_of_range const&) { return; }
    throw std::runtime_error(message);
}
}

int main() {
    BooleanLiteral first(0u);
    require(first.variable()==0u and first.positive() and first.dimacs()==1,
            "First variable must serialize as DIMACS +1");
    require(first.negation()==BooleanLiteral(0u,false), "Negation must reverse polarity");
    require((-first).dimacs()==-1, "Unary minus must negate a literal");
    require(-(-first)==first, "Double negation must preserve a literal");
    require(first!= -first, "Opposite polarities must differ");
    require(not (first!=first), "Equal literals must not differ");

    Int const largest=std::numeric_limits<Int>::max();
    SizeType const last_variable=static_cast<SizeType>(largest)-1u;
    for(Int value: {Int(1),Int(-1),largest,-largest}) {
        BooleanLiteral literal=BooleanLiteral::from_dimacs(value);
        require(literal.dimacs()==value, "DIMACS round trip must preserve the signed value");
        require(literal.positive()==(value>0), "DIMACS sign must determine polarity");
        require(literal.variable()==(value==1 or value==-1 ? 0u : last_variable),
                "DIMACS magnitude must map to a zero-based variable");
        require(literal.negation().dimacs()==-value, "Boundary literals must negate safely");
    }
    require(BooleanLiteral(last_variable).dimacs()==largest, "Largest positive literal");
    require(BooleanLiteral(last_variable,false).dimacs()==-largest, "Largest negative literal");
    require_out_of_range([] { (void)BooleanLiteral::from_dimacs(0); }, "DIMACS zero must be rejected");
    require_out_of_range([] { (void)BooleanLiteral::from_dimacs(std::numeric_limits<Int>::min()); },
                         "Most negative DIMACS integer must be rejected");
    require_out_of_range([] { (void)BooleanLiteral(static_cast<SizeType>(std::numeric_limits<Int>::max())); },
                         "First unrepresentable variable must be rejected");
    require_out_of_range([] { (void)BooleanLiteral(std::numeric_limits<SizeType>::max()); },
                         "Variable conversion must reject overflow");
    return 0;
}
