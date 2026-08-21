#include "obs-app.hpp"

#include <Windows.h>
#include <shlobj.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <functional>
#include <string>

#include <QApplication>
#include <QDir>
#include <QMetaObject>
#include <QStandardPaths>

#include <util/base.h>
#include <util/platform.h>

#include "app-config.hpp"

static OBSApp *instance = nullptr;

OBSApp *OBSApp::Get()
{
	return instance;
}

/* Tasks queued from libobs worker threads are marshalled onto the Qt event
 * loop, mirroring the upstream OBS frontend behavior. */
static void ui_task_handler(obs_task_t task, void *param, bool wait)
{
	auto doTask = [=]() { task(param); };

	if (wait) {
		QMetaObject::invokeMethod(qApp, doTask, Qt::BlockingQueuedConnection);
	} else {
		QMetaObject::invokeMethod(qApp, doTask, Qt::QueuedConnection);
	}
}

OBSApp::OBSApp()
{
	instance = this;
}

OBSApp::~OBSApp()
{
	Shutdown();
	instance = nullptr;
}

static std::string GetAppDataDir()
{
	wchar_t path[MAX_PATH];
	if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) == S_OK) {
		std::wstring wpath(path);
		return std::string(wpath.begin(), wpath.end()) + "\\obs-light";
	}
	return std::string("obs-light");
}

std::string OBSApp::GetAppDataPath()
{
	return GetAppDataDir();
}

/* ------------------------------------------------------------------------ */
/* Logging                                                                  */
/* ------------------------------------------------------------------------ */

static FILE *logFile = nullptr;

static void open_log_file()
{
	std::string dir = GetAppDataDir() + "\\logs";
	os_mkdirs(dir.c_str());

	time_t now = time(nullptr);
	struct tm tm_now;
	localtime_s(&tm_now, &now);

	char name[64];
	strftime(name, sizeof(name), "obs-light-%Y-%m-%d_%H-%M-%S.log", &tm_now);

	std::string path = dir + "\\" + name;
	fopen_s(&logFile, path.c_str(), "w");
}

static void log_handler(int log_level, const char *format, va_list args, void *param)
{
	UNUSED_PARAMETER(param);

	va_list args2;
	va_copy(args2, args);

	int len = vsnprintf(nullptr, 0, format, args);
	if (len > 0) {
		std::string msg(len + 1, '\0');
		vsnprintf(&msg[0], len + 1, format, args2);
		msg.resize(len);

		if (log_level <= LOG_INFO) {
			OutputDebugStringA(msg.c_str());
			OutputDebugStringA("\n");
		}

		if (logFile) {
			time_t now = time(nullptr);
			struct tm tm_now;
			localtime_s(&tm_now, &now);
			char ts[32];
			strftime(ts, sizeof(ts), "%H:%M:%S", &tm_now);
			fprintf(logFile, "[%s] %s\n", ts, msg.c_str());
			fflush(logFile);
		}
	}

	va_end(args2);
}

/* ------------------------------------------------------------------------ */
/* Startup                                                                  */
/* ------------------------------------------------------------------------ */

bool OBSApp::StartupObs()
{
	open_log_file();

	base_set_log_handler(log_handler, nullptr);

	if (!obs_startup("en-US", nullptr, nullptr)) {
		blog(LOG_ERROR, "obs_startup failed");
		return false;
	}

	libobsInitialized = true;
	blog(LOG_INFO, "obs-light %s (libobs %s)", obs_get_version_string(),
	     obs_get_version_string());
	return true;
}

bool OBSApp::LoadModules()
{
	obs_load_all_modules();
	obs_log_loaded_modules();
	obs_post_load_modules();
	return true;
}

bool OBSApp::InitAudio()
{
	struct obs_audio_info ai = {};
	ai.samples_per_sec = 48000;
	ai.speakers = SPEAKERS_STEREO;

	if (!obs_reset_audio(&ai)) {
		blog(LOG_ERROR, "obs_reset_audio failed");
		return false;
	}

	audioInitialized = true;
	return true;
}

bool OBSApp::ResetVideo()
{
	struct obs_video_info ovi = {};
	ovi.graphics_module = "libobs-d3d11";
	ovi.fps_num = AppConfig::GetFPSNum();
	ovi.fps_den = AppConfig::GetFPSDen();
	ovi.base_width = AppConfig::GetBaseWidth();
	ovi.base_height = AppConfig::GetBaseHeight();
	ovi.output_width = AppConfig::GetOutputWidth();
	ovi.output_height = AppConfig::GetOutputHeight();
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_709;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BILINEAR;

	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		blog(LOG_ERROR, "obs_reset_video failed (%d)", ret);
		return false;
	}

	videoInitialized = true;
	return true;
}

bool OBSApp::ResetAudio()
{
	return InitAudio();
}

struct obs_video_info OBSApp::GetVideoInfo()
{
	struct obs_video_info ovi = {};
	obs_get_video_info(&ovi);
	return ovi;
}

