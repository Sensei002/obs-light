#include "capture-manager.hpp"

#include <cstdlib>
#include <cstring>

#include <util/windows/window-helpers.h>

#include "app-config.hpp"

/* The OBS window string format is "<title>:<class>:<exe>" with ':' and '#'
 * escaped as "#3A" and "#22" (see libobs/util/windows/window-helpers.c).
 * ms_fill_window_list builds the encoded strings for us. */

CaptureManager::~CaptureManager()
{
	Shutdown();
}

bool CaptureManager::Initialize()
{
	if (initialized)
		return true;

	CreateSources();

	scene = obs_scene_create("Scene");
	if (!scene) {
		blog(LOG_ERROR, "failed to create scene");
		DestroySources();
		return false;
	}

	/* Attach the scene to the main canvas channel 0 */
	obs_set_output_source(0, obs_scene_get_source(scene));

	/* Add all sources (video and audio) to the scene as visible items.
	 * Audio-only sources render nothing visually but must be in the scene
	 * to become active and start capturing. */
	RebuildSceneItems();

	/* Restore source settings from config */
	SetAppAudioWindow(AppConfig::GetAppAudioWindow());
	SetAppAudioEnabled(AppConfig::AppAudioEnabled());
	SetDesktopAudioEnabled(true);
	SetDesktopAudioDevice("default");
	SetMicDevice(AppConfig::GetMicDevice());
	SetMicEnabled(true);
	SetGameCaptureWindow(AppConfig::GetGameCaptureWindow(),
			     AppConfig::GetGameCapturePriority());
	SetDisplayMonitor(AppConfig::GetDisplayMonitorId());

	initialized = true;
	blog(LOG_INFO, "capture manager initialized (single scene)");
	return true;
}

void CaptureManager::Shutdown()
{
	if (!initialized)
		return;

	obs_set_output_source(0, nullptr);

	if (scene) {
		obs_scene_release(scene);
		scene = nullptr;
	}

	DestroySources();
	initialized = false;
}

void CaptureManager::CreateSources()
{
	obs_data_t *gameSettings = obs_data_create();
	obs_data_set_string(gameSettings, "capture_mode", "any_fullscreen");
	obs_data_set_bool(gameSettings, "capture_cursor",
			  AppConfig::GetCaptureCursor());
	gameCapture = obs_source_create("game_capture", "Game Capture",
					gameSettings, nullptr);
	obs_data_release(gameSettings);

	obs_data_t *displaySettings = obs_data_create();
	displayCapture = obs_source_create("monitor_capture",
					   "Display Capture", displaySettings,
					   nullptr);
	obs_data_release(displaySettings);

	/* Desktop audio (wasapi_output_capture) → Track 1 */
	obs_data_t *desktopSettings = obs_data_create();
	desktopAudio = obs_source_create("wasapi_output_capture",
					 "Desktop Audio", desktopSettings,
					 nullptr);
	obs_data_release(desktopSettings);
	obs_source_set_audio_mixers(desktopAudio, 1 << 0);

	/* Microphone (wasapi_input_capture) → Track 2 */
	obs_data_t *micSettings = obs_data_create();
	micCapture = obs_source_create("wasapi_input_capture",
				       "Microphone", micSettings,
				       nullptr);
	obs_data_release(micSettings);
	obs_source_set_audio_mixers(micCapture, 1 << 1);

	/* Application audio capture (wasapi_process_output_capture) → Track 3 */
	obs_data_t *audioSettings = obs_data_create();
	appAudio = obs_source_create("wasapi_process_output_capture",
				     "Application Audio", audioSettings,
				     nullptr);
	obs_data_release(audioSettings);
	obs_source_set_audio_mixers(appAudio, 1 << 2);
}

void CaptureManager::DestroySources()
{
	auto release = [](obs_source_t *&s) {
		if (s) { obs_source_release(s); s = nullptr; }
	};
	release(gameCapture);
	release(displayCapture);
	release(desktopAudio);
	release(micCapture);
	release(appAudio);
	gameItem = nullptr;
	displayItem = nullptr;
}

