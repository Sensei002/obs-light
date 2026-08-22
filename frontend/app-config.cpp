#include "app-config.hpp"

#include <Windows.h>
#include <shlobj.h>

#include <cstring>
#include <filesystem>

#include <util/platform.h>

#include <QDir>

namespace AppConfig {

static obs_data_t *config = nullptr;
static std::string configPath;

static std::string GetAppDataDir()
{
	wchar_t path[MAX_PATH];
	if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) == S_OK) {
		std::wstring wpath(path);
		return std::string(wpath.begin(), wpath.end()) + "\\obs-lite";
	}
	return std::string("obs-lite");
}

static void EnsureDirs()
{
	std::string base = GetAppDataDir();
	os_mkdirs(base.c_str());
	os_mkdirs((base + "\\logs").c_str());

	const char *recordingDir = obs_data_get_string(config, "General.RecordingDir");
	if (!recordingDir || !*recordingDir) {
		obs_data_set_string(config, "General.RecordingDir", (base + "\\recordings").c_str());
	}
	os_mkdirs(obs_data_get_string(config, "General.RecordingDir"));

	const char *replayDir = obs_data_get_string(config, "Replay.ReplayDir");
	if (!replayDir || !*replayDir) {
		obs_data_set_string(config, "Replay.ReplayDir", (base + "\\replays").c_str());
	}
	os_mkdirs(obs_data_get_string(config, "Replay.ReplayDir"));
}

void Load()
{
	config = obs_data_create();

	/* defaults */
	obs_data_set_string(config, "General.RecordingDir", "");
	obs_data_set_string(config, "General.ReplayDir", "");
	obs_data_set_bool(config, "General.StartWithWindows", false);
	obs_data_set_bool(config, "General.MinimizeToTray", true);
	obs_data_set_bool(config, "General.StartMinimized", false);
	obs_data_set_bool(config, "General.StartWithPreview", true);

	obs_data_set_int(config, "Video.BaseWidth", 1920);
	obs_data_set_int(config, "Video.BaseHeight", 1080);
	obs_data_set_int(config, "Video.OutputWidth", 1920);
	obs_data_set_int(config, "Video.OutputHeight", 1080);
	obs_data_set_int(config, "Video.FPS", 60);
	obs_data_set_string(config, "Video.Encoder", "auto");
	obs_data_set_string(config, "Video.RateControl", "CBR");
	obs_data_set_int(config, "Video.Bitrate", 8000);
	obs_data_set_int(config, "Video.CQP", 20);
	obs_data_set_int(config, "Video.KeyintSec", 2);
	obs_data_set_string(config, "Video.Preset", "p5");

	obs_data_set_int(config, "Audio.Bitrate", 160);

	obs_data_set_int(config, "Replay.DurationSec", 60);
	obs_data_set_int(config, "Replay.MaxSizeMB", 2048);
	obs_data_set_string(config, "Replay.Extension", "mp4");
	obs_data_set_bool(config, "Replay.RemuxToMP4", true);

	obs_data_set_string(config, "Capture.GameWindow", "");
	obs_data_set_int(config, "Capture.GamePriority", 2); /* WINDOW_PRIORITY_EXE */
	obs_data_set_bool(config, "Capture.CaptureCursor", true);
	obs_data_set_string(config, "Capture.DisplayMonitor", "");
	obs_data_set_string(config, "Capture.AppAudioWindow", "");
	obs_data_set_bool(config, "Capture.AppAudioEnabled", true);

	obs_data_set_int(config, "Hotkey.StartRecording.VK", VK_F9);
	obs_data_set_int(config, "Hotkey.StartRecording.Mod", MOD_CONTROL | MOD_ALT);
	obs_data_set_int(config, "Hotkey.StopRecording.VK", VK_F9);
	obs_data_set_int(config, "Hotkey.StopRecording.Mod", MOD_CONTROL | MOD_ALT | MOD_SHIFT);
	obs_data_set_int(config, "Hotkey.SaveReplay.VK", VK_F10);
	obs_data_set_int(config, "Hotkey.SaveReplay.Mod", MOD_CONTROL | MOD_ALT);
	obs_data_set_int(config, "Hotkey.StartReplayBuffer.VK", VK_F11);
	obs_data_set_int(config, "Hotkey.StartReplayBuffer.Mod", MOD_CONTROL | MOD_ALT);
	obs_data_set_int(config, "Hotkey.StopReplayBuffer.VK", VK_F11);
	obs_data_set_int(config, "Hotkey.StopReplayBuffer.Mod", MOD_CONTROL | MOD_ALT | MOD_SHIFT);

	std::string path = GetAppDataDir() + "\\config.json";
	configPath = path;

	obs_data_t *loaded = obs_data_create_from_json_file_safe(path.c_str(), "bak");
	if (loaded) {
		obs_data_apply(config, loaded);
		obs_data_release(loaded);
	}

	EnsureDirs();
}

void Save()
{
	if (!config)
		return;
	obs_data_save_json_safe(config, configPath.c_str(), "tmp", "bak");
}

void ResetToDefaults()
{
	obs_data_release(config);
	config = obs_data_create();
	Load();
}

std::string GetRecordingDir()
{
	return obs_data_get_string(config, "General.RecordingDir");
}

std::string GetReplayDir()
{
	return obs_data_get_string(config, "Replay.ReplayDir");
}