bool OBSApp::Initialize()
{
	configPath = GetAppDataDir() + "\\config.json";

	if (!StartupObs())
		return false;
	if (!LoadModules())
		return false;

	obs_set_ui_task_handler(ui_task_handler);

	AppConfig::Load();

	if (!InitAudio())
		return false;
	if (!ResetVideo())
		return false;

	blog(LOG_INFO, "obs-light initialized successfully");
	return true;
}

void OBSApp::Shutdown()
{
	if (!instance)
		return;

	if (libobsInitialized) {
		obs_shutdown();
		libobsInitialized = false;
	}

	if (logFile) {
		fclose(logFile);
		logFile = nullptr;
	}
}

/* ------------------------------------------------------------------------ */
/* CI smoke test                                                            */
/* ------------------------------------------------------------------------ */

int OBSApp::RunSmokeTest()
{
	open_log_file();
	base_set_log_handler(log_handler, nullptr);

	blog(LOG_INFO, "obs-light smoke test starting");

	if (!obs_startup("en-US", nullptr, nullptr)) {
		blog(LOG_ERROR, "SMOKE TEST FAILED: obs_startup");
		return 1;
	}

	obs_load_all_modules();
	obs_log_loaded_modules();
	obs_post_load_modules();

	/* Required source types */
	static const char *required_sources[] = {
		"game_capture",
		"monitor_capture",
		"wasapi_process_output_capture",
	};
	int failures = 0;

	for (const char *id : required_sources) {
		bool found = false;
		const char *source_id;
		size_t idx = 0;
		while (obs_enum_source_types(idx++, &source_id)) {
			if (strcmp(source_id, id) == 0) {
				found = true;
				break;
			}
		}
		blog(LOG_INFO, "smoke test source '%s': %s", id, found ? "OK" : "MISSING");
		if (!found)
			failures++;
	}

	/* Required encoder types (NVENC encoders only register when an NVIDIA
	 * driver is present, so they are verified separately below). */
	static const char *required_encoders[] = {
		"obs_x264",
		"ffmpeg_aac",
	};
	for (const char *id : required_encoders) {
		bool found = false;
		const char *enc_id;
		size_t idx = 0;
		while (obs_enum_encoder_types(idx++, &enc_id)) {
			if (strcmp(enc_id, id) == 0) {
				found = true;
				break;
			}
		}
		blog(LOG_INFO, "smoke test encoder '%s': %s", id, found ? "OK" : "MISSING");
		if (!found)
			failures++;
	}

	/* NVENC: required on machines with an NVIDIA GPU, expected to be absent
	 * on GPU-less CI runners (obs-nvenc's module load requires the driver). */
	static const char *nvenc_encoders[] = {
		"obs_nvenc_h264_tex",
		"obs_nvenc_h264_soft",
	};
	bool nvenc_found = false;
	for (const char *id : nvenc_encoders) {
		const char *enc_id;
		size_t idx = 0;
		while (obs_enum_encoder_types(idx++, &enc_id)) {
			if (strcmp(enc_id, id) == 0) {
				nvenc_found = true;
				break;
			}
		}
	}
	if (!nvenc_found) {
		blog(LOG_INFO, "smoke test NVENC encoders: SKIPPED (no NVIDIA GPU/driver)");
	} else {
		for (const char *id : nvenc_encoders) {
			bool found = false;
			const char *enc_id;
			size_t idx = 0;
			while (obs_enum_encoder_types(idx++, &enc_id)) {
				if (strcmp(enc_id, id) == 0) {
					found = true;
					break;
				}
			}
			blog(LOG_INFO, "smoke test encoder '%s': %s", id, found ? "OK" : "MISSING");
			if (!found)
				failures++;
		}
	}

	/* Required output types */
	static const char *required_outputs[] = {
		"ffmpeg_muxer",
		"replay_buffer",
	};
	for (const char *id : required_outputs) {
		bool found = false;
		const char *out_id;
		size_t idx = 0;
		while (obs_enum_output_types(idx++, &out_id)) {
			if (strcmp(out_id, id) == 0) {
				found = true;
				break;
			}
		}
		blog(LOG_INFO, "smoke test output '%s': %s", id, found ? "OK" : "MISSING");
		if (!found)
			failures++;
	}

	blog(LOG_INFO, "smoke test %s (%d failures)",
	     failures == 0 ? "PASSED" : "FAILED", failures);

	/* IMPORTANT: obs_shutdown() is intentionally NOT called here.
	 *
	 * win-capture spawns a background thread during module load which runs
	 * get-graphics-offsets64.exe; that helper creates a D3D9/D3D10 device,
	 * which never completes on GPU-less CI runners. obs_shutdown() then
	 * blocks forever waiting for that thread in obs_module_unload.
	 * The smoke test verifies startup and module registration only, so the
	 * process exits directly; the hung helper dies with it and the runner
	 * cleans up orphaned processes at job end. */
	if (logFile)
		fflush(logFile);
	ExitProcess(failures == 0 ? 0 : 1);
	return failures == 0 ? 0 : 1;
}