void CaptureManager::RebuildSceneItems()
{
	if (!scene)
		return;

	if (gameItem) {
		obs_sceneitem_remove(gameItem);
		gameItem = nullptr;
	}
	if (displayItem) {
		obs_sceneitem_remove(displayItem);
		displayItem = nullptr;
	}

	obs_sceneitem_t *item;

	if (gameCapture)
		gameItem = obs_scene_add(scene, gameCapture);
	if (displayCapture)
		displayItem = obs_scene_add(scene, displayCapture);

	/* Audio sources — added as visible items so they become active.
	 * They have no video output so nothing renders. */
	if (desktopAudio) {
		item = obs_scene_find_source(scene, "Desktop Audio");
		if (!item)
			obs_scene_add(scene, desktopAudio);
	}
	if (micCapture) {
			item = obs_scene_find_source(scene, "Microphone");
		if (!item)
			obs_scene_add(scene, micCapture);
	}
	if (appAudio) {
		item = obs_scene_find_source(scene, "Application Audio");
		if (!item)
			obs_scene_add(scene, appAudio);
	}

	SetVideoSource(videoSource);
}

void CaptureManager::SetVideoSource(VideoSource source)
{
	videoSource = source;
	if (gameItem)
		obs_sceneitem_set_visible(gameItem,
					  source == VideoSource::GameCapture);
	if (displayItem)
		obs_sceneitem_set_visible(displayItem,
					  source == VideoSource::DisplayCapture);
	blog(LOG_INFO, "video source set to %s",
	     source == VideoSource::GameCapture ? "game capture"
	     : source == VideoSource::DisplayCapture ? "display capture"
						     : "none");
}

void CaptureManager::SetGameCaptureWindow(const std::string &window,
					  int priority)
{
	if (!gameCapture)
		return;

	obs_data_t *settings = obs_source_get_settings(gameCapture);
	obs_data_set_string(settings, "window", window.c_str());
	obs_data_set_int(settings, "priority", priority);
	obs_source_update(gameCapture, settings);
	obs_data_release(settings);
}

std::string CaptureManager::GetGameCaptureWindow() const
{
	if (!gameCapture)
		return "";
	obs_data_t *settings = obs_source_get_settings(gameCapture);
	std::string window = obs_data_get_string(settings, "window");
	obs_data_release(settings);
	return window;
}

void CaptureManager::SetDisplayMonitor(const std::string &monitorId)
{
	if (!displayCapture)
		return;

	obs_data_t *settings = obs_source_get_settings(displayCapture);
	obs_data_set_string(settings, "monitor_id", monitorId.c_str());
	obs_data_set_int(settings, "monitor",
			 monitorId.empty() ? 0 : atoi(monitorId.c_str()));
	obs_source_update(displayCapture, settings);
	obs_data_release(settings);
}

std::string CaptureManager::GetDisplayMonitorId() const
{
	if (!displayCapture)
	return "";
	obs_data_t *settings = obs_source_get_settings(displayCapture);
	std::string monitor = obs_data_get_string(settings, "monitor_id");
	obs_data_release(settings);
	return monitor;
}

void CaptureManager::SetDesktopAudioDevice(const std::string &deviceId)
{
	if (!desktopAudio)
		return;
	obs_data_t *settings = obs_source_get_settings(desktopAudio);
	obs_data_set_string(settings, "device_id", deviceId.c_str());
	obs_source_update(desktopAudio, settings);
	obs_data_release(settings);
}

void CaptureManager::SetDesktopAudioEnabled(bool enabled)
{
	desktopAudioEnabled = enabled;
	if (!desktopAudio)
		return;
	obs_source_set_audio_active(desktopAudio, enabled);
}

