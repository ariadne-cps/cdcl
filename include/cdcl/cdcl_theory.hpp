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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne CDCL. If not, see <https://www.gnu.org/licenses/>.
 */

/** @file cdcl/cdcl_theory.hpp
 *  @brief Interface and feedback contract for theory solvers.
 */

#ifndef ARIADNE_CDCL_THEORY_HPP
#define ARIADNE_CDCL_THEORY_HPP

#include <optional>
#include <vector>
#include "cdcl/boolean_literal.hpp"

namespace Ariadne {

//! A partial Boolean assignment; nullopt means unassigned.
using CdclAssignment = std::vector<std::optional<Bool>>;
/** @brief Represent a disjunction of Boolean literals. */
using BooleanClause = std::vector<BooleanLiteral>;

//! CONSISTENT means "not disproved", not a certified satisfying witness.
//! UNKNOWN means the theory check was inconclusive, never a logical conflict.
enum class CdclTheoryStatus { CONSISTENT, CONFLICT, UNKNOWN };

/** @brief Carry a theory check result and any conflict explanation. */
class CdclTheoryFeedback {
  public:
    /** @brief Report that the current assignment has not been disproved. */
    static CdclTheoryFeedback consistent();
    /** @brief Report a theory conflict with a sound explaining clause. */
    static CdclTheoryFeedback conflict(BooleanClause explanation);
    /** @brief Report that a theory check is inconclusive. */
    static CdclTheoryFeedback unknown();

    /** @brief Return the theory feedback classification. */
    CdclTheoryStatus status() const { return _status; }
    /** @brief Return the clause explaining a theory conflict. */
    BooleanClause const& explanation() const { return _explanation; }

  private:
    /** @brief Construct feedback from a status and explaining clause. */
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
    /** @brief Destroy the theory oracle through its abstract interface. */
    virtual ~CdclTheory() = default;
    /** @brief Check a partial Boolean assignment for theory consistency. */
    virtual CdclTheoryFeedback check_partial(
        CdclAssignment const& assignment) = 0;
};

} // namespace Ariadne
#endif // ARIADNE_CDCL_THEORY_HPP
