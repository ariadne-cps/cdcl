if(NOT TARGET utility)
    ariadne_add_submodule(utility)
endif()

if(BUILD_PYTHON_BINDINGS AND NOT TARGET pyariadne-python-common)
    ariadne_add_submodule(python-common)
endif()
