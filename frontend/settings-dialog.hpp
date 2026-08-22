#pragma once

#include <Windows.h>

#include <QDialog>
#include <QKeyEvent>
#include <QPushButton>

#include <obs.h>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QCheckBox;

/* Button that captures a global hotkey combination when clicked. */
class HotkeyCaptureButton : public QPushButton {
	Q_OBJECT

public:
	explicit HotkeyCaptureButton(QWidget *parent = nullptr);

	void StartCapture();
	void StopCapture(bool keepValue);
	void UpdateText();

	int vk = 0;
	int modifiers = 0;

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;

private:
	bool capturing = false;
};

/* Settings dialog for obs-lite.
 *
 * General tab: recording dir, replay dir, start with Windows, minimize to
 * tray, start minimized, show preview by default.
 * Video tab: canvas/output resolution, FPS, encoder, rate control, bitrate,
 * CQP, keyframe interval, encoder preset.
 * Audio tab: audio bitrate.
 * Replay tab: duration, max size, extension, remux.
 * Hotkeys tab: five configurable global hotkeys.
 */
class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog() override;

signals:
	void settingsApplied();

private slots:
	void OnApply();
	void OnCancel();

private:
	void BuildUI();
	void LoadSettings();
	void SaveSettings();
	void UpdateEncoderVisibility();

	/* General */
	QLineEdit *recordingDir = nullptr;
	QLineEdit *replayDir = nullptr;
	QComboBox *recordingFormat = nullptr;
	QCheckBox *startWithWindows = nullptr;
	QCheckBox *minimizeToTray = nullptr;
	QCheckBox *startMinimized = nullptr;
	QCheckBox *startWithPreview = nullptr;

	/* Video */
	QComboBox *baseRes = nullptr;
	QComboBox *outputRes = nullptr;
	QComboBox *fps = nullptr;
	QComboBox *encoder = nullptr;
	QComboBox *rateControl = nullptr;
	QSpinBox *bitrate = nullptr;
	QSpinBox *cqp = nullptr;
	QSpinBox *keyint = nullptr;
	QComboBox *preset = nullptr;

	/* Audio */
	QSpinBox *audioBitrate = nullptr;

	/* Replay */
	QComboBox *replayDuration = nullptr;
	QSpinBox *replayMaxSize = nullptr;
	QComboBox *replayExt = nullptr;
	QCheckBox *remuxToMP4 = nullptr;

	/* Hotkeys */
	HotkeyCaptureButton *hotkeyButtons[5] = {};
};