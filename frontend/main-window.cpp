#include "main-window.hpp"

#include <cstring>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include <graphics/graphics.h>
#include <util/platform.h>

#include "app-config.hpp"
#include "capture-manager.hpp"
#include "hotkeys.hpp"
#include "obs-app.hpp"
#include "preview-widget.hpp"
#include "recorder.hpp"
#include "settings-dialog.hpp"

static MainWindow *instance = nullptr;

MainWindow *MainWindow::Instance()
{
	return instance;
}

/* ------------------------------------------------------------------------ */
/* Preview rendering                                                        */
/* ------------------------------------------------------------------------ */

static void DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	UNUSED_PARAMETER(data);

	gs_viewport_t viewport = {0, 0, cx, cy, true};
	gs_set_viewport(&viewport);
	obs_render_main_texture();
}

/* ------------------------------------------------------------------------ */

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	instance = this;

	setWindowTitle("obs-light");
	setMinimumSize(640, 400);

	captureManager = new CaptureManager(this);
	recorder = new Recorder(this);
	hotkeys = new Hotkeys(this);

	BuildUI();
	SetupTray();

	captureManager->Initialize();
	recorder->Initialize();

	hotkeys->Initialize((HWND)winId());

	connect(hotkeys, &Hotkeys::startRecording, this,
		&MainWindow::OnStartRecording);
	connect(hotkeys, &Hotkeys::stopRecording, this,
		&MainWindow::OnStopRecording);
	connect(hotkeys, &Hotkeys::saveReplay, this, &MainWindow::OnSaveReplay);
	connect(hotkeys, &Hotkeys::startReplayBuffer, this,
		&MainWindow::OnStartReplayBuffer);
	connect(hotkeys, &Hotkeys::stopReplayBuffer, this,
		&MainWindow::OnStopReplayBuffer);

	connect(recorder, &Recorder::recordingStarted, this, [this]() {
		recordingActive = true;
		recordingElapsed.start();
		SetRecordingButtons(true);
		UpdateStatus();
	});
	connect(recorder, &Recorder::recordingStopped, this, [this](int) {
		recordingActive = false;
		SetRecordingButtons(false);
		UpdateStatus();
	});
	connect(recorder, &Recorder::replayBufferStarted, this, [this]() {
		replayActive = true;
		SetReplayButtons(true);
		UpdateStatus();
	});
	connect(recorder, &Recorder::replayBufferStopped, this, [this](int) {
		replayActive = false;
		SetReplayButtons(false);
		UpdateStatus();
	});
	connect(recorder, &Recorder::replaySaved, this,
		&MainWindow::UpdateStatus);
	connect(recorder, &Recorder::errorOccurred, this,
		&MainWindow::ShowError);

	statsTimer = new QTimer(this);
	connect(statsTimer, &QTimer::timeout, this, &MainWindow::OnStatsTimer);
	statsTimer->start(1000);

	/* initial window lists */
	ReloadWindowLists();

	previewEnabled = AppConfig::StartWithPreview();
	UpdatePreview();

	QMetaObject::invokeMethod(this, [this]() {
		bool startMinimized = AppConfig::StartMinimized() ||
				      QCoreApplication::arguments().contains(
					      QStringLiteral("-minimized"));
		if (startMinimized)
			showMinimized();
		else
			show();
	}, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
	hotkeys->Shutdown();

	if (recorder)
		recorder->Shutdown();
	if (captureManager)
		captureManager->Shutdown();

	AppConfig::Save();
	instance = nullptr;
}

