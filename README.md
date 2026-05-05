# Key Note Overlay C++

Windows rhythm-game key overlay for OBS. It captures configured keyboard lanes with a low-level Win32 keyboard hook and renders notes in a straight lane layout. Unregistered keys are ignored before rendering.

## Features

- Low-latency native C++/Win32 implementation
- 5K, 8K, and 10K presets
- Custom lane order from left to right
- Long-note rendering while a key is held
- Long notes are split visually into head, low-alpha body, and tail cap
- Box or circle note heads
- Classic, Neon, and Minimal visual presets
- Transparent lane background, thin note outline, and key press glow/pulse
- Top or bottom compact key labels
- Adjustable whole-overlay opacity
- Chord alignment: keydowns within 15 ms render on the first keydown row
- Adjustable overlay position, size, note travel, and release fade
- Always-on-top and click-through overlay options
- Separate capture-safe overlay/settings windows for OBS

## Build

Requirements:

- Windows
- CMake 3.20+
- Ninja
- MinGW g++ at `C:/msys64/mingw64/bin/g++.exe`, or edit `build_cpp_overlay.bat` for your compiler path

Build:

```bat
build_cpp_overlay.bat
```

Run:

```bat
run_cpp_overlay.bat
```

The executable is produced at:

```text
build-cpp-ninja\key_note_overlay_cpp.exe
```

## Usage

Start the app, then use the settings window to configure:

- lane preset: `5K`, `8K`, `10K`, or custom
- lane keys, ordered from left to right
- overlay `x`, `y`, `width`, and `height`
- release fade duration
- note travel distance
- note width and height
- lane spacing percentage
- visual preset: `Classic`, `Neon`, or `Minimal`
- opacity percentage
- circle note mode
- lane background, rounded boxes, thin outline, and key label placement
- debug background
- click-through and always-on-top

`Apply` changes the running overlay immediately. `Save` writes the local config file.

Quit with the settings window close button or `Ctrl+Shift+F12`.

## OBS Capture

Use `Window Capture` and select:

```text
Key Note Overlay C++ Overlay
```

Do not capture `Key Note Overlay C++ Settings`; that is only the configuration GUI.

## Config

The local runtime config is:

```text
key_note_cpp_config.json
```

That file is ignored by Git because it can contain personal key layout and capture settings. Use `key_note_cpp_config.example.json` as a template. The app still writes `key_note_cpp_config.ini` for backward compatibility.

Example:

```json
{
  "overlay": {
    "x": 80,
    "y": 40,
    "width": 900,
    "height": 220,
    "duration_ms": 850,
    "travel_px": 180,
    "note_width": 58,
    "note_height": 34,
    "visual_preset": "Classic",
    "opacity_percent": 100,
    "lane_spacing_percent": 100,
    "long_note_alpha": 92,
    "debug_background": true,
    "click_through": true,
    "always_on_top": true,
    "circle_notes": false,
    "lane_background": true,
    "rounded_boxes": true,
    "thin_outline": true,
    "key_text_bottom": true
  },
  "input": {
    "mode": "8K",
    "lanes": ["A", "S", "D", "F", "J", "K", "L", "SEMICOLON"]
  }
}
```

Special key names are normalized when saving. For example, `;` becomes `SEMICOLON` and `[` becomes `LBRACKET`.
