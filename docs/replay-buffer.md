# Replay Buffer

The replay buffer is a first-class feature of obs-light.

## How it works

obs-light uses the upstream OBS `replay_buffer` output (implemented in
`plugins/obs-ffmpeg/obs-ffmpeg-mux.c`). It works at the **encoded packet**
level:

1. Video and audio encoders run continuously while the buffer is active.
2. Every encoded packet is appended to an in-memory ring, bounded by:
   - `max_time_sec` (default 60 s, choices 15/30/60/120/300), and
   - `max_size_mb` (default 2048 MB safety cap).
3. Old packets are purged from the front of the ring (always on keyframe
   boundaries) as new ones arrive.
4. When **Save Replay** is triggered, the relevant packet range is copied,
   timestamps are re-based, and the packets are handed to the `ffmpeg-mux`
   helper process on a **separate thread** to write the container file.

Because the ring stores encoded packets, saving a replay involves **no
re-encoding and no re-rendering**: the save cannot stutter the game, and the
capture pipeline keeps running while the file is written.

## Memory usage

The ring is bounded by `max_time_sec` and `max_size_mb`. Estimated memory is
roughly the average bitrate × duration; for example at 8 Mbps for 60 s the
video ring holds ≈ 60 MB plus audio. The hard `max_size_mb` cap guarantees
the buffer cannot grow without bound.

## API surface used by the frontend

- Output type: `replay_buffer` (registered by the obs-ffmpeg plugin)
- Settings:
  - `max_time_sec` (int)
  - `max_size_mb` (int)
  - `directory` (string) — where replay files are written
  - `format` (string) — filename format, default
    `Replay_%CCYY-%MM-%DD_%hh-%mm-%ss`
  - `extension` (string) — `mp4` (default) or `mkv`
  - `allow_spaces` (bool) — false for obs-light (safe filenames)
- Save trigger: `proc_handler_call(ph, "save", nullptr)` — the plugin sets a
  save timestamp and performs the packet copy + mux on its next packet, on
  the mux thread.
- Completion signal: `saved` — the frontend uses it to update state and
  re-arm the "Save Replay" button.
- Encoders: the replay buffer owns its own video/audio encoder instances
  (same settings as recording), so recording and replay buffering can run
  simultaneously.

## Behavior contract

- Saving while the buffer is stopped shows a message and does nothing else.
- A save that is already in progress is not started a second time.
- Saving never blocks the game capture pipeline.
- If the disk is full or unwritable, the save fails with an error message
  and the buffer keeps running.
