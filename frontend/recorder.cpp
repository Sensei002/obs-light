#include "recorder.hpp"

#include <ctime>

#include <util/platform.h>

#include "app-config.hpp"

/* ------------------------------------------------------------------------ */
/* Signal handlers (called from libobs worker threads; must not touch Qt UI
 * directly - the corresponding Qt signals are emitted via the queued
 * connection through the QObject machinery). */

static int signal_code(calldata_t *cd)
{
	return (int)calldata_int(cd, "code");
}

void Recorder::OnRecordStart(void *data, calldata_t *)
{
	Recorder *recorder = static_cast<Recorder *>(data);
	blog(LOG_INFO, "recording started");
	emit recorder->recordingStarted();
}

void Recorder::OnRecordStop(void *data, calldata_t *cd)
{
	Recorder *recorder = static_cast<Recorder *>(data);
	int code = signal_code(cd);
	blog(LOG_INFO, "recording stopped (code %d)", code);
	if (code != OBS_OUTPUT_SUCCESS && code != OBS_OUTPUT_STOPPED) {
		const char *err = obs_output_get_last_error(recorder->recordOutput);
		emit recorder->errorOccurred(
			QString("Recording stopped with an error: %1")
				.arg(err ? err : "unknown error"));
	}
	emit recorder->recordingStopped(code);
}

void Recorder::OnReplayStart(void *data, calldata_t *)
{
	Recorder *recorder = static_cast<Recorder *>(data);
	blog(LOG_INFO, "replay buffer started");
	emit recorder->replayBufferStarted();
}

void Recorder::OnReplayStop(void *data, calldata_t *cd)
{
	Recorder *recorder = static_cast<Recorder *>(data);
	int code = signal_code(cd);
	blog(LOG_INFO, "replay buffer stopped (code %d)", code);
	if (code != OBS_OUTPUT_SUCCESS && code != OBS_OUTPUT_STOPPED) {
		const char *err = obs_output_get_last_error(recorder->replayOutput);
		emit recorder->errorOccurred(
			QString("Replay buffer stopped with an error: %1")
				.arg(err ? err : "unknown error"));
	}
	emit recorder->replayBufferStopped(code);
}

void Recorder::OnReplaySaved(void *data, calldata_t *)
{
	Recorder *recorder = static_cast<Recorder *>(data);
	recorder->replaySaving = false;
	blog(LOG_INFO, "replay saved");
	emit recorder->replaySaved();
}

/* ------------------------------------------------------------------------ */

Recorder::~Recorder()
{
	Shutdown();
}

bool Recorder::Initialize()
{
	if (initialized)
		return true;

	initialized = true;
	blog(LOG_INFO, "recorder initialized (encoder: %s)",
	     EncoderId().empty() ? "none available" : EncoderId().c_str());
	return true;
}

void Recorder::Shutdown()
{
	DestroyOutputs();
	initialized = false;
}

void Recorder::DestroyOutputs()
{
	if (recordOutput) {
		obs_output_force_stop(recordOutput);
		obs_output_release(recordOutput);
		recordOutput = nullptr;
	}
	if (replayOutput) {
		obs_output_force_stop(replayOutput);
		obs_output_release(replayOutput);
		replayOutput = nullptr;
	}
	recordingPath.clear();
	replaySaving = false;
}

std::string Recorder::EncoderId() const
{
	return AppConfig::GetEncoderId();
}

std::string Recorder::RecordingExtension() const
{
	return AppConfig::RemuxToMP4() ? "mkv" : "mkv";
}

