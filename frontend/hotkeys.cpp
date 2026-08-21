#include "hotkeys.hpp"

#include <cstring>

#include <obs.h>

#include "app-config.hpp"

const int Hotkeys::hotkeyIds[ActionCount] = {
	0xC001, /* ActionStartRecording */
	0xC002, /* ActionStopRecording */
	0xC003, /* ActionSaveReplay */
	0xC004, /* ActionStartReplayBuffer */
	0xC005, /* ActionStopReplayBuffer */
};

const char *Hotkeys::ActionName(Action action)
{
	switch (action) {
	case ActionStartRecording:
		return "Hotkey.StartRecording";
	case ActionStopRecording:
		return "Hotkey.StopRecording";
	case ActionSaveReplay:
		return "Hotkey.SaveReplay";
	case ActionStartReplayBuffer:
		return "Hotkey.StartReplayBuffer";
	case ActionStopReplayBuffer:
		return "Hotkey.StopReplayBuffer";
	default:
		return "";
	}
}

const char *Hotkeys::ActionDescription(Action action)
{
	switch (action) {
	case ActionStartRecording:
		return "Start Recording";
	case ActionStopRecording:
		return "Stop Recording";
	case ActionSaveReplay:
		return "Save Replay";
	case ActionStartReplayBuffer:
		return "Start Replay Buffer";
	case ActionStopReplayBuffer:
		return "Stop Replay Buffer";
	default:
		return "";
	}
}

void Hotkeys::Initialize(HWND hwnd)
{
	window = hwnd;

	for (int i = 0; i < ActionCount; i++) {
		const char *base = ActionName((Action)i);
		bindings[i].action = (Action)i;
		bindings[i].vk = AppConfig::GetHotkeyVK(base);
		bindings[i].modifiers = AppConfig::GetHotkeyModifiers(base);
	}

	ApplyBindings();
	blog(LOG_INFO, "global hotkeys initialized");
}

void Hotkeys::Shutdown()
{
	if (!window)
		return;

	for (int i = 0; i < ActionCount; i++)
		UnregisterHotKey(window, hotkeyIds[i]);
	window = nullptr;
}

void Hotkeys::ApplyBindings()
{
	if (!window)
		return;

	for (int i = 0; i < ActionCount; i++) {
		UnregisterHotKey(window, hotkeyIds[i]);

		HotkeyBinding &b = bindings[i];
		b.vk = AppConfig::GetHotkeyVK(ActionName(b.action));
		b.modifiers = AppConfig::GetHotkeyModifiers(ActionName(b.action));

		if (b.vk == 0)
			continue;

		if (!RegisterHotKey(window, hotkeyIds[i], (UINT)b.modifiers,
				    (UINT)b.vk)) {
			blog(LOG_WARNING, "failed to register hotkey %s (error %lu)",
			     ActionDescription(b.action), GetLastError());
		}
	}
}

bool Hotkeys::HandleHotkeyMessage(WPARAM wParam)
{
	int id = (int)wParam;

	for (int i = 0; i < ActionCount; i++) {
		if (hotkeyIds[i] != id)
			continue;

		switch ((Action)i) {
		case ActionStartRecording:
			emit startRecording();
			break;
		case ActionStopRecording:
			emit stopRecording();
			break;
		case ActionSaveReplay:
			emit saveReplay();
			break;
		case ActionStartReplayBuffer:
			emit startReplayBuffer();
			break;
		case ActionStopReplayBuffer:
			emit stopReplayBuffer();
			break;
		default:
			break;
		}
		return true;
	}
	return false;
}
