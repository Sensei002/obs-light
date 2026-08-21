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

	RebuildSceneItems();

	SetAppAudioWindow(AppConfig::GetAppAudioWindow());
	SetAppAudioEnabled(AppConfig::AppAudioEnabled());
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

	obs_data_t *audioSettings = obs_data_create();
	appAudio = obs_source_create("wasapi_process_output_capture",
				     "Application Audio", audioSettings,
				     nullptr);
	obs_data_release(audioSettings);
}

void CaptureManager::DestroySources()
{
	if (gameCapture) {
		obs_source_release(gameCapture);
		gameCapture = nullptr;
	}
	if (displayCapture) {
		obs_source_release(displayCapture);
		displayCapture = nullptr;
	}
	if (appAudio) {
		obs_source_release(appAudio);
		appAudio = nullptr;
	}
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

	if (gameCapture)
		gameItem = obs_scene_add(scene, gameCapture);
	if (displayCapture)
		displayItem = obs_scene_add(scene, displayCapture);

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
	std::string monitor = std::to_string(obs_data_get_int(settings, "monitor"));
	obs_data_release(settings);
	return monitor;
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
