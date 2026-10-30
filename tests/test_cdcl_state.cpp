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
#include "macro/testing.hpp"
#include <vector>
#include "cdcl/cdcl_state.hpp"

using namespace Ariadne;

namespace {
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
    using CdclState::_original_clause_count;
    using CdclState::_clause_count;
    using CdclState::_clause;
    using CdclState::_is_theory_learned_clause;
    using CdclState::_active_non_theory_learned_clause_count;
    using CdclState::_learned_clause_locked;
};
}

Void test_cdcl_state() {
    ARIADNE_PRINT_TEST_CASE_TITLE("Root unit propagation");
    // Unit propagation with zero-based logical variables.
    std::vector<Clause> clauses{
        {BooleanLiteral(0u)},
        {BooleanLiteral(0u,false),BooleanLiteral(1u)}
    };
    CoreProbe core(2u,clauses);
    ARIADNE_TEST_ASSERT(core._unit_propagate());
    ARIADNE_TEST_ASSERT(core._assignment[0u].value==1);
    ARIADNE_TEST_ASSERT(core._assignment[1u].value==1);
    ARIADNE_TEST_ASSERT(core._next_unassigned_variable()==2u);
    ARIADNE_TEST_ASSERT(core._cdcl_statistics.boolean_propagations==2u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Original clauses survive learned-clause insertion");
    // The original clauses are untouched by learned-clause storage.
    const SizeType learned=core._add_learned_clause({
        BooleanLiteral(1u,false)
    });
    ARIADNE_TEST_ASSERT(learned==2u);
    ARIADNE_TEST_ASSERT(not core._unit_propagate());
    ARIADNE_TEST_ASSERT(core._last_boolean_conflict_clause==learned);
    core._maybe_prune_learned_clauses(learned);

    ARIADNE_PRINT_TEST_CASE_TITLE("Decision assignment rollback");
    // Backtracking restores only assignments made at the later level.
    std::vector<Clause> no_clauses;
    CoreProbe decisions(2u,no_clauses);
    decisions._push_decision_level();
    decisions._assign_literal(BooleanLiteral(0u));
    ARIADNE_TEST_ASSERT(decisions._decision_level()==1u);
    ARIADNE_TEST_ASSERT(decisions._next_unassigned_variable()==1u);
    decisions._backtrack_to_level(0u);
    ARIADNE_TEST_ASSERT(decisions._decision_level()==0u);
    ARIADNE_TEST_ASSERT(decisions._next_unassigned_variable()==0u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Nested backtrack and root propagation retention");
    // An inner backtrack must not discard decisions at its parent level,
    // and a backtrack to root must retain root-level unit propagations.
    std::vector<Clause> root_clauses{{BooleanLiteral(0u)}};
    CoreProbe nested(3u,root_clauses);
    ARIADNE_TEST_ASSERT(nested._unit_propagate());
    ARIADNE_TEST_ASSERT(nested._assignment[0u].value==1);
    nested._push_decision_level();
    nested._assign_literal(BooleanLiteral(1u));
    nested._push_decision_level();
    nested._assign_literal(BooleanLiteral(2u,false));
    ARIADNE_TEST_ASSERT(nested._decision_level()==2u);
    nested._backtrack_to_level(1u);
    ARIADNE_TEST_ASSERT(nested._decision_level()==1u);
    ARIADNE_TEST_ASSERT(nested._assignment[0u].value==1);
    ARIADNE_TEST_ASSERT(nested._assignment[1u].value==1);
    ARIADNE_TEST_ASSERT(nested._assignment[2u].value<0);
    ARIADNE_TEST_ASSERT(nested._next_unassigned_variable()==2u);
    nested._backtrack_to_level(1u);
    ARIADNE_TEST_ASSERT(nested._assignment[1u].value==1);
    nested._backtrack_to_level(0u);
    ARIADNE_TEST_ASSERT(nested._decision_level()==0u);
    ARIADNE_TEST_ASSERT(nested._assignment[0u].value==1);
    ARIADNE_TEST_ASSERT(nested._assignment[1u].value<0);
    ARIADNE_TEST_ASSERT(nested._next_unassigned_variable()==1u);
    ARIADNE_TEST_ASSERT(nested._unit_propagate());
    ARIADNE_TEST_ASSERT(nested._cdcl_statistics.boolean_propagations==1u);

    ARIADNE_TEST_THROWS(nested._backtrack_to_level(1u),std::out_of_range);

    ARIADNE_PRINT_TEST_CASE_TITLE("Watch relocation and implied third literal");
    // Watch movement: a falsified watch moves to a third literal.
    // A second false decision then forces that third literal.
    std::vector<Clause> ternary{{
        BooleanLiteral(0u),BooleanLiteral(1u),BooleanLiteral(2u)}};
    CoreProbe watched(3u,ternary);
    ARIADNE_TEST_ASSERT(watched._unit_propagate());
    watched._push_decision_level();
    watched._assign_literal(BooleanLiteral(0u,false));
    ARIADNE_TEST_ASSERT(watched._unit_propagate());
    ARIADNE_TEST_ASSERT(watched._watched_positions[0u][0u]==2u);
    watched._assign_literal(BooleanLiteral(1u,false));
    ARIADNE_TEST_ASSERT(watched._unit_propagate());
    ARIADNE_TEST_ASSERT(watched._assignment[2u].value==1);
    ARIADNE_TEST_ASSERT(watched._assignment[2u].reason_clause==std::optional<SizeType>(0u));
    ARIADNE_TEST_ASSERT(watched._cdcl_statistics.boolean_propagations==1u);
    ARIADNE_TEST_ASSERT(watched._propagation_head==3u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Pending theory clause after backjump");
    // After a backjump, a newly learned clause must be inspected even
    // when no retained assignment generates a fresh watch event.
    std::vector<Clause> fixed_root{{BooleanLiteral(0u)}};
    CoreProbe learning(2u,fixed_root);
    ARIADNE_TEST_ASSERT(learning._unit_propagate());
    learning._push_decision_level();
    learning._assign_literal(BooleanLiteral(1u,false));
    const SizeType theory_clause=learning._add_learned_clause({
        BooleanLiteral(0u,false),BooleanLiteral(1u)},true);
    ARIADNE_TEST_ASSERT(not learning._unit_propagate());
    ARIADNE_TEST_ASSERT(learning._last_boolean_conflict_clause==theory_clause);
    learning._backtrack_to_level(0u);
    ARIADNE_TEST_ASSERT(learning._unit_propagate());
    ARIADNE_TEST_ASSERT(learning._assignment[0u].value==1);
    ARIADNE_TEST_ASSERT(learning._assignment[1u].value==1);
    ARIADNE_TEST_ASSERT(learning._assignment[1u].reason_clause==std::optional<SizeType>(theory_clause));
    ARIADNE_TEST_ASSERT(learning._cdcl_statistics.theory_learned_clause_propagations==1u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Delayed watches for a learned ternary clause");
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
    const SizeType delayed_learned=delayed._add_learned_clause({
        BooleanLiteral(0u,false),
        BooleanLiteral(1u,false),
        BooleanLiteral(2u)});
    ARIADNE_TEST_ASSERT(not delayed._unit_propagate());
    ARIADNE_TEST_ASSERT(delayed._last_boolean_conflict_clause==delayed_learned);
    delayed._backtrack_to_level(1u);
    ARIADNE_TEST_ASSERT(delayed._unit_propagate());
    ARIADNE_TEST_ASSERT(delayed._assignment[2u].value==1);
    ARIADNE_TEST_ASSERT(delayed._assignment[2u].reason_clause==std::optional<SizeType>(delayed_learned));
    ARIADNE_TEST_ASSERT(delayed._watched_positions[delayed_learned][0u]==2u
        or delayed._watched_positions[delayed_learned][1u]==2u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Repeated literals and tautological clauses");
    // Repeated literals are semantically unit, whereas a tautological
    // binary clause must never force an assignment.
    std::vector<Clause> duplicates{{BooleanLiteral(0u),BooleanLiteral(0u)},
                                  {BooleanLiteral(1u),BooleanLiteral(1u,false)}};
    CoreProbe repeated(2u,duplicates);
    ARIADNE_TEST_ASSERT(repeated._unit_propagate());
    ARIADNE_TEST_ASSERT(repeated._assignment[0u].value==1);
    ARIADNE_TEST_ASSERT(repeated._assignment[1u].value<0);
    ARIADNE_TEST_ASSERT(repeated._cdcl_statistics.boolean_propagations==1u);

    ARIADNE_PRINT_TEST_CASE_TITLE("Empty clause conflict");
    std::vector<Clause> empty{{}};
    CoreProbe contradictory(0u,empty);
    ARIADNE_TEST_ASSERT(not contradictory._unit_propagate());
    ARIADNE_TEST_ASSERT(contradictory._last_boolean_conflict_clause==std::optional<SizeType>(0u));

    ARIADNE_PRINT_TEST_CASE_TITLE("Clause classification and live reason locks");
    // Learned-clause queries distinguish original, theory and Boolean
    // clauses. Locks follow live propagation reasons, never decisions or
    // assignments removed by backtracking.
    std::vector<Clause> original{{BooleanLiteral(0u),BooleanLiteral(1u)}};
    CoreProbe locks(3u,original);
    ARIADNE_TEST_ASSERT(not locks._is_theory_learned_clause(0u));
    ARIADNE_TEST_ASSERT(locks._active_non_theory_learned_clause_count()==0u);
    locks._maybe_prune_learned_clauses(0u);
    ARIADNE_TEST_ASSERT(locks._cdcl_statistics.learned_clause_pruning_runs==0u);
    SizeType const theory_unit=locks._add_learned_clause({BooleanLiteral(0u)},true);
    Clause const binary{BooleanLiteral(1u),BooleanLiteral(2u)};
    SizeType const boolean_binary=locks._add_learned_clause(binary);
    ARIADNE_TEST_ASSERT(locks._is_theory_learned_clause(theory_unit));
    ARIADNE_TEST_ASSERT(not locks._is_theory_learned_clause(boolean_binary));
    ARIADNE_TEST_ASSERT(locks._active_non_theory_learned_clause_count()==1u);
    ARIADNE_TEST_ASSERT(not locks._learned_clause_locked(theory_unit));
    ARIADNE_TEST_ASSERT(not locks._learned_clause_locked(boolean_binary));
    ARIADNE_TEST_ASSERT(locks._unit_propagate());
    ARIADNE_PRINT_TEST_COMMENT("A live theory reason must lock its clause");
    ARIADNE_TEST_ASSERT(locks._learned_clause_locked(theory_unit));
    ARIADNE_TEST_ASSERT(not locks._learned_clause_locked(boolean_binary));
    locks._push_decision_level();
    locks._assign_literal(BooleanLiteral(1u,false));
    ARIADNE_TEST_ASSERT(not locks._learned_clause_locked(boolean_binary));
    ARIADNE_TEST_ASSERT(locks._unit_propagate());
    ARIADNE_TEST_ASSERT(locks._assignment[2u].value==1);
    ARIADNE_TEST_ASSERT(locks._assignment[2u].reason_clause==boolean_binary);
    ARIADNE_PRINT_TEST_COMMENT("Boolean propagation must lock its reason clause");
    ARIADNE_TEST_ASSERT(locks._learned_clause_locked(boolean_binary));
    locks._maybe_prune_learned_clauses(boolean_binary);
    ARIADNE_TEST_ASSERT(locks._cdcl_statistics.learned_clause_pruning_runs==1u);
    ARIADNE_TEST_ASSERT(locks._cdcl_statistics.learned_clauses_pruned==0u);
    ARIADNE_TEST_ASSERT(locks._clause_count()==3u and locks._original_clause_count()==1u);
    ARIADNE_TEST_ASSERT(locks._clause(boolean_binary)==binary);
    locks._backtrack_to_level(0u);
    ARIADNE_PRINT_TEST_COMMENT("Backtracking must release the Boolean reason lock");
    ARIADNE_TEST_ASSERT(not locks._learned_clause_locked(boolean_binary));
    ARIADNE_TEST_ASSERT(locks._learned_clause_locked(theory_unit));
    locks._push_decision_level();
    locks._assign_literal(BooleanLiteral(2u,false));
    ARIADNE_TEST_ASSERT(locks._unit_propagate());
    ARIADNE_PRINT_TEST_COMMENT("Reduction must preserve clauses needed after backtracking");
    ARIADNE_TEST_ASSERT(locks._assignment[1u].value==1 and locks._assignment[1u].reason_clause==boolean_binary);

    ARIADNE_PRINT_TEST_CASE_TITLE("Deferred reduction retains old long clauses");
    // Reduction is deliberately deferred even for old, unprotected, long
    // clauses. These learned copies are entailed by the original formula.
    std::vector<Clause> long_original{{BooleanLiteral(0u),BooleanLiteral(1u),BooleanLiteral(2u)}};
    CoreProbe retained(3u,long_original);
    for(SizeType i=0u;i<4u;++i) {
        retained._add_learned_clause(long_original[0u]);
    }
    retained._maybe_prune_learned_clauses(4u);
    ARIADNE_PRINT_TEST_COMMENT("Deferred reduction must retain even old candidate clauses");
    ARIADNE_TEST_ASSERT(retained._clause_count()==5u and retained._cdcl_statistics.learned_clauses_pruned==0u);
    ARIADNE_TEST_ASSERT(retained._cdcl_statistics.learned_clause_pruning_runs==1u);
    for(SizeType i=1u;i<5u;++i) { ARIADNE_TEST_ASSERT(retained._clause(i)==long_original[0u]); }
    ARIADNE_TEST_ASSERT(retained._unit_propagate());
    retained._push_decision_level();
    retained._assign_literal(BooleanLiteral(0u,false));
    ARIADNE_TEST_ASSERT(retained._unit_propagate());
    retained._assign_literal(BooleanLiteral(1u,false));
    ARIADNE_TEST_ASSERT(retained._unit_propagate());
    ARIADNE_TEST_ASSERT(retained._assignment[2u].value==1);
}

int main() {
    ARIADNE_TEST_CALL(test_cdcl_state());
    return ARIADNE_TEST_FAILURES;
}
