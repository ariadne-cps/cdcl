/***************************************************************************
 *            cdcl/cdcl_state.hpp
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

/*! \file cdcl/cdcl_state.hpp
 *  \brief Propositional implication trail, clauses and conflict analysis.
 */

#ifndef ARIADNE_CDCL_STATE_HPP
#define ARIADNE_CDCL_STATE_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "cdcl/boolean_literal.hpp"

namespace Ariadne {

struct CdclStatistics {
    SizeType learned_clauses = 0u;
    SizeType theory_learned_clauses = 0u;
    SizeType theory_learned_clause_literals = 0u;
    SizeType peak_active_non_theory_learned_clauses = 0u;
    SizeType learned_clause_activity_bumps = 0u;
    SizeType learned_clause_pruning_runs = 0u;
    SizeType learned_clauses_pruned = 0u;
    SizeType boolean_conflicts = 0u;
    SizeType boolean_propagations = 0u;
    SizeType boolean_reasoned_propagations = 0u;
    SizeType learned_clause_propagations = 0u;
    SizeType theory_learned_clause_propagations = 0u;
    SizeType max_decision_level = 0u;
    SizeType boolean_conflicts_analyzed = 0u;
    SizeType learned_clause_literals = 0u;
    SizeType last_learned_clause_literals = 0u;
    SizeType last_learned_current_level_literals = 0u;
    SizeType last_backjump_level = 0u;
    SizeType nonchronological_backjumps = 0u;
    SizeType boolean_backtracks = 0u;
    SizeType boolean_decisions = 0u;
};

//! \brief Propositional CDCL trail, clauses, unit propagation and 1-UIP analysis.
//! \details Theory-free state: owns only variables, clauses and CDCL statistics.
//! The caller supplies the formula and interprets theory checks separately.
class CdclState {
  protected:
    CdclState(SizeType variable_count,
              std::vector<std::vector<BooleanLiteral>> const& original_clauses,
              SizeType learned_clause_limit);

    struct AssignmentInfo {
        int8_t value = -1;
        SizeType decision_level = 0u;
        std::optional<SizeType> reason_clause;
    };

    struct ConflictAnalysis {
        std::vector<BooleanLiteral> learned_clause;
        SizeType backjump_level = 0u;
    };

    Bool _literal_true(BooleanLiteral literal) const;

    Void _assign_literal(BooleanLiteral literal, std::optional<SizeType> reason_clause = std::nullopt);

    SizeType _original_clause_count() const;

    SizeType _clause_count() const;

    std::vector<BooleanLiteral> const& _clause(SizeType index) const;

    Bool _is_learned_clause(SizeType index) const;

    SizeType _add_learned_clause(std::vector<BooleanLiteral> const& clause, Bool theory_clause = false);

    Bool _is_theory_learned_clause(SizeType index) const;

    SizeType _active_non_theory_learned_clause_count() const;

    Bool _learned_clause_locked(SizeType index) const;

    Void _bump_learned_clause_activity(SizeType index);

    Void _maybe_prune_learned_clauses(SizeType protected_clause);

    Bool _unit_propagate();

    // Each non-unit clause watches two literal positions. The watch lists
    // store stable original/learned clause indices (never clause pointers).
    SizeType _literal_watch_index(BooleanLiteral literal) const;
    Void _register_clause_watchers(SizeType clause_index);
    Bool _process_pending_clause(SizeType clause_index);
    Void _assign_propagated_literal(BooleanLiteral literal,SizeType clause_index);
    Bool _report_boolean_conflict(SizeType clause_index);

    Bool _clause_contains_variable(std::vector<BooleanLiteral> const& clause, SizeType variable) const;

    SizeType _current_level_literal_count(std::vector<BooleanLiteral> const& clause) const;

    std::vector<BooleanLiteral> _resolve_on_variable(
        std::vector<BooleanLiteral> const& lhs,
        std::vector<BooleanLiteral> const& rhs,
        SizeType variable) const;

    ConflictAnalysis _analyze_boolean_conflict(SizeType conflict_clause_index);

    SizeType _next_unassigned_variable() const;

    SizeType _decision_level() const;

    Void _push_decision_level();

    Void _backtrack_to_level(SizeType level);


    SizeType _variable_count;
    std::vector<std::vector<BooleanLiteral>> const& _original_clauses;
    SizeType _learned_clause_limit;
    std::vector<AssignmentInfo> _assignment;
    std::vector<SizeType> _trail;
    std::vector<SizeType> _decision_level_markers;
    std::vector<std::vector<BooleanLiteral>> _learned_clauses;
    std::vector<Bool> _learned_clause_is_theory;
    std::vector<SizeType> _learned_clause_activity;
    std::vector<SizeType> _learned_clause_generation;
    std::optional<SizeType> _last_boolean_conflict_clause;
    std::vector<std::array<SizeType,2>> _watched_positions;
    std::vector<std::vector<SizeType>> _watch_lists;
    std::vector<SizeType> _pending_clauses;
    SizeType _pending_head = 0u;
    SizeType _propagation_head = 0u;
    CdclStatistics _cdcl_statistics;
};

} // namespace Ariadne

#endif // ARIADNE_CDCL_STATE_HPP
