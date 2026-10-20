add_executable(iland iland.c)
target_link_libraries(iland PRIVATE ILandModel)

if(MINGW)
    target_link_options(iland PRIVATE -static)
endif()
