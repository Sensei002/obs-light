#include "settings-dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <util/platform.h>

#include "app-config.hpp"
#include "hotkeys.hpp"
#include "obs-app.hpp"

namespace {

struct Resolution {
	const char *name;
	uint32_t width;
	uint32_t height;
};

const Resolution resolutions[] = {
	{"1280 x 720 (720p)", 1280, 720},
	{"1600 x 900", 1600, 900},
	{"1920 x 1080 (1080p)", 1920, 1080},
	{"2560 x 1440 (1440p)", 2560, 1440},
	{"3840 x 2160 (4K)", 3840, 2160},
};

const int replayDurations[] = {15, 30, 60, 120, 300};

static int findResolutionIndex(uint32_t w, uint32_t h)
{
	for (size_t i = 0; i < sizeof(resolutions) / sizeof(Resolution); i++) {
		if (resolutions[i].width == w && resolutions[i].height == h)
			return (int)i;
	}
	return 2;
}

static QString modifiersToString(int modifiers)
{
	QStringList parts;
	if (modifiers & MOD_CONTROL)
		parts << "Ctrl";
	if (modifiers & MOD_ALT)
		parts << "Alt";
	if (modifiers & MOD_SHIFT)
		parts << "Shift";
	if (modifiers & MOD_WIN)
		parts << "Win";
	return parts.join(" + ");
}

/* VK code -> Qt key (only the keys we support) */
static const struct {
	int vk;
	int qtKey;
} vkToQt[] = {
	{VK_F1, Qt::Key_F1},   {VK_F2, Qt::Key_F2},   {VK_F3, Qt::Key_F3},
	{VK_F4, Qt::Key_F4},   {VK_F5, Qt::Key_F5},   {VK_F6, Qt::Key_F6},
	{VK_F7, Qt::Key_F7},   {VK_F8, Qt::Key_F8},   {VK_F9, Qt::Key_F9},
	{VK_F10, Qt::Key_F10}, {VK_F11, Qt::Key_F11}, {VK_F12, Qt::Key_F12},
};

static int qtKeyToVK(int qtKey)
{
	for (const auto &entry : vkToQt) {
		if (entry.qtKey == qtKey)
			return entry.vk;
	}
	return 0;
}

} // namespace

/* ------------------------------------------------------------------------ */

HotkeyCaptureButton::HotkeyCaptureButton(QWidget *parent)
	: QPushButton(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	connect(this, &QPushButton::clicked, this, [this]() {
		if (capturing)
			StopCapture(false);
		else
			StartCapture();
	});
}

void HotkeyCaptureButton::StartCapture()
{
	capturing = true;
	setText("Press a key combination...");
	setFocus();
	grabKeyboard();
}

void HotkeyCaptureButton::StopCapture(bool keepValue)
{
	if (!capturing)
		return;
	capturing = false;
	releaseKeyboard();
	if (!keepValue)
		UpdateText();
}

void HotkeyCaptureButton::keyPressEvent(QKeyEvent *event)
{
	if (!capturing) {
		QPushButton::keyPressEvent(event);
		return;
	}

	if (event->key() == Qt::Key_Escape) {
		StopCapture(false);
		return;
	}

	int newVk = qtKeyToVK(event->key());
	if (newVk == 0) {
		setText("Use an F1-F12 key with a modifier");
		return;
	}

	int modifiers = 0;
	if (event->modifiers() & Qt::ControlModifier)
		modifiers |= MOD_CONTROL;
	if (event->modifiers() & Qt::AltModifier)
		modifiers |= MOD_ALT;
	if (event->modifiers() & Qt::ShiftModifier)
		modifiers |= MOD_SHIFT;
	if (event->modifiers() & Qt::MetaModifier)
		modifiers |= MOD_WIN;

	if (modifiers == 0) {
		setText("At least one modifier (Ctrl/Alt/Shift) required");
		return;
	}

	vk = newVk;
	this->modifiers = modifiers;
	StopCapture(true);
}

