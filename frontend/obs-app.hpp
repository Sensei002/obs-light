#pragma once

#include <string>

#include <obs.h>

class QApplication;

/* Owns the libobs runtime for obs-light: startup, module loading, audio and
 * video setup, logging and shutdown.  The single instance is created in
 * main() and torn down after the Qt event loop finishes. */
class OBSApp {
public:
	OBSApp();
	~OBSApp();

	OBSApp(const OBSApp &) = delete;
	OBSApp &operator=(const OBSApp &) = delete;

	static OBSApp *Get();

	/* Full application initialization (UI build runs after this succeeds). */
	bool Initialize();
	void Shutdown();

	bool Initialized() const { return libobsInitialized; }

	/* Headless smoke test used by CI: starts libobs, loads the bundled
	 * plugins and verifies the required source/encoder/output types exist.
	 * Returns a process exit code (0 = success). */
	int RunSmokeTest();

	/* Video/audio helpers */
	bool ResetVideo();
	bool ResetAudio();
	struct obs_video_info GetVideoInfo();

	const std::string &ConfigFilePath() const { return configPath; }

private:
	bool StartupObs();
	bool LoadModules();
	bool InitAudio();
	bool InitVideo();
	std::string GetAppDataPath();

	bool libobsInitialized = false;
	bool videoInitialized = false;
	bool audioInitialized = false;
	std::string configPath;
};
