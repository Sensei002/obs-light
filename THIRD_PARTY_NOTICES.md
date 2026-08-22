# Third-Party Notices

obs-lite is a fork of **OBS Studio** (https://obsproject.com), which is
Copyright (C) 2012-2026 Lain Bailey and the OBS Project contributors, and is
distributed under the GNU General Public License version 2 (or later) â€” see
[COPYING](COPYING).

The following components are retained from the upstream OBS Studio source
tree and carry their own licenses. The upstream project maintains the
authoritative license texts in its repository
(https://github.com/obsproject/obs-studio).

## Runtime components linked into obs-lite

| Component                 | License           | Notes                                          |
| ------------------------- | ----------------- | ---------------------------------------------- |
| libobs (OBS core)         | GPL-2.0-or-later  | Copyright (c) 2012-2026 Lain Bailey et al.     |
| libobs-d3d11 / -opengl    | GPL-2.0-or-later  | Graphics subsystems                            |
| libobs-winrt              | GPL-2.0-or-later  | Windows.Graphics.Capture support               |
| obs-ffmpeg plugin         | GPL-2.0-or-later  | Muxing, replay buffer, ffmpeg_aac              |
| obs-nvenc plugin          | GPL-2.0-or-later  | NVIDIA NVENC encoders                          |
| obs-x264 plugin           | GPL-2.0-or-later  | Software H.264 encoder                         |
| win-capture plugin        | GPL-2.0-or-later  | Game/display/window capture, graphics hook     |
| win-wasapi plugin         | GPL-2.0-or-later  | WASAPI audio capture (incl. app audio)         |
| ffmpeg-mux helper         | GPL-2.0-or-later  | External muxer process                         |

## Third-party libraries

The prebuilt dependency packages (obs-deps) used by CI contain the following
libraries. Their licenses are included in the obs-deps distribution and in
the upstream OBS Studio source tree:

| Library              | License           | Used by                                    |
| -------------------- | ----------------- | ------------------------------------------ |
| FFmpeg               | LGPL-2.1-or-later | obs-ffmpeg (muxing/encoding), ffmpeg-mux   |
| x264                 | GPL-2.0-or-later  | obs-x264 software encoder                  |
| Qt 6                 | LGPL-3.0-only     | obs-lite frontend UI                      |
| zlib                 | zlib              | via FFmpeg                                 |
| w32-pthreads         | LGPL-2.1-or-later | libobs, obs-ffmpeg                         |
| NvEncodeAPI headers  | NVIDIA license    | obs-nvenc (headers only; driver provides   |
|                      |                   | the runtime)                               |

Additional bundled upstream components inside `deps/` (libcaption, glad,
json11, blake2) retain their original licenses as shipped by OBS Studio.

## Trademarks

"OBS" and the OBS logo are trademarks of the OBS Project. obs-lite is an
independent fork and is not endorsed by or affiliated with the OBS Project.
This project does not use OBS branding for the application identity.

## Distribution

When redistributing obs-lite (binary or source), you must comply with the
GPL-2.0 license of the retained OBS code, including providing the complete
corresponding source code and this notice.
