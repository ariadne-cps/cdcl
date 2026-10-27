/***************************************************************************
 *            cdcl_solver.hpp
 *
 *  Copyright 2026 Luca Geretti
 *  SPDX-License-Identifier: GPL-3.0-or-later
 ****************************************************************************/
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
enum class CdclCompletionStatus { CERTIFIED, REJECTED, UNKNOWN };
enum class CdclSolveStatus { SAT, UNSAT, UNKNOWN };

using CdclCompletionCallback=
    std::function<CdclCompletionStatus(CdclAssignment const&)>;

//! Complete DPLL-style Boolean search with 1-UIP learning and backjumping.
//! The optional theory oracle may return only sound explanations for CONFLICT.
//! Without a theory oracle or completion callback this solves pure Boolean CNF.
class CdclSolver : protected CdclState {
  public:
    CdclSolver(SizeType variable_count,
               std::vector<BooleanClause> const& original_clauses,
               SizeType learned_clause_limit = std::numeric_limits<SizeType>::max());

    CdclSolveStatus solve(
        CdclTheory* theory = nullptr,
        CdclCompletionCallback const& complete = {});

    CdclAssignment assignment() const;
    CdclStatistics const& statistics() const { return _cdcl_statistics; }

  protected:
    // Provided for theory adapters requiring trail and decision-level order.
    using CdclState::AssignmentInfo;
    using CdclState::ConflictAnalysis;
    using CdclState::_assignment;
    using CdclState::_trail;
    using CdclState::_cdcl_statistics;

  private:
    struct SearchOutcome {
        Bool found = false;
        std::optional<SizeType> backjump_level;
        static SearchOutcome exhausted() { return {}; }
        static SearchOutcome satisfied() { return {true,std::nullopt}; }
        static SearchOutcome backjump(SizeType level) { return {false,level}; }
    };

    SearchOutcome _search_boolean();
    SearchOutcome _handle_conflict(SizeType clause_index, Bool theory_conflict);
    std::optional<SearchOutcome> _propagate_child_outcome(
        SearchOutcome const& outcome, SizeType parent_level);

    CdclTheory* _theory = nullptr;
    CdclCompletionCallback _complete;
    Bool _encountered_unknown = false;
    Bool _already_solved = false;
};

} // namespace Ariadne
#endif // ARIADNE_CDCL_SOLVER_HPP
