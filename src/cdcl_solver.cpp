/***************************************************************************
 *            cdcl/cdcl_solver.cpp
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

/*! \file cdcl/cdcl_solver.cpp
 *  \brief Implementation of conflict-driven Boolean search.
 */

#include "cdcl/cdcl_solver.hpp"

#include <stdexcept>
#include <utility>

namespace Ariadne {

CdclSolver::CdclSolver(
    SizeType variable_count,
    std::vector<BooleanClause> const& original_clauses,
    SizeType learned_clause_limit)
    : CdclState(variable_count,original_clauses,learned_clause_limit)
{ } // CdclState validates every original literal before construction returns.

CdclAssignment CdclSolver::assignment() const
{
    CdclAssignment result(_assignment.size());
    for(SizeType index=0u;index<_assignment.size();++index) {
        if(_assignment[index].value>=0) {
            result[index]=(_assignment[index].value==1);
        }
    }
    return result;
}

CdclSolveStatus CdclSolver::solve(
    CdclTheory* theory, CdclCompletionCallback const& complete)
{
    if(_already_solved) {
        throw std::logic_error("CdclSolver supports one solve per instance");
    }
    _already_solved=true;
    _theory=theory;
    _complete=complete;
    _encountered_unknown=false;

    SearchOutcome outcome=this->_search_boolean();
    if(outcome.found) {
        return CdclSolveStatus::SAT;
    }
    return _encountered_unknown ? CdclSolveStatus::UNKNOWN
                                : CdclSolveStatus::UNSAT;
}

CdclSolver::SearchOutcome CdclSolver::_handle_conflict(
    SizeType clause_index, Bool theory_conflict)
{
    if(this->_decision_level()==0u) {
        if(not theory_conflict) {
            _last_boolean_conflict_clause.reset();
        }
        return SearchOutcome::exhausted();
    }

    SizeType conflict_level=this->_decision_level();
    ConflictAnalysis analysis=this->_analyze_boolean_conflict(clause_index);
    ++_cdcl_statistics.boolean_conflicts_analyzed;
    _cdcl_statistics.learned_clause_literals+=analysis.learned_clause.size();
    _cdcl_statistics.last_learned_clause_literals=analysis.learned_clause.size();
    _cdcl_statistics.last_learned_current_level_literals=
        this->_current_level_literal_count(analysis.learned_clause);
    _cdcl_statistics.last_backjump_level=analysis.backjump_level;

    SizeType learned_index=this->_add_learned_clause(analysis.learned_clause);
    if(not theory_conflict) {
        _last_boolean_conflict_clause.reset();
    }
    if(analysis.backjump_level+1u<conflict_level) {
        ++_cdcl_statistics.nonchronological_backjumps;
    }
    this->_backtrack_to_level(analysis.backjump_level);
    ++_cdcl_statistics.boolean_backtracks;
    this->_maybe_prune_learned_clauses(learned_index);
    return SearchOutcome::backjump(analysis.backjump_level);
}

std::optional<CdclSolver::SearchOutcome> CdclSolver::_propagate_child_outcome(
    SearchOutcome const& outcome,SizeType parent_level)
{
    if(outcome.found) {
        return outcome;
    }
    if(outcome.backjump_level.has_value()) {
        if(*outcome.backjump_level<parent_level) {
            return outcome;
        }
        return this->_search_boolean();
    }
    return std::nullopt;
}

CdclSolver::SearchOutcome CdclSolver::_search_boolean()
{
    if(not this->_unit_propagate()) {
        return this->_handle_conflict(*_last_boolean_conflict_clause,false);
    }

    SizeType variable=this->_next_unassigned_variable();
    if(_theory!=nullptr) {
        CdclAssignment current=this->assignment();
        CdclTheoryFeedback feedback=_theory->check_partial(current);
        validate_cdcl_theory_feedback(feedback,current);
        if(feedback.status()==CdclTheoryStatus::CONFLICT) {
            SizeType index=this->_add_learned_clause(feedback.explanation(),true);
            return this->_handle_conflict(index,true);
        }
        if(feedback.status()==CdclTheoryStatus::UNKNOWN) {
            _encountered_unknown=true;
        }
    }

    if(variable==_variable_count) {
        CdclCompletionStatus complete=_complete
            ? _complete(this->assignment())
            : CdclCompletionStatus::CERTIFIED;
        if(complete==CdclCompletionStatus::CERTIFIED) {
            return SearchOutcome::satisfied();
        }
        if(complete==CdclCompletionStatus::UNKNOWN) {
            _encountered_unknown=true;
        }
        return SearchOutcome::exhausted();
    }

    ++_cdcl_statistics.boolean_decisions;
    SizeType parent_level=this->_decision_level();

    this->_push_decision_level();
    this->_assign_literal(BooleanLiteral(variable,false));
    SearchOutcome first=this->_search_boolean();
    if(auto outcome=this->_propagate_child_outcome(first,parent_level);
       outcome.has_value()) {
        return *outcome;
    }

    this->_backtrack_to_level(parent_level);
    ++_cdcl_statistics.boolean_backtracks;
    this->_push_decision_level();
    this->_assign_literal(BooleanLiteral(variable));
    SearchOutcome second=this->_search_boolean();
    if(auto outcome=this->_propagate_child_outcome(second,parent_level);
       outcome.has_value()) {
        return *outcome;
    }

    this->_backtrack_to_level(parent_level);
    ++_cdcl_statistics.boolean_backtracks;
    return SearchOutcome::exhausted();
}

} // namespace Ariadne
