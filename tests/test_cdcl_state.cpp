/***************************************************************************
 *            test_cdcl_state.cpp
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

/*! \file test_cdcl_state.cpp
 *  \brief Ariadne CDCL regression tests.
 */

#include <stdexcept>
#include <vector>
#include "cdcl/cdcl_state.hpp"

using namespace Ariadne;

namespace {
Void require(Bool condition) {
    if(not condition) {
        throw std::runtime_error("CDCL core test failed");
    }
}
using Clause=std::vector<BooleanLiteral>;

class CoreProbe : public CdclState {
  public:
    CoreProbe(SizeType variables, std::vector<Clause> const& clauses)
        : CdclState(variables,clauses,0u) { }
    using CdclState::_assign_literal;
    using CdclState::_unit_propagate;
    using CdclState::_decision_level;
    using CdclState::_push_decision_level;
    using CdclState::_backtrack_to_level;
    using CdclState::_add_learned_clause;
    using CdclState::_analyze_boolean_conflict;
    using CdclState::_next_unassigned_variable;
    using CdclState::_maybe_prune_learned_clauses;
    using CdclState::_cdcl_statistics;
    using CdclState::_assignment;
    using CdclState::_last_boolean_conflict_clause;
    using CdclState::_watch_lists;
    using CdclState::_watched_positions;
    using CdclState::_propagation_head;
};
}