void HotkeyCaptureButton::focusOutEvent(QFocusEvent *event)
{
	StopCapture(false);
	QPushButton::focusOutEvent(event);
}

void HotkeyCaptureButton::UpdateText()
{
	if (vk == 0) {
		setText("None");
		return;
	}

	QString mods = modifiersToString(modifiers);
	QString keyName;
	for (const auto &entry : vkToQt) {
		if (entry.vk == vk) {
			keyName = QKeySequence((Qt::Key)entry.qtKey).toString();
			break;
		}
	}
	if (keyName.isEmpty())
		keyName = QString("VK %1").arg(vk);

	setText(mods.isEmpty() ? keyName : mods + " + " + keyName);
}

/* ------------------------------------------------------------------------ */

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("obs-light Settings");
	BuildUI();
	LoadSettings();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::BuildUI()
{
	auto *layout = new QVBoxLayout(this);
	auto *tabs = new QTabWidget(this);

	/* ---------------- General ---------------- */
	auto *generalTab = new QWidget(tabs);
	auto *generalLayout = new QFormLayout(generalTab);

	recordingDir = new QLineEdit(generalTab);
	auto *recordingBrowse = new QPushButton("Browse...", generalTab);
	auto *recordingRow = new QHBoxLayout;
	recordingRow->addWidget(recordingDir);
	recordingRow->addWidget(recordingBrowse);
	generalLayout->addRow("Recording directory:", recordingRow);

	replayDir = new QLineEdit(generalTab);
	auto *replayBrowse = new QPushButton("Browse...", generalTab);
	auto *replayRow = new QHBoxLayout;
	replayRow->addWidget(replayDir);
	replayRow->addWidget(replayBrowse);
	generalLayout->addRow("Replay directory:", replayRow);

	startWithWindows = new QCheckBox("Start obs-light with Windows", generalTab);
	generalLayout->addRow("", startWithWindows);
	minimizeToTray = new QCheckBox("Minimize to system tray instead of closing",
				     generalTab);
	generalLayout->addRow("", minimizeToTray);
	startMinimized = new QCheckBox("Start minimized", generalTab);
	generalLayout->addRow("", startMinimized);
	startWithPreview = new QCheckBox("Show preview on startup", generalTab);
	generalLayout->addRow("", startWithPreview);

	connect(recordingBrowse, &QPushButton::clicked, this, [this]() {
		QString dir = QFileDialog::getExistingDirectory(
			this, "Select recording directory", recordingDir->text());
		if (!dir.isEmpty())
			recordingDir->setText(dir);
	});
	connect(replayBrowse, &QPushButton::clicked, this, [this]() {
		QString dir = QFileDialog::getExistingDirectory(
			this, "Select replay directory", replayDir->text());
		if (!dir.isEmpty())
			replayDir->setText(dir);
	});

	tabs->addTab(generalTab, "General");

	/* ---------------- Video ---------------- */
	auto *videoTab = new QWidget(tabs);
	auto *videoLayout = new QFormLayout(videoTab);

	baseRes = new QComboBox(videoTab);
	for (const auto &res : resolutions)
		baseRes->addItem(res.name);
	videoLayout->addRow("Canvas resolution:", baseRes);

	outputRes = new QComboBox(videoTab);
	outputRes->addItem("Same as canvas");
	for (const auto &res : resolutions)
		outputRes->addItem(res.name);
	videoLayout->addRow("Output resolution:", outputRes);

	fps = new QComboBox(videoTab);
	fps->addItem("30 FPS", 30);
	fps->addItem("60 FPS", 60);
	fps->addItem("120 FPS", 120);
	videoLayout->addRow("FPS:", fps);

	encoder = new QComboBox(videoTab);
	encoder->addItem("Auto (NVENC preferred)", "auto");
	encoder->addItem("NVIDIA NVENC H.264", "nvenc");
	encoder->addItem("Software x264", "x264");
	videoLayout->addRow("Encoder:", encoder);

	rateControl = new QComboBox(videoTab);
	rateControl->addItem("CBR (constant bitrate)", "CBR");
	rateControl->addItem("CQP (constant quality)", "CQP");
	videoLayout->addRow("Rate control:", rateControl);

	bitrate = new QSpinBox(videoTab);
	bitrate->setRange(500, 100000);
	bitrate->setSuffix(" Kbps");
	bitrate->setSingleStep(500);
	videoLayout->addRow("Bitrate:", bitrate);

	cqp = new QSpinBox(videoTab);
	cqp->setRange(0, 51);
	videoLayout->addRow("CQP value (lower = better):", cqp);

	keyint = new QSpinBox(videoTab);
	keyint->setRange(0, 10);
	keyint->setSuffix(" s");
	videoLayout->addRow("Keyframe interval:", keyint);

	preset = new QComboBox(videoTab);
	preset->addItem("p1 (fastest)", "p1");
	preset->addItem("p2", "p2");
	preset->addItem("p3", "p3");
	preset->addItem("p4", "p4");
	preset->addItem("p5 (balanced)", "p5");
	preset->addItem("p6", "p6");
	preset->addItem("p7 (best quality)", "p7");
	videoLayout->addRow("Encoder preset:", preset);

	connect(encoder, &QComboBox::currentIndexChanged, this, [this](int) {
		UpdateEncoderVisibility();
	});
	connect(rateControl, &QComboBox::currentIndexChanged, this, [this](int) {
		UpdateEncoderVisibility();
	});

	tabs->addTab(videoTab, "Video");

	/* ---------------- Audio ---------------- */
	auto *audioTab = new QWidget(tabs);
	auto *audioLayout = new QFormLayout(audioTab);

	audioBitrate = new QSpinBox(audioTab);
	audioBitrate->setRange(32, 512);
	audioBitrate->setSuffix(" Kbps");
	audioLayout->addRow("Audio bitrate:", audioBitrate);

	auto *audioNote = new QLabel(
		"Application audio is captured from the window selected on the "
		"main window.",
		audioTab);
	audioNote->setWordWrap(true);
	audioLayout->addRow("", audioNote);

	tabs->addTab(audioTab, "Audio");

	/* ---------------- Replay ---------------- */
	auto *replayTab = new QWidget(tabs);
	auto *replayLayout = new QFormLayout(replayTab);

	replayDuration = new QComboBox(replayTab);
	for (int d : replayDurations)
		replayDuration->addItem(QString("%1 seconds").arg(d), d);
	replayLayout->addRow("Replay duration:", replayDuration);

	replayMaxSize = new QSpinBox(replayTab);
	replayMaxSize->setRange(64, 16384);
	replayMaxSize->setSuffix(" MB");
	replayLayout->addRow("Max replay size:", replayMaxSize);

	replayExt = new QComboBox(replayTab);
	replayExt->addItem("MP4", "mp4");
	replayExt->addItem("MKV", "mkv");
	replayLayout->addRow("Replay format:", replayExt);

	remuxToMP4 = new QCheckBox("Remux recordings to MP4 after recording",
				   replayTab);
	replayLayout->addRow("", remuxToMP4);

	tabs->addTab(replayTab, "Replay");

	/* ---------------- Hotkeys ---------------- */
	auto *hotkeyTab = new QWidget(tabs);
	auto *hotkeyLayout = new QFormLayout(hotkeyTab);

	for (int i = 0; i < 5; i++) {
		auto *button = new HotkeyCaptureButton(hotkeyTab);
		auto *clear = new QPushButton("Clear", hotkeyTab);
		auto *row = new QHBoxLayout;
		row->addWidget(button, 1);
		row->addWidget(clear);

		hotkeyButtons[i] = button;

		connect(clear, &QPushButton::clicked, button, [button]() {
			button->vk = 0;
			button->modifiers = 0;
			button->UpdateText();
		});

		hotkeyLayout->addRow(
			Hotkeys::ActionDescription((Hotkeys::Action)i), row);
	}

	auto *hotkeyNote = new QLabel(
		"Hotkeys work globally while a game is focused. At least one "
		"modifier key (Ctrl, Alt, Shift) is required.",
		hotkeyTab);
	hotkeyNote->setWordWrap(true);
	hotkeyLayout->addRow("", hotkeyNote);

	tabs->addTab(hotkeyTab, "Hotkeys");

	layout->addWidget(tabs);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
					    QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setText("Apply");
	layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this,
		&SettingsDialog::OnApply);
	connect(buttons, &QDialogButtonBox::rejected, this,
		&SettingsDialog::OnCancel);
}

