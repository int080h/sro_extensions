add_library(version_proxy SHARED version.cpp)
set_target_properties(version_proxy PROPERTIES OUTPUT_NAME "version" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output")
