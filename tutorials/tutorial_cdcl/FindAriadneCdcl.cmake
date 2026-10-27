# Try to find the standalone Ariadne CDCL library.
#
# Once done this will define:
#
#  AriadneCdcl_FOUND - system has Ariadne CDCL
#  ARIADNE_CDCL_INCLUDE_DIRS - the Ariadne CDCL include directories
#  ARIADNE_CDCL_LIBRARIES - libraries required to use Ariadne CDCL
#
# and, when found:
#
#  AriadneCdcl::ariadne-cdcl - imported library target
#
# Copyright  2026  Luca Geretti

find_library(ARIADNE_CDCL_LIBRARY NAMES ariadne-cdcl)
find_path(ARIADNE_CDCL_INCLUDE_DIR ariadne-cdcl.hpp PATH_SUFFIXES ariadne-cdcl)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AriadneCdcl DEFAULT_MSG
  ARIADNE_CDCL_LIBRARY
  ARIADNE_CDCL_INCLUDE_DIR
)

set(ARIADNE_CDCL_FOUND ${AriadneCdcl_FOUND})

if(AriadneCdcl_FOUND)
  set(ARIADNE_CDCL_INCLUDE_DIRS ${ARIADNE_CDCL_INCLUDE_DIR})
  set(ARIADNE_CDCL_LIBRARIES ${ARIADNE_CDCL_LIBRARY})

  if(NOT TARGET AriadneCdcl::ariadne-cdcl)
    add_library(AriadneCdcl::ariadne-cdcl UNKNOWN IMPORTED)
    set_target_properties(AriadneCdcl::ariadne-cdcl PROPERTIES
      IMPORTED_LOCATION "${ARIADNE_CDCL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ARIADNE_CDCL_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(
  ARIADNE_CDCL_INCLUDE_DIR
  ARIADNE_CDCL_LIBRARY
)
