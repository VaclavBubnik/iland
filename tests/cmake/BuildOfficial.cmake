foreach(required SOURCE_DIR BUILD_DIR QMAKE MAKE_PROGRAM QT_BIN COMPILER_BIN CXX_COMPILER ENV_PATH)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "BuildOfficial.cmake requires ${required}")
    endif()
endforeach()

file(MAKE_DIRECTORY "${BUILD_DIR}")
file(COPY "${SOURCE_DIR}/" DESTINATION "${BUILD_DIR}")
# Keep vendored dependencies present in clean build trees, including FreeImage.
file(COPY "${SOURCE_DIR}/3rdparty" DESTINATION "${BUILD_DIR}")

# The upstream Windows .pro embeds `date /t` (which contains spaces) in a
# compiler define. Replace only that build stamp in this disposable source copy.
set(console_project "${BUILD_DIR}/ilandc/ilandc.pro")
file(COPY_FILE "${SOURCE_DIR}/ilandc/ilandc.pro" "${console_project}" ONLY_IF_DIFFERENT)
file(READ "${console_project}" console_project_text)
string(REPLACE "$$quote($$system(date /t))" "benchmark"
    console_project_text "${console_project_text}")
file(WRITE "${console_project}" "${console_project_text}")

set(build_path "${QT_BIN};${COMPILER_BIN};${ENV_PATH}")

set(compiler_marker "${BUILD_DIR}/.qmake-compiler")
set(previous_compiler "")
if(EXISTS "${compiler_marker}")
    file(READ "${compiler_marker}" previous_compiler)
endif()
if(NOT previous_compiler STREQUAL CXX_COMPILER)
    file(GLOB_RECURSE qmake_output_dirs LIST_DIRECTORIES true
        "${BUILD_DIR}/*/release" "${BUILD_DIR}/*/debug")
    foreach(output_dir IN LISTS qmake_output_dirs)
        file(REMOVE_RECURSE "${output_dir}")
    endforeach()
    file(GLOB plugin_archives "${BUILD_DIR}/plugins/libiland_*.a")
    if(plugin_archives)
        file(REMOVE ${plugin_archives})
    endif()
    file(WRITE "${compiler_marker}" "${CXX_COMPILER}")
endif()

function(qmake_build directory project_file)
    if(NOT EXISTS "${directory}/${project_file}")
        message(FATAL_ERROR "Missing Qt project file ${directory}/${project_file}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${build_path}"
            "${QMAKE}" "${project_file}" "CONFIG+=release"
            "GIT_HASH=benchmark" "GIT_BRANCH=benchmark" "BUILD_TIMESTAMP=benchmark"
        WORKING_DIRECTORY "${directory}"
        RESULT_VARIABLE qmake_result
        OUTPUT_VARIABLE qmake_output
        ERROR_VARIABLE qmake_error)
    if(NOT qmake_result EQUAL 0)
        message(FATAL_ERROR "qmake failed for ${project_file}:\n${qmake_output}\n${qmake_error}")
    endif()

    get_filename_component(make_name "${MAKE_PROGRAM}" NAME)
    if(make_name MATCHES "mingw32-make|make")
        set(make_args -j8)
    else()
        set(make_args)
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${build_path}"
            "${MAKE_PROGRAM}" ${make_args}
        WORKING_DIRECTORY "${directory}"
        RESULT_VARIABLE make_result
        OUTPUT_VARIABLE make_output
        ERROR_VARIABLE make_error)
    if(NOT make_result EQUAL 0)
        message(FATAL_ERROR "Qt make failed in ${directory}:\n${make_output}\n${make_error}")
    endif()
endfunction()

foreach(plugin fire wind barkbeetle)
    file(COPY "${SOURCE_DIR}/plugins/${plugin}" DESTINATION "${BUILD_DIR}/plugins")
    qmake_build("${BUILD_DIR}/plugins/${plugin}" "${plugin}.pro")
endforeach()
qmake_build("${BUILD_DIR}/ilandc" "ilandc.pro")

if(EXISTS "${BUILD_DIR}/3rdparty/FreeImage/FreeImage.dll")
    file(MAKE_DIRECTORY "${BUILD_DIR}/ilandc/release")
    file(COPY_FILE "${BUILD_DIR}/3rdparty/FreeImage/FreeImage.dll"
        "${BUILD_DIR}/ilandc/release/FreeImage.dll" ONLY_IF_DIFFERENT)
endif()