int main() {
    // Unit propagation with zero-based logical variables.
    std::vector<Clause> clauses{
        {BooleanLiteral(0u)},
        {BooleanLiteral(0u,false),BooleanLiteral(1u)}
    };
    CoreProbe core(2u,clauses);
    require(core._unit_propagate());
    require(core._assignment[0u].value==1);
    require(core._assignment[1u].value==1);
    require(core._next_unassigned_variable()==2u);
    require(core._cdcl_statistics.boolean_propagations==2u);

    // The original clauses are untouched by learned-clause storage.
    const SizeType learned=core._add_learned_clause({
        BooleanLiteral(1u,false)
    });
    require(learned==2u);
    require(not core._unit_propagate());
    require(core._last_boolean_conflict_clause==learned);
    core._maybe_prune_learned_clauses(learned);

    // Backtracking restores only assignments made at the later level.
    std::vector<Clause> no_clauses;
    CoreProbe decisions(2u,no_clauses);
    decisions._push_decision_level();
    decisions._assign_literal(BooleanLiteral(0u));
    require(decisions._decision_level()==1u);
    require(decisions._next_unassigned_variable()==1u);
    decisions._backtrack_to_level(0u);
    require(decisions._decision_level()==0u);
    require(decisions._next_unassigned_variable()==0u);

    // An inner backtrack must not discard decisions at its parent level,
    // and a backtrack to root must retain root-level unit propagations.
    std::vector<Clause> root_clauses{{BooleanLiteral(0u)}};
    CoreProbe nested(3u,root_clauses);
    require(nested._unit_propagate());
    require(nested._assignment[0u].value==1);
    nested._push_decision_level();
    nested._assign_literal(BooleanLiteral(1u));
    nested._push_decision_level();
    nested._assign_literal(BooleanLiteral(2u,false));
    require(nested._decision_level()==2u);
    nested._backtrack_to_level(1u);
    require(nested._decision_level()==1u);
    require(nested._assignment[0u].value==1);
    require(nested._assignment[1u].value==1);
    require(nested._assignment[2u].value<0);
    require(nested._next_unassigned_variable()==2u);
    nested._backtrack_to_level(1u);
    require(nested._assignment[1u].value==1);
    nested._backtrack_to_level(0u);
    require(nested._decision_level()==0u);
    require(nested._assignment[0u].value==1);
    require(nested._assignment[1u].value<0);
    require(nested._next_unassigned_variable()==1u);
    require(nested._unit_propagate());
    require(nested._cdcl_statistics.boolean_propagations==1u);

    Bool rejected_future_level=false;
    try { nested._backtrack_to_level(1u); }
    catch(std::out_of_range const&) { rejected_future_level=true; }
    require(rejected_future_level);

    // Watch movement: a falsified watch moves to a third literal.
    // A second false decision then forces that third literal.
    std::vector<Clause> ternary{{
        BooleanLiteral(0u),BooleanLiteral(1u),BooleanLiteral(2u)}};
    CoreProbe watched(3u,ternary);
    require(watched._unit_propagate());
    watched._push_decision_level();
    watched._assign_literal(BooleanLiteral(0u,false));
    require(watched._unit_propagate());
    require(watched._watched_positions[0u][0u]==2u);
    watched._assign_literal(BooleanLiteral(1u,false));
    require(watched._unit_propagate());
    require(watched._assignment[2u].value==1);
    require(watched._assignment[2u].reason_clause==std::optional<SizeType>(0u));
    require(watched._cdcl_statistics.boolean_propagations==1u);
    require(watched._propagation_head==3u);

    // After a backjump, a newly learned clause must be inspected even
    // when no retained assignment generates a fresh watch event.
    std::vector<Clause> fixed_root{{BooleanLiteral(0u)}};
    CoreProbe learning(2u,fixed_root);
    require(learning._unit_propagate());
    learning._push_decision_level();
    learning._assign_literal(BooleanLiteral(1u,false));
    const SizeType theory_clause=learning._add_learned_clause({
        BooleanLiteral(0u,false),BooleanLiteral(1u)},true);
    require(not learning._unit_propagate());
    require(learning._last_boolean_conflict_clause==theory_clause);
    learning._backtrack_to_level(0u);
    require(learning._unit_propagate());
    require(learning._assignment[0u].value==1);
    require(learning._assignment[1u].value==1);
    require(learning._assignment[1u].reason_clause==std::optional<SizeType>(theory_clause));
    require(learning._cdcl_statistics.theory_learned_clause_propagations==1u);

    // A conflict clause inserted at decision level two has every
    // literal false. After jumping to level one, its third literal
    // becomes unit, and one of its watches must follow that literal.
    std::vector<Clause> none;
    CoreProbe delayed(3u,none);
    delayed._push_decision_level();
    delayed._assign_literal(BooleanLiteral(0u));
    delayed._assign_literal(BooleanLiteral(1u));
    delayed._push_decision_level();
    delayed._assign_literal(BooleanLiteral(2u,false));
    const SizeType learned=delayed._add_learned_clause({
        BooleanLiteral(0u,false),
        BooleanLiteral(1u,false),
        BooleanLiteral(2u)});
    require(not delayed._unit_propagate());
    require(delayed._last_boolean_conflict_clause==learned);
    delayed._backtrack_to_level(1u);
    require(delayed._unit_propagate());
    require(delayed._assignment[2u].value==1);
    require(delayed._assignment[2u].reason_clause==std::optional<SizeType>(learned));
    require(delayed._watched_positions[learned][0u]==2u
        or delayed._watched_positions[learned][1u]==2u);

    // Repeated literals are semantically unit, whereas a tautological
    // binary clause must never force an assignment.
    std::vector<Clause> duplicates{{BooleanLiteral(0u),BooleanLiteral(0u)},
                                  {BooleanLiteral(1u),BooleanLiteral(1u,false)}};
    CoreProbe repeated(2u,duplicates);
    require(repeated._unit_propagate());
    require(repeated._assignment[0u].value==1);
    require(repeated._assignment[1u].value<0);
    require(repeated._cdcl_statistics.boolean_propagations==1u);

    std::vector<Clause> empty{{}};
    CoreProbe contradictory(0u,empty);
    require(not contradictory._unit_propagate());
    require(contradictory._last_boolean_conflict_clause==std::optional<SizeType>(0u));
    return 0;
}