void MainWindow::BuildUI()
{
	auto *central = new QWidget(this);
	auto *layout = new QVBoxLayout(central);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(8);

	/* Preview */
	preview = new PreviewWidget(central);
	preview->setMinimumHeight(240);
	layout->addWidget(preview, 1);

	connect(preview, &PreviewWidget::DisplayCreated, this, [this](
							 obs_display_t *display) {
		obs_display_add_draw_callback(display, DrawPreview, nullptr);
		obs_display_set_enabled(display, previewEnabled);
	});

	/* Capture row */
	auto *captureRow = new QHBoxLayout;

	auto *captureLabel = new QLabel("Capture:", central);
	captureRow->addWidget(captureLabel);

	captureSource = new QComboBox(central);
	captureSource->addItem("Game Capture",
			       (int)CaptureManager::VideoSource::GameCapture);
	captureSource->addItem("Display Capture",
			       (int)CaptureManager::VideoSource::DisplayCapture);
	captureSource->addItem("None", (int)CaptureManager::VideoSource::None);
	captureRow->addWidget(captureSource);

	gameWindow = new QComboBox(central);
	gameWindow->setMinimumWidth(220);
	gameWindow->setVisible(false);
	captureRow->addWidget(gameWindow);

	displayMonitor = new QComboBox(central);
	displayMonitor->setVisible(false);
	captureRow->addWidget(displayMonitor);

	auto *audioLabel = new QLabel("App Audio:", central);
	captureRow->addWidget(audioLabel);

	appAudioWindow = new QComboBox(central);
	appAudioWindow->setMinimumWidth(200);
	captureRow->addWidget(appAudioWindow, 1);

	layout->addLayout(captureRow);

	/* Status rows */
	statusLabel = new QLabel(central);
	statusLabel->setStyleSheet("font-weight: bold;");
	layout->addWidget(statusLabel);

	statsLabel = new QLabel(central);
	layout->addWidget(statsLabel);

	/* Buttons */
	auto *buttonRow = new QHBoxLayout;

	recordButton = new QPushButton("Start Recording", central);
	stopRecordButton = new QPushButton("Stop Recording", central);
	stopRecordButton->setVisible(false);
	replayButton = new QPushButton("Start Replay Buffer", central);
	stopReplayButton = new QPushButton("Stop Replay Buffer", central);
	stopReplayButton->setVisible(false);
	saveReplayButton = new QPushButton("Save Replay", central);
	auto *settingsButton = new QPushButton("Settings", central);

	buttonRow->addWidget(recordButton);
	buttonRow->addWidget(stopRecordButton);
	buttonRow->addWidget(replayButton);
	buttonRow->addWidget(stopReplayButton);
	buttonRow->addWidget(saveReplayButton);
	buttonRow->addStretch();
	buttonRow->addWidget(settingsButton);

	layout->addLayout(buttonRow);

	setCentralWidget(central);

	connect(captureSource, &QComboBox::currentIndexChanged, this,
		&MainWindow::OnCaptureSourceChanged);
	connect(gameWindow, &QComboBox::currentIndexChanged, this,
		&MainWindow::OnGameWindowChanged);
	connect(displayMonitor, &QComboBox::currentIndexChanged, this,
		&MainWindow::OnDisplayMonitorChanged);
	connect(appAudioWindow, &QComboBox::currentIndexChanged, this,
		&MainWindow::OnAppAudioWindowChanged);

	connect(recordButton, &QPushButton::clicked, this,
		&MainWindow::OnStartRecording);
	connect(stopRecordButton, &QPushButton::clicked, this,
		&MainWindow::OnStopRecording);
	connect(replayButton, &QPushButton::clicked, this,
		&MainWindow::OnStartReplayBuffer);
	connect(stopReplayButton, &QPushButton::clicked, this,
		&MainWindow::OnStopReplayBuffer);
	connect(saveReplayButton, &QPushButton::clicked, this,
		&MainWindow::OnSaveReplay);
	connect(settingsButton, &QPushButton::clicked, this,
		&MainWindow::OnOpenSettings);
}

void MainWindow::SetupTray()
{
	QIcon icon = QIcon(":/obs-light.png");
	trayIcon = new QSystemTrayIcon(icon, this);
	trayIcon->setToolTip("obs-light");

	auto *menu = new QMenu(this);
	menu->addAction("Start Recording", this, &MainWindow::OnStartRecording);
	menu->addAction("Stop Recording", this, &MainWindow::OnStopRecording);
	menu->addSeparator();
	menu->addAction("Start Replay Buffer", this,
			&MainWindow::OnStartReplayBuffer);
	menu->addAction("Stop Replay Buffer", this,
			&MainWindow::OnStopReplayBuffer);
	menu->addAction("Save Replay", this, &MainWindow::OnSaveReplay);
	menu->addSeparator();
	menu->addAction("Show obs-light", this, [this]() {
		showNormal();
		raise();
		activateWindow();
	});
	menu->addAction("Exit", this, [this]() {
		shutdownRequested = true;
		qApp->quit();
	});

	trayIcon->setContextMenu(menu);
	trayIcon->show();
}

void MainWindow::OnStartRecording()
{
	recorder->StartRecording();
}

void MainWindow::OnStopRecording()
{
	recorder->StopRecording();
}

