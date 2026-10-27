/***************************************************************************
 *            cdcl_state.cpp
 *
 *  Copyright 2026 Luca Geretti
 ****************************************************************************/

/*
 *  This file is part of the Ariadne CDCL component.
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdcl/cdcl_state.hpp"

#include <algorithm>

namespace Ariadne {

CdclState::CdclState(SizeType variable_count,
          std::vector<std::vector<BooleanLiteral>> const& original_clauses,
          SizeType learned_clause_limit)
    : _variable_count(variable_count),
      _original_clauses(original_clauses),
      _learned_clause_limit(learned_clause_limit),
      _assignment(variable_count)
{
    _trail.reserve(variable_count);
    _decision_level_markers.reserve(variable_count+1u);
    _decision_level_markers.push_back(0u);
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

Void CdclState::_maybe_prune_learned_clauses(SizeType protected_clause)
{
    SizeType active=this->_active_non_theory_learned_clause_count();
    if(active<=_learned_clause_limit) {
        return;
    }

    ++_cdcl_statistics.learned_clause_pruning_runs;
    // Maintain candidate selection but do not deactivate learned clauses:
    // removal must wait for a progress-preserving reduction algorithm.
    struct Candidate {
        SizeType index;
        SizeType activity;
        SizeType size;
    };
    std::vector<Candidate> candidates;
    for(SizeType i=0u; i<_learned_clauses.size(); ++i) {
        SizeType clause_index=this->_original_clause_count()+i;
        Bool const recent=_cdcl_statistics.learned_clauses<=
            _learned_clause_generation[i]+2u;
        Bool const excluded=_learned_clause_is_theory[i] or recent
            or _learned_clauses[i].size()<=2u
            or _learned_clause_activity[i]>1u
            or clause_index==protected_clause
            or this->_learned_clause_locked(clause_index);
        if(not excluded) {
            candidates.push_back({
                clause_index,_learned_clause_activity[i],_learned_clauses[i].size()
            });
        }
    }
    std::stable_sort(candidates.begin(),candidates.end(),
        [](Candidate const& lhs,Candidate const& rhs) {
            if(lhs.activity!=rhs.activity) {
                return lhs.activity<rhs.activity;
            }
            return lhs.size>rhs.size;
        });
    (void)candidates;
}

Bool CdclState::_unit_propagate()
{
    _last_boolean_conflict_clause.reset();
    Bool changed=true;
    while(changed) {
        changed=false;
        for(SizeType clause_index=0u; clause_index<this->_clause_count(); ++clause_index) {
            auto const& clause=this->_clause(clause_index);
            Bool satisfied=false;
            SizeType unassigned_count=0u;
            std::optional<BooleanLiteral> unit_literal;

            for(BooleanLiteral literal:clause) {
                SizeType variable=literal.variable();
                int8_t value=_assignment[variable].value;
                if(value<0) {
                    ++unassigned_count;
                    unit_literal=literal;
                } else if(this->_literal_true(literal)) {
                    satisfied=true;
                    break;
                }
            }

            if(satisfied) {
                continue;
            }

            if(unassigned_count==0u) {
                ++_cdcl_statistics.boolean_conflicts;
                this->_bump_learned_clause_activity(clause_index);
                _last_boolean_conflict_clause=clause_index;
                return false;
            }

            if(unassigned_count==1u) {
                this->_assign_literal(*unit_literal,clause_index);
                ++_cdcl_statistics.boolean_propagations;
                ++_cdcl_statistics.boolean_reasoned_propagations;
                if(this->_is_learned_clause(clause_index)) {
                    this->_bump_learned_clause_activity(clause_index);
                    ++_cdcl_statistics.learned_clause_propagations;
                    if(this->_is_theory_learned_clause(clause_index)) {
                        ++_cdcl_statistics.theory_learned_clause_propagations;
                    }
                }
                changed=true;
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
}


} // namespace Ariadne
