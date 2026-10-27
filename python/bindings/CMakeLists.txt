include_directories(${Python3_INCLUDE_DIRS})

get_target_property(PYTHON_COMMON_SOURCE_DIR pyariadne-python-common SOURCE_DIR)

add_library(pyariadne-cdcl-submodule-obj OBJECT
    cdcl_submodule.cpp
)
target_include_directories(pyariadne-cdcl-submodule-obj PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PYTHON_COMMON_SOURCE_DIR}/include"
)
target_link_libraries(pyariadne-cdcl-submodule-obj PRIVATE
    pyariadne-python-common
    pybind11::module
    cdcl
)
set_target_properties(pyariadne-cdcl-submodule-obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(pyariadne-cdcl INTERFACE)
target_sources(pyariadne-cdcl INTERFACE
    $<TARGET_OBJECTS:pyariadne-cdcl-submodule-obj>
)
target_include_directories(pyariadne-cdcl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(pyariadne-cdcl INTERFACE
    pyariadne-python-common
)

ariadne_add_python_module(
    NAME cdcl
    MODULE_SOURCE cdcl_module.cpp
)
