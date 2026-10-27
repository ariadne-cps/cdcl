from pyariadne import BooleanLiteral, CdclTheoryFeedback, validate_cdcl_theory_feedback

x=BooleanLiteral(0)
feedback=CdclTheoryFeedback.conflict([x.negation()])
validate_cdcl_theory_feedback(feedback,[True])
print("CDCL theory interface tutorial completed successfully")
