cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_PLUGINS "Enable building OBS plugins" ON)

if(NOT ENABLE_PLUGINS)
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_DISABLED "Plugin Support")
  return()
endif()

set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_ENABLED "Plugin Support")

# obs-light plugin selection.
#
# Only the components required for local recording, replay buffer, game
# capture, display capture and application audio capture are built:
#   - obs-ffmpeg   : MP4/MKV muxing (ffmpeg-mux), replay buffer output,
#                    ffmpeg_aac audio encoder
#   - obs-nvenc    : NVIDIA NVENC H.264/HEVC hardware encoders
#   - obs-x264     : software H.264 fallback encoder
#   - win-capture  : game capture, display capture, window capture
#   - win-wasapi   : application audio capture (process output loopback)
#
# All streaming-related plugins (obs-outputs, rtmp-services, obs-websocket,
# obs-browser, ...) have been removed from the tree.

add_obs_plugin(obs-ffmpeg)
add_obs_plugin(obs-nvenc PLATFORMS WINDOWS LINUX ARCHITECTURES x64 x86_64)
add_obs_plugin(obs-x264)
add_obs_plugin(win-capture PLATFORMS WINDOWS)
add_obs_plugin(win-wasapi PLATFORMS WINDOWS)
