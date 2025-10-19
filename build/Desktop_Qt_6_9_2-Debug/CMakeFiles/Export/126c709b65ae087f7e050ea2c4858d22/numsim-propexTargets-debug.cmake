#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "numsim-propex::numsim-propex" for configuration "Debug"
set_property(TARGET numsim-propex::numsim-propex APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(numsim-propex::numsim-propex PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libnumsim-propex.a"
  )

list(APPEND _cmake_import_check_targets numsim-propex::numsim-propex )
list(APPEND _cmake_import_check_files_for_numsim-propex::numsim-propex "${_IMPORT_PREFIX}/lib/libnumsim-propex.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