void SettingsDialog::LoadSettings()
{
	recordingDir->setText(
		QString::fromStdString(AppConfig::GetRecordingDir()));
	replayDir->setText(QString::fromStdString(AppConfig::GetReplayDir()));
	startWithWindows->setChecked(AppConfig::StartWithWindows());
	minimizeToTray->setChecked(AppConfig::MinimizeToTray());
	startMinimized->setChecked(AppConfig::StartMinimized());
	startWithPreview->setChecked(AppConfig::StartWithPreview());

	baseRes->setCurrentIndex(findResolutionIndex(AppConfig::GetBaseWidth(),
						     AppConfig::GetBaseHeight()));

	int outputIdx = 0;
	if (AppConfig::GetOutputWidth() != AppConfig::GetBaseWidth() ||
	    AppConfig::GetOutputHeight() != AppConfig::GetBaseHeight()) {
		outputIdx = findResolutionIndex(AppConfig::GetOutputWidth(),
						AppConfig::GetOutputHeight()) + 1;
	}
	outputRes->setCurrentIndex(outputIdx);

	int fpsIndex = fps->findData((int)AppConfig::GetFPSNum());
	fps->setCurrentIndex(fpsIndex < 0 ? 1 : fpsIndex);

	const char *enc = obs_data_get_string(AppConfig::Config(),
					      "Video.Encoder");
	int encIdx = encoder->findData(enc);
	encoder->setCurrentIndex(encIdx < 0 ? 0 : encIdx);

	const char *rc = obs_data_get_string(AppConfig::Config(),
					     "Video.RateControl");
	int rcIdx = rateControl->findData(rc);
	rateControl->setCurrentIndex(rcIdx < 0 ? 0 : rcIdx);

	bitrate->setValue(AppConfig::BitrateKbps());
	cqp->setValue(AppConfig::CQPValue());
	keyint->setValue(AppConfig::KeyintSec());

	const char *presetStr = obs_data_get_string(AppConfig::Config(),
						    "Video.Preset");
	int presetIdx = preset->findData(presetStr);
	preset->setCurrentIndex(presetIdx < 0 ? 4 : presetIdx);

	audioBitrate->setValue(AppConfig::AudioBitrateKbps());

	int durationIdx =
		replayDuration->findData(AppConfig::ReplayDurationSec());
	replayDuration->setCurrentIndex(durationIdx < 0 ? 2 : durationIdx);
	replayMaxSize->setValue(AppConfig::ReplayMaxSizeMB());
	replayExt->setCurrentIndex(AppConfig::ReplayExtension() == "mkv" ? 1
									 : 0);
	remuxToMP4->setChecked(AppConfig::RemuxToMP4());

	for (int i = 0; i < 5; i++) {
		const char *base = Hotkeys::ActionName((Hotkeys::Action)i);
		hotkeyButtons[i]->vk = AppConfig::GetHotkeyVK(base);
		hotkeyButtons[i]->modifiers = AppConfig::GetHotkeyModifiers(base);
		hotkeyButtons[i]->UpdateText();
	}

	UpdateEncoderVisibility();
}

