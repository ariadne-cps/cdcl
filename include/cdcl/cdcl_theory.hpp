#ifndef ARIADNE_CDCL_THEORY_HPP
#define ARIADNE_CDCL_THEORY_HPP

#include <optional>
#include <vector>
#include "cdcl/boolean_literal.hpp"

namespace Ariadne {

//! A partial Boolean assignment; nullopt means unassigned.
using CdclAssignment = std::vector<std::optional<Bool>>;
using BooleanClause = std::vector<BooleanLiteral>;

//! CONSISTENT means "not disproved", not a certified satisfying witness.
//! UNKNOWN means the theory check was inconclusive, never a logical conflict.
enum class CdclTheoryStatus { CONSISTENT, CONFLICT, UNKNOWN };

class CdclTheoryFeedback {
  public:
    static CdclTheoryFeedback consistent();
    static CdclTheoryFeedback conflict(BooleanClause explanation);
    static CdclTheoryFeedback unknown();

    CdclTheoryStatus status() const { return _status; }
    BooleanClause const& explanation() const { return _explanation; }

  private:
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
    virtual ~CdclTheory() = default;
    virtual CdclTheoryFeedback check_partial(
        CdclAssignment const& assignment) = 0;
};

} // namespace Ariadne
#endif // ARIADNE_CDCL_THEORY_HPP
