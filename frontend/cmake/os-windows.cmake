# obs-light Windows-specific frontend build setup

if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

target_link_libraries(obs-studio PRIVATE OBS::w32-pthreads)

# Copy the icon to the build directory so the .rc can reference it by filename
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/obs-light.ico"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

configure_file(cmake/templates/obs-light.rc.in obs-light.rc)

target_sources(obs-studio PRIVATE obs-light.rc)

target_compile_definitions(obs-studio PRIVATE PSAPI_VERSION=2)

set_target_properties(
  obs-studio
  PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/bin/64bit"
)

set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT obs-studio)
