foreach(required MODEL_DIR OFFICIAL_EXE WORKSPACE_EXE COMPARE_EXE QT_BIN QT_PREFIX COMPILER_BIN ENV_PATH RUN_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "RunBenchmark.cmake requires ${required}")
    endif()
endforeach()

set(official_run "${RUN_ROOT}/official-2.1")
set(workspace_run "${RUN_ROOT}/workspace-library")
file(MAKE_DIRECTORY "${RUN_ROOT}")
file(COPY "${MODEL_DIR}/" DESTINATION "${official_run}")
file(COPY "${MODEL_DIR}/" DESTINATION "${workspace_run}")

foreach(run_directory "${official_run}" "${workspace_run}")
    file(REMOVE
        "${run_directory}/output/output.sqlite"
        "${run_directory}/temp/debug_water_cycle.csv"
        "${run_directory}/log.txt")
    set(project_file "${run_directory}/permafrost_bore.hole_project_file.xml")
    file(READ "${project_file}" project_xml)
    string(REGEX REPLACE "<multithreading>[^<]*</multithreading>"
        "<multithreading>false</multithreading>" project_xml "${project_xml}")
    string(REGEX REPLACE "<randomSeed>[^<]*</randomSeed>"
        "<randomSeed>314159</randomSeed>" project_xml "${project_xml}")
    if(NOT project_xml MATCHES "<multithreading>false</multithreading>" OR
       NOT project_xml MATCHES "<randomSeed>314159</randomSeed>")
        message(FATAL_ERROR "Could not set deterministic benchmark options in ${project_file}")
    endif()
    file(WRITE "${project_file}" "${project_xml}")
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=${QT_BIN};${COMPILER_BIN};${ENV_PATH}"
        "QT_PLUGIN_PATH=${QT_PREFIX}/plugins"
        "${OFFICIAL_EXE}" permafrost_bore.hole_project_file.xml 18
    WORKING_DIRECTORY "${official_run}"
    RESULT_VARIABLE official_result
    OUTPUT_FILE "${official_run}/console.stdout.log"
    ERROR_FILE "${official_run}/console.stderr.log")
if(NOT official_result EQUAL 0)
    message(FATAL_ERROR "Official iLand 2.1 failed (${official_result}); see ${official_run}/console.*.log and log.txt")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=${QT_BIN};${COMPILER_BIN};${ENV_PATH}"
        "${WORKSPACE_EXE}" permafrost_bore.hole_project_file.xml 19
    WORKING_DIRECTORY "${workspace_run}"
    RESULT_VARIABLE workspace_result
    OUTPUT_FILE "${workspace_run}/console.stdout.log"
    ERROR_FILE "${workspace_run}/console.stderr.log")
if(NOT workspace_result EQUAL 0)
    message(FATAL_ERROR "Workspace library failed (${workspace_result}); see ${workspace_run}/console.*.log and log.txt")
endif()

execute_process(
    COMMAND "${COMPARE_EXE}"
        "${official_run}/output/output.sqlite"
        "${workspace_run}/output/output.sqlite"
    RESULT_VARIABLE compare_result)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "Official 2.1 and workspace library outputs differ. Run folders are under ${RUN_ROOT}")
endif()
