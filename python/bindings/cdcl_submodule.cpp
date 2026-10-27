#include "pybind11.hpp"
#include "cdcl/cdcl_theory.hpp"

using namespace Ariadne;

void cdcl_submodule(pybind11::module& module) {
    pybind11::class_<BooleanLiteral>(module,"BooleanLiteral")
        .def(pybind11::init<SizeType,Bool>(),
            pybind11::arg("variable"),pybind11::arg("positive")=true)
        .def_static("from_dimacs",&BooleanLiteral::from_dimacs)
        .def("variable",&BooleanLiteral::variable)
        .def("positive",&BooleanLiteral::positive)
        .def("dimacs",&BooleanLiteral::dimacs)
        .def("negation",&BooleanLiteral::negation)
        .def("__eq__",[](BooleanLiteral a,BooleanLiteral b){return a==b;})
        .def("__ne__",[](BooleanLiteral a,BooleanLiteral b){return a!=b;});

    pybind11::enum_<CdclTheoryStatus>(module,"CdclTheoryStatus")
        .value("CONSISTENT",CdclTheoryStatus::CONSISTENT)
        .value("CONFLICT",CdclTheoryStatus::CONFLICT)
        .value("UNKNOWN",CdclTheoryStatus::UNKNOWN);

    pybind11::class_<CdclTheoryFeedback>(module,"CdclTheoryFeedback")
        .def_static("consistent",&CdclTheoryFeedback::consistent)
        .def_static("conflict",&CdclTheoryFeedback::conflict)
        .def_static("unknown",&CdclTheoryFeedback::unknown)
        .def("status",&CdclTheoryFeedback::status)
        .def("explanation",&CdclTheoryFeedback::explanation,
             pybind11::return_value_policy::copy);

    module.def("validate_cdcl_theory_feedback",&validate_cdcl_theory_feedback);
}
