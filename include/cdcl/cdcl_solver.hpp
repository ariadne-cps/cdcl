/***************************************************************************
 *            cdcl/cdcl_solver.hpp
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

/** @file cdcl/cdcl_solver.hpp
 *  @brief CDCL solver search and theory integration interface.
 */

#ifndef ARIADNE_CDCL_SOLVER_HPP
#define ARIADNE_CDCL_SOLVER_HPP

#include <functional>
#include <optional>
#include <vector>

#include "cdcl/cdcl_state.hpp"
#include "cdcl/cdcl_theory.hpp"

namespace Ariadne {

//! The complete-assignment callback must certify a model to return CERTIFIED.
//! REJECTED means proven infeasible; UNKNOWN means inconclusive.
enum class CdclCompletionStatus { CERTIFIED,
                                  REJECTED,
                                  UNKNOWN };
/** @brief Distinguish a certified satisfying assignment, unsatisfiability and inconclusive search. */
enum class CdclSolveStatus { SAT,
                             UNSAT,
                             UNKNOWN };

/** @brief Callback that validates whether a complete assignment is a model. */
using CdclCompletionCallback =
    std::function<CdclCompletionStatus(const CdclAssignment&)>;

//! Complete DPLL-style Boolean search with 1-UIP learning and backjumping.
//! The optional theory oracle may return only sound explanations for CONFLICT.
//! Without a theory oracle or completion callback this solves pure Boolean CNF.
class CdclSolver : protected CdclState {
  public:
    /** @brief Initialize a solver from a fixed Boolean formula and a learned-clause limit. */
    CdclSolver(SizeType variable_count,
               const std::vector<BooleanClause>& original_clauses,
               SizeType learned_clause_limit = std::numeric_limits<SizeType>::max());

    /** @brief Search for a model using optional theory consistency and completion checks. */
    CdclSolveStatus solve(
        CdclTheory* theory = nullptr,
        const CdclCompletionCallback& complete = {});

    /** @brief Return the current Boolean assignment after search. */
    CdclAssignment assignment() const;
    /** @brief Return counters collected during the solver's search. */
    const CdclStatistics& statistics() const { return _cdcl_statistics; }

  protected:
    // Provided for theory adapters requiring trail and decision-level order.
    using CdclState::_assignment;
    using CdclState::_cdcl_statistics;
    using CdclState::_trail;
    using CdclState::AssignmentInfo;
    using CdclState::ConflictAnalysis;

  private:
    /** @brief Describe whether recursive search succeeded or requires backjumping. */
    struct SearchOutcome {
        Bool found = false;
        std::optional<SizeType> backjump_level;
        /** @brief Represent a branch with no satisfying model. */
        static SearchOutcome exhausted() { return {}; }
        /** @brief Represent a branch containing a satisfying assignment. */
        static SearchOutcome satisfied() { return {true, std::nullopt}; }
        /** @brief Request a backjump to an earlier decision level. */
        static SearchOutcome backjump(SizeType level) { return {false, level}; }
    };

    /** @brief Search Boolean assignments with learning and backtracking. */
    SearchOutcome _search_boolean();
    /** @brief Analyze a Boolean or theory conflict and determine the backjump. */
    SearchOutcome _handle_conflict(SizeType clause_index, Bool theory_conflict);
    /** @brief Propagate a child search outcome toward its parent decision level. */
    std::optional<SearchOutcome> _propagate_child_outcome(
        const SearchOutcome& outcome, SizeType parent_level);

    CdclTheory* _theory = nullptr;
    CdclCompletionCallback _complete;
    Bool _encountered_unknown = false;
    Bool _already_solved = false;
};

} // namespace Ariadne
#endif // ARIADNE_CDCL_SOLVER_HPP
