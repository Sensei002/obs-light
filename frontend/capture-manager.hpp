#pragma once

#include <string>
#include <vector>

#include <obs.h>

/* Manages the single obs-lite scene and the three supported source types:
 * game capture, display capture and application audio capture.
 *
 * Exactly one scene exists and is attached to the main canvas channel 0.
 * The visible video source is either the game capture or the display
 * capture source; switching just toggles scene item visibility.
 */
class CaptureManager {
public:
	CaptureManager() = default;
	~CaptureManager();

	CaptureManager(const CaptureManager &) = delete;
	CaptureManager &operator=(const CaptureManager &) = delete;

	bool Initialize();
	void Shutdown();

	enum class VideoSource { None, GameCapture, DisplayCapture };

	VideoSource GetVideoSource() const { return videoSource; }
	void SetVideoSource(VideoSource source);

	/* Game capture window selection (OBS window string: title:class:exe) */
	void SetGameCaptureWindow(const std::string &window, int priority);
	std::string GetGameCaptureWindow() const;

	/* Display capture monitor selection ("0".."N", empty = primary) */
	void SetDisplayMonitor(const std::string &monitorId);
	std::string GetDisplayMonitorId() const;

	/* Application audio capture */
	void SetAppAudioWindow(const std::string &window);
	std::string GetAppAudioWindow() const;
	void SetAppAudioEnabled(bool enabled);

	/* Refresh the window lists used by the UI pickers. */
	struct WindowEntry {
		std::string name;
		std::string value; /* encoded OBS window string */
	};
	static std::vector<WindowEntry> EnumerateWindows(bool includeMinimized);

	obs_source_t *GetGameCaptureSource() const { return gameCapture; }
	obs_source_t *GetDisplayCaptureSource() const { return displayCapture; }

private:
	void CreateSources();
	void DestroySources();
	void RebuildSceneItems();

	obs_scene_t *scene = nullptr;
	obs_source_t *gameCapture = nullptr;
	obs_source_t *displayCapture = nullptr;
	obs_source_t *appAudio = nullptr;
	obs_sceneitem_t *gameItem = nullptr;
	obs_sceneitem_t *displayItem = nullptr;

	VideoSource videoSource = VideoSource::GameCapture;
	bool appAudioEnabled = true;
	bool initialized = false;
};
