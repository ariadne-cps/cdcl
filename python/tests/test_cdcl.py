import pytest
from pyariadne import (BooleanLiteral, CdclTheoryFeedback, CdclTheoryStatus,
                      validate_cdcl_theory_feedback)

def test_literals():
    x=BooleanLiteral(0)
    assert x.dimacs()==1
    assert x.negation().dimacs()==-1
    assert BooleanLiteral.from_dimacs(-2).variable()==1
    with pytest.raises(Exception):
        BooleanLiteral.from_dimacs(0)

def test_theory_contract():
    feedback=CdclTheoryFeedback.conflict([BooleanLiteral(0,False)])
    assert feedback.status()==CdclTheoryStatus.CONFLICT
    validate_cdcl_theory_feedback(feedback,[True,None])
    with pytest.raises(Exception):
        validate_cdcl_theory_feedback(feedback,[False,None])
    assert CdclTheoryFeedback.unknown().status()==CdclTheoryStatus.UNKNOWN
