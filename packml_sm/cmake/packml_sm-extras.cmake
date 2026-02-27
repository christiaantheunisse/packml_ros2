include(CMakeFindDependencyMacro)
find_dependency(Qt5 COMPONENTS Core Gui Widgets)

# Make the packml_sm_generate_modes() function available to downstream packages.
include("${CMAKE_CURRENT_LIST_DIR}/packml_sm_generate_modes.cmake")
