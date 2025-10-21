cmake_minimum_required(VERSION 3.22)

project(numsim_propex_test)

set(CMAKE_CXX_STANDARD 20)

enable_testing()
include(CTest)

#google test
include(FetchContent)
FetchContent_Declare(
  googletest
#  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
#

include(GoogleTest)
macro(add_numsim_propex_test TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})    
    target_link_libraries(${TARGET_NAME} PRIVATE gtest_main numsim-propex)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    gtest_discover_tests(${TARGET_NAME})
endmacro()

add_numsim_propex_test(
  ${PROJECT_NAME}
    main.cpp
    #key_traits_test.h
)

target_sources(numsim_propex_test
  PRIVATE
    key_traits_test.h
    registry_test.h
)
