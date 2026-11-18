# Redesigning the artwork and interface

[MazeRunner V5.4.3a wiki](Home.md)

## Redesign path A: keep the single-file art pipeline

This is the smallest engineering change for a graphic designer's first pass. Keep all simulation code and the appearance registry order intact. Redraw each sprite on an exact 20×20 canvas using the existing palette, preserving a common foot position and the knight's approximate cell footprint. Replace the twenty corresponding string rows. Each row must contain exactly twenty symbols, and every symbol must be in `valid_sprite`'s whitelist. Compile-time assertions catch malformed dimensions and unknown symbols before the game runs.

Use silhouette to distinguish creatures before relying on color: wings, horns, eye shape, body mass, leg count, and cap outline remain meaningful at small scale. Keep intentional transparency around limbs. Reserve `E` for visible state-sensitive eye details and inspect the roaming, hunting, and searching versions. Avoid changing a shared palette entry casually: modifying `W`, for example, changes every sprite using that symbol, including UI illustrations. For new colors, update both `sprite_color` and the whitelist, then consider whether tests expecting palette ranges need a deliberate update.

Edit theme colors in `UiRenderer` and stone brightness in `build_wall_mesh`. Maintain contrast between black corridors, gray raised walls, the player's blue ring, exit art, and route overlay. At 101×101 fitted into one window, individual sprite pixels become very small; judge navigation readability at whole-map scale as well as at zoom. The existing ring is a useful player locator. A future exit beacon or an icon outline could improve readability, but neither is currently implemented.

For UI rearrangement, change rectangles in `UiState::widgets` and matching descriptive panel/text positions in `UiRenderer::setup`, `hud`, and `draw`. Background labels and panels are not generated automatically from the widget list, so moving only a button rectangle can create overlaps even though clicks remain correctly aligned to that button. Keep focus outlines and disabled/pressed states distinct. Test the minimum 800×600 window, wide and tall windows, and a high-DPI transform. A portrait offscreen screenshot is a robustness check, not a promise the native minimum-size limits permit that exact window.

