/***************************************************************************
 *            cdcl/cdcl_state.cpp
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

/*! \file cdcl/cdcl_state.cpp
 *  \brief CDCL propagation, clause learning and backtracking implementation.
 */

#include "cdcl/cdcl_state.hpp"

#include <algorithm>
#include <stdexcept>

namespace Ariadne {

CdclState::CdclState(SizeType variable_count,
          std::vector<std::vector<BooleanLiteral>> const& original_clauses,
          SizeType learned_clause_limit)
    : _variable_count(variable_count),
      _original_clauses(original_clauses),
      _learned_clause_limit(learned_clause_limit),
      _assignment(variable_count),
      _watch_lists(variable_count*2u)
{
    _trail.reserve(variable_count);
    _decision_level_markers.reserve(variable_count+1u);
    _decision_level_markers.push_back(0u);
    _watched_positions.reserve(original_clauses.size());
    _pending_clauses.reserve(original_clauses.size());
    for(SizeType index=0u;index<original_clauses.size();++index) {
        this->_register_clause_watchers(index);
    }
}

Bool CdclState::_literal_true(BooleanLiteral literal) const
{
    SizeType variable=literal.variable();
    int8_t value=_assignment[variable].value;
    int8_t expected_value=static_cast<int8_t>(literal.positive());
    return value==expected_value;
}

Void CdclState::_assign_literal(BooleanLiteral literal, std::optional<SizeType> reason_clause)
{
    SizeType variable=literal.variable();
    int8_t value=static_cast<int8_t>(literal.positive());
    AssignmentInfo& assignment=_assignment[variable];
    assignment.value=value;
    assignment.decision_level=this->_decision_level();
    assignment.reason_clause=reason_clause;
    _trail.push_back(variable);
}

SizeType CdclState::_original_clause_count() const
{
    return _original_clauses.size();
}

SizeType CdclState::_clause_count() const
{
    return this->_original_clause_count()+_learned_clauses.size();
}

std::vector<BooleanLiteral> const& CdclState::_clause(SizeType index) const
{
    if(index<this->_original_clause_count()) {
        return _original_clauses[index];
    }
    return _learned_clauses[index-this->_original_clause_count()];
}

Bool CdclState::_is_learned_clause(SizeType index) const
{
    return index>=this->_original_clause_count();
}

SizeType CdclState::_add_learned_clause(std::vector<BooleanLiteral> const& clause, Bool theory_clause)
{
    _learned_clauses.emplace_back(clause.begin(),clause.end());
    this->_register_clause_watchers(this->_clause_count()-1u);
    _learned_clause_is_theory.push_back(theory_clause);
    _learned_clause_activity.push_back(1u);
    _learned_clause_generation.push_back(_cdcl_statistics.learned_clauses);
    ++_cdcl_statistics.learned_clauses;
    if(theory_clause) {
        ++_cdcl_statistics.theory_learned_clauses;
        _cdcl_statistics.theory_learned_clause_literals+=clause.size();
    } else {
        _cdcl_statistics.peak_active_non_theory_learned_clauses=std::max(
            _cdcl_statistics.peak_active_non_theory_learned_clauses,
            this->_active_non_theory_learned_clause_count());
    }
    return this->_original_clause_count()+_learned_clauses.size()-1u;
}

Bool CdclState::_is_theory_learned_clause(SizeType index) const
{
    return this->_is_learned_clause(index)
        and _learned_clause_is_theory[index-this->_original_clause_count()];
}

SizeType CdclState::_active_non_theory_learned_clause_count() const
{
    SizeType count=0u;
    for(SizeType i=0u; i<_learned_clauses.size(); ++i) {
        if(not _learned_clause_is_theory[i]) {
            ++count;
        }
    }
    return count;
}

Bool CdclState::_learned_clause_locked(SizeType index) const
{
    for(AssignmentInfo const& assignment:_assignment) {
        if(assignment.value>=0
                and assignment.reason_clause==std::optional<SizeType>(index)) {
            return true;
        }
    }
    return false;
}

Void CdclState::_bump_learned_clause_activity(SizeType index)
{
    if(not this->_is_learned_clause(index)) {
        return;
    }
    SizeType learned_index=index-this->_original_clause_count();
    ++_learned_clause_activity[learned_index];
    ++_cdcl_statistics.learned_clause_activity_bumps;
}

Void CdclState::_maybe_prune_learned_clauses(SizeType /* protected_clause */)
{
    SizeType active=this->_active_non_theory_learned_clause_count();
    if(active<=_learned_clause_limit) {
        return;
    }

    ++_cdcl_statistics.learned_clause_pruning_runs;
    // Reduction is deferred until a progress-preserving algorithm exists.
    // Selecting and sorting candidates had no effect: the local vector was
    // discarded. Retain every clause, its watches and its propagation reasons.
}

SizeType CdclState::_literal_watch_index(BooleanLiteral literal) const
{
    return 2u*literal.variable()+(literal.positive() ? 0u : 1u);
}

Void CdclState::_register_clause_watchers(SizeType clause_index)
{
    auto const& clause=this->_clause(clause_index);
    for(BooleanLiteral literal:clause) {
        if(literal.variable()>=_variable_count) {
            throw std::out_of_range("CDCL clause uses an undeclared Boolean variable");
        }
    }

    // Install watches only when processing the pending clause. A learned
    // clause can be conflicting at insertion and become unit after a
    // nonchronological backjump; watches must reflect the latter trail.
    _watched_positions.push_back({0u,0u});
    _pending_clauses.push_back(clause_index);
}

Void CdclState::_assign_propagated_literal(BooleanLiteral literal,SizeType clause_index)
{
    this->_assign_literal(literal,clause_index);
    ++_cdcl_statistics.boolean_propagations;
    ++_cdcl_statistics.boolean_reasoned_propagations;
    if(this->_is_learned_clause(clause_index)) {
        this->_bump_learned_clause_activity(clause_index);
        ++_cdcl_statistics.learned_clause_propagations;
        if(this->_is_theory_learned_clause(clause_index)) {
            ++_cdcl_statistics.theory_learned_clause_propagations;
        }
    }
}

Bool CdclState::_report_boolean_conflict(SizeType clause_index)
{
    ++_cdcl_statistics.boolean_conflicts;
    this->_bump_learned_clause_activity(clause_index);
    _last_boolean_conflict_clause=clause_index;
    return false;
}

Bool CdclState::_process_pending_clause(SizeType clause_index)
{
    auto const& clause=this->_clause(clause_index);
    Bool satisfied=false;
    std::optional<BooleanLiteral> unit;
    Bool only_one_distinct_unassigned=true;

    for(BooleanLiteral literal:clause) {
        if(this->_literal_true(literal)) {
            satisfied=true;
        } else if(_assignment[literal.variable()].value<0) {
            if(not unit.has_value()) {
                unit=literal;
            } else if(literal!=*unit) {
                only_one_distinct_unassigned=false;
            }
        }
    }
    if(not satisfied and not unit.has_value()) {
        // Leave this clause pending so it can be registered and propagated
        // against the retained trail after conflict analysis and backjump.
        return this->_report_boolean_conflict(clause_index);
    }

    if(clause.size()>=2u) {
        // Choose two different literal positions, preferring non-false
        // literals. This is intentionally delayed until after backjump.
        std::array<SizeType,2> watched{0u,1u};
        SizeType nonfalse=0u;
        for(SizeType i=0u;i<clause.size() and nonfalse<2u;++i) {
            BooleanLiteral literal=clause[i];
            if(_assignment[literal.variable()].value<0 or this->_literal_true(literal)) {
                watched[nonfalse++]=i;
            }
        }
        // The conflict guard above ensures at least one non-false literal:
        // either a satisfying literal or an unassigned one was found.
        if(nonfalse==1u) {
            watched[1u]=(watched[0u]==0u ? 1u : 0u);
        }
        _watched_positions[clause_index]=watched;
        _watch_lists[this->_literal_watch_index(clause[watched[0u]])].push_back(clause_index);
        _watch_lists[this->_literal_watch_index(clause[watched[1u]])].push_back(clause_index);
    }

    if(not satisfied and only_one_distinct_unassigned) {
        this->_assign_propagated_literal(*unit,clause_index);
    }
    return true;
}

Bool CdclState::_unit_propagate()
{
    _last_boolean_conflict_clause.reset();

    while(_pending_head<_pending_clauses.size()) {
        SizeType index=_pending_clauses[_pending_head];
        if(not this->_process_pending_clause(index)) {
            return false;
        }
        ++_pending_head;
    }
    _pending_clauses.clear();
    _pending_head=0u;

    while(_propagation_head<_trail.size()) {
        SizeType variable=_trail[_propagation_head++];
        BooleanLiteral falsified(variable,_assignment[variable].value==0);
        SizeType watch_index=this->_literal_watch_index(falsified);
        auto& watchlist=_watch_lists[watch_index];

        SizeType position=0u;
        while(position<watchlist.size()) {
            SizeType index=watchlist[position];
            auto const& clause=this->_clause(index);
            auto& watched=_watched_positions[index];
            SizeType slot=(clause[watched[0u]]==falsified) ? 0u : 1u;
            BooleanLiteral other_literal=clause[watched[1u-slot]];

            if(this->_literal_true(other_literal)) {
                ++position;
                continue;
            }

            Bool moved=false;
            for(SizeType i=0u;i<clause.size();++i) {
                if(i==watched[0u] or i==watched[1u]) continue;
                BooleanLiteral candidate=clause[i];
                if(_assignment[candidate.variable()].value<0
                    or this->_literal_true(candidate)) {
                    watched[slot]=i;
                    _watch_lists[this->_literal_watch_index(candidate)].push_back(index);
                    watchlist[position]=watchlist.back();
                    watchlist.pop_back();
                    moved=true;
                    break;
                }
            }
            if(moved) continue;

            if(_assignment[other_literal.variable()].value<0) {
                this->_assign_propagated_literal(other_literal,index);
                ++position;
            } else {
                return this->_report_boolean_conflict(index);
            }
        }
    }
    return true;
}

Bool CdclState::_clause_contains_variable(std::vector<BooleanLiteral> const& clause, SizeType variable) const
{
    for(BooleanLiteral literal:clause) {
        SizeType literal_variable=literal.variable();
        if(literal_variable==variable) {
            return true;
        }
    }
    return false;
}

SizeType CdclState::_current_level_literal_count(std::vector<BooleanLiteral> const& clause) const
{
    SizeType count=0u;
    for(BooleanLiteral literal:clause) {
        SizeType variable=literal.variable();
        if(_assignment[variable].decision_level==this->_decision_level()) {
            ++count;
        }
    }
    return count;
}

std::vector<BooleanLiteral> CdclState::_resolve_on_variable(
    std::vector<BooleanLiteral> const& lhs,
    std::vector<BooleanLiteral> const& rhs,
    SizeType variable) const
{
    std::vector<BooleanLiteral> resolved;
    resolved.reserve(lhs.size()+rhs.size());
    auto append_unique=[&](BooleanLiteral literal) {
        if(literal.variable()==variable) {
            return;
        }
        if(std::find(resolved.begin(),resolved.end(),literal)==resolved.end()) {
            resolved.push_back(literal);
        }
    };
    for(BooleanLiteral literal:lhs) { append_unique(literal); }
    for(BooleanLiteral literal:rhs) { append_unique(literal); }
    return resolved;
}

CdclState::ConflictAnalysis CdclState::_analyze_boolean_conflict(SizeType conflict_clause_index)
{
    this->_bump_learned_clause_activity(conflict_clause_index);
    ConflictAnalysis analysis;
    auto const& conflict_clause=this->_clause(conflict_clause_index);
    analysis.learned_clause.assign(conflict_clause.begin(),conflict_clause.end());

    while(this->_current_level_literal_count(analysis.learned_clause)>1u) {
        auto iter=_trail.rbegin();
        for(;;) {
            SizeType pivot_variable=*iter;
            AssignmentInfo const& assignment=_assignment[pivot_variable];
            Bool const current_level=
                assignment.decision_level==this->_decision_level();
            Bool const has_reason=assignment.reason_clause.has_value();
            Bool const appears_in_clause=this->_clause_contains_variable(
                analysis.learned_clause,pivot_variable);
            Bool const resolvable=
                static_cast<unsigned>(current_level)
                & static_cast<unsigned>(has_reason)
                & static_cast<unsigned>(appears_in_clause);
            if(resolvable) {
                SizeType reason_index=*assignment.reason_clause;
                this->_bump_learned_clause_activity(reason_index);
                analysis.learned_clause=this->_resolve_on_variable(
                    analysis.learned_clause,
                    this->_clause(reason_index),
                    pivot_variable);
                break;
            }
            ++iter;
        }
    }

    SizeType current_level=this->_decision_level();
    SizeType backjump_level=0u;
    for(BooleanLiteral literal:analysis.learned_clause) {
        SizeType variable=literal.variable();
        SizeType level=_assignment[variable].decision_level;
        if(level!=current_level) {
            backjump_level=std::max(backjump_level,level);
        }
    }
    analysis.backjump_level=backjump_level;
    return analysis;
}

SizeType CdclState::_next_unassigned_variable() const
{
    for(SizeType variable=0u; variable<_variable_count; ++variable) {
        if(_assignment[variable].value<0) {
            return variable;
        }
    }
    return _variable_count;
}

SizeType CdclState::_decision_level() const
{
    return _decision_level_markers.size()-1u;
}

Void CdclState::_push_decision_level()
{
    _decision_level_markers.push_back(_trail.size());
    _cdcl_statistics.max_decision_level=std::max(
        _cdcl_statistics.max_decision_level,this->_decision_level());
}

Void CdclState::_backtrack_to_level(SizeType level)
{
    SizeType current_level=this->_decision_level();
    if(level>current_level) {
        throw std::out_of_range("Cannot backtrack to a future CDCL decision level");
    }
    if(level==current_level) {
        return;
    }

    // A marker at index k denotes the start of level k, not its end.
    // To preserve every assignment through 'level', use the start of
    // level+1. In particular, root-level unit propagations survive a
    // backtrack to level zero.
    SizeType marker=_decision_level_markers[level+1u];
    while(_trail.size()>marker) {
        SizeType variable=_trail.back();
        _trail.pop_back();
        _assignment[variable]=AssignmentInfo();
    }
    _decision_level_markers.resize(level+1u);
    if(_propagation_head>_trail.size()) {
        _propagation_head=_trail.size();
    }
}


} // namespace Ariadne
