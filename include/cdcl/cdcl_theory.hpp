/***************************************************************************
 *            cdcl/cdcl_theory.hpp
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

/*! \file cdcl/cdcl_theory.hpp
 *  \brief Interface and feedback contract for theory solvers.
 */

#ifndef ARIADNE_CDCL_THEORY_HPP
#define ARIADNE_CDCL_THEORY_HPP

#include <optional>
#include <vector>
#include "cdcl/boolean_literal.hpp"

namespace Ariadne {

//! A partial Boolean assignment; nullopt means unassigned.
using CdclAssignment = std::vector<std::optional<Bool>>;
using BooleanClause = std::vector<BooleanLiteral>;

//! CONSISTENT means "not disproved", not a certified satisfying witness.
//! UNKNOWN means the theory check was inconclusive, never a logical conflict.
enum class CdclTheoryStatus { CONSISTENT, CONFLICT, UNKNOWN };

class CdclTheoryFeedback {
  public:
    static CdclTheoryFeedback consistent();
    static CdclTheoryFeedback conflict(BooleanClause explanation);
    static CdclTheoryFeedback unknown();

    CdclTheoryStatus status() const { return _status; }
    BooleanClause const& explanation() const { return _explanation; }

  private:
    CdclTheoryFeedback(CdclTheoryStatus status, BooleanClause explanation);
    CdclTheoryStatus _status;
    BooleanClause _explanation;
};

//! Rejects malformed feedback; conflict explanations must be falsified by
//! the current assignment. Logical soundness is the theory solver's obligation.
//! An empty conflict clause is valid only for a globally inconsistent theory.
Void validate_cdcl_theory_feedback(
    CdclTheoryFeedback const& feedback, CdclAssignment const& assignment);

//! Interface for partial assignment consistency checks.
//! Complete model certification is theory-specific and stays in the caller.
class CdclTheory {
  public:
    virtual ~CdclTheory() = default;
    virtual CdclTheoryFeedback check_partial(
        CdclAssignment const& assignment) = 0;
};

} // namespace Ariadne
#endif // ARIADNE_CDCL_THEORY_HPP