obs_data_t *Recorder::BuildVideoEncoderSettings() const
{
	obs_data_t *settings = obs_data_create();

	std::string id = AppConfig::GetEncoderId();

	if (id == "obs_nvenc_h264_tex" || id == "obs_nvenc_h264_soft") {
		/* NVENC */
		obs_data_set_int(settings, "keyint_sec", AppConfig::KeyintSec());
		obs_data_set_string(settings, "preset",
				    AppConfig::GetEncoderPreset().c_str());
		obs_data_set_string(settings, "profile", "high");
		if (AppConfig::UseCQP()) {
			obs_data_set_string(settings, "rate_control", "CQP");
			obs_data_set_int(settings, "cqp", AppConfig::CQPValue());
		} else {
			obs_data_set_string(settings, "rate_control", "CBR");
			obs_data_set_int(settings, "bitrate",
					 AppConfig::BitrateKbps());
			obs_data_set_int(settings, "max_bitrate",
					 AppConfig::BitrateKbps());
		}
	} else {
		/* x264 software fallback */
		obs_data_set_string(settings, "preset", "veryfast");
		obs_data_set_int(settings, "keyint_sec", AppConfig::KeyintSec());
		if (AppConfig::UseCQP()) {
			obs_data_set_string(settings, "rate_control", "CRF");
			obs_data_set_int(settings, "crf", AppConfig::CQPValue());
		} else {
			obs_data_set_string(settings, "rate_control", "CBR");
			obs_data_set_int(settings, "bitrate",
					 AppConfig::BitrateKbps());
			obs_data_set_int(settings, "buffer_size",
					 AppConfig::BitrateKbps());
		}
	}

	return settings;
}

bool Recorder::CreateEncoders(obs_encoder_t **videoEncoderOut,
			      obs_encoder_t **audioEncoderOut)
{
	std::string videoId = EncoderId();
	if (videoId.empty()) {
		emit errorOccurred(
			"No video encoder available. Install an NVIDIA driver or "
			"switch to the software x264 encoder in Settings.");
		return false;
	}

	obs_data_t *videoSettings = BuildVideoEncoderSettings();
	*videoEncoderOut = obs_video_encoder_create(
		videoId.c_str(), "obs-light video", videoSettings, nullptr);
	obs_data_release(videoSettings);

	if (!*videoEncoderOut) {
		emit errorOccurred(QString("Failed to create the '%1' encoder.")
					   .arg(videoId.c_str()));
		return false;
	}

	obs_data_t *audioSettings = obs_data_create();
	obs_data_set_int(audioSettings, "bitrate", AppConfig::AudioBitrateKbps());
	*audioEncoderOut = obs_audio_encoder_create(
		"ffmpeg_aac", "obs-light audio", audioSettings, 0, nullptr);
	obs_data_release(audioSettings);

	if (!*audioEncoderOut) {
		obs_encoder_release(*videoEncoderOut);
		*videoEncoderOut = nullptr;
		emit errorOccurred("Failed to create the AAC audio encoder.");
		return false;
	}

	return true;
}

std::string Recorder::BuildOutputPath() const
{
	time_t now = time(nullptr);
	struct tm tm_now;
	localtime_s(&tm_now, &now);

	char name[128];
	strftime(name, sizeof(name), "Recording_%Y-%m-%d_%H-%M-%S", &tm_now);

	std::string dir = AppConfig::GetRecordingDir();
	os_mkdirs(dir.c_str());

	return dir + "\\" + name + "." + RecordingExtension();
}

bool Recorder::StartRecording()
{
	if (RecordingActive())
		return false;

	obs_encoder_t *videoEncoder = nullptr;
	obs_encoder_t *audioEncoder = nullptr;
	if (!CreateEncoders(&videoEncoder, &audioEncoder))
		return false;

	recordingPath = BuildOutputPath();

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "path", recordingPath.c_str());

	recordOutput = obs_output_create("ffmpeg_muxer", "Recording", settings,
					 nullptr);
	obs_data_release(settings);

	if (!recordOutput) {
		obs_encoder_release(videoEncoder);
		obs_encoder_release(audioEncoder);
		recordingPath.clear();
		emit errorOccurred("Failed to create the recording output.");
		return false;
	}

	obs_output_set_video_encoder(recordOutput, videoEncoder);
	obs_output_set_audio_encoder(recordOutput, audioEncoder, 0);
	obs_encoder_release(videoEncoder);
	obs_encoder_release(audioEncoder);

	signal_handler_t *sh = obs_output_get_signal_handler(recordOutput);
	signal_handler_connect(sh, "start", OnRecordStart, this);
	signal_handler_connect(sh, "stop", OnRecordStop, this);

	if (!obs_output_start(recordOutput)) {
		const char *err = obs_output_get_last_error(recordOutput);
		emit errorOccurred(QString("Failed to start recording: %1")
					   .arg(err ? err : "output start failed"));
		obs_output_force_stop(recordOutput);
		return false;
	}

	blog(LOG_INFO, "recording started: %s", recordingPath.c_str());
	return true;
}

