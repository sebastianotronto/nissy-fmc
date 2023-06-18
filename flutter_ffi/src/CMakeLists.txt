# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(nissy_flutter_ffi_library VERSION 1.0.0 LANGUAGES C)

add_library(nissy_flutter_ffi SHARED
  coord.c coord.h cube.c cube.h nissy.c solve.c solve.h steps.c steps.h
)

set_target_properties(nissy_flutter_ffi PROPERTIES
  PUBLIC_HEADER nissy.h
  OUTPUT_NAME "nissy_flutter_ffi"
)

if (WIN32)
set_target_properties(ffigen_app PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)
endif (WIN32)

target_compile_definitions(nissy_flutter_ffi PUBLIC DART_SHARED_LIB)