void CaptureManager::SetMicDevice(const std::string &deviceId)
{
	if (!micCapture)
		return;
	obs_data_t *settings = obs_source_get_settings(micCapture);
	obs_data_set_string(settings, "device_id", deviceId.c_str());
	obs_source_update(micCapture, settings);
	obs_data_release(settings);
	obs_data_set_string(AppConfig::Config(), "Capture.MicDevice",
			    deviceId.c_str());
	AppConfig::Save();
}

std::string CaptureManager::GetMicDevice() const
{
	return obs_data_get_string(AppConfig::Config(), "Capture.MicDevice");
}

void CaptureManager::SetMicEnabled(bool enabled)
{
	micEnabled = enabled;
	if (!micCapture)
		return;
	obs_source_set_audio_active(micCapture, enabled);
}

void CaptureManager::SetAppAudioWindow(const std::string &window)
{
	if (!appAudio)
		return;

	obs_data_t *settings = obs_source_get_settings(appAudio);
	obs_data_set_string(settings, "window", window.c_str());
	obs_data_set_int(settings, "priority", WINDOW_PRIORITY_TITLE);
	obs_source_update(appAudio, settings);
	obs_data_release(settings);
}

std::string CaptureManager::GetAppAudioWindow() const
{
	if (!appAudio)
		return "";
	obs_data_t *settings = obs_source_get_settings(appAudio);
	std::string window = obs_data_get_string(settings, "window");
	obs_data_release(settings);
	return window;
}

void CaptureManager::SetAppAudioEnabled(bool enabled)
{
	appAudioEnabled = enabled;
	if (!appAudio)
		return;

	obs_source_set_audio_active(appAudio, enabled);
	blog(LOG_INFO, "application audio capture %s",
	     enabled ? "enabled" : "disabled");
}

/* ------------------------------------------------------------------------ */
/* Window enumeration                                                       */
/* ------------------------------------------------------------------------ */

std::vector<CaptureManager::WindowEntry> CaptureManager::EnumerateWindows(
	bool includeMinimized)
{
	std::vector<WindowEntry> entries;

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list = obs_properties_add_list(
		props, "window", "Window", OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);

	ms_fill_window_list(list,
			    includeMinimized ? INCLUDE_MINIMIZED
					     : EXCLUDE_MINIMIZED,
			    nullptr);

	size_t count = obs_property_list_item_count(list);
	for (size_t i = 0; i < count; i++) {
		WindowEntry entry;
		entry.name = obs_property_list_item_name(list, i);
		entry.value = obs_property_list_item_string(list, i);
		entries.push_back(std::move(entry));
	}

	obs_properties_destroy(props);
	return entries;
}

/* Audio device enumeration by creating a temporary source and reading its
 * device list property (the same pattern as EnumerateWindows). */
std::vector<CaptureManager::AudioDeviceEntry> CaptureManager::EnumerateAudioDevices(
	bool input)
{
std::vector<AudioDeviceEntry> entries;

	obs_data_t *settings = obs_data_create();
	const char *sourceId = input ? "wasapi_input_capture"
				     : "wasapi_output_capture";
	/* Create a temporary source just to get the property list. */
	obs_source_t *tmp = obs_source_create(sourceId, "_tmp_audio_enum",
					      settings, nullptr);
	obs_data_release(settings);

	if (!tmp)
		return entries;

	obs_properties_t *props = obs_source_properties(tmp);
	if (props) {
		obs_property_t *list = obs_properties_get(props, "device_id");
		if (list && obs_property_list_type(list) == OBS_COMBO_TYPE_LIST) {
			size_t count = obs_property_list_item_count(list);
			for (size_t i = 0; i < count; i++) {
				AudioDeviceEntry entry;
				entry.name = obs_property_list_item_name(list, i);
				entry.id = obs_property_list_item_string(list, i);
				entries.push_back(std::move(entry));
			}
		}
		obs_properties_destroy(props);
	}

	obs_source_release(tmp);
	return entries;
}