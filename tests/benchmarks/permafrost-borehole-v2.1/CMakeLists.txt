cmake_minimum_required(VERSION 3.24)
project(PermafrostBoreholeBenchmark LANGUAGES C CXX)

include(CTest)

set(ILAND_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(ILAND_TEST_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
set(ILAND_V21_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../iland-official-source-v2.1")
set(ILAND_MODEL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../permafrost-borehole-benchmark/model")

if(NOT EXISTS "${ILAND_V21_SOURCE}/src/ilandc/ilandc.pro")
    message(FATAL_ERROR "Official iLand 2.1 source not found at ${ILAND_V21_SOURCE}")
endif()

# Build the workspace library and its command-line runner in this build tree.
add_subdirectory("${ILAND_SOURCE_ROOT}" "${CMAKE_BINARY_DIR}/workspace")

set(ILAND_QT_ROOT "" CACHE PATH "Qt kit used to build the official 2.1 console")
set(ILAND_QMAKE "" CACHE FILEPATH "Path to the Qt kit's qmake executable")
set(ILAND_QT_MAKE "" CACHE FILEPATH "Path to the make program for the Qt kit")
set(ILAND_QT_COMPILER_BIN "" CACHE PATH "Directory containing the Qt kit's C++ compiler and runtime")

if(NOT ILAND_QMAKE)
    find_program(ILAND_QMAKE NAMES qmake qmake6 HINTS "${ILAND_QT_ROOT}/bin" REQUIRED)
endif()
execute_process(
    COMMAND "${ILAND_QMAKE}" -query QT_INSTALL_PREFIX
    OUTPUT_VARIABLE ILAND_QT_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(
    COMMAND "${ILAND_QMAKE}" -query QT_INSTALL_BINS
    OUTPUT_VARIABLE ILAND_QT_BIN
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
if(NOT ILAND_QT_COMPILER_BIN)
    get_filename_component(ILAND_QT_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
endif()
if(NOT ILAND_QT_MAKE)
    find_program(ILAND_QT_MAKE NAMES mingw32-make nmake make
        HINTS "${ILAND_QT_COMPILER_BIN}" "${ILAND_QT_ROOT}/../Tools/mingw1310_64/bin" REQUIRED)
endif()

set(ILAND_V21_BUILD_SOURCE "${CMAKE_BINARY_DIR}/official-src")
set(ILAND_V21_EXECUTABLE "${ILAND_V21_BUILD_SOURCE}/ilandc/release/ilandc${CMAKE_EXECUTABLE_SUFFIX}")
file(GLOB_RECURSE ILAND_V21_INPUTS CONFIGURE_DEPENDS
    "${ILAND_V21_SOURCE}/src/*.cpp"
    "${ILAND_V21_SOURCE}/src/*.h"
    "${ILAND_V21_SOURCE}/src/*.pro"
    "${ILAND_V21_SOURCE}/src/*.pri"
    "${ILAND_V21_SOURCE}/src/*.ui"
    "${ILAND_V21_SOURCE}/src/*.qrc")
add_custom_command(
    OUTPUT "${ILAND_V21_EXECUTABLE}"
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${ILAND_V21_SOURCE}/src"
        "-DBUILD_DIR=${ILAND_V21_BUILD_SOURCE}"
        "-DQMAKE=${ILAND_QMAKE}"
        "-DMAKE_PROGRAM=${ILAND_QT_MAKE}"
        "-DQT_BIN=${ILAND_QT_BIN}"
        "-DCOMPILER_BIN=${ILAND_QT_COMPILER_BIN}"
        "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DENV_PATH=$ENV{PATH}"
        -P "${ILAND_TEST_CMAKE_DIR}/BuildOfficial.cmake"
    DEPENDS ${ILAND_V21_INPUTS} "${ILAND_TEST_CMAKE_DIR}/BuildOfficial.cmake"
    COMMENT "Building official iLand 2.1 console with Qt"
    VERBATIM)
add_custom_target(iland-v2.1-console DEPENDS "${ILAND_V21_EXECUTABLE}")

add_executable(iland-sqlite-output-compare "${CMAKE_CURRENT_LIST_DIR}/../../common/sqlite_compare.c")
target_link_libraries(iland-sqlite-output-compare PRIVATE sqlite3)

if(BUILD_TESTING)
    add_test(NAME permafrost-v2.1-comparison
        COMMAND "${CMAKE_COMMAND}"
            "-DMODEL_DIR=${ILAND_MODEL_DIR}"
            "-DOFFICIAL_EXE=${ILAND_V21_EXECUTABLE}"
            "-DWORKSPACE_EXE=$<TARGET_FILE:iland>"
            "-DCOMPARE_EXE=$<TARGET_FILE:iland-sqlite-output-compare>"
            "-DQT_BIN=${ILAND_QT_BIN}"
            "-DQT_PREFIX=${ILAND_QT_PREFIX}"
            "-DCOMPILER_BIN=${ILAND_QT_COMPILER_BIN}"
            "-DENV_PATH=$ENV{PATH}"
            "-DRUN_ROOT=${CMAKE_BINARY_DIR}/runs"
            -P "${CMAKE_CURRENT_LIST_DIR}/RunBenchmark.cmake")
    set_tests_properties(permafrost-v2.1-comparison PROPERTIES
        TIMEOUT 900)
    add_dependencies(iland-sqlite-output-compare iland-v2.1-console iland)
endif()
