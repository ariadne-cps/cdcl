if(NOT TARGET utility)
    ensure_submodule_initialized(utility)
    add_subdirectory(utility)
endif()

if(BUILD_PYTHON_BINDINGS AND NOT TARGET pyariadne-python-common)
    ensure_submodule_initialized(python-common)
    add_subdirectory(python-common)
endif()
