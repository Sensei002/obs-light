cmake_minimum_required(VERSION 3.28...3.30)

if(NOT ENABLE_FRONTEND)
  return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat avdevice avfilter swscale swresample)
find_package(Libx264 REQUIRED)

if(NOT TARGET OBS::libobs)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" libobs)
endif()

# Target is named "obs-studio" so that OBS's cmake/windows/helpers.cmake
# bundles the runtime dependencies (Qt DLLs, FFmpeg, obs.dll, ...) into the
# rundir and installs the executable next to them.  The actual file name is
# obs-light.exe via OUTPUT_NAME below.
add_executable(obs-studio)
add_executable(OBS::studio ALIAS obs-studio)

target_sources(
  obs-studio
  PRIVATE
    app-config.cpp
    app-config.hpp
    capture-manager.cpp
    capture-manager.hpp
    hotkeys.cpp
    hotkeys.hpp
    main-window.cpp
    main-window.hpp
    obs-app.cpp
    obs-app.hpp
    preview-widget.cpp
    preview-widget.hpp
    recorder.cpp
    recorder.hpp
    settings-dialog.cpp
    settings-dialog.hpp
    main.cpp
)

target_include_directories(obs-studio PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_CURRENT_SOURCE_DIR}")

# FFmpeg and x264 are linked (though not directly used) so that OBS's
# dependency bundling copies their DLLs into the rundir for the plugins.
target_link_libraries(
  obs-studio
  PRIVATE
    OBS::libobs
    Qt::Widgets
    FFmpeg::avcodec
    FFmpeg::avutil
    FFmpeg::avformat
    FFmpeg::avdevice
    FFmpeg::avfilter
    FFmpeg::swscale
    FFmpeg::swresample
    Libx264::Libx264
    user32
    shell32
)

set_target_properties(
  obs-studio
  PROPERTIES AUTOMOC TRUE AUTORCC TRUE WIN32_EXECUTABLE TRUE OUTPUT_NAME obs-light FOLDER frontend
)

target_sources(obs-studio PRIVATE resources/obs-light.qrc)

set_target_properties_obs(obs-studio PROPERTIES FOLDER frontend)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
endif()
