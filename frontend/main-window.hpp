#pragma once

#include <Windows.h>

#include <QElapsedTimer>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include <obs.h>

class QComboBox;
class QLabel;
class QPushButton;
class QTimer;
class PreviewWidget;
class CaptureManager;
class Recorder;
class Hotkeys;
class SettingsDialog;

/* obs-lite main window: preview, capture selectors, record/replay controls,
 * status line and system tray integration. */
class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

	static MainWindow *Instance();

	/* Called from libobs worker threads via Recorder signals (queued) */
	void OnOutputSignal(const char *signal, obs_output_t *output);

	static void ShowError(const QString &message);
	static void ShowInfo(const QString &message);

	bool nativeEvent(const QByteArray &eventType, void *message,
			 qintptr *result) override;

protected:
	void closeEvent(QCloseEvent *event) override;
	void changeEvent(QEvent *event) override;

private slots:
	void OnStartRecording();
	void OnStopRecording();
	void OnSaveReplay();
	void OnStartReplayBuffer();
	void OnStopReplayBuffer();
	void OnOpenSettings();
	void OnSettingsApplied();
	void OnStatsTimer();
	void OnCaptureSourceChanged(int index);
	void OnGameWindowChanged(int index);
	void OnDisplayMonitorChanged(int index);
	void OnMicDeviceChanged(int index);
	void OnAppAudioWindowChanged(int index);
	void OnAppAudioToggled(bool checked);

private:
	void BuildUI();
	void SetupTray();
	void UpdateStatus();
	void UpdatePreview();
	void ReloadWindowLists();
	void SetRecordingButtons(bool active);
	void SetReplayButtons(bool active);

	PreviewWidget *preview = nullptr;
	QLabel *statusLabel = nullptr;
	QLabel *statsLabel = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *stopRecordButton = nullptr;
	QPushButton *replayButton = nullptr;
	QPushButton *stopReplayButton = nullptr;
	QPushButton *saveReplayButton = nullptr;
	QComboBox *captureSource = nullptr;
	QComboBox *gameWindow = nullptr;
	QComboBox *displayMonitor = nullptr;
	QComboBox *micDevice = nullptr;
	QComboBox *appAudioWindow = nullptr;
	QSystemTrayIcon *trayIcon = nullptr;
	QTimer *statsTimer = nullptr;

	CaptureManager *captureManager = nullptr;
	Recorder *recorder = nullptr;
	Hotkeys *hotkeys = nullptr;

	bool recordingActive = false;
	bool replayActive = false;
	bool previewEnabled = true;
	bool shutdownRequested = false;
	QElapsedTimer recordingElapsed;
};