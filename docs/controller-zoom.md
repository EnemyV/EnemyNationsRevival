# Controller zoom

Set `EN_GAMEPAD_ZOOM=1` to enable native SDL controller zoom: the left shoulder zooms in and the right shoulder zooms out. Mouse wheel, trackpad scrolling, and the zoom keys remain available while a controller is connected.

When using Steam Input with native controller zoom, configure each shoulder zoom binding to emit only the corresponding gamepad shoulder button. Do not also bind that action to a mouse wheel or zoom key: both events would be applied and cause duplicate zoom. Alternatively, leave `EN_GAMEPAD_ZOOM` disabled and use a Steam layout that emits wheel or keyboard zoom inputs. SDL2 does not reliably identify the source of emulated wheel and keyboard events, so the game does not automatically deduplicate them.
