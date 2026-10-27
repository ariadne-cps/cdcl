/***************************************************************************
 *            test_smt_cdcl_core.cpp
 *
 *  Copyright 2026 Luca Geretti
 *  SPDX-License-Identifier: GPL-3.0-or-later
 ****************************************************************************/

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
    return 0;
}
