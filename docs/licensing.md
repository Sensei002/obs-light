# Licensing

obs-lite is a fork of **OBS Studio** (https://obsproject.com).

## Retained upstream license

The OBS Studio source code retained in this repository (libobs, libobs-d3d11,
libobs-opengl, libobs-winrt, and the five retained plugin directories) is
licensed under the **GNU General Public License version 2** (or at your
option any later version). The full license text is in the `COPYING` file at
the root of this repository.

In accordance with the GPL, obs-lite as a whole is also distributed under
GPL-2.0-or-later.

## Author attribution

```
obs-lite â€” Lightweight Windows game recording and instant replay.
Copyright (C) 2026 obs-lite contributors.

Based on OBS Studio (https://obsproject.com)
Copyright (C) 2012-2026 Lain Bailey and the OBS Project contributors.
```

See also `AUTHORS` (from the upstream tree) for the full list of OBS Studio
contributors.

## Third-party components

The obs-lite installer bundles prebuilt third-party libraries (FFmpeg,
x264, Qt 6, etc.) from the official OBS dependency packages
(`obs-deps`). These carry their own licenses (LGPL-2.1, LGPL-3.0, etc.);
the full list and attribution are in
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

## Compliance

- The `COPYING` file contains the GPL-2.0 license text.
- The `THIRD_PARTY_NOTICES.md` file lists all retained third-party components
  and their licenses.
- The `AUTHORS` file from the upstream tree is preserved.
- No upstream copyright notices or license headers have been removed from
  source files.
- When distributing binaries (via installer or ZIP), the corresponding
  source is available at https://github.com/obs-lite/obs-lite and the
  upstream OBS Studio source at https://github.com/obsproject/obs-studio.