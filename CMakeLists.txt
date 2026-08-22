cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(obs-lite VERSION ${OBS_VERSION_CANONICAL})

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/architecture.cmake")
  if(NOT OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
    return()
  endif()
endif()

include(compilerconfig)
include(defaults)
include(helpers)

# obs-lite branding (overrides upstream values set by bootstrap.cmake)
set(OBS_COMPANY_NAME "obs-lite project")
set(OBS_PRODUCT_NAME "obs-lite")
set(OBS_WEBSITE "https://github.com/Sensei002/obs-light")
set(OBS_COMMENTS "Lightweight Windows game recording and instant replay")
set(OBS_LEGAL_COPYRIGHT "(C) obs-lite contributors; upstream (C) Lain Bailey and OBS Project contributors")

option(ENABLE_FRONTEND "Enable building with UI (requires Qt)" ON)
option(ENABLE_SCRIPTING "Enable scripting support" OFF)
option(ENABLE_HEVC "Enable HEVC encoders" ON)

add_subdirectory(libobs)
if(OS_WINDOWS)
  add_subdirectory(libobs-d3d11)
  add_subdirectory(libobs-winrt)
endif()
add_subdirectory(libobs-opengl)
if(OS_MACOS)
  add_subdirectory(libobs-metal)
endif()
add_subdirectory(plugins)

add_subdirectory(frontend)

message_configuration()