| Desired edit | Primary source location | Related constraints |
|---|---|---|
| Knight or exit drawing | [L2054](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2054), [L2186](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2186) | 20 rows × 20 symbols; shared palette |
| Ten monster drawings | [L2066](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2066) onward | Keep registry pointers and stable IDs |
| Registry names/hover flags | [L2205](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2205) | Hover is cosmetic; registry size must cover max count |
| Sprite validation | [L2226](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2226) | Add new palette symbols here as well as color mapping |
| Wall style | [L2261](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2261) | Static upload, exposed faces, large-map policy |
| Sprite and eye colors | [L2305](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2305) | Global effect across all art using a symbol |
| World sprite size/anchor | [L2330](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2330) | Camera-facing construction and wall occlusion |
| Camera fit/tilt/zoom | [L2362](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2362) | Projection and sprite orientation must agree |
| Route colors/thickness | [L2393](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2393) | Preserve monster-free/amber semantics |
| Widget rectangles and labels | [L2485](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2485) | Shared hit testing and keyboard order |
| Font shapes | [L2610](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2610) | Seven rows, five bits; text metrics elsewhere |
| Menu palette and layout | [L2809](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2809) | Labels, panels, and widgets need coordinated spacing |
| HUD text and counters | [L2897](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l2897) | Keep start prompt and counters clear of the HUD buttons |
| Difficulty balance | [L0112](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/01-Source-Line-by-Line.md#l0112) | Re-run the calibration script after any change |

## Redesign path B: external texture sprites and richer animation

An external sprite sheet would let a designer iterate in an image editor without rewriting C++ strings. This is a **proposed extension**, not a loader already present in V5.4.3a. A concrete initial asset contract could preserve 20×20 logical frames, transparent RGBA backgrounds, a documented foot pivot, and named clips for idle, walk, hunting, and searching. A separate manifest would map the ten stable appearance IDs to sheet regions and frame timings. Larger source art is also possible, but the logical world footprint must be chosen explicitly rather than inferred from image dimensions.

Engineering work would include image decoding, texture lifetime ownership, UV coordinates in the vertex format, a texture sampler in the shader, atlas rectangles, and an asset-location policy. Nearest-neighbor sampling would preserve crisp pixel edges. Atlas gutters and careful UV boundaries would avoid neighboring-frame bleed. Transparent texels could be discarded for hard-edged pixel art while retaining depth writes; translucent effects would require an intentional blend/depth-order policy. Color-space handling should be consistent across images and vertex colors rather than relying on incidental driver defaults.

Animation should live in render state and be keyed to simulation events or timestamps. Keep collision and AI on the existing grid unless the gameplay redesign explicitly changes them. Render interpolation can make movement smoother, but it should not determine whether capture occurs. Aesthetic random variations should use deterministic hashes or a separate visual PRNG, preserving the simulation RNG stream and its seeded tests. Asset failures should produce useful diagnostics and a visible fallback, and asset paths should work when launching from a different current directory.

A designer handoff should specify canvas/frame size, foot pivot, intended world size, transparent margin, palette/color-space policy, direction conventions, animation durations, and the three monster-state cues (roaming, hunting, searching). Deliver an overview sheet plus isolated frames and preview both at actual small in-game scale. Those deliverables are recommendations for a future art pass; the current package supplies the existing embedded-art screenshots and source, not a new texture pipeline.

## Redesign path C: true 3D characters or a broader renderer change

Replacing billboard actors with real models is a larger change. It requires mesh/asset import, normals, material definitions, model transforms, animation data, and lighting if the models should appear shaded. A rigged knight also needs bone transforms or another explicit animation scheme. Merely extruding current pixel quads does not create articulated character animation.

Preserve the tile-centered simulation as the authoritative gameplay representation while adding a render transform for each entity. Define a shared world scale, actor origin, feet-on-floor convention, and maximum silhouette footprint. Retain a readable player marker and distinct monster silhouettes under the fixed camera. If the camera becomes rotatable or perspective, update billboard handling, projection bounds, zoom behavior, and the relationship between input directions and screen directions together.

The current portable renderer targets OpenGL 3.3 core and uses GLSL 3.30. Any new shader or asset technique must fit the chosen supported platform baseline or introduce an explicitly documented alternative renderer. macOS's existing OpenGL route should be treated as a compatibility target when evaluating new effects. A backend migration would be a separate engineering project; there is no existing abstraction here that automatically converts GL draw calls to another graphics API.

## Validation workflow for visual changes

Begin with a reproducible seed and unchanged dimensions, count, difficulty, compiler, and standard library. Compare headless simulation outputs before/after after excluding measured host compute time. An art-only edit should not change the simulation state or consume additional simulation randomness. Run the portable regression suite for sprite validity, geometry/camera invariants, all profiles, the V5.4.2 feature tests, and UI state. Then run the real-GL render and injected-application suites on Linux (both run under CTest when EGL is available) and inspect the generated PNGs. A gameplay or balance change should also re-run `scripts/run_calibration.py`.

Use the gallery to check that all ten appearances are distinct and visible, then inspect a naturally generated crowded map. Review whole-map and zoomed views, amber and cycling routes, player overlap/occlusion, the green exit, the start prompt and hunting/searching counters, invalid setup fields, keyboard focus, pause, and results. Rerun native-window checks on each intended release platform: offscreen rendering cannot establish native DPI behavior, compositor behavior, actual keyboard mappings, or macOS/Windows driver compatibility.

For new assets or shader effects, extend tests around the changed contract rather than weakening old assertions until everything passes. A deliberate palette redesign may require updating pixel-color expectations, but depth ordering, missing actors, map aspect, input hit areas, source gameplay behavior, and GPU cleanup should remain verifiable. Store the chosen screenshots with seed/configuration and capture method so they remain useful references rather than undocumented pictures.
