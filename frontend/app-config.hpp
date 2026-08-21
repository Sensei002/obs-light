#pragma once

#include <string>

#include <obs.h>

/* App configuration stored in %APPDATA%/obs-light/config.json
 *
 * All state is kept in a single obs_data object that is persisted with
 * obs_data_save_json_safe on exit and after setting changes. */

namespace AppConfig {

/* General */
std::string GetRecordingDir();
std::string GetReplayDir();
bool StartWithWindows();
void SetStartWithWindows(bool enabled);
bool MinimizeToTray();
bool StartMinimized();
bool StartWithPreview();

/* Video */
uint32_t GetBaseWidth();
uint32_t GetBaseHeight();
uint32_t GetOutputWidth();
uint32_t GetOutputHeight();
uint32_t GetFPSNum();
uint32_t GetFPSDen();
std::string GetEncoderId(); /* resolved encoder id, e.g. "obs_nvenc_h264_tex" */
bool UseCQP();
int BitrateKbps();
int CQPValue();
int KeyintSec();
std::string GetEncoderPreset();

/* Audio */
int AudioBitrateKbps();

/* Replay */
int ReplayDurationSec();
int ReplayMaxSizeMB();
std::string ReplayExtension();
bool RemuxToMP4();

/* Capture */
std::string GetGameCaptureWindow();
int GetGameCapturePriority();
bool GetCaptureCursor();
std::string GetDisplayMonitorId(); /* "" = primary/auto */
std::string GetAppAudioWindow();
bool AppAudioEnabled();

/* Hotkeys: vk code + modifier bitmask (MOD_* from winuser.h); the name is
 * the base key such as "Hotkey.StartRecording" (.VK / .Mod suffixes are
 * appended internally). */
int GetHotkeyVK(const char *name);
int GetHotkeyModifiers(const char *name);

/* Raw config access (for settings dialog read/write) */
obs_data_t *Config();

/* Persistence */
void Load();
void Save();
void ResetToDefaults();

} // namespace AppConfig
