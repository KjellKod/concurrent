# ==================================================================
# 2014 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own
#    risk and comes  with no warranties.
#
# This code is yours to share, use and modify with no strings attached
#   and no restrictions or obligations.
# ===================================================================

cmake_minimum_required (VERSION 2.6)
ENABLE_LANGUAGE(CXX)
project (ConcurrentWrapper)
set(CONCURRENT_SRC ${ConcurrentWrapper_SOURCE_DIR}/src)
set(DIR_UNIT_TEST ${ConcurrentWrapper_SOURCE_DIR}/test)
set(DIR_3RDPARTY  ${ConcurrentWrapper_SOURCE_DIR}/3rdparty)


MESSAGE("  CONCURRENT_SRC = : ${CONCURRENT_SRC}")
include_directories(${CONCURRENT_SRC})
SET(ACTIVE_CPP0xx_DIR "Release")


#cmake -DCMAKE_CXX_COMPILER=clang++ ..
  # WARNING: If Clang for Linux does not work with full C++11 support it might be your
  # installation that is faulty. When I tested Clang on Ubuntu I followed the following
  # description 
  #  1) http://kjellkod.wordpress.com/2013/09/23/experimental-g2log-with-clang/
  #  2) https://github.com/maidsafe/MaidSafe/wiki/Hacking-with-Clang-llvm-abi-and-llvm-libc
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   MESSAGE("")
   MESSAGE("cmake for Clang ")
   if (APPLE)
       set(PLATFORM_LINK_LIBRIES c++abi)
   else()
       set(PLATFORM_LINK_LIBRIES rt  c++abi)
   endif()
  set(CMAKE_CXX_FLAGS  "-Wall -std=c++14  -stdlib=libc++ -Wunused -D_GLIBCXX_USE_NANOSLEEP")

ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   MESSAGE("")
   MESSAGE("cmake for GCC ")
   if (APPLE)
       set(CMAKE_CXX_FLAGS "-Wall -Wunused -std=c++14  -pthread -D_GLIBCXX_USE_NANOSLEEP")
   else()
       set(PLATFORM_LINK_LIBRIES rt)
       set(CMAKE_CXX_FLAGS "-Wall -rdynamic -Wunused -std=c++14 -pthread -D_GLIBCXX_USE_NANOSLEEP")
   endif()
ENDIF()


# GENERIC STEPS
file(GLOB SRC_FILES ${CONCURRENT_SRC}/*.hpp ${CONCURRENT_SRC}/*.cpp)
 
set(GTEST_DIR ${DIR_3RDPARTY}/gtest-1.7.0)
set(GTEST_INCLUDE_DIRECTORIES ${GTEST_DIR}/include ${GTEST_DIR} ${GTEST_DIR}/src)
MESSAGE( "Attempt to build gtest. gtest directory: " ${GTEST_DIR})
include_directories(${GTEST_INCLUDE_DIRECTORIES})
add_library(gtest_170_lib ${GTEST_DIR}/src/gtest-all.cc)
set_target_properties(gtest_170_lib  PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_RTTI=0")
SET_TARGET_PROPERTIES(gtest_170_lib PROPERTIES COMPILE_DEFINITIONS "GTEST_USE_OWN_TR1_TUPLE=1")


file(GLOB TEST_SRC_FILES ${DIR_UNIT_TEST}/*.hpp ${DIR_UNIT_TEST}/*.cpp)
MESSAGE("test files: ${TEST_SRC_FILES}")
ADD_EXECUTABLE(UnitTestRunner 3rdparty/test_main.cpp  ${TEST_SRC_FILES} ${SRC_FILES})
set_target_properties(UnitTestRunner PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_RTTI=0")
SET_TARGET_PROPERTIES(UnitTestRunner PROPERTIES COMPILE_DEFINITIONS "GTEST_USE_OWN_TR1_TUPLE=1")
TARGET_LINK_LIBRARIES(UnitTestRunner )
target_link_libraries(UnitTestRunner gtest_170_lib )

  