void SettingsDialog::SaveSettings()
{
	obs_data_t *config = AppConfig::Config();

	obs_data_set_string(config, "General.RecordingDir",
			    recordingDir->text().toUtf8().constData());
	obs_data_set_string(config, "Replay.ReplayDir",
			    replayDir->text().toUtf8().constData());
	obs_data_set_bool(config, "General.StartWithWindows",
			  startWithWindows->isChecked());
	obs_data_set_bool(config, "General.MinimizeToTray",
			  minimizeToTray->isChecked());
	obs_data_set_bool(config, "General.StartMinimized",
			  startMinimized->isChecked());
	obs_data_set_bool(config, "General.StartWithPreview",
			  startWithPreview->isChecked());

	AppConfig::SetStartWithWindows(startWithWindows->isChecked());

	os_mkdirs(recordingDir->text().toUtf8().constData());
	os_mkdirs(replayDir->text().toUtf8().constData());

	int baseIdx = baseRes->currentIndex();
	int outIdx = outputRes->currentIndex();
	uint32_t baseW = resolutions[baseIdx].width;
	uint32_t baseH = resolutions[baseIdx].height;
	uint32_t outW = baseW;
	uint32_t outH = baseH;
	if (outIdx > 0) {
		outW = resolutions[outIdx - 1].width;
		outH = resolutions[outIdx - 1].height;
	}

	obs_data_set_int(config, "Video.BaseWidth", baseW);
	obs_data_set_int(config, "Video.BaseHeight", baseH);
	obs_data_set_int(config, "Video.OutputWidth", outW);
	obs_data_set_int(config, "Video.OutputHeight", outH);
	obs_data_set_int(config, "Video.FPS", fps->currentData().toInt());
	obs_data_set_string(config, "Video.Encoder",
			    encoder->currentData().toString().toUtf8().constData());
	obs_data_set_string(config, "Video.RateControl",
			    rateControl->currentData().toString().toUtf8().constData());
	obs_data_set_int(config, "Video.Bitrate", bitrate->value());
	obs_data_set_int(config, "Video.CQP", cqp->value());
	obs_data_set_int(config, "Video.KeyintSec", keyint->value());
	obs_data_set_string(config, "Video.Preset",
			    preset->currentData().toString().toUtf8().constData());

	obs_data_set_int(config, "Audio.Bitrate", audioBitrate->value());

	obs_data_set_int(config, "Replay.DurationSec",
			 replayDuration->currentData().toInt());
	obs_data_set_int(config, "Replay.MaxSizeMB", replayMaxSize->value());
	obs_data_set_string(config, "Replay.Extension",
			    replayExt->currentData().toString().toUtf8().constData());
	obs_data_set_bool(config, "Replay.RemuxToMP4",
			  remuxToMP4->isChecked());

	for (int i = 0; i < 5; i++) {
		const char *base = Hotkeys::ActionName((Hotkeys::Action)i);
		std::string vkKey = std::string(base) + ".VK";
		std::string modKey = std::string(base) + ".Mod";
		obs_data_set_int(config, vkKey.c_str(),
				 hotkeyButtons[i]->vk);
		obs_data_set_int(config, modKey.c_str(),
				 hotkeyButtons[i]->modifiers);
	}

	AppConfig::Save();
	emit settingsApplied();
}

void SettingsDialog::OnApply()
{
	SaveSettings();
	accept();
}

void SettingsDialog::OnCancel()
{
	reject();
}

void SettingsDialog::UpdateEncoderVisibility()
{
	bool isNvenc = encoder->currentData().toString() != "x264";
	bool cqp = rateControl->currentData().toString() == "CQP";

	bitrate->setVisible(isNvenc && !cqp);
	cqp->setVisible(isNvenc && cqp);
	preset->setVisible(isNvenc);
}