void MainWindow::OnSaveReplay()
{
	recorder->SaveReplay();
}

void MainWindow::OnStartReplayBuffer()
{
	recorder->StartReplayBuffer();
}

void MainWindow::OnStopReplayBuffer()
{
	recorder->StopReplayBuffer();
}

void MainWindow::OnOpenSettings()
{
	auto *dialog = new SettingsDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &SettingsDialog::settingsApplied, this,
		&MainWindow::OnSettingsApplied);
	dialog->show();
}

void MainWindow::OnSettingsApplied()
{
	hotkeys->ApplyBindings();

	/* video settings require a video reset; refuse while recording */
	if (recorder->RecordingActive()) {
		ShowInfo("Video settings will apply after recording stops.");
		return;
	}

	if (OBSApp::Get()->ResetVideo()) {
		ReloadWindowLists();
	}

	previewEnabled = AppConfig::StartWithPreview();
	UpdatePreview();
	UpdateStatus();
}

void MainWindow::OnStatsTimer()
{
	QString stats;

	if (recordingActive) {
		int seconds = (int)(recordingElapsed.elapsed() / 1000);
		QString time = QString("%1:%2")
				       .arg(seconds / 60)
				       .arg(seconds % 60, 2, 10, QChar('0'));
		stats = QString("Recording: %1 | Dropped: %2 | Total: %3")
				.arg(time)
				.arg(recorder->GetDroppedFrames())
				.arg(recorder->GetTotalFrames());
	} else {
		stats = QString("Dropped frames: %1").arg(recorder->GetDroppedFrames());
	}

	uint64_t bytes = recorder->GetTotalBytes();
	if (bytes > 0) {
		stats += QString(" | Size: %1 MB")
				 .arg((double)bytes / (1024.0 * 1024.0), 0, 'f', 1);
	}

	struct obs_video_info ovi = OBSApp::Get()->GetVideoInfo();
	stats += QString(" | FPS: %1 | Encoder: %2")
			 .arg(ovi.fps_num / (ovi.fps_den ? ovi.fps_den : 1))
			 .arg(recorder->EncoderId().empty()
				      ? "none"
				      : QString::fromStdString(recorder->EncoderId()));

	statsLabel->setText(stats);
	UpdateStatus();
}

void MainWindow::UpdateStatus()
{
	QString status;

	if (recordingActive && replayActive) {
		status = "● RECORDING + REPLAY BUFFERING";
	} else if (recordingActive) {
		status = "● RECORDING";
	} else if (replayActive) {
		status = "● REPLAY BUFFERING";
	} else {
		status = "● READY";
	}

	status += QString("   |   Replay: %1 sec")
			  .arg(AppConfig::ReplayDurationSec());

	if (recorder->ReplaySaveInProgress())
		status += "   |   Saving replay...";

	statusLabel->setText(status);
}

void MainWindow::SetRecordingButtons(bool active)
{
	recordButton->setVisible(!active);
	stopRecordButton->setVisible(active);
}

void MainWindow::SetReplayButtons(bool active)
{
	replayButton->setVisible(!active);
	stopReplayButton->setVisible(active);
}

void MainWindow::OnCaptureSourceChanged(int index)
{
	auto source = (CaptureManager::VideoSource)captureSource->currentData().toInt();
	captureManager->SetVideoSource(source);

	bool game = source == CaptureManager::VideoSource::GameCapture;
	bool display = source == CaptureManager::VideoSource::DisplayCapture;

	gameWindow->setVisible(game);
	displayMonitor->setVisible(display);

	if (game && gameWindow->count() == 0)
		ReloadWindowLists();
}

void MainWindow::OnGameWindowChanged(int index)
{
	if (index < 0)
		return;
	QString value = gameWindow->itemData(index).toString();
	captureManager->SetGameCaptureWindow(
		value.toStdString(), AppConfig::GetGameCapturePriority());

	obs_data_set_string(AppConfig::Config(), "Capture.GameWindow",
			    value.toUtf8().constData());
	AppConfig::Save();
}

void MainWindow::OnDisplayMonitorChanged(int index)
{
	if (index < 0)
		return;
	QString value = displayMonitor->itemData(index).toString();
	captureManager->SetDisplayMonitor(value.toStdString());

	obs_data_set_string(AppConfig::Config(), "Capture.DisplayMonitor",
			    value.toUtf8().constData());
	AppConfig::Save();
}

