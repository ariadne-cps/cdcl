#include "pybind11.hpp"

void cdcl_submodule(pybind11::module& module);

void cdcl_module(pybind11::module& module) {
    cdcl_submodule(module);
}
