/***************************************************************************
 *            cdcl/boolean_literal.hpp
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

/*! \file cdcl/boolean_literal.hpp
 *  \brief Strongly typed Boolean literals with zero-based variable indices.
 */

#ifndef ARIADNE_CDCL_BOOLEAN_LITERAL_HPP
#define ARIADNE_CDCL_BOOLEAN_LITERAL_HPP

#include <limits>
#include <stdexcept>
#include <type_traits>

#include "utility/typedefs.hpp"

namespace Ariadne {

//! \brief A signed Boolean literal with a zero-based logical variable index.
//! \details One-based signed integers are used only for DIMACS serialization.
//! Zero and the most-negative signed integer are not valid DIMACS literals.
class BooleanLiteral {
  public:
    explicit BooleanLiteral(SizeType variable, Bool positive=true) {
        if(variable>=static_cast<SizeType>(std::numeric_limits<Int>::max())) {
            throw std::out_of_range("Boolean variable exceeds DIMACS literal range");
        }
        Int magnitude=static_cast<Int>(variable+1u);
        _dimacs=positive ? magnitude : -magnitude;
    }

    static BooleanLiteral from_dimacs(Int value) {
        if(value==0 or value==std::numeric_limits<Int>::min()) {
            throw std::out_of_range("Invalid signed Boolean literal");
        }
        return BooleanLiteral(value,RawTag{});
    }

    SizeType variable() const {
        return static_cast<SizeType>(_dimacs<0 ? -_dimacs : _dimacs)-1u;
    }
    Bool positive() const { return _dimacs>0; }
    Int dimacs() const { return _dimacs; }
    BooleanLiteral negation() const {
        return BooleanLiteral(-_dimacs,RawTag{});
    }

    friend BooleanLiteral operator-(BooleanLiteral literal) {
        return literal.negation();
    }
    friend Bool operator==(BooleanLiteral lhs,BooleanLiteral rhs) {
        return lhs._dimacs==rhs._dimacs;
    }
    friend Bool operator!=(BooleanLiteral lhs,BooleanLiteral rhs) {
        return not (lhs==rhs);
    }

  private:
    struct RawTag { };
    BooleanLiteral(Int value,RawTag) : _dimacs(value) { }
    Int _dimacs;
};

static_assert(sizeof(BooleanLiteral)==sizeof(Int));
static_assert(std::is_trivially_copyable_v<BooleanLiteral>);

} // namespace Ariadne

#endif // ARIADNE_CDCL_BOOLEAN_LITERAL_HPP