void MainWindow::OnAppAudioWindowChanged(int index)
{
	if (index < 0)
		return;
	QString value = appAudioWindow->itemData(index).toString();
	captureManager->SetAppAudioWindow(value.toStdString());
	captureManager->SetAppAudioEnabled(true);

	obs_data_set_string(AppConfig::Config(), "Capture.AppAudioWindow",
			    value.toUtf8().constData());
	obs_data_set_bool(AppConfig::Config(), "Capture.AppAudioEnabled", true);
	AppConfig::Save();
}

void MainWindow::OnAppAudioToggled(bool checked)
{
	captureManager->SetAppAudioEnabled(checked);
	obs_data_set_bool(AppConfig::Config(), "Capture.AppAudioEnabled",
			  checked);
	AppConfig::Save();
}

void MainWindow::UpdatePreview()
{
	obs_display_t *display = preview->GetDisplay();
	if (display)
		obs_display_set_enabled(display, previewEnabled);
}

void MainWindow::ReloadWindowLists()
{
	/* Game capture windows */
	QString currentGame = gameWindow->currentData().toString();
	gameWindow->blockSignals(true);
	gameWindow->clear();
	gameWindow->addItem("Any fullscreen game", "");
	auto windows = CaptureManager::EnumerateWindows(false);
	for (const auto &w : windows) {
		gameWindow->addItem(QString::fromStdString(w.name),
				    QString::fromStdString(w.value));
	}
	int gameIdx = gameWindow->findData(currentGame);
	gameWindow->setCurrentIndex(gameIdx < 0 ? 0 : gameIdx);
	gameWindow->blockSignals(false);

	/* Display monitors (same enumeration order as monitor_capture) */
	QString currentMonitor = displayMonitor->currentData().toString();
	displayMonitor->blockSignals(true);
	displayMonitor->clear();
	displayMonitor->addItem("Primary display", "");
	int monitorCount = 0;
	EnumDisplayMonitors(NULL, NULL,
			    [](HMONITOR, HDC, LPRECT, LPARAM param) -> BOOL {
				    (*(int *)param)++;
				    return TRUE;
			    },
			    (LPARAM)&monitorCount);
	for (int i = 0; i < monitorCount; i++) {
		displayMonitor->addItem(QString("Display %1").arg(i + 1),
					QString::number(i));
	}
	int monitorIdx = displayMonitor->findData(currentMonitor);
	displayMonitor->setCurrentIndex(monitorIdx < 0 ? 0 : monitorIdx);
	displayMonitor->blockSignals(false);

	/* Application audio windows */
	QString currentAudio = appAudioWindow->currentData().toString();
	appAudioWindow->blockSignals(true);
	appAudioWindow->clear();
	appAudioWindow->addItem("No application", "");
	for (const auto &w : windows) {
		appAudioWindow->addItem(QString::fromStdString(w.name),
					QString::fromStdString(w.value));
	}
	int audioIdx = appAudioWindow->findData(currentAudio);
	appAudioWindow->setCurrentIndex(audioIdx < 0 ? 0 : audioIdx);
	appAudioWindow->blockSignals(false);
}

void MainWindow::OnOutputSignal(const char *signal, obs_output_t *output)
{
	UNUSED_PARAMETER(output);
	if (strcmp(signal, "saved") == 0)
		UpdateStatus();
}

void MainWindow::ShowError(const QString &message)
{
	QMessageBox::critical(Instance(), "obs-light", message);
	blog(LOG_ERROR, "%s", message.toUtf8().constData());
}

void MainWindow::ShowInfo(const QString &message)
{
	QMessageBox::information(Instance(), "obs-light", message);
}

bool MainWindow::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	if (msg.message == WM_HOTKEY) {
		hotkeys->HandleHotkeyMessage(msg.wParam);
		return true;
	}
#else
	UNUSED_PARAMETER(message);
#endif
	return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (shutdownRequested || !AppConfig::MinimizeToTray()) {
		event->accept();
		qApp->quit();
		return;
	}

	event->ignore();
	hide();
}

void MainWindow::changeEvent(QEvent *event)
{
	QMainWindow::changeEvent(event);

	if (event->type() == QEvent::WindowStateChange) {
		if (isMinimized()) {
			/* Stop preview rendering while minimized to reduce
			 * GPU work; recording/replay continue normally. */
			previewEnabled = false;
			UpdatePreview();
		}
	}
}