void Recorder::StopRecording()
{
	if (!RecordingActive())
		return;
	obs_output_stop(recordOutput);
}

bool Recorder::RecordingActive() const
{
	return recordOutput && obs_output_active(recordOutput);
}

bool Recorder::StartReplayBuffer()
{
	if (ReplayBufferActive())
		return false;

	obs_encoder_t *videoEncoder = nullptr;
	obs_encoder_t *audioEncoder = nullptr;
	if (!CreateEncoders(&videoEncoder, &audioEncoder))
		return false;

	obs_data_t *settings = obs_data_create();
	obs_data_set_int(settings, "max_time_sec", AppConfig::ReplayDurationSec());
	obs_data_set_int(settings, "max_size_mb", AppConfig::ReplayMaxSizeMB());
	obs_data_set_string(settings, "directory",
			    AppConfig::GetReplayDir().c_str());
	obs_data_set_string(settings, "format",
			    "Replay_%CCYY-%MM-%DD_%hh-%mm-%ss");
	obs_data_set_string(settings, "extension",
			    AppConfig::ReplayExtension().c_str());
	obs_data_set_bool(settings, "allow_spaces", false);

	replayOutput = obs_output_create("replay_buffer", "Replay Buffer",
					 settings, nullptr);
	obs_data_release(settings);

	if (!replayOutput) {
		obs_encoder_release(videoEncoder);
		obs_encoder_release(audioEncoder);
		emit errorOccurred("Failed to create the replay buffer output.");
		return false;
	}

	obs_output_set_video_encoder(replayOutput, videoEncoder);
	obs_output_set_audio_encoder(replayOutput, audioEncoder, 0);
	obs_encoder_release(videoEncoder);
	obs_encoder_release(audioEncoder);

	signal_handler_t *sh = obs_output_get_signal_handler(replayOutput);
	signal_handler_connect(sh, "start", OnReplayStart, this);
	signal_handler_connect(sh, "stop", OnReplayStop, this);
	signal_handler_connect(sh, "saved", OnReplaySaved, this);

	if (!obs_output_start(replayOutput)) {
		const char *err = obs_output_get_last_error(replayOutput);
		emit errorOccurred(QString("Failed to start the replay buffer: %1")
					   .arg(err ? err : "output start failed"));
		obs_output_force_stop(replayOutput);
		return false;
	}

	blog(LOG_INFO, "replay buffer started (%d sec)",
	     AppConfig::ReplayDurationSec());
	return true;
}

void Recorder::StopReplayBuffer()
{
	if (!ReplayBufferActive())
		return;
	obs_output_stop(replayOutput);
}

bool Recorder::ReplayBufferActive() const
{
	return replayOutput && obs_output_active(replayOutput);
}

void Recorder::SaveReplay()
{
	if (!ReplayBufferActive()) {
		emit errorOccurred("The replay buffer is not running.");
		return;
	}
	if (replaySaving) {
		emit errorOccurred("A replay save is already in progress.");
		return;
	}

	replaySaving = true;
	proc_handler_t *ph = obs_output_get_proc_handler(replayOutput);
	proc_handler_call(ph, "save", nullptr);
	/* OnReplaySaved clears replaySaving when muxing completes */
}

int Recorder::GetDroppedFrames() const
{
	if (!recordOutput)
		return 0;
	return obs_output_get_frames_dropped(recordOutput);
}

int Recorder::GetTotalFrames() const
{
	if (!recordOutput)
		return 0;
	return obs_output_get_total_frames(recordOutput);
}

uint64_t Recorder::GetTotalBytes() const
{
	if (!recordOutput)
		return 0;
	return obs_output_get_total_bytes(recordOutput);
}