std::string RecordingFormat()
{
	const char *format = obs_data_get_string(config, "General.RecordingFormat");
	return (format && *format) ? format : "mkv";
}

bool StartWithWindows()
{
	return obs_data_get_bool(config, "General.StartWithWindows");
}

void SetStartWithWindows(bool enabled)
{
	obs_data_set_bool(config, "General.StartWithWindows", enabled);

	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	HKEY key;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
			  L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE,
			  &key) == ERROR_SUCCESS) {
		if (enabled) {
			std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" -minimized";
			RegSetValueExW(key, L"obs-lite", 0, REG_SZ, (const BYTE *)cmd.c_str(),
				       (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
		} else {
			RegDeleteValueW(key, L"obs-lite");
		}
		RegCloseKey(key);
	}
}

bool MinimizeToTray()
{
	return obs_data_get_bool(config, "General.MinimizeToTray");
}

bool StartMinimized()
{
	return obs_data_get_bool(config, "General.StartMinimized");
}

bool StartWithPreview()
{
	return obs_data_get_bool(config, "General.StartWithPreview");
}

uint32_t GetBaseWidth()
{
	return (uint32_t)obs_data_get_int(config, "Video.BaseWidth");
}

uint32_t GetBaseHeight()
{
	return (uint32_t)obs_data_get_int(config, "Video.BaseHeight");
}

uint32_t GetOutputWidth()
{
	uint32_t w = (uint32_t)obs_data_get_int(config, "Video.OutputWidth");
	return w ? w : GetBaseWidth();
}

uint32_t GetOutputHeight()
{
	uint32_t h = (uint32_t)obs_data_get_int(config, "Video.OutputHeight");
	return h ? h : GetBaseHeight();
}

uint32_t GetFPSNum()
{
	return (uint32_t)obs_data_get_int(config, "Video.FPS");
}

uint32_t GetFPSDen()
{
	return 1;
}

/* Encoder resolution:
 *   "auto"  -> prefer NVENC texture encoder, fall back to software x264
 *   "nvenc" -> force NVENC H.264
 *   "x264"  -> force software x264
 */
static bool encoder_type_available(const char *id)
{
	const char *enc_id;
	size_t idx = 0;
	while (obs_enum_encoder_types(idx++, &enc_id)) {
		if (strcmp(enc_id, id) == 0)
			return true;
	}
	return false;
}

std::string GetEncoderId()
{
	const char *pref = obs_data_get_string(config, "Video.Encoder");

	if (strcmp(pref, "nvenc") == 0) {
		if (encoder_type_available("obs_nvenc_h264_tex"))
			return "obs_nvenc_h264_tex";
		if (encoder_type_available("obs_nvenc_h264_soft"))
			return "obs_nvenc_h264_soft";
		return "";
	}

	if (strcmp(pref, "x264") == 0) {
		return encoder_type_available("obs_x264") ? "obs_x264" : "";
	}

	/* auto */
	if (encoder_type_available("obs_nvenc_h264_tex"))
		return "obs_nvenc_h264_tex";
	if (encoder_type_available("obs_nvenc_h264_soft"))
		return "obs_nvenc_h264_soft";
	if (encoder_type_available("obs_x264"))
		return "obs_x264";
	return "";
}

bool UseCQP()
{
	const char *rc = obs_data_get_string(config, "Video.RateControl");
	return strcmp(rc, "CQP") == 0;
}

int BitrateKbps()
{
	return (int)obs_data_get_int(config, "Video.Bitrate");
}

int CQPValue()
{
	return (int)obs_data_get_int(config, "Video.CQP");
}

int KeyintSec()
{
	return (int)obs_data_get_int(config, "Video.KeyintSec");
}

std::string GetEncoderPreset()
{
	return obs_data_get_string(config, "Video.Preset");
}

int AudioBitrateKbps()
{
	return (int)obs_data_get_int(config, "Audio.Bitrate");
}

int ReplayDurationSec()
{
	return (int)obs_data_get_int(config, "Replay.DurationSec");
}

int ReplayMaxSizeMB()
{
	return (int)obs_data_get_int(config, "Replay.MaxSizeMB");
}

std::string ReplayExtension()
{
	return obs_data_get_string(config, "Replay.Extension");
}

bool RemuxToMP4()
{
	return obs_data_get_bool(config, "Replay.RemuxToMP4");
}

std::string GetGameCaptureWindow()
{
	return obs_data_get_string(config, "Capture.GameWindow");
}

int GetGameCapturePriority()
{
	return (int)obs_data_get_int(config, "Capture.GamePriority");
}

bool GetCaptureCursor()
{
	return obs_data_get_bool(config, "Capture.CaptureCursor");
}

std::string GetDisplayMonitorId()
{
	return obs_data_get_string(config, "Capture.DisplayMonitor");
}

std::string GetAppAudioWindow()
{
	return obs_data_get_string(config, "Capture.AppAudioWindow");
}

bool AppAudioEnabled()
{
	return obs_data_get_bool(config, "Capture.AppAudioEnabled");
}

int GetHotkeyVK(const char *name)
{
	std::string key = std::string(name) + ".VK";
	return (int)obs_data_get_int(config, key.c_str());
}

int GetHotkeyModifiers(const char *name)
{
	std::string key = std::string(name) + ".Mod";
	return (int)obs_data_get_int(config, key.c_str());
}

obs_data_t *Config()
{
	return config;
}

} // namespace AppConfig
