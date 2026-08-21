#pragma once

#include <string>

#include <QObject>

#include <obs.h>

class Recorder : public QObject {
	Q_OBJECT

public:
	Recorder() = default;
	~Recorder() override;

	Recorder(const Recorder &) = delete;
	Recorder &operator=(const Recorder &) = delete;

	bool Initialize();
	void Shutdown();

	bool StartRecording();
	void StopRecording();
	bool RecordingActive() const;
	std::string LastRecordingFile() const { return recordingPath; }

	bool StartReplayBuffer();
	void StopReplayBuffer();
	bool ReplayBufferActive() const;
	void SaveReplay();
	bool ReplaySaveInProgress() const { return replaySaving; }

	std::string EncoderId() const;
	std::string RecordingExtension() const;

	/* Stats */
	int GetDroppedFrames() const;
	int GetTotalFrames() const;
	uint64_t GetTotalBytes() const;

signals:
	void recordingStarted();
	void recordingStopped(int code);
	void replayBufferStarted();
	void replayBufferStopped(int code);
	void replaySaved();
	void errorOccurred(const QString &message);

private:
	void DestroyOutputs();
	bool CreateEncoders(obs_encoder_t **videoEncoderOut,
			    obs_encoder_t **audioEncoderOut);
	obs_data_t *BuildVideoEncoderSettings() const;
	std::string BuildOutputPath() const;

	/* Signal handlers (C callbacks) */
	static void OnRecordStart(void *data, calldata_t *cd);
	static void OnRecordStop(void *data, calldata_t *cd);
	static void OnReplayStart(void *data, calldata_t *cd);
	static void OnReplayStop(void *data, calldata_t *cd);
	static void OnReplaySaved(void *data, calldata_t *cd);

	obs_output_t *recordOutput = nullptr;
	obs_output_t *replayOutput = nullptr;
	std::string recordingPath;
	bool replaySaving = false;
	bool initialized = false;
};