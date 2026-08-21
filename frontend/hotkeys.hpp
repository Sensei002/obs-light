#pragma once

#include <Windows.h>

#include <QObject>

/* Global hotkeys implemented with the Win32 RegisterHotKey API.  Hotkeys
 * are registered against the main window handle so WM_HOTKEY messages are
 * delivered there even while the game has focus. */
class Hotkeys : public QObject {
	Q_OBJECT

public:
	Hotkeys() = default;

	void Initialize(HWND window);
	void Shutdown();
	void ApplyBindings();

	/* Maps a WM_HOTKEY id back to the action; call from nativeEvent. */
	bool HandleHotkeyMessage(WPARAM wParam);

	enum Action {
		ActionStartRecording,
		ActionStopRecording,
		ActionSaveReplay,
		ActionStartReplayBuffer,
		ActionStopReplayBuffer,
		ActionCount,
	};

	static const char *ActionName(Action action);
	static const char *ActionDescription(Action action);

signals:
	void startRecording();
	void stopRecording();
	void saveReplay();
	void startReplayBuffer();
	void stopReplayBuffer();

private:
	struct HotkeyBinding {
		Action action;
		int vk = 0;
		int modifiers = 0;
	};

	static const int hotkeyIds[ActionCount];

	HWND window = nullptr;
	HotkeyBinding bindings[ActionCount];
};