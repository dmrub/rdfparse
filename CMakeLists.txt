cmake_minimum_required(VERSION 2.8)

project(rdfparse)

# User options

#--------------------------------------------------
# load script for checking out projects from git
#--------------------------------------------------
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake ${PROJECT_SOURCE_DIR}/CMake/common)
include(GitExternal)

#--------------------------------------------------
# checkout common cmake tools library
# args are : location here, repo URL, repo branch
#--------------------------------------------------
Git_External(
  ${PROJECT_SOURCE_DIR}/CMake/common
  https://github.com/Eyescale/CMake.git
  master
  VERBOSE
)

#--------------------------------------------------
# checkout projects we want to build together
#--------------------------------------------------

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/external)

Git_External(
  ${PROJECT_SOURCE_DIR}/external/seord
  https://github.com/dmrub/seord.git
  master
  VERBOSE
)

#--------------------------------------------------
# load add_subproject macro from common tools
#--------------------------------------------------
include(SubProject)

# Detect compiler
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(C_IS_GNU   1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(C_IS_GNU   1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(C_IS_INTEL 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(C_IS_MSVC  1)
endif()

# Enable C99 mode
if(C_IS_GNU)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

if(C_IS_INTEL)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -restrict")
endif()

# Enable C++11 mode
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


#--------------------------------------------------
# Add the subprojects to our project
# args are : project name in subproject, path from here
#--------------------------------------------------

set(seord_SKIP_FIND 1)
add_subproject(external/seord)

# Find packages

find_package(Redland)
find_package(Raptor)
find_package(Rasqal)
find_package(POCO)
find_package(Threads REQUIRED)

find_package(Boost 1.54.0 COMPONENTS regex system thread coroutine context filesystem date_time REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(
  src
  src/catch
  external/seord/src/3rd/serd
  external/seord/src/3rd/sord
  external/seord/src/3rd/sord/src
  ${POCO_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR})

add_executable(sordmm_test_writer src/sordmm_test_writer.cpp ${LIBHEADERS})
target_link_libraries(sordmm_test_writer seord)

add_executable(sordmm_test_reader src/sordmm_test_reader.cpp ${LIBHEADERS})
target_link_libraries(sordmm_test_reader seord)

if(REDLAND_FOUND)
  message(STATUS "Found Redland library")

  message(STATUS "Found Raptor header: ${RAPTOR_INCLUDE_DIR}")

  include_directories(
    ${RAPTOR_INCLUDE_DIR}
    ${REDLAND_INCLUDE_DIR}
    ${RASQAL_INCLUDE_DIR}
    )

  add_executable(redland_test_writer src/redland_test_writer.cpp   ${LIBHEADERS})
  target_link_libraries(redland_test_writer ${REDLAND_LIBRARIES} ${RAPTOR_LIBRARIES})
  
endif()
