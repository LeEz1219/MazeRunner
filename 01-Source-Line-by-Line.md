# MazeRunner V5.4.3a — complete source explanation

This guide explains every one of the **3,261 physical lines** in [mazerunnerV5.4.3a.cpp](src/mazerunnerV5.4.3a.cpp). It covers executable code, declarations, preprocessor branches, comments, blank separators, sprite rows, and glyph data. The final newline terminates line 3,261; it is not an additional source line.

**Source identity:** 173,782 bytes; SHA-256 `62fa1254b9c509ae3002f812bfa6b26823cd5b97eae71921fa2d153ad249a403`. Line numbers apply only to this exact copy.

Read [02-AI-Graphics-and-Redesign.md](02-AI-Graphics-and-Redesign.md) for a connected explanation of the architecture and design choices. Use [03-Build-Test-and-Reproduce.md](03-Build-Test-and-Reproduce.md) for compilation, evidence, screenshots, and reproduction.

## How to use this reference

Each `L####` entry reproduces one source line and then explains its purpose and consequences. Source snippets are HTML-escaped inside code tags so C++ angle brackets and bitwise operators remain readable in Markdown. Whitespace-only entries are explicitly marked. Data rows describe their exact image/glyph contribution rather than pretending they execute as algorithms. Structural braces and comments are annotated separately; they do not independently implement behavior.

Coordinates are `(row,column)` in the simulation and `(x=column,y=row,z=height)` in geometry. Times such as 40 or 560 are simulation milliseconds unless explicitly described as host time. A “path” in the generation checks is a distinct simple entrance-to-exit route; it does not mean a monster-free route or a guaranteed win.

The core is a discrete-event simulation. The graphical layer advances it only while Playing, and the CLI advances the same core without a window. Monsters stay frozen until the player’s first move or auto-solve toggle. The full route overlay and the short tactical auto-player forecast are separate computations. The AI guide explains why this distinction matters.

## Section index

| Source start | Section |
|---|---|
| [L0001](#l0001) | Build commands, dependencies, and the version macro |
| [L0038](#l0038) | Simulation constants and value types |
| [L0146](#l0146) | Hints, actors, and simulation state |
| [L0215](#l0215) | Construction and grid access |
| [L0250](#l0250) | Maze generation, route-diversity validation, and fallback |
| [L0466](#l0466) | Graph rebuilding, debug guard, reset, and spawning |
| [L0589](#l0589) | Distance fields and bounded threat fields |
| [L0669](#l0669) | Private caches, exit chokepoint tree, and patrol regions |
| [L0764](#l0764) | Logging, corridor hearing, director radio, and interceptors |
| [L1019](#l1019) | Director control |
| [L1110](#l1110) | Scent expiry, searching, and corridor sight |
| [L1177](#l1177) | Event scheduler, monsters, and player controls |
| [L1334](#l1334) | Private simulation bookkeeping and movement commits |
| [L1401](#l1401) | Monster targets, reservations, and forecasting |
| [L1570](#l1570) | Timing-aware guidance and recovery navigation |
| [L1758](#l1758) | Beam planning, adaptive horizon, and complete-route overlay |
| [L1870](#l1870) | Automatic action selection and execution |
| [L1984](#l1984) | Simulation CLI and parsing |
| [L2044](#l2044) | Vertex types and embedded artwork |
| [L2199](#l2199) | Appearance registry and art validation |
| [L2244](#l2244) | Wall, sprite, camera, and route geometry |
| [L2407](#l2407) | UI model and editable setup |
| [L2464](#l2464) | UI state, widgets, and interaction |
| [L2609](#l2609) | Embedded bitmap font |
| [L2641](#l2641) | OpenGL shader compilation and linking |
| [L2695](#l2695) | World renderer and GPU ownership |
| [L2807](#l2807) | UI renderer and screen composition |
| [L2980](#l2980) | Application lifecycle, hold-to-move, and asynchronous loading |
| [L3064](#l3064) | Input callbacks |
| [L3141](#l3141) | GL loader, frame limiter, and native window loop |
| [L3218](#l3218) | Program entry point |

## Function and method index

Constructors, destructors, deleted copy operations, and both embedded GLSL entry points are included. Local lambdas are explained at their individual source lines. Public search/accessor helpers are documented even when the main live loop uses another path.

| Line | Function or method | Role |
|---|---|---|
| [L0084](#l0084) | `Point::operator==` | Coordinate equality compares both axes; vector<Point> route comparisons rely on this operation. |
| [L0086](#l0086) | `Point::operator!=` | Inequality delegates to equality so the two operators remain consistent. |
| [L0112](#l0112) | `difficulty_settings` | Return a complete immutable balance profile. constexpr permits compile-time evaluation for known difficulty values. |
| [L0122](#l0122) | `parse_difficulty` | Normalize a supplied difficulty token and return the corresponding enum; used by CLI parsing and the retained stream-prompt helper. |
| [L0134](#l0134) | `prompt_difficulty` | Retained stream-based prompt, useful to tests/legacy callers; the graphical startup path no longer invokes it. |
| [L0152](#l0152) | `LocationHint::contains` | Membership query for candidate hunting destinations inside a clue rectangle. |
| [L0215](#l0215) | `Maze::Maze` | Constructor accepts dimensions, an unsigned seed, and an optional logging stream; its parameter list continues below. |
| [L0231](#l0231) | `Maze::id` | Checked coordinate-to-index conversion; returning -1 lets callers reject out-of-bounds neighbors uniformly. |
| [L0235](#l0235) | `Maze::point` | Inverse flattening for valid, nonnegative indices: integer division yields row and remainder yields column. |
| [L0237](#l0237) | `Maze::walkable` | Terrain-only traversability query, deliberately independent of monsters and the player. |
| [L0241](#l0241) | `Maze::manhattan` | Compute the Manhattan heuristic between two valid cells; it ignores walls and therefore underestimates corridor distance. |
| [L0246](#l0246) | `Maze::neighbors` | Return the precomputed four-neighbor array by const reference; consumers must supply a valid cell index. |
| [L0248](#l0248) | `Maze::degree` | Expose cached graph degree for junction and dead-end heuristics. |
| [L0253](#l0253) | `Maze::generate` | Generate terrain until the profile topology certificate passes, falling back to adding connections after 64 attempts so generation always terminates; then spawn monsters. |
| [L0335](#l0335) | `Maze::route_diversity_met` | Apply the selected profile’s route-diversity requirement to the current terrain. |
| [L0343](#l0343) | `Maze::add_connections_until_diverse` | Generation fallback: open connecting walls in seeded batches until the requirement holds; always terminates. |
| [L0373](#l0373) | `Maze::has_at_least_two_solutions` | Determine whether at least two distinct simple entrance-to-exit paths exist in O(V+E), using iterative bridge analysis. |
| [L0415](#l0415) | `Maze::solution_diversity` | Compute a capped lower-bound certificate for up to four simple solutions, with optional minimum-size filtering of useful cyclic blocks. |
| [L0467](#l0467) | `Maze::rebuild_graph` | Rebuild adjacency, dead-end structure, cached fields, the exit chokepoint tree, patrol regions, and the terrain signature after terrain changes. |
| [L0521](#l0521) | `Maze::terrain_signature` | FNV-1a hash of the terrain, recorded when caches are rebuilt. |
| [L0529](#l0529) | `Maze::verify_graph` | Debug-only check that terrain was not edited without rebuild_graph(). |
| [L0537](#l0537) | `Maze::reset_game` | Reset a run’s dynamic state, including clues, detour history, and patrol regions, without carving a map or reseeding the RNG. |
| [L0563](#l0563) | `Maze::spawn_monsters` | Populate monsters in unique cells far from the player; their spawn-relative deadlines stay frozen until the first input. |
| [L0591](#l0591) | `Maze::distance_field` | Return a shared reverse BFS field from a least-recently-used cache; a miss traverses the whole terrain graph. |
| [L0624](#l0624) | `Maze::LocalField::at` | Read the distance under this field’s generation stamp, or return its radius plus one. |
| [L0629](#l0629) | `Maze::local_field` | Build or reuse a bounded BFS by source and radius. Threat forecasts and step listeners can request different bounds. |
| [L0711](#l0711) | `Maze::build_exit_tree` | Iterative Tarjan traversal rooted at the exit that marks route chokepoints. |
| [L0738](#l0738) | `Maze::exit_chokepoints` | List cells every route from a clue cell to the exit must pass, with exact distances. |
| [L0752](#l0752) | `Maze::mark_region` | Record that a monster covered an 8x8 patrol region. |
| [L0757](#l0757) | `Maze::region_age` | Time since any monster entered the region containing a cell. |
| [L0765](#l0765) | `Maze::configuration_summary` | Compose the startup summary, including hearing radius, interceptors, sight, and search time, without modifying state. |
| [L0789](#l0789) | `Maze::director_hint_width` | Nominal uncertain rectangle width equals base side length plus twice the padding; actual bounds may be clipped by map size. |
| [L0790](#l0790) | `Maze::director_phase` | Expose the director's current intensity phase. Positive clue padding corresponds to RELIEF. |
| [L0791](#l0791) | `Maze::director_reports` | Return the cumulative broadcast count for the HUD, terminal and regressions. |
| [L0793](#l0793) | `Maze::radio_area_label` | Format the current strategic footprint for diagnostics without changing simulation state. |
| [L0798](#l0798) | `Maze::auto_recovery_target` | Expose only the active recovery waypoint ID for testing/diagnostics, or -1 if none. |
| [L0801](#l0801) | `Maze::hearing_radius` | Compute acoustic reach in walkable steps using the profile and longer map axis. |
| [L0805](#l0805) | `Maze::auto_boldness` | Solver loop-detection level from detours in the last 20 seconds (0–4). |
| [L0811](#l0811) | `Maze::cell_label` | Format a terrain-valid cell for logs; this utility does not grant monsters player-position access. |
| [L0818](#l0818) | `Maze::log_event` | Append one timestamped diagnostic event when a logging destination exists. |
| [L0826](#l0826) | `Maze::log_status` | Emit a snapshot of player state, director status, and hunting/searching counts. |
| [L0845](#l0845) | `Maze::monster_name` | One-based monster label for logs. |
| [L0848](#l0848) | `Maze::radio_sector_size` | Calculate the normal map-wide radio sector before relief padding or clipping by axis. |
| [L0853](#l0853) | `Maze::widen_hint` | Work on a copy of a remembered rectangle so callers choose which stored memory to replace. |
| [L0865](#l0865) | `Maze::make_location_hint` | Quantize an actual displacement by local or radio sector size, clip it to the board and apply current relief. |
| [L0874](#l0874) | `Maze::deliver_hint` | Give a hearing monster the latest clue, switching it to hunting and keeping a valid target. |
| [L0893](#l0893) | `Maze::investigate_report` | Deliver a coarse report to a monster eligible for remote investigation. |
| [L0908](#l0908) | `Maze::publish_movement_hint` | Determine acoustic listeners, renew physical contact, publish eligible local clues and independently broadcast a strategic area. |
| [L0965](#l0965) | `Maze::assign_interceptors` | Reserve existing guards and assign eligible listeners to distinct chokepoints within the global profile cap. |
| [L1022](#l1022) | `Maze::set_director_precision` | Widen each stored area on escalation, release interceptors and discard old plans. Normal precision applies to future reports. |
| [L1052](#l1052) | `Maze::update_director` | Run one fixed-time director sample: confinement, relief escalation, and recovery hysteresis. A manual player standing still is not counted as confined. |
| [L1111](#l1111) | `Maze::next_monster_state_change` | Find the next scent, search or interceptor deadline. |
| [L1123](#l1123) | `Maze::expire_monster_states` | Release expired guards, end scent-driven hunts and complete area searches at their respective deadlines. |
| [L1162](#l1162) | `Maze::sees_player` | Corridor sight from a monster to the player (High only). |
| [L1163](#l1163) | `Maze::line_of_sight` | Straight open-corridor visibility between 3 and the profile’s range, shared by game and forecast. |
| [L1180](#l1180) | `Maze::advance_to` | Advance the discrete-event simulation; monster moves and state expiries are scheduled only after the player’s first input. |
| [L1212](#l1212) | `Maze::update_monsters` | Move due monsters with a short reservation list, apply High corridor sightings and interceptor hold expiry, and track patrol coverage. |
| [L1276](#l1276) | `Maze::set_auto_solve` | Toggle automatic assistance, reset its tactical memory, and update the overlay while preserving the existing map and actor positions. |
| [L1293](#l1293) | `Maze::move_player` | Manual movement wrapper; both manual and automatic displacements eventually use commit_player_move. |
| [L1302](#l1302) | `Maze::begin_held_move` | Start held movement with a direction supplied by the graphical input layer. |
| [L1311](#l1311) | `Maze::cancel_held_move` | Set the next repeat to the inactive sentinel without advancing time. |
| [L1314](#l1314) | `Maze::check_outcome` | Resolve outcomes with capture precedence, then cancel all pending scent and search deadlines. |
| [L1336](#l1336) | `Maze::remember` | Append an action cell to a bounded history deque; shared by player history and monster recent-cell memory. |
| [L1341](#l1341) | `Maze::start_timer` | Start the clock once and release frozen monsters, preserving their spawn staggering. |
| [L1358](#l1358) | `Maze::update_elapsed` | Elapsed gameplay time is now minus timer_start when active; pre-movement patrol time is therefore excluded. |
| [L1362](#l1362) | `Maze::visit_heat` | Return a linearly time-decayed revisit penalty without mutating stored memory. |
| [L1369](#l1369) | `Maze::commit_player_move` | Common transaction for one cardinal move or wait: validate, update clocks/memory, publish movement information, and resolve capture/escape. |
| [L1402](#l1402) | `Maze::target_claimed` | Determine whether another monster of the same behavior state has already selected a candidate waypoint. |
| [L1412](#l1412) | `Maze::choose_target` | Select a charge cell, interceptor chokepoint, clue-area cell (hunting or searching), or a patrol waypoint that favors stale regions and avoids crowding; never the exact player cell except after a High sighting. |
| [L1459](#l1459) | `Maze::is_reserved` | Membership test for the short reservation list. |
| [L1465](#l1465) | `Maze::choose_monster_step` | Choose one step toward a waypoint from a reverse BFS field while respecting a short list of reserved cells. |
| [L1490](#l1490) | `Maze::forecast_monsters` | Predict monster positions at each action boundary up to the chosen horizon, applying the sight rule, using bounded threat fields. |
| [L1532](#l1532) | `Maze::earliest_arrival` | Lower bound on a monster’s arrival time from bounded corridor distance and its schedule; not a probability estimate. |
| [L1540](#l1540) | `Maze::evaluate_action` | Evaluate one time-indexed player action under forecast occupancy and conservative/soft uncertainty rules. |
| [L1574](#l1574) | `Maze::route_guidance` | Weighted exit guidance: near or parked monsters are walls, distant moving ones add a soft detour cost (Dial’s algorithm). |
| [L1624](#l1624) | `Maze::terrain_exit_step` | Select a shortest-path successor using the cached terrain field. |
| [L1632](#l1632) | `Maze::recovery_distance_field` | Compute an independent occupancy-aware BFS toward a recovery waypoint; this avoids redirecting the visual exit overlay toward a temporary detour. |
| [L1647](#l1647) | `Maze::finish_recovery` | Retire a committed detour and remember its neighborhood so the next attempt avoids oscillating between the same choices. |
| [L1661](#l1661) | `Maze::begin_recovery` | Choose a detour waypoint within 4,096 expansions and depth 32; recent detours raise boldness, which weights exit progress more. |
| [L1718](#l1718) | `Maze::update_recovery` | Maintain an existing recovery commitment, reacting to progress, changed enemy occupancy, arrival, cutoff, or timeout. |
| [L1745](#l1745) | `Maze::recovery_step` | Select a safe adjacent step that makes progress toward the committed recovery goal, even when that step moves away from the maze exit. |
| [L1761](#l1761) | `Maze::beam_plan` | Time-expanded beam search to the forecast’s horizon with a conservative or relaxed first-step policy. |
| [L1826](#l1826) | `Maze::choose_horizon` | Use 18 actions of lookahead while the nearest threat is 10–36 steps away, otherwise 12. |
| [L1837](#l1837) | `Maze::refresh_solution_overlay` | Trace weighted successors to the exit or follow terrain when blocked. Mark occupied routes amber. |
| [L1873](#l1873) | `Maze::plan_auto_route` | Top-level automatic policy: no-threat shortest path, loop-aware scent wait (never beside a monster), committed recovery, conservative beam, relaxed beam, then emergency clearance. |
| [L1969](#l1969) | `Maze::auto_step` | Execute one tactical action and let the common movement transaction update state, scent, outcomes, and the complete overlay. |
| [L1985](#l1985) | `outcome_name` | Map the outcome enum to consistent labels shared by HUD-adjacent diagnostics and simulation output. |
| [L1994](#l1994) | `parse_number` | Parse a whole nonnegative decimal argument with a caller-selected maximum, reporting overflow as a readable range error. |
| [L2006](#l2006) | `run_simulation` | Headless CLI entry point using exactly the same Maze implementation; no OpenGL context or desktop is needed. |
| [L2221](#l2221) | `monster_appearance` | Resolve an integer spawn ID to an immutable appearance record. |
| [L2226](#l2226) | `valid_sprite` | Compile-time-capable validation of one sprite's dimensions and palette membership. |
| [L2236](#l2236) | `valid_monster_sprites` | Validate the entire monster registry, including entries added in future revisions. |
| [L2245](#l2245) | `shade` | Multiply all RGB channels by a brightness factor. This is baked vertex-color shading, not a light calculation; the function itself does not clamp values. |
| [L2248](#l2248) | `append_quad` | Accept four quad corners and a constant color, appending to a caller-owned vertex batch. |
| [L2253](#l2253) | `append_floor_rect` | Start a horizontal rectangle helper using opposite x/y corners. |
| [L2261](#l2261) | `build_wall_mesh` | Build the static wall mesh, first counting exposed faces so the vertex reservation matches the mesh exactly. |
| [L2305](#l2305) | `sprite_color` | Map an embedded pixel symbol to a color; eyes are red while hunting and orange while searching. |
| [L2330](#l2330) | `append_sprite` | Emit an actor or exit as colored pixel quads in one grid cell, with optional searching-eye coloring. |
| [L2348](#l2348) | `append_actor_ring` | Create a ground marker around the knight so its position remains recognizable when the sprite becomes small. |
| [L2362](#l2362) | `scene_projection` | Calculate a column-major 4×4 orthographic projection with fixed tilt, aspect preservation, and optional player-following zoom. |
| [L2382](#l2382) | `hue_color` | Convert a hue parameter into full-saturation, full-value RGB for the route highlight. |
| [L2393](#l2393) | `append_route` | Render the already-computed complete player-to-exit overlay; this function performs no path search. |
| [L2420](#l2420) | `UiRect::contains` | Use half-open bounds: left/top edges are included, right/bottom excluded. Adjacent buttons therefore do not both claim a shared boundary. |
| [L2426](#l2426) | `UiTransform::UiTransform` | Construct the transform from framebuffer width and height, not GLFW window-coordinate dimensions. |
| [L2431](#l2431) | `UiTransform::pointer` | Convert a GLFW cursor position to UI coordinates, taking both window size and framebuffer size to account for high-DPI displays. |
| [L2441](#l2441) | `SetupForm::number` | Parse a bounded decimal field into an output integer, reporting validity without throwing. |
| [L2449](#l2449) | `SetupForm::values` | Validate the three setup fields and obtain normalized integer game settings. |
| [L2454](#l2454) | `SetupForm::error` | Return the most relevant validation message for the current edit state. |
| [L2471](#l2471) | `UiState::is_field` | Identify the three numeric fields, used consistently by typing, selection, and keyboard handling. |
| [L2473](#l2473) | `UiState::field` | Return a mutable reference to the string associated with a numeric control. |
| [L2479](#l2479) | `UiState::show` | Transition to a new screen while resetting transient pointer/focus state. |
| [L2485](#l2485) | `UiState::widgets` | Construct the current screen's widget list from live UI and optional maze state; the same list governs drawing and interaction. |
| [L2525](#l2525) | `UiState::select` | Move keyboard focus and arm whole-field replacement for the next digit typed. |
| [L2527](#l2527) | `UiState::tab` | Advance or reverse keyboard focus through enabled widgets only. |
| [L2536](#l2536) | `UiState::digit` | Handle one text-input codepoint for numeric editing. |
| [L2544](#l2544) | `UiState::erase` | Implement Backspace or whole-field deletion without leaving the setup model. |
| [L2551](#l2551) | `UiState::step` | Adjust a numeric field using its +/- button or Up/Down while editing. |
| [L2560](#l2560) | `UiState::request_start` | Validate a request to generate a game before entering Loading. |
| [L2567](#l2567) | `UiState::activate` | Dispatch one enabled visible control, optionally operating on the current maze. |
| [L2597](#l2597) | `UiState::hit` | Find the control under a logical UI point. |
| [L2602](#l2602) | `UiState::pointer_button` | Track a press/release pair for one logical pointer location. |
| [L2610](#l2610) | `ui_glyph` | Resolve a character to seven five-bit font rows; this embedded bitmap font needs no font files or text library. |
| [L2643](#l2643) | `compile_shader` | Compile one GLSL shader stage in the current OpenGL context and return its owned object handle. |
| [L2657](#l2657) | `create_shader_program` | Create the shared position/color shader program used by the world and UI renderers. |
| [L2665](#l2665) | `GLSL vertex::main` | Transform each position to homogeneous clip space and forward its color. There are no normals, UVs, skeletal transforms, or lighting calculations. |
| [L2672](#l2672) | `GLSL fragment::main` | Write opaque RGB with alpha one. Pixel-art holes are omitted geometry rather than sampled alpha. |
| [L2711](#l2711) | `Renderer::release` | Centralize cleanup for normal destruction and partially completed construction. |
| [L2720](#l2720) | `Renderer::Renderer` | Construct the world renderer from immutable maze terrain while an OpenGL context is current. |
| [L2742](#l2742) | `Renderer::Renderer` | Delete the copy constructor to prevent two Renderers from owning and deleting the same GL handles. |
| [L2744](#l2744) | `Renderer::operator=` | Delete copy assignment for the same exclusive-ownership reason. |
| [L2746](#l2746) | `Renderer::~Renderer` | Release GL objects in the destructor; the application must destroy the renderer before destroying the context. |
| [L2749](#l2749) | `Renderer::change_zoom` | Apply a scroll/key zoom request without changing any Maze field or simulation clock. |
| [L2753](#l2753) | `Renderer::reset_view` | Reset the visual camera to a full-maze fit. |
| [L2755](#l2755) | `Renderer::zoom_factor` | Expose the current zoom as a const accessor for verification and diagnostics. |
| [L2759](#l2759) | `Renderer::draw` | Render the world into a caller-selected vertical viewport within the current framebuffer. |
| [L2816](#l2816) | `UiRenderer::rect` | Convert a logical UI rectangle into a flat colored quad at z = 0, appending to the overlay batch. |
| [L2818](#l2818) | `UiRenderer::border` | Draw an outline from four filled rectangles, with configurable thickness defaulting to two logical pixels. |
| [L2823](#l2823) | `UiRenderer::text` | Render a string as enlarged 5×7 bitmap glyphs at a requested logical pixel size. |
| [L2832](#l2832) | `UiRenderer::centered` | Center one line of bitmap text within a rectangle. |
| [L2837](#l2837) | `UiRenderer::icon` | Draw one embedded sprite in flat UI coordinates; no world tilt or camera transform is used for this illustration. |
| [L2842](#l2842) | `UiRenderer::widget` | Render a widget from the same model used by focus and pointer interaction. |
| [L2862](#l2862) | `UiRenderer::setup` | Draw setup labels, form guidance, the live settings summary, and decorative player/monster art. |
| [L2897](#l2897) | `UiRenderer::hud` | Draw status and help panels: a start prompt before the first input, then time, steps, and hunting/searching counts. |
| [L2917](#l2917) | `UiRenderer::release` | Release UI GPU resources during normal destruction or initialization failure. |
| [L2925](#l2925) | `UiRenderer::UiRenderer` | Construct the UI renderer with a current GL context, before any game maze is required. |
| [L2936](#l2936) | `UiRenderer::UiRenderer` | Disable copying so UI GPU handles have one owner. |
| [L2937](#l2937) | `UiRenderer::operator=` | Disable copy assignment for the same reason. |
| [L2939](#l2939) | `UiRenderer::~UiRenderer` | Delete UI resources while the context is still alive. |
| [L2941](#l2941) | `UiRenderer::draw` | Draw either a full menu screen or an overlay above a supplied maze frame. |
| [L2997](#l2997) | `GameApplication::release_hold` | Forget the physical key and disarm the simulation repeat. |
| [L3002](#l3002) | `GameApplication::press_move` | Submit a first move to the maze and retain the physical key while its timed repeats are armed. |
| [L3007](#l3007) | `GameApplication::GameApplication` | Construct the application from startup options, before a maze has been generated. |
| [L3012](#l3012) | `GameApplication::discard_game` | Destroy world graphics before the Maze, reset reporting, and cancel any held movement key. |
| [L3014](#l3014) | `GameApplication::begin_generation` | Start background generation from a validated snapshot of the form. |
| [L3029](#l3029) | `GameApplication::finish_generation` | Poll and install a completed Maze on the thread with the current GL context. |
| [L3043](#l3043) | `GameApplication::tick` | Advance the full nonnegative elapsed interval while Playing. The maze chronologically dispatches due manual repeats. |
| [L3053](#l3053) | `GameApplication::draw` | Draw the world in the central reserved area and then draw the full-frame UI above it. |
| [L3065](#l3065) | `ui_cursor` | Convert the live GLFW cursor into the logical coordinate system used by widget rectangles. |
| [L3072](#l3072) | `key_callback` | Dispatch key events: Esc pauses, resumes, or leaves a field but never quits; movement keys start a hold; releases stop it. |
| [L3117](#l3117) | `character_callback` | Receive GLFW text codepoints separately from physical-key events, enabling consistent numeric typing. |
| [L3122](#l3122) | `mouse_callback` | Translate GLFW mouse button actions into UI press/release behavior. |
| [L3129](#l3129) | `scroll_callback` | Handle mouse-wheel input as a visual zoom operation. |
| [L3135](#l3135) | `focus_callback` | Pause on focus loss and cancel any held movement key. |
| [L3144](#l3144) | `initialize_glew` | Initialize GLEW after making an OpenGL context current, then verify the capabilities this renderer actually requires. |
| [L3163](#l3163) | `run_graphical` | Open the native window with the shared title and run the loop, sleeping to a frame deadline while playing. |
| [L3220](#l3220) | `main` | Program entry point for both graphical and headless executables. |

## Build commands, dependencies, and the version macro

<a id="l0001"></a>
**L0001** — <code>// Build: g++ -std=c++17 -O2 -DNDEBUG mazerunnerV5.4.3a.cpp -o mazerunnerV5.4.3a -lglfw -lGLEW -lGL -pthread</code>

Compile a desktop executable from the standalone file. The final libraries resolve the GLFW window API, GLEW entry points, OpenGL drawing calls and threading support.

<a id="l0002"></a>
**L0002** — <code>// Headless: g++ -std=c++17 -O2 -DNDEBUG -DMAZERUNNER_HEADLESS mazerunnerV5.4.3a.cpp -o mazerunnerV5.4.3a-headless -pthread</code>

Define MAZERUNNER_HEADLESS to compile the same simulation and CLI without the graphics sections. The filename is relative to the directory containing the source.

<a id="l0003"></a>
**L0003** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0004"></a>
**L0004** — <code>#ifndef MAZERUNNER_HEADLESS</code>

Begin the conditional graphics-header region; defining MAZERUNNER_HEADLESS excludes these dependencies entirely.

<a id="l0005"></a>
**L0005** — <code>#include &lt;GL/glew.h&gt;</code>

GLEW declares modern OpenGL types/functions and must precede GLFW's possible OpenGL header inclusion.

<a id="l0006"></a>
**L0006** — <code>#include &lt;GLFW/glfw3.h&gt;</code>

GLFW supplies window/context creation and keyboard, pointer, focus, and resize-related APIs.

<a id="l0007"></a>
**L0007** — <code>#endif</code>

Close the currently active conditional-compilation region; this directive itself executes no runtime code.

<a id="l0008"></a>
**L0008** — <code>#include &lt;algorithm&gt;</code>

Include the standard <algorithm> declarations for sorting, searching, clamping, shuffling, and other standard sequence algorithms. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0009"></a>
**L0009** — <code>#include &lt;array&gt;</code>

Include the standard <array> declarations for fixed-size adjacency, glyph, sprite, and matrix containers. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0010"></a>
**L0010** — <code>#include &lt;chrono&gt;</code>

Include the standard <chrono> declarations for monotonic host timing and duration conversions. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0011"></a>
**L0011** — <code>#include &lt;cctype&gt;</code>

Include the standard <cctype> declarations for safe ASCII-oriented case normalization. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0012"></a>
**L0012** — <code>#include &lt;cmath&gt;</code>

Include the standard <cmath> declarations for square roots, trigonometry, exponentiation, and floating-point helpers. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0013"></a>
**L0013** — <code>#include &lt;cstdint&gt;</code>

Include the standard <cstdint> declarations for fixed-width integer types for seeds and time-related bookkeeping. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0014"></a>
**L0014** — <code>#include &lt;deque&gt;</code>

Include the standard <deque> declarations for bounded recent-position and director-history queues. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0015"></a>
**L0015** — <code>#include &lt;iomanip&gt;</code>

Include the standard <iomanip> declarations for fixed precision and formatted terminal output. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0016"></a>
**L0016** — <code>#include &lt;iostream&gt;</code>

Include the standard <iostream> declarations for terminal input/output and error streams. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0017"></a>
**L0017** — <code>#include &lt;limits&gt;</code>

Include the standard <limits> declarations for numeric sentinel and parser-bound definitions. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0018"></a>
**L0018** — <code>#include &lt;memory&gt;</code>

Include the standard <memory> declarations for unique/shared ownership of mazes, renderers, and distance fields. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0019"></a>
**L0019** — <code>#include &lt;queue&gt;</code>

Include the standard <queue> declarations for BFS/frontier containers and priority queues. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0020"></a>
**L0020** — <code>#include &lt;random&gt;</code>

Include the standard <random> declarations for seeded simulation/replay randomness and distributions. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0021"></a>
**L0021** — <code>#include &lt;sstream&gt;</code>

Include the standard <sstream> declarations for composing status messages and UI time strings. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0022"></a>
**L0022** — <code>#include &lt;stdexcept&gt;</code>

Include the standard <stdexcept> declarations for typed exceptions for invalid inputs and runtime failures. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0023"></a>
**L0023** — <code>#include &lt;string&gt;</code>

Include the standard <string> declarations for owning configuration values and diagnostics. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0024"></a>
**L0024** — <code>#include &lt;string_view&gt;</code>

Include the standard <string_view> declarations for non-owning compile-time sprite rows and appearance names. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0025"></a>
**L0025** — <code>#include &lt;cstddef&gt;</code>

Include the standard <cstddef> declarations for offsetof for the interleaved GPU vertex layout. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0026"></a>
**L0026** — <code>#include &lt;utility&gt;</code>

Include the standard <utility> declarations for pairs, moves, and other small utility operations. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0027"></a>
**L0027** — <code>#include &lt;vector&gt;</code>

Include the standard <vector> declarations for dynamically sized grids, search workspaces, vertices, and widgets. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0028"></a>
**L0028** — <code>#include &lt;future&gt;</code>

Include the standard <future> declarations for std::async and future-based background maze generation. Header inclusion supplies declarations/types rather than executing a game action.

<a id="l0029"></a>
**L0029** — <code>#include &lt;thread&gt;</code>

Include std::this_thread so the native window loop can sleep until a frame deadline when vsync is unavailable. Header inclusion supplies declarations rather than executing a game action.

<a id="l0030"></a>
**L0030** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0031"></a>
**L0031** — <code>// One version string for logs, the window title, the setup screen, and --help.</code>

Documentation fragment: “One version string for logs, the window title, the setup screen, and --help.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0032"></a>
**L0032** — <code>// CMake supplies the release label. Standalone builds use the same default.</code>

Describes the single release label shared by standalone and CMake builds.

<a id="l0033"></a>
**L0033** — <code>#ifndef MAZERUNNER_VERSION</code>

Begin a guard so a build system can supply the version string; CMake passes its project version through this macro.

<a id="l0034"></a>
**L0034** — <code>#define MAZERUNNER_VERSION "5.4.3a"</code>

Supply the fallback version string used in terminal output, the window title and the setup UI.

<a id="l0035"></a>
**L0035** — <code>#endif</code>

Close the version-override guard.

<a id="l0036"></a>
**L0036** — <code>#define MAZERUNNER_TITLE "Maze Runner V" MAZERUNNER_VERSION</code>

Compose the title "Maze Runner V5.4.2" by string-literal concatenation; logs, help text, and the window title all use it.

<a id="l0037"></a>
**L0037** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Simulation constants and value types

<a id="l0038"></a>
**L0038** — <code>// Times are integer simulation milliseconds. The renderer uses a monotonic clock.</code>

Documentation fragment: “Times are integer simulation milliseconds. The renderer uses a monotonic clock.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0039"></a>
**L0039** — <code>constexpr int AUTO_SOLVE_SPEED_MS = 40;</code>

Automatic actions are scheduled every 40 simulation milliseconds: 25 opportunities per simulated second, including waits; this is not the rendering frame rate.

<a id="l0040"></a>
**L0040** — <code>constexpr int MANUAL_HOLD_DELAY_MS = 220;</code>

Store the initial key-hold delay in simulation milliseconds. A successful press moves immediately.

<a id="l0041"></a>
**L0041** — <code>constexpr int MANUAL_HOLD_REPEAT_MS = 120;</code>

Set the interval between successive held-key steps after the initial delay.

<a id="l0042"></a>
**L0042** — <code>constexpr int MAX_MONSTERS = 10;</code>

Cap spawns at ten; sprite registration and the UI use the same limit.

<a id="l0043"></a>
**L0043** — <code>constexpr int MAX_MAZE_DIMENSION = 501;</code>

Bound each grid dimension at 501, limiting allocation and search sizes to at most 251,001 cells.

<a id="l0044"></a>
**L0044** — <code>constexpr int MONSTER_REPLAN_MS = 700;</code>

A monster's target is reviewed at least every 700 ms; target retention can still preserve a valid waypoint.

<a id="l0045"></a>
**L0045** — <code>constexpr int AUTO_HORIZON = 12;          // Default lookahead, in player actions.</code>

Default tactical lookahead of 12 player actions (480 simulation milliseconds at 40 ms per action).

<a id="l0046"></a>
**L0046** — <code>constexpr int AUTO_HORIZON_MAX = 18;      // Extended lookahead while threats are at mid range.</code>

Extended lookahead of 18 actions, used only while the nearest threat is at mid range (see choose_horizon).

<a id="l0047"></a>
**L0047** — <code>constexpr int AUTO_BEAM_WIDTH = 64;</code>

Keep at most 64 candidate states at each beam-search depth, bounding tactical search effort.

<a id="l0048"></a>
**L0048** — <code>// Navigation retains the same intelligence and action rate on every difficulty.</code>

Documentation fragment: “Navigation retains the same intelligence and action rate on every difficulty.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0049"></a>
**L0049** — <code>constexpr int AUTO_STALL_MS = 2400;</code>

After 2,400 ms without recognized navigation progress, auto-solving may seek a committed detour.

<a id="l0050"></a>
**L0050** — <code>constexpr int AUTO_RECOVERY_COMMIT_MS = 4000;</code>

A recovery waypoint may remain committed for at most 4,000 ms before retirement/reselection.

<a id="l0051"></a>
**L0051** — <code>constexpr int AUTO_VISIT_MEMORY_MS = 12000;</code>

Revisit heat decays over 12,000 ms; recently attempted recovery neighborhoods use the same retention duration.

<a id="l0052"></a>
**L0052** — <code>// The solver hard-blocks only threats it could meet before they move away.</code>

Documentation fragment: “The solver hard-blocks only threats it could meet before they move away.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0053"></a>
**L0053** — <code>// Farther monsters add a soft detour cost instead of closing the route outright.</code>

Documentation fragment: “Farther monsters add a soft detour cost instead of closing the route outright.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0054"></a>
**L0054** — <code>constexpr int AUTO_NEAR_THREAT_CELLS = 14;</code>

Monsters within 14 corridor steps of the player are hard obstacles for exit guidance: the player could reach them before they are likely to move away.

<a id="l0055"></a>
**L0055** — <code>constexpr int AUTO_FAR_THREAT_COST = 24;</code>

A distant moving monster adds 24 extra steps of cost to any route through its cell, so routes detour around it without the route being declared blocked.

<a id="l0056"></a>
**L0056** — <code>constexpr int AUTO_STATIONARY_MS = 1500;</code>

A monster that has not moved for 1.5 seconds counts as parked and becomes a hard obstacle at any distance.

<a id="l0057"></a>
**L0057** — <code>// Repeated detours inside this window raise boldness and shorten waiting.</code>

Documentation fragment: “Repeated detours inside this window raise boldness and shorten waiting.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0058"></a>
**L0058** — <code>constexpr int AUTO_LOOP_WINDOW_MS = 20000;</code>

Detours started within the last 20 seconds count toward loop detection.

<a id="l0059"></a>
**L0059** — <code>constexpr int AUTO_MAX_BOLDNESS = 4;</code>

Boldness is capped at four, bounding how much waiting can shrink and how strongly recovery favors exit progress.

<a id="l0060"></a>
**L0060** — <code>// Bounded per-monster distance fields; beyond this radius a lower bound is used.</code>

Documentation fragment: “Bounded per-monster distance fields; beyond this radius a lower bound is used.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0061"></a>
**L0061** — <code>constexpr int THREAT_RADIUS = 64;</code>

Bounded per-monster threat fields search 64 corridor steps; cells beyond report 65, a lower bound that can only make arrival estimates earlier.

<a id="l0062"></a>
**L0062** — <code>// Interceptors walk at most this far along the exit tree looking for chokepoints.</code>

Documentation fragment: “Interceptors walk at most this far along the exit tree looking for chokepoints.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0063"></a>
**L0063** — <code>constexpr int CUTTER_SEARCH_STEPS = 80;</code>

An interceptor search walks at most 80 parent links up the exit-rooted tree looking for chokepoints.

<a id="l0064"></a>
**L0064** — <code>constexpr int CUTTER_PACE_MS = 80;</code>

Assumed player pace for interception: a monster is sent only if it can reach the chokepoint before a player moving one cell per 80 ms.

<a id="l0065"></a>
**L0065** — <code>constexpr int CUTTER_CAMP_MS = 4000;</code>

An interceptor holds its chokepoint for about four seconds after arriving, then closes in on the clue area.

<a id="l0066"></a>
**L0066** — <code>// Patrols spread across coarse regions by preferring ones not recently visited.</code>

Documentation fragment: “Patrols spread across coarse regions by preferring ones not recently visited.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0067"></a>
**L0067** — <code>constexpr int PATROL_REGION = 8;</code>

Patrol regions are 8x8-cell blocks; each records when a monster last entered it.

<a id="l0068"></a>
**L0068** — <code>// Corridor sight (High) only triggers at this distance or more.</code>

Documentation fragment: “Corridor sight (High) only triggers at this distance or more.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0069"></a>
**L0069** — <code>constexpr int SIGHT_MIN_CELLS = 3;</code>

Corridor sight ignores the player at one or two cells; at close quarters monsters behave normally instead of every adjacent monster becoming a capture.

<a id="l0070"></a>
**L0070** — <code>// Generation retries before deterministically adding connections instead.</code>

Documentation fragment: “Generation retries before deterministically adding connections instead.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0071"></a>
**L0071** — <code>constexpr int MAX_GENERATION_ATTEMPTS = 64;</code>

After 64 unlucky layouts, generation stops reseeding and adds connections to the current maze instead.

<a id="l0072"></a>
**L0072** — <code>// The director changes information precision, never movement speed or collisions.</code>

Documentation fragment: “The director changes information precision, never movement speed or collisions.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0073"></a>
**L0073** — <code>constexpr int DIRECTOR_TICK_MS = 200;</code>

Sample director confinement every 200 simulation milliseconds, independently of render cadence.

<a id="l0074"></a>
**L0074** — <code>constexpr int DIRECTOR_RECOVERY_MS = 2000;</code>

Require 2,000 ms of sustained recovery before removing relief once its minimum hold also expires.

<a id="l0075"></a>
**L0075** — <code>constexpr int INF_DISTANCE = 100000000;</code>

Large finite sentinel for unreachable graph distances; comfortably exceeds any path in the maximum grid.

<a id="l0076"></a>
**L0076** — <code>using Millis = std::int64_t;</code>

Use a signed 64-bit integer for simulation milliseconds, supporting negative sentinel timestamps and long runs.

<a id="l0077"></a>
**L0077** — <code>constexpr Millis NEVER = std::numeric_limits&lt;Millis&gt;::max() / 4;</code>

Use one quarter of int64 maximum as an unscheduled-event sentinel, leaving arithmetic headroom when adding durations.

<a id="l0078"></a>
**L0078** — <code>constexpr char WALL = '#', PATH = ' ', EXIT = 'E';</code>

Terrain symbols distinguish wall, traversable passage, and exit. Actors are stored separately, so movement never overwrites terrain.

<a id="l0079"></a>
**L0079** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0080"></a>
**L0080** — <code>// Integer row/column coordinates; grid cells and moving entities are independent.</code>

Documentation fragment: “Integer row/column coordinates; grid cells and moving entities are independent.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0081"></a>
**L0081** — <code>struct Point {</code>

A grid coordinate is an aggregate of row and column; callers can construct it with brace initialization.

<a id="l0082"></a>
**L0082** — <code>    int r = -1, c = -1;</code>

The default (-1,-1) is an invalid coordinate, useful before an actual cell is assigned.

<a id="l0083"></a>
**L0083** — <code>    /// Compare both coordinates.</code>

Documentation fragment: “Compare both coordinates.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0084"></a>
**L0084** — <code>    bool operator==(const Point&amp; p) const { return r == p.r &amp;&amp; c == p.c; }</code>

Coordinate equality compares both axes; vector<Point> route comparisons rely on this operation.

<a id="l0085"></a>
**L0085** — <code>    /// Compare coordinates for inequality.</code>

Documentation fragment: “Compare coordinates for inequality.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0086"></a>
**L0086** — <code>    bool operator!=(const Point&amp; p) const { return !(*this == p); }</code>

Inequality delegates to equality so the two operators remain consistent.

<a id="l0087"></a>
**L0087** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0088"></a>
**L0088** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0089"></a>
**L0089** — <code>enum class MonsterState { Patrolling, Hunting, Searching };</code>

Monsters have three behavior states: roaming patrol, area-directed hunting, and searching the last clue area after losing the scent.

<a id="l0090"></a>
**L0090** — <code>enum class MonsterRole { Chaser, Cutter };</code>

A hunting monster is either a chaser that converges on the clue area or a cutter sent to an exit-route chokepoint.

<a id="l0091"></a>
**L0091** — <code>enum class Outcome { Playing, Won, Caught };</code>

One authoritative outcome distinguishes active play, escape, and capture.

<a id="l0092"></a>
**L0092** — <code>enum class Difficulty { Low, Medium, High };</code>

Difficulty is a scoped enumeration, preventing accidental mixing with unrelated integer settings.

<a id="l0093"></a>
**L0093** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0094"></a>
**L0094** — <code>// Difficulty changes opposition and terrain, not the player's planning ability.</code>

Documentation fragment: “Difficulty changes opposition and terrain, not the player's planning ability.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0095"></a>
**L0095** — <code>// All profiles share the same planner, director rules, and collision handling.</code>

Documentation fragment: “All profiles share the same planner, director rules, and collision handling.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0096"></a>
**L0096** — <code>// hearing_cells is a budget in walkable corridor steps, widened on larger maps.</code>

Documents the units of acoustic reach. Walls can lengthen the shortest corridor route between geometrically close cells.

<a id="l0097"></a>
**L0097** — <code>// search_ms is how long a monster sweeps the last clue area after losing the scent.</code>

Documentation fragment: “search_ms is how long a monster sweeps the last clue area after losing the scent.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0098"></a>
**L0098** — <code>// cutters is how many hunters may be sent to a chokepoint between the clue and the exit.</code>

Documentation fragment: “cutters is how many hunters may be sent to a chokepoint between the clue and the exit.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0099"></a>
**L0099** — <code>// sight_cells lets a monster see the player down a straight open corridor (0 = never).</code>

Documentation fragment: “sight_cells lets a monster see the player down a straight open corridor (0 = never).” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0100"></a>
**L0100** — <code>struct DifficultySettings {</code>

Aggregate configuration record; its positional initialization order below matters when adjusting balance.

<a id="l0101"></a>
**L0101** — <code>    const char* name;</code>

Human-readable difficulty name used in terminal output and HUD labels.

<a id="l0102"></a>
**L0102** — <code>    int minimum_solutions, loop_density_percent;</code>

Route-diversity certificate threshold and percentage multiplier for the extra-connection carving budget.

<a id="l0103"></a>
**L0103** — <code>    int monster_move_ms, hint_area_size, hint_interval_ms, scent_ms;</code>

Monster action period, quantized clue side length, minimum report spacing, and time since movement before scent expires; durations are milliseconds.

<a id="l0104"></a>
**L0104** — <code>    int confinement_ms, relief_ms, padding_step, max_padding;</code>

Confinement observation window, minimum relief hold, uncertainty padding increment, and maximum padding per side.

<a id="l0105"></a>
**L0105** — <code>    int hearing_cells, hearing_per_100, search_ms, cutters, sight_cells;</code>

Store acoustic scaling, the search period, the global interceptor limit and the range of direct corridor sight.

<a id="l0106"></a>
**L0106** — <code>    int report_area_size, report_interval_ms;  // Strategic radio, independent of local hearing.</code>

Keep the nominal side length and minimum interval of a director broadcast independent of acoustic settings.

<a id="l0107"></a>
**L0107** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0108"></a>
**L0108** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0109"></a>
**L0109** — <code>/// Keep all balancing values together; scent memory is independent of move speed.</code>

Documentation fragment: “Keep all balancing values together; scent memory is independent of move speed.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0110"></a>
**L0110** — <code>/// Escape-rate calibration uses 21x21/31x31 maps, five monsters, and a 60 s limit;</code>

Documentation fragment: “Escape-rate calibration uses 21x21/31x31 maps, five monsters, and a 60 s limit;” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0111"></a>
**L0111** — <code>/// those aggregate targets are not guarantees for other sizes or monster counts.</code>

Documentation fragment: “those aggregate targets are not guarantees for other sizes or monster counts.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0112"></a>
**L0112** — <code>constexpr DifficultySettings difficulty_settings(Difficulty difficulty) {</code>

Return a complete immutable balance profile. constexpr permits compile-time evaluation for known difficulty values.

<a id="l0113"></a>
**L0113** — <code>    switch (difficulty) {</code>

Select the aggregate whose field order is declared in DifficultySettings.

<a id="l0114"></a>
**L0114** — <code>        case Difficulty::Low:    return {"Low",    4, 225, 220, 9, 200, 420, 2500, 8000, 4, 16, 18, 10, 1500, 0, 0, 19, 3000};</code>

Construct Low's four-route profile. Monster steps take 220 ms, local clues cover nine cells per side, and normal radio transmits a nominal 19-cell sector every three seconds of eligible movement.

<a id="l0115"></a>
**L0115** — <code>        case Difficulty::Medium: return {"Medium", 2, 135, 148, 5,   0, 560, 4000, 6000, 4, 16, 18, 14, 2500, 1, 0, 17, 1800};</code>

Construct Medium's two-route profile. Monsters step every 148 ms, hear 18 plus the map adjustment, and can search a nominal 17-cell radio sector every 1,800 ms. One interceptor is allowed.

<a id="l0116"></a>
**L0116** — <code>        case Difficulty::High:   return {"High",   2, 100, 118, 5,   0, 840, 6000, 4000, 2,  8, 28, 18, 3500, 2, 6, 11, 1000};</code>

Construct High's two-route profile. Monsters step every 118 ms, hear from a base of 28 corridor steps, and receive an 11-cell nominal report every second. Two interceptors and six-cell maximum sight are enabled.

<a id="l0117"></a>
**L0117** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0118"></a>
**L0118** — <code>    throw std::invalid_argument("Unknown difficulty.");</code>

Reject an invalid enum instead of silently selecting a profile; ordinary UI/CLI parsing produces only supported values.

<a id="l0119"></a>
**L0119** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0120"></a>
**L0120** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0121"></a>
**L0121** — <code>/// Accept menu numbers or case-insensitive names, trimming surrounding whitespace.</code>

Documentation fragment: “Accept menu numbers or case-insensitive names, trimming surrounding whitespace.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0122"></a>
**L0122** — <code>Difficulty parse_difficulty(std::string text) {</code>

Normalize a supplied difficulty token and return the corresponding enum; used by CLI parsing and the retained stream-prompt helper.

<a id="l0123"></a>
**L0123** — <code>    const auto first = text.find_first_not_of(" \t\r\n");</code>

Find the first character that is not ordinary space, tab, carriage return, or newline.

<a id="l0124"></a>
**L0124** — <code>    if (first == std::string::npos) throw std::invalid_argument("Difficulty cannot be empty.");</code>

Reject an all-whitespace token before substring arithmetic.

<a id="l0125"></a>
**L0125** — <code>    text = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);</code>

Trim both ends while preserving any internal characters, which must subsequently match a supported choice exactly.

<a id="l0126"></a>
**L0126** — <code>    for (char&amp; c : text) c = static_cast&lt;char&gt;(std::tolower(static_cast&lt;unsigned char&gt;(c)));</code>

Lowercase safely: cast through unsigned char because cctype functions otherwise have undefined behavior for negative signed-char values.

<a id="l0127"></a>
**L0127** — <code>    if (text == "1" || text == "low") return Difficulty::Low;</code>

Map either menu digit 1 or the normalized name low to Low.

<a id="l0128"></a>
**L0128** — <code>    if (text == "2" || text == "medium") return Difficulty::Medium;</code>

Map 2 or medium to Medium.

<a id="l0129"></a>
**L0129** — <code>    if (text == "3" || text == "high") return Difficulty::High;</code>

Map 3 or high to High.

<a id="l0130"></a>
**L0130** — <code>    throw std::invalid_argument("Choose 1/Low, 2/Medium, or 3/High.");</code>

Provide the accepted spellings in the parsing exception.

<a id="l0131"></a>
**L0131** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0132"></a>
**L0132** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0133"></a>
**L0133** — <code>/// Prompt before maze generation. Enter selects Medium; invalid choices retry.</code>

Documentation fragment: “Prompt before maze generation. Enter selects Medium; invalid choices retry.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0134"></a>
**L0134** — <code>Difficulty prompt_difficulty(std::istream&amp; input, std::ostream&amp; output) {</code>

Retained stream-based prompt, useful to tests/legacy callers; the graphical startup path no longer invokes it.

<a id="l0135"></a>
**L0135** — <code>    output &lt;&lt; "Difficulty: 1 = Low | 2 = Medium | 3 = High\n";</code>

Write the three choices to the supplied stream rather than hard-coding stdout.

<a id="l0136"></a>
**L0136** — <code>    for (;;) {</code>

Repeat until a valid selection or end-of-input; an invalid choice does not abort the prompt.

<a id="l0137"></a>
**L0137** — <code>        output &lt;&lt; "Select difficulty [Enter = Medium]: " &lt;&lt; std::flush;</code>

Flush the prompt before blocking for input, ensuring it is visible without a newline.

<a id="l0138"></a>
**L0138** — <code>        std::string line;</code>

Allocate one input-line buffer for this attempt.

<a id="l0139"></a>
**L0139** — <code>        if (!std::getline(input, line)) throw std::runtime_error("Difficulty input closed.");</code>

Read a whole line; propagate closed input as a runtime error rather than looping forever.

<a id="l0140"></a>
**L0140** — <code>        if (line.find_first_not_of(" \t\r\n") == std::string::npos) return Difficulty::Medium;</code>

Treat an empty or whitespace-only response as the documented Medium default.

<a id="l0141"></a>
**L0141** — <code>        try { return parse_difficulty(line); }</code>

Delegate accepted-number/name interpretation to the shared parser and return immediately on success.

<a id="l0142"></a>
**L0142** — <code>        catch (const std::invalid_argument&amp; e) { output &lt;&lt; e.what() &lt;&lt; '\n'; }</code>

Catch only invalid_argument from parsing, print its message, then retry; other failures are not swallowed.

<a id="l0143"></a>
**L0143** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0144"></a>
**L0144** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0145"></a>
**L0145** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Hints, actors, and simulation state

<a id="l0146"></a>
**L0146** — <code>// Fixed map regions conceal the exact position: the rectangle is not centered on</code>

Documentation fragment: “Fixed map regions conceal the exact position: the rectangle is not centered on” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0147"></a>
**L0147** — <code>// the player. Hunting decisions receive only these bounds and a movement revision.</code>

Documentation fragment: “the player. Hunting decisions receive only these bounds and a movement revision.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0148"></a>
**L0148** — <code>struct LocationHint {</code>

A clue stores bounds of an uncertain rectangle, not an exact target coordinate; only the director constructs it from player position.

<a id="l0149"></a>
**L0149** — <code>    int min_row = -1, max_row = -1, min_col = -1, max_col = -1;</code>

Inclusive row/column limits default to invalid values until a clue is published.

<a id="l0150"></a>
**L0150** — <code>    std::uint64_t revision = 0;</code>

Zero revision means no clue. Later revisions let monsters detect new information even across different render frames.

<a id="l0151"></a>
**L0151** — <code>    /// Test whether a grid coordinate lies inside this approximate location area.</code>

Documentation fragment: “Test whether a grid coordinate lies inside this approximate location area.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0152"></a>
**L0152** — <code>    bool contains(Point p) const {</code>

Membership query for candidate hunting destinations inside a clue rectangle.

<a id="l0153"></a>
**L0153** — <code>        return revision != 0 &amp;&amp; p.r &gt;= min_row &amp;&amp; p.r &lt;= max_row &amp;&amp;</code>

Require an issued clue and inclusive row membership; the logical conjunction continues on the next line.

<a id="l0154"></a>
**L0154** — <code>               p.c &gt;= min_col &amp;&amp; p.c &lt;= max_col;</code>

Require inclusive column membership as well, completing the rectangle test.

<a id="l0155"></a>
**L0155** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0156"></a>
**L0156** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0157"></a>
**L0157** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0158"></a>
**L0158** — <code>// A radio report starts an area search. Local hearing and High corridor sight</code>

Distinguishes a strategic area search from direct sensory evidence that can initiate a hunt.

<a id="l0159"></a>
**L0159** — <code>// start a hunt. Each monster's scent clock is independent of the radio.</code>

Documents why a radio broadcast can affect a distant monster while its physical scent clock stays expired.

<a id="l0160"></a>
**L0160** — <code>struct Monster {</code>

Per-monster behavior and schedule record; entity IDs also select artwork but do not assign abilities.

<a id="l0161"></a>
**L0161** — <code>    int id = 0;</code>

Zero-based stable spawn identifier, assigned in spawn order.

<a id="l0162"></a>
**L0162** — <code>    int cell = -1;</code>

Flattened grid position; -1 means not placed yet.

<a id="l0163"></a>
**L0163** — <code>    MonsterState state = MonsterState::Patrolling;</code>

Newly constructed monsters begin in the patrolling state; they stay frozen until the player's first move or auto-solve toggle.

<a id="l0164"></a>
**L0164** — <code>    MonsterRole role = MonsterRole::Chaser;</code>

Every monster starts as a chaser; assign_interceptors may promote it to a cutter.

<a id="l0165"></a>
**L0165** — <code>    LocationHint hint;</code>

Most recently received uncertain-area clue; its default revision is zero.

<a id="l0166"></a>
**L0166** — <code>    Millis next_move = 0;</code>

Next absolute simulation time at which this monster may attempt movement.

<a id="l0167"></a>
**L0167** — <code>    Millis next_replan = 0;</code>

Next absolute time for periodic target review.

<a id="l0168"></a>
**L0168** — <code>    Millis scent_until = NEVER, search_until = NEVER, role_until = NEVER;</code>

Per-monster deadlines: scent_until ends a hunt, search_until ends a search sweep, and role_until ends an interceptor's hold.

<a id="l0169"></a>
**L0169** — <code>    Millis last_moved_at = 0;</code>

Time of this monster's last step; the auto-solver treats a long-unmoving monster as parked.

<a id="l0170"></a>
**L0170** — <code>    std::uint64_t planned_revision = 0;</code>

Revision of the clue used during the last target-planning pass.

<a id="l0171"></a>
**L0171** — <code>    int target = -1;</code>

Chosen waypoint: a patrol cell, a clue-area cell, an interceptor chokepoint, or a sighted player cell; -1 requests a new target.

<a id="l0172"></a>
**L0172** — <code>    int blocked_turns = 0;</code>

Consecutive failed movement attempts, enabling replanning and sidesteps after blockage.

<a id="l0173"></a>
**L0173** — <code>    bool sighted = false;</code>

True while the monster is charging toward a cell where it actually saw the player (High only).

<a id="l0174"></a>
**L0174** — <code>    std::deque&lt;int&gt; recent_cells;</code>

Bounded recent-cell history discourages immediate repeated patrol targets and steps.

<a id="l0175"></a>
**L0175** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0176"></a>
**L0176** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0177"></a>
**L0177** — <code>// Time-based memory distinguishes actual progress from repeated moves and waits.</code>

Documentation fragment: “Time-based memory distinguishes actual progress from repeated moves and waits.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0178"></a>
**L0178** — <code>struct VisitMemory {</code>

One cell's persistent visitation record for the player's recovery heuristics.

<a id="l0179"></a>
**L0179** — <code>    Millis at = -NEVER;</code>

Default visit timestamp is far in the past; zero heat makes an untouched cell inexpensive regardless.

<a id="l0180"></a>
**L0180** — <code>    double heat = 0;</code>

Accumulated revisit heat, capped on arrival and decayed by elapsed simulation time when queried.

<a id="l0181"></a>
**L0181** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0182"></a>
**L0182** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0183"></a>
**L0183** — <code>// Only the director sees player motion samples. Monsters receive LocationHint.</code>

Documentation fragment: “Only the director sees player motion samples. Monsters receive LocationHint.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0184"></a>
**L0184** — <code>struct DirectorState {</code>

Director-only observation history and relief-control state; monsters are not passed this structure.

<a id="l0185"></a>
**L0185** — <code>    struct Sample { Millis at; int cell; };</code>

A sample pairs an absolute simulation timestamp with the observed player cell.

<a id="l0186"></a>
**L0186** — <code>    std::deque&lt;Sample&gt; samples;</code>

Sliding chronological history for confinement detection.

<a id="l0187"></a>
**L0187** — <code>    Millis next_update = NEVER, relief_until = NEVER, recovered_since = NEVER;</code>

Next sample event, relief minimum-hold expiry, and start of sustained recovery; NEVER marks unscheduled/unset values.

<a id="l0188"></a>
**L0188** — <code>    Millis last_report_at = -NEVER;</code>

Initialize the time of the last broadcast far in the past, making the first successful displacement eligible immediately.

<a id="l0189"></a>
**L0189** — <code>    LocationHint last_report;  // Shared radio memory, including for hunters using fresher local evidence.</code>

Store the last transmitted rectangle and revision. A monster with fresher physical evidence may consult it once that evidence expires.

<a id="l0190"></a>
**L0190** — <code>    std::uint64_t reports = 0;</code>

Count transmissions independently of clue revisions and of the number of recipients.

<a id="l0191"></a>
**L0191** — <code>    int hint_padding = 0, relief_anchor = -1, relief_exit_distance = INF_DISTANCE;</code>

Current extra clue padding, player cell where relief started, and best exit distance recorded at relief onset.

<a id="l0192"></a>
**L0192** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l0193"></a>
**L0193** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0194"></a>
**L0194** — <code>// All game rules live here, independent of GLFW. Tests use this same simulation.</code>

Documentation fragment: “All game rules live here, independent of GLFW. Tests use this same simulation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0195"></a>
**L0195** — <code>class Maze {</code>

Maze owns terrain, actors, rules, RNG, searches, timers, and logging; it contains no GLFW window or GPU handles.

<a id="l0196"></a>
**L0196** — <code>public:</code>

Expose state and simulation operations used by the renderer, application controller, and tests.

<a id="l0197"></a>
**L0197** — <code>    int rows, cols;</code>

Row and column counts after constructor validation and odd-size normalization.

<a id="l0198"></a>
**L0198** — <code>    std::vector&lt;char&gt; grid;</code>

Flat row-major terrain array; grid[r*cols+c] stores a terrain symbol.

<a id="l0199"></a>
**L0199** — <code>    int player = -1, entrance = -1, exit_cell = -1;</code>

Actor and boundary cell indices; entrance and exit are fixed once dimensions are accepted.

<a id="l0200"></a>
**L0200** — <code>    std::vector&lt;Monster&gt; monsters;</code>

Current monster records in stable spawn order.

<a id="l0201"></a>
**L0201** — <code>    std::vector&lt;Point&gt; solution_overlay;</code>

A complete sequence of coordinates for visualization from player to exit, independent of the tactical horizon.

<a id="l0202"></a>
**L0202** — <code>    bool overlay_avoids_monsters = true;</code>

Whether the displayed route avoids currently occupied monster cells; false selects the amber warning presentation.

<a id="l0203"></a>
**L0203** — <code>    std::ostream* log_output;</code>

Optional logging destination; nullptr disables state-event output without altering simulation logic.

<a id="l0204"></a>
**L0204** — <code>    bool verbose_logs = false;</code>

Additional diagnostic verbosity, particularly per-monster movement and changed-route details.

<a id="l0205"></a>
**L0205** — <code>    Outcome outcome = Outcome::Playing;</code>

Initial game state is active; collision/exit checks transition it permanently until reset.

<a id="l0206"></a>
**L0206** — <code>    bool auto_solve = false;</code>

Manual play is the default; explicit UI/keyboard activation enables the automatic planner.

<a id="l0207"></a>
**L0207** — <code>    bool timer_started = false;</code>

The elapsed gameplay timer begins with a successful movement or auto-solve activation, not maze generation.

<a id="l0208"></a>
**L0208** — <code>    int manual_steps = 0, automatic_steps = 0;</code>

Separate counters for actual manual and automatic displacements; waits do not count as steps.

<a id="l0209"></a>
**L0209** — <code>    Millis now = 0, timer_start = 0, elapsed_ms = 0, next_auto = NEVER;</code>

Current simulation time, elapsed-timer origin, cached elapsed duration, and next automatic action deadline.

<a id="l0210"></a>
**L0210** — <code>    std::uint32_t seed;</code>

Original 32-bit random seed, retained for diagnostics and reproducible generation.

<a id="l0211"></a>
**L0211** — <code>    const Difficulty difficulty;</code>

Difficulty enum is const for a particular Maze instance; changing setup creates another Maze.

<a id="l0212"></a>
**L0212** — <code>    const DifficultySettings settings;</code>

Copy of the selected settings is also immutable throughout a run.

<a id="l0213"></a>
**L0213** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0214"></a>
**L0214** — <code>    /// Validate dimensions, round upward to odd values, and allocate a solid grid.</code>

Documentation fragment: “Validate dimensions, round upward to odd values, and allocate a solid grid.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

## Construction and grid access

<a id="l0215"></a>
**L0215** — <code>    Maze(int r, int c, std::uint32_t random_seed, std::ostream* logs = &amp;std::cout,</code>

Constructor accepts dimensions, an unsigned seed, and an optional logging stream; its parameter list continues below.

<a id="l0216"></a>
**L0216** — <code>         Difficulty level = Difficulty::Medium)</code>

Default difficulty is Medium when callers omit the final parameter.

<a id="l0217"></a>
**L0217** — <code>        : rows(r), cols(c), log_output(logs), seed(random_seed),</code>

Initialize dimension fields, logging pointer, and seed directly before the constructor body.

<a id="l0218"></a>
**L0218** — <code>          difficulty(level), settings(difficulty_settings(level)), rng(random_seed) {</code>

Resolve immutable settings and seed the private mt19937 engine; member declaration order governs actual initialization order.

<a id="l0219"></a>
**L0219** — <code>        if (r &lt; 5 || c &lt; 5 || r &gt; MAX_MAZE_DIMENSION || c &gt; MAX_MAZE_DIMENSION)</code>

Reject dimensions outside the supported inclusive range before allocating the grid.

<a id="l0220"></a>
**L0220** — <code>            throw std::runtime_error("Maze dimensions must be between 5 and 501.");</code>

Throw a readable setup error rather than accepting a malformed map size.

<a id="l0221"></a>
**L0221** — <code>        if (rows % 2 == 0) ++rows;</code>

Increase an even row count by one, preserving an odd boundary/interior carving layout.

<a id="l0222"></a>
**L0222** — <code>        if (cols % 2 == 0) ++cols;</code>

Apply the same normalization to columns; an allowed even 500 becomes 501.

<a id="l0223"></a>
**L0223** — <code>        grid.assign(rows * cols, WALL);</code>

Allocate rows*cols cells initialized entirely as walls.

<a id="l0224"></a>
**L0224** — <code>        entrance = id({1, 0});</code>

Place the entrance on the left boundary at row 1.

<a id="l0225"></a>
**L0225** — <code>        exit_cell = id({rows - 2, cols - 1});</code>

Place the exit on the right boundary at the penultimate row.

<a id="l0226"></a>
**L0226** — <code>        player = entrance;</code>

The player initially occupies the entrance; the constructor does not carve passages or spawn enemies.

<a id="l0227"></a>
**L0227** — <code>        graph_signature = terrain_signature();</code>

Record the terrain signature of the freshly allocated all-wall grid so the debug guard accepts this initial state.

<a id="l0228"></a>
**L0228** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0229"></a>
**L0229** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0230"></a>
**L0230** — <code>    /// Convert valid coordinates to a flat index; return -1 outside the grid.</code>

Documentation fragment: “Convert valid coordinates to a flat index; return -1 outside the grid.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0231"></a>
**L0231** — <code>    int id(Point p) const {</code>

Checked coordinate-to-index conversion; returning -1 lets callers reject out-of-bounds neighbors uniformly.

<a id="l0232"></a>
**L0232** — <code>        return p.r &gt;= 0 &amp;&amp; p.r &lt; rows &amp;&amp; p.c &gt;= 0 &amp;&amp; p.c &lt; cols ? p.r * cols + p.c : -1;</code>

Require both axes inside bounds, then calculate the row-major index; the conditional expression returns -1 otherwise.

<a id="l0233"></a>
**L0233** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0234"></a>
**L0234** — <code>    /// Convert a valid flat cell index to row and column.</code>

Documentation fragment: “Convert a valid flat cell index to row and column.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0235"></a>
**L0235** — <code>    Point point(int cell) const { return {cell / cols, cell % cols}; }</code>

Inverse flattening for valid, nonnegative indices: integer division yields row and remainder yields column.

<a id="l0236"></a>
**L0236** — <code>    /// Test bounds and terrain, without making assumptions about entity occupancy.</code>

Documentation fragment: “Test bounds and terrain, without making assumptions about entity occupancy.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0237"></a>
**L0237** — <code>    bool walkable(int cell) const {</code>

Terrain-only traversability query, deliberately independent of monsters and the player.

<a id="l0238"></a>
**L0238** — <code>        return cell &gt;= 0 &amp;&amp; cell &lt; static_cast&lt;int&gt;(grid.size()) &amp;&amp; grid[cell] != WALL;</code>

Check bounds before indexing, then accept every non-wall symbol, including the exit.

<a id="l0239"></a>
**L0239** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0240"></a>
**L0240** — <code>    /// Return four-neighbor Manhattan distance for valid cell indices.</code>

Documentation fragment: “Return four-neighbor Manhattan distance for valid cell indices.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0241"></a>
**L0241** — <code>    int manhattan(int a, int b) const {</code>

Compute the Manhattan heuristic between two valid cells; it ignores walls and therefore underestimates corridor distance.

<a id="l0242"></a>
**L0242** — <code>        Point p = point(a), q = point(b);</code>

Convert both flat indices to coordinates before comparing axes.

<a id="l0243"></a>
**L0243** — <code>        return std::abs(p.r - q.r) + std::abs(p.c - q.c);</code>

Sum absolute axis differences. This lower bound cheaply rejects out-of-range listeners before a corridor search.

<a id="l0244"></a>
**L0244** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0245"></a>
**L0245** — <code>    /// Expose cached cardinal adjacency, with -1 marking absent neighbors.</code>

Documentation fragment: “Expose cached cardinal adjacency, with -1 marking absent neighbors.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0246"></a>
**L0246** — <code>    const std::array&lt;int, 4&gt;&amp; neighbors(int cell) const { return adjacency[cell]; }</code>

Return the precomputed four-neighbor array by const reference; consumers must supply a valid cell index.

<a id="l0247"></a>
**L0247** — <code>    /// Return the number of open cardinal neighbors of a cell.</code>

Documentation fragment: “Return the number of open cardinal neighbors of a cell.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0248"></a>
**L0248** — <code>    int degree(int cell) const { return degrees[cell]; }</code>

Expose cached graph degree for junction and dead-end heuristics.

<a id="l0249"></a>
**L0249** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Maze generation, route-diversity validation, and fallback

<a id="l0250"></a>
**L0250** — <code>    /// Generate until the selected profile's route-diversity requirement is met.</code>

Documentation fragment: “Generate until the selected profile's route-diversity requirement is met.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0251"></a>
**L0251** — <code>    /// The RNG is seeded once, so a failed attempt produces a new maze on the retry.</code>

Documentation fragment: “The RNG is seeded once, so a failed attempt produces a new maze on the retry.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0252"></a>
**L0252** — <code>    /// Spawn monsters only after the topology passes validation.</code>

Documentation fragment: “Spawn monsters only after the topology passes validation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0253"></a>
**L0253** — <code>    void generate(int count) {</code>

Generate terrain until the difficulty's topology certificate passes, then spawn actors. Iterative carving avoids recursion limits.

<a id="l0254"></a>
**L0254** — <code>        if (count &lt; 0 || count &gt; MAX_MONSTERS)</code>

Validate the requested monster count independently of UI validation, since CLI/tests can call this method directly.

<a id="l0255"></a>
**L0255** — <code>            throw std::runtime_error("Monster count must be between 0 and 10.");</code>

Reject negative counts or counts exceeding ten.

<a id="l0256"></a>
**L0256** — <code>        rng.seed(seed);</code>

Reseed once per generate call. Failed topology attempts consume subsequent RNG values rather than repeating the same failed map forever.

<a id="l0257"></a>
**L0257** — <code>        reset_game();</code>

Clear run state before carving; this also removes monsters and any old overlay.

<a id="l0258"></a>
**L0258** — <code>        int attempts = 0;</code>

Count regeneration attempts for diagnostics.

<a id="l0259"></a>
**L0259** — <code>        for (;;) {</code>

Repeat whole-maze generation until the route-diversity check succeeds, or until the attempt cap hands over to the connection fallback.

<a id="l0260"></a>
**L0260** — <code>            ++attempts;</code>

Increment the attempt count at the start of each independently carved candidate.

<a id="l0261"></a>
**L0261** — <code>            std::fill(grid.begin(), grid.end(), WALL);</code>

Restore all cells to walls so a retry does not retain passages from its predecessor.

<a id="l0262"></a>
**L0262** — <code>            std::vector&lt;int&gt; stack{id({1, 1})};</code>

Begin an explicit depth-first carving stack at interior cell (1,1).

<a id="l0263"></a>
**L0263** — <code>            grid[stack.back()] = PATH;</code>

Open the root before exploring neighbors.

<a id="l0264"></a>
**L0264** — <code>            while (!stack.empty()) {</code>

Continue until all reachable odd-lattice cells have been explored and backtracked.

<a id="l0265"></a>
**L0265** — <code>                Point p = point(stack.back());</code>

Convert the current stack tip into coordinates for two-cell carving moves.

<a id="l0266"></a>
**L0266** — <code>                std::array&lt;int, 4&gt; order{0, 1, 2, 3};</code>

Prepare indices for the four cardinal direction vectors.

<a id="l0267"></a>
**L0267** — <code>                std::shuffle(order.begin(), order.end(), rng);</code>

Shuffle direction preference with the game RNG, producing a randomized DFS spanning tree.

<a id="l0268"></a>
**L0268** — <code>                bool carved = false;</code>

Track whether this iteration found an unvisited neighboring room.

<a id="l0269"></a>
**L0269** — <code>                for (int d : order) {</code>

Try shuffled directions in order until the first valid unvisited destination is found.

<a id="l0270"></a>
**L0270** — <code>                    Point q{p.r + 2 * directions[d].r, p.c + 2 * directions[d].c};</code>

Move by two grid cells so the intermediate wall can become a corridor.

<a id="l0271"></a>
**L0271** — <code>                    if (q.r &lt;= 0 || q.r &gt;= rows - 1 || q.c &lt;= 0 || q.c &gt;= cols - 1) continue;</code>

Keep candidate room centers strictly inside the outer boundary.

<a id="l0272"></a>
**L0272** — <code>                    int v = id(q);</code>

Flatten the valid room coordinate for terrain access.

<a id="l0273"></a>
**L0273** — <code>                    if (grid[v] != WALL) continue;</code>

Skip a room already carved; this preserves a tree during the initial DFS phase.

<a id="l0274"></a>
**L0274** — <code>                    grid[v] = PATH;</code>

Open the previously unvisited destination room.

<a id="l0275"></a>
**L0275** — <code>                    grid[id({p.r + directions[d].r, p.c + directions[d].c})] = PATH;</code>

Open the single intervening wall to connect the old and new room.

<a id="l0276"></a>
**L0276** — <code>                    stack.push_back(v);</code>

Push the new room, making it the next DFS expansion point.

<a id="l0277"></a>
**L0277** — <code>                    carved = true;</code>

Record that carving succeeded; the parent remains on the stack for later backtracking.

<a id="l0278"></a>
**L0278** — <code>                    break;</code>

Stop trying alternate directions from this room during this iteration.

<a id="l0279"></a>
**L0279** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0280"></a>
**L0280** — <code>                if (!carved) stack.pop_back();</code>

If every direction failed, pop the exhausted room and resume its parent.

<a id="l0281"></a>
**L0281** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0282"></a>
**L0282** — <code>            grid[entrance] = PATH;</code>

Open the entrance after the interior spanning tree is complete.

<a id="l0283"></a>
**L0283** — <code>            grid[exit_cell] = EXIT;</code>

Mark the connected right-boundary exit with its distinct terrain symbol.

<a id="l0284"></a>
**L0284** — <code>            std::vector&lt;int&gt; walls;</code>

Collect interior wall cells eligible to become extra graph connections.

<a id="l0285"></a>
**L0285** — <code>            for (int r = 1; r &lt; rows - 1; ++r) {</code>

Scan interior rows only, preserving the exterior boundary apart from entrance/exit.

<a id="l0286"></a>
**L0286** — <code>                for (int c = 1; c &lt; cols - 1; ++c) {</code>

Scan interior columns only.

<a id="l0287"></a>
**L0287** — <code>                    int u = id({r, c});</code>

Flatten the candidate wall location.

<a id="l0288"></a>
**L0288** — <code>                    if (grid[u] != WALL) continue;</code>

Ignore passages; only currently solid walls can be opened as extra connections.

<a id="l0289"></a>
**L0289** — <code>                    if ((walkable(u - 1) &amp;&amp; walkable(u + 1)) ||</code>

First eligibility alternative: passages exist immediately to both left and right.

<a id="l0290"></a>
**L0290** — <code>                        (walkable(u - cols) &amp;&amp; walkable(u + cols))) walls.push_back(u);</code>

Second alternative: passages exist immediately above and below; either arrangement adds a connection through this wall.

<a id="l0291"></a>
**L0291** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0292"></a>
**L0292** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0293"></a>
**L0293** — <code>            std::shuffle(walls.begin(), walls.end(), rng);</code>

Randomize the order in which connection candidates will be opened.

<a id="l0294"></a>
**L0294** — <code>            // Integer percentages allow gradual route-density tuning, with no</code>

Documentation fragment: “Integer percentages allow gradual route-density tuning, with no” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0295"></a>
**L0295** — <code>            // extra RNG draws and the same count at legacy 100%/200% settings.</code>

Documentation fragment: “extra RNG draws and the same count at legacy 100%/200% settings.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0296"></a>
**L0296** — <code>            int loop_count = settings.loop_density_percent *</code>

Multiply the map-scaled base carving budget by the difficulty's loop-density percentage.

<a id="l0297"></a>
**L0297** — <code>                             std::max(1, rows * cols / (std::max(rows, cols) &gt; 40 ? 150 : 100)) / 100;</code>

Base budget is at least one, using area/100 for maps with largest dimension <=40 and area/150 otherwise; integer arithmetic truncates.

<a id="l0298"></a>
**L0298** — <code>            if (difficulty == Difficulty::Low) {</code>

Low refreshes the candidate frontier after carving to make extra useful connections possible even on tiny maps.

<a id="l0299"></a>
**L0299** — <code>                // Refresh candidates after each carve: even a 5x5 maze can gain</code>

Documentation fragment: “Refresh candidates after each carve: even a 5x5 maze can gain” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0300"></a>
**L0300** — <code>                // a second connection that was not available in the DFS tree.</code>

Documentation fragment: “a second connection that was not available in the DFS tree.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0301"></a>
**L0301** — <code>                std::vector&lt;unsigned char&gt; queued(grid.size(), 0);</code>

Track whether each wall is already in the candidate list, preventing duplicate additions.

<a id="l0302"></a>
**L0302** — <code>                for (int u : walls) queued[u] = 1;</code>

Mark the initially collected candidates as queued.

<a id="l0303"></a>
**L0303** — <code>                for (int i = 0; i &lt; loop_count; ++i) {</code>

Open candidates until the Low loop budget is exhausted.

<a id="l0304"></a>
**L0304** — <code>                    if (i &gt;= static_cast&lt;int&gt;(walls.size())) break;</code>

Stop early if the expanding candidate list has no further entries.

<a id="l0305"></a>
**L0305** — <code>                    int opened = walls[i]; grid[opened] = PATH;</code>

Retrieve the next candidate and make it traversable.

<a id="l0306"></a>
**L0306** — <code>                    Point p = point(opened);</code>

Convert the just-opened connection to coordinates to inspect neighboring walls.

<a id="l0307"></a>
**L0307** — <code>                    for (Point d : directions) {</code>

Consider all four immediate neighbors because opening one wall may make another eligible.

<a id="l0308"></a>
**L0308** — <code>                        Point q{p.r + d.r, p.c + d.c}; int u = id(q);</code>

Calculate a neighbor coordinate and its possibly invalid flat index.

<a id="l0309"></a>
**L0309** — <code>                        if (q.r &lt;= 0 || q.r &gt;= rows - 1 || q.c &lt;= 0 || q.c &gt;= cols - 1 || grid[u] != WALL) continue;</code>

Short-circuit boundary rejection before accessing grid[u]; ignore neighbors already open.

<a id="l0310"></a>
**L0310** — <code>                        if (((walkable(u - 1) &amp;&amp; walkable(u + 1)) ||</code>

Test whether the newly considered wall connects opposite horizontal passages.

<a id="l0311"></a>
**L0311** — <code>                             (walkable(u - cols) &amp;&amp; walkable(u + cols))) &amp;&amp;</code>

Also accept opposite vertical passages; the predicate continues with duplicate prevention.

<a id="l0312"></a>
**L0312** — <code>                            !queued[u]) { walls.push_back(u); queued[u] = 1; }</code>

Append a newly eligible, not-yet-queued wall and mark it, preserving a finite frontier.

<a id="l0313"></a>
**L0313** — <code>                    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0314"></a>
**L0314** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0315"></a>
**L0315** — <code>            } else {</code>

For Medium/High, use the initially shuffled candidate walls without Low's incremental candidate refresh.

<a id="l0316"></a>
**L0316** — <code>                for (int i = 0; i &lt; std::min(loop_count, static_cast&lt;int&gt;(walls.size())); ++i)</code>

Medium/High carve only the original shuffled candidates, bounded by both requested budget and available count.

<a id="l0317"></a>
**L0317** — <code>                    grid[walls[i]] = PATH;</code>

Turn each selected candidate wall into a passage.

<a id="l0318"></a>
**L0318** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0319"></a>
**L0319** — <code>            rebuild_graph();</code>

Recompute all graph/search metadata against the candidate terrain before validating routes.

<a id="l0320"></a>
**L0320** — <code>            if (route_diversity_met()) break;</code>

Accept the layout as soon as the profile's route-diversity requirement holds.

<a id="l0321"></a>
**L0321** — <code>            if (attempts &gt;= MAX_GENERATION_ATTEMPTS) {</code>

Bound the number of reseeded layouts so generation always terminates.

<a id="l0322"></a>
**L0322** — <code>                add_connections_until_diverse();</code>

Keep this layout and open extra walls until the requirement is met.

<a id="l0323"></a>
**L0323** — <code>                break;</code>

Leave the generation loop with an accepted maze.

<a id="l0324"></a>
**L0324** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0325"></a>
**L0325** — <code>            log_event("Maze generation attempt " + std::to_string(attempts) +</code>

Begin a timestamped rejection diagnostic containing the failed attempt number.

<a id="l0326"></a>
**L0326** — <code>                      ": Insufficient entrance-to-exit route diversity; regenerating.");</code>

Explain that route diversity was insufficient and another full candidate will be generated.

<a id="l0327"></a>
**L0327** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0328"></a>
**L0328** — <code>        log_event("Maze generated after " + std::to_string(attempts) +</code>

Begin the success diagnostic and report how many attempts were required.

<a id="l0329"></a>
**L0329** — <code>                  " attempt(s) | At least " + (settings.minimum_solutions == 4 ? "FOUR" : "TWO") +</code>

Describe the selected profile's certified minimum, distinguishing four from two.

<a id="l0330"></a>
**L0330** — <code>                  " distinct entrance-to-exit paths verified.");</code>

Finish the generation diagnostic; it concerns topology rather than guaranteed safe routes around live monsters.

<a id="l0331"></a>
**L0331** — <code>        spawn_monsters(count);</code>

Only after successful validation, place the requested number of unique monsters.

<a id="l0332"></a>
**L0332** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0333"></a>
**L0333** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0334"></a>
**L0334** — <code>    /// Apply the selected profile's route-diversity requirement to the current terrain.</code>

Documentation fragment: “Apply the selected profile's route-diversity requirement to the current terrain.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0335"></a>
**L0335** — <code>    bool route_diversity_met() const {</code>

Return whether the current terrain meets the selected profile's route-diversity requirement.

<a id="l0336"></a>
**L0336** — <code>        return difficulty == Difficulty::Low ? solution_diversity(true) &gt;= settings.minimum_solutions</code>

Low requires a substantial four-solution certificate; other profiles use the two-solution bridge test.

<a id="l0337"></a>
**L0337** — <code>                                             : has_at_least_two_solutions();</code>

Medium and High accept any terrain with at least two distinct simple entrance-to-exit paths.

<a id="l0338"></a>
**L0338** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0339"></a>
**L0339** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0340"></a>
**L0340** — <code>    /// Guarantee termination after repeated unlucky layouts: keep this maze and</code>

Documentation fragment: “Guarantee termination after repeated unlucky layouts: keep this maze and” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0341"></a>
**L0341** — <code>    /// open extra connections in batches. Every pass opens at least one wall, and a</code>

Documentation fragment: “open extra connections in batches. Every pass opens at least one wall, and a” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0342"></a>
**L0342** — <code>    /// fully opened interior always satisfies every profile, so the loop is finite.</code>

Documentation fragment: “fully opened interior always satisfies every profile, so the loop is finite.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0343"></a>
**L0343** — <code>    void add_connections_until_diverse() {</code>

Fallback that keeps the current maze and adds connections in batches until the profile requirement holds.

<a id="l0344"></a>
**L0344** — <code>        int passes = 0;</code>

Count batches for the log message.

<a id="l0345"></a>
**L0345** — <code>        while (!route_diversity_met()) {</code>

Repeat until the route-diversity requirement passes.

<a id="l0346"></a>
**L0346** — <code>            std::vector&lt;int&gt; candidates;</code>

Collect wall cells that would connect two open cells if opened.

<a id="l0347"></a>
**L0347** — <code>            for (int r = 1; r &lt; rows - 1; ++r)</code>

Scan interior rows only, preserving the outer boundary.

<a id="l0348"></a>
**L0348** — <code>                for (int c = 1; c &lt; cols - 1; ++c) {</code>

Scan interior columns only.

<a id="l0349"></a>
**L0349** — <code>                    int u = id({r, c});</code>

Flatten the interior coordinate.

<a id="l0350"></a>
**L0350** — <code>                    if (grid[u] == WALL &amp;&amp; ((walkable(u - 1) &amp;&amp; walkable(u + 1)) ||</code>

A wall qualifies if both horizontal neighbors are open...

<a id="l0351"></a>
**L0351** — <code>                                            (walkable(u - cols) &amp;&amp; walkable(u + cols)))) candidates.push_back(u);</code>

...or both vertical neighbors are open; opening it creates a new connection.

<a id="l0352"></a>
**L0352** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0353"></a>
**L0353** — <code>            if (candidates.empty()) {</code>

If no wall can connect anything, the interior is effectively saturated.

<a id="l0354"></a>
**L0354** — <code>                for (int r = 1; r &lt; rows - 1; ++r) for (int c = 1; c &lt; cols - 1; ++c) grid[id({r, c})] = PATH;</code>

Open the entire interior, which always satisfies every profile.

<a id="l0355"></a>
**L0355** — <code>                grid[exit_cell] = EXIT;</code>

Restore the exit marker that the interior fill may have overwritten.

<a id="l0356"></a>
**L0356** — <code>                rebuild_graph();</code>

Rebuild adjacency and caches for the fully open interior.

<a id="l0357"></a>
**L0357** — <code>                break;</code>

Stop: the fully open interior meets the requirement.

<a id="l0358"></a>
**L0358** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0359"></a>
**L0359** — <code>            std::shuffle(candidates.begin(), candidates.end(), rng);</code>

Shuffle candidates with the seeded simulation RNG so the fallback stays reproducible.

<a id="l0360"></a>
**L0360** — <code>            std::size_t batch = std::max&lt;std::size_t&gt;(1, candidates.size() / 8);</code>

Open one eighth of the candidates (at least one) per pass, so every pass makes progress.

<a id="l0361"></a>
**L0361** — <code>            for (std::size_t i = 0; i &lt; batch; ++i) grid[candidates[i]] = PATH;</code>

Carve the selected walls.

<a id="l0362"></a>
**L0362** — <code>            rebuild_graph();</code>

Rebuild adjacency, distance caches, and the exit tree for the next check.

<a id="l0363"></a>
**L0363** — <code>            ++passes;</code>

Count this pass.

<a id="l0364"></a>
**L0364** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0365"></a>
**L0365** — <code>        log_event("Maze generation fallback: added connections in " + std::to_string(passes) +</code>

Log that the fallback ran, how many passes it needed...

<a id="l0366"></a>
**L0366** — <code>                  " pass(es) after " + std::to_string(MAX_GENERATION_ATTEMPTS) + " attempts.");</code>

...and after how many reseeded attempts it took over.

<a id="l0367"></a>
**L0367** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0368"></a>
**L0368** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0369"></a>
**L0369** — <code>    /// Verify two distinct simple solutions in O(V+E), ignoring moving entities.</code>

Documentation fragment: “Verify two distinct simple solutions in O(V+E), ignoring moving entities.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0370"></a>
**L0370** — <code>    /// An iterative DFS marks bridges. If its entrance-to-exit tree path contains</code>

Documentation fragment: “An iterative DFS marks bridges. If its entrance-to-exit tree path contains” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0371"></a>
**L0371** — <code>    /// a non-bridge edge, an alternate route exists. A cycle on a side branch alone</code>

Documentation fragment: “a non-bridge edge, an alternate route exists. A cycle on a side branch alone” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0372"></a>
**L0372** — <code>    /// does not qualify. Iteration avoids recursion overflow on large mazes.</code>

Documentation fragment: “does not qualify. Iteration avoids recursion overflow on large mazes.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0373"></a>
**L0373** — <code>    bool has_at_least_two_solutions() const {</code>

Determine whether at least two distinct simple entrance-to-exit paths exist in O(V+E), using iterative bridge analysis.

<a id="l0374"></a>
**L0374** — <code>        if (!walkable(entrance) || !walkable(exit_cell)) return false;</code>

Reject a missing entrance or exit before starting graph traversal.

<a id="l0375"></a>
**L0375** — <code>        const int n = static_cast&lt;int&gt;(grid.size());</code>

Allocate graph metadata by the flat grid size, including indices that correspond to walls.

<a id="l0376"></a>
**L0376** — <code>        std::vector&lt;int&gt; discovered(n, -1), low(n), parent(n, -1), next_neighbor(n, 0);</code>

Discovery times, low-link values, DFS parents, and per-vertex neighbor cursors simulate recursive DFS explicitly.

<a id="l0377"></a>
**L0377** — <code>        std::vector&lt;unsigned char&gt; bridge_to_parent(n, 0);</code>

Each non-root vertex will record whether its parent edge is a bridge.

<a id="l0378"></a>
**L0378** — <code>        std::vector&lt;int&gt; stack{entrance};</code>

Start traversal at the entrance; unreachable components are irrelevant to escape.

<a id="l0379"></a>
**L0379** — <code>        int sequence = 0;</code>

Monotonic DFS discovery counter.

<a id="l0380"></a>
**L0380** — <code>        discovered[entrance] = low[entrance] = sequence++;</code>

Assign the root its first discovery and low-link time.

<a id="l0381"></a>
**L0381** — <code>        while (!stack.empty()) {</code>

Explore until the explicit DFS stack is exhausted.

<a id="l0382"></a>
**L0382** — <code>            int u = stack.back();</code>

Inspect the active vertex at the stack top without popping it prematurely.

<a id="l0383"></a>
**L0383** — <code>            if (next_neighbor[u] &lt; 4) {</code>

If fewer than four neighbor slots have been inspected, advance the DFS scan.

<a id="l0384"></a>
**L0384** — <code>                int v = adjacency[u][next_neighbor[u]++];</code>

Fetch the next cached neighbor and advance this vertex's cursor.

<a id="l0385"></a>
**L0385** — <code>                if (v &lt; 0 || v == parent[u]) continue;</code>

Ignore missing neighbors and the undirected edge leading immediately back to the parent.

<a id="l0386"></a>
**L0386** — <code>                if (discovered[v] &lt; 0) {</code>

An undiscovered neighbor becomes a DFS tree child.

<a id="l0387"></a>
**L0387** — <code>                    parent[v] = u;</code>

Record its parent so bridge results and the final entrance/exit chain can be reconstructed.

<a id="l0388"></a>
**L0388** — <code>                    discovered[v] = low[v] = sequence++;</code>

Assign discovery and initial low-link values from the same timestamp.

<a id="l0389"></a>
**L0389** — <code>                    stack.push_back(v);</code>

Push the child; exploration continues there on the next iteration.

<a id="l0390"></a>
**L0390** — <code>                } else {</code>

A previously discovered non-parent neighbor supplies a back-edge discovery time for the low-link calculation.

<a id="l0391"></a>
**L0391** — <code>                    low[u] = std::min(low[u], discovered[v]);</code>

A previously discovered neighbor lowers the current low-link bound; taking min with a later discovery cannot increase it.

<a id="l0392"></a>
**L0392** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0393"></a>
**L0393** — <code>            } else {</code>

After all four neighbors have been examined, finish this DFS vertex and propagate its low-link value to its parent.

<a id="l0394"></a>
**L0394** — <code>                stack.pop_back();</code>

All neighbor slots are done; complete the vertex and return to its parent.

<a id="l0395"></a>
**L0395** — <code>                int p = parent[u];</code>

Read the parent, with -1 identifying the root.

<a id="l0396"></a>
**L0396** — <code>                if (p &gt;= 0) {</code>

Only non-root vertices have a parent edge to classify.

<a id="l0397"></a>
**L0397** — <code>                    low[p] = std::min(low[p], low[u]);</code>

Propagate the child's reachable low-link minimum upward.

<a id="l0398"></a>
**L0398** — <code>                    bridge_to_parent[u] = low[u] &gt; discovered[p];</code>

A parent edge is a bridge precisely when the child subtree cannot reach the parent or an ancestor without that edge.

<a id="l0399"></a>
**L0399** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0400"></a>
**L0400** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0401"></a>
**L0401** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0402"></a>
**L0402** — <code>        if (discovered[exit_cell] &lt; 0) return false;</code>

No reachable exit means zero solutions.

<a id="l0403"></a>
**L0403** — <code>        for (int u = exit_cell; u != entrance; u = parent[u])</code>

Walk the DFS tree route backward from exit to entrance.

<a id="l0404"></a>
**L0404** — <code>            if (!bridge_to_parent[u]) return true;</code>

Any non-bridge edge along that route admits an alternative simple route; an unrelated side-branch cycle is excluded.

<a id="l0405"></a>
**L0405** — <code>        return false;</code>

If every entrance/exit tree edge is a bridge, the simple solution is unique.

<a id="l0406"></a>
**L0406** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0407"></a>
**L0407** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0408"></a>
**L0408** — <code>    /// Return a conservative lower bound on simple solutions, capped at four.</code>

Documentation fragment: “Return a conservative lower bound on simple solutions, capped at four.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0409"></a>
**L0409** — <code>    /// In a biconnected block, cycle rank r guarantees at least r+1 paths between</code>

Documentation fragment: “In a biconnected block, cycle rank r guarantees at least r+1 paths between” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0410"></a>
**L0410** — <code>    /// its entry/exit vertices (start with a cycle and add ears). Independent</code>

Documentation fragment: “its entry/exit vertices (start with a cycle and add ears). Independent” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0411"></a>
**L0411** — <code>    /// blocks along the entrance/exit route multiply those choices. Off-route</code>

Documentation fragment: “blocks along the entrance/exit route multiply those choices. Off-route” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0412"></a>
**L0412** — <code>    /// cycles contribute nothing. This iterative Tarjan traversal is O(V+E).</code>

Documentation fragment: “cycles contribute nothing. This iterative Tarjan traversal is O(V+E).” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0413"></a>
**L0413** — <code>    /// Low additionally ignores tiny blocks: useful blocks need at least twelve</code>

Documentation fragment: “Low additionally ignores tiny blocks: useful blocks need at least twelve” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0414"></a>
**L0414** — <code>    /// vertices, scaled down for small maps where such separation cannot fit.</code>

Documentation fragment: “vertices, scaled down for small maps where such separation cannot fit.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0415"></a>
**L0415** — <code>    int solution_diversity(bool substantial_only = false) const {</code>

Compute a capped lower-bound certificate for up to four simple solutions, with optional minimum-size filtering of useful cyclic blocks.

<a id="l0416"></a>
**L0416** — <code>        if (!walkable(entrance) || !walkable(exit_cell)) return 0;</code>

Missing traversable endpoints imply zero certified solutions.

<a id="l0417"></a>
**L0417** — <code>        const int n = static_cast&lt;int&gt;(grid.size());</code>

Allocate per-cell DFS metadata for the complete grid index domain.

<a id="l0418"></a>
**L0418** — <code>        std::vector&lt;int&gt; discovered(n, -1), low(n), parent(n, -1), cursor(n, 0);</code>

Track discovery, low-link, parent, and next-neighbor position for iterative Tarjan traversal.

<a id="l0419"></a>
**L0419** — <code>        std::vector&lt;int&gt; tree_block(n, -1), marked(n, -1), choices;</code>

Map tree edges to blocks, count unique vertices per block, and store each block's path-choice contribution.

<a id="l0420"></a>
**L0420** — <code>        std::vector&lt;int&gt; stack{entrance};</code>

Initialize the DFS vertex stack with the entrance.

<a id="l0421"></a>
**L0421** — <code>        std::vector&lt;std::pair&lt;int, int&gt;&gt; edges;</code>

Maintain a separate edge stack so completed biconnected blocks can be extracted.

<a id="l0422"></a>
**L0422** — <code>        int sequence = 0;</code>

Monotonic discovery numbering for low-link calculations.

<a id="l0423"></a>
**L0423** — <code>        const int minimum_block = substantial_only ? std::min(12, ((rows - 1) / 2) * ((cols - 1) / 2)) : 2;</code>

Useful blocks need >=12 vertices when substantial_only is true, reduced to the odd-room count for very small maps; otherwise the threshold is two.

<a id="l0424"></a>
**L0424** — <code>        discovered[entrance] = low[entrance] = sequence++;</code>

Discover the root and initialize its low-link value.

<a id="l0425"></a>
**L0425** — <code>        while (!stack.empty()) {</code>

Process vertices until all reachable blocks are extracted.

<a id="l0426"></a>
**L0426** — <code>            int u = stack.back();</code>

Read the active DFS vertex.

<a id="l0427"></a>
**L0427** — <code>            if (cursor[u] &lt; 4) {</code>

Inspect another one of its four neighbor slots when available.

<a id="l0428"></a>
**L0428** — <code>                int v = adjacency[u][cursor[u]++];</code>

Fetch and advance the vertex-specific adjacency cursor.

<a id="l0429"></a>
**L0429** — <code>                if (v &lt; 0 || v == parent[u]) continue;</code>

Skip absent neighbors and the direct parent edge in this undirected graph.

<a id="l0430"></a>
**L0430** — <code>                if (discovered[v] &lt; 0) {</code>

Create a tree edge when the neighbor is newly discovered.

<a id="l0431"></a>
**L0431** — <code>                    parent[v] = u; edges.emplace_back(u, v);</code>

Record both its parent relation and the edge on the pending block stack.

<a id="l0432"></a>
**L0432** — <code>                    discovered[v] = low[v] = sequence++; stack.push_back(v);</code>

Initialize the child times and descend into it.

<a id="l0433"></a>
**L0433** — <code>                } else if (discovered[v] &lt; discovered[u]) {</code>

For an already discovered ancestor, handle a back edge only once rather than also recording its reverse direction.

<a id="l0434"></a>
**L0434** — <code>                    low[u] = std::min(low[u], discovered[v]); edges.emplace_back(u, v);</code>

Lower the low-link value and push that ancestor edge into the pending block.

<a id="l0435"></a>
**L0435** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0436"></a>
**L0436** — <code>            } else {</code>

Finish a DFS vertex when its adjacency cursor is exhausted, allowing a completed biconnected block to be extracted.

<a id="l0437"></a>
**L0437** — <code>                stack.pop_back(); int p = parent[u];</code>

Finish the current vertex and obtain its DFS parent.

<a id="l0438"></a>
**L0438** — <code>                if (p &lt; 0) continue;</code>

The root has no parent edge to extract here.

<a id="l0439"></a>
**L0439** — <code>                low[p] = std::min(low[p], low[u]);</code>

Propagate low-link information to the parent.

<a id="l0440"></a>
**L0440** — <code>                if (low[u] &lt; discovered[p]) continue;</code>

If the subtree still connects above its parent, this child does not yet close a biconnected block.

<a id="l0441"></a>
**L0441** — <code>                const int block = static_cast&lt;int&gt;(choices.size());</code>

Assign the next block ID based on the number of previously completed blocks.

<a id="l0442"></a>
**L0442** — <code>                int edge_count = 0, vertex_count = 0;</code>

Start counting edges and distinct vertices in the block being popped.

<a id="l0443"></a>
**L0443** — <code>                for (;;) {</code>

Pop edges until the just-completed parent/child tree edge is reached.

<a id="l0444"></a>
**L0444** — <code>                    auto edge = edges.back(); edges.pop_back(); ++edge_count;</code>

Remove one edge and increase the block edge count.

<a id="l0445"></a>
**L0445** — <code>                    for (int v : {edge.first, edge.second}) if (marked[v] != block) {</code>

Inspect its two endpoints, counting each vertex only once for this block ID.

<a id="l0446"></a>
**L0446** — <code>                        marked[v] = block; ++vertex_count;</code>

Stamp the endpoint with the block ID and increase the vertex count.

<a id="l0447"></a>
**L0447** — <code>                    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0448"></a>
**L0448** — <code>                    if (parent[edge.second] == edge.first) tree_block[edge.second] = block;</code>

Associate DFS tree-child edges with this block for later entrance/exit chain traversal.

<a id="l0449"></a>
**L0449** — <code>                    if (edge.first == p &amp;&amp; edge.second == u) break;</code>

The original parent-child edge terminates extraction of the current block.

<a id="l0450"></a>
**L0450** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0451"></a>
**L0451** — <code>                choices.push_back(vertex_count &lt; minimum_block ? 1 : std::min(4, edge_count - vertex_count + 2));</code>

Below-threshold blocks contribute one choice; otherwise use min(4,E-V+2), equivalent to capped cycle rank plus one.

<a id="l0452"></a>
**L0452** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0453"></a>
**L0453** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0454"></a>
**L0454** — <code>        if (discovered[exit_cell] &lt; 0) return 0;</code>

A disconnected exit has no route certificate.

<a id="l0455"></a>
**L0455** — <code>        std::vector&lt;unsigned char&gt; used(choices.size(), 0);</code>

Track which blocks have already contributed so multiple tree edges in the same block are not multiplied repeatedly.

<a id="l0456"></a>
**L0456** — <code>        int total = 1;</code>

Start with one route choice before applying independent block contributions.

<a id="l0457"></a>
**L0457** — <code>        for (int u = exit_cell; u != entrance; u = parent[u]) {</code>

Follow the entrance/exit DFS parent chain, excluding blocks found only on unrelated branches.

<a id="l0458"></a>
**L0458** — <code>            int block = tree_block[u];</code>

Find the biconnected block assigned to this chain edge.

<a id="l0459"></a>
**L0459** — <code>            if (!used[block]) {</code>

Multiply a block's contribution only the first time it is encountered.

<a id="l0460"></a>
**L0460** — <code>                total = std::min(4, total * choices[block]); used[block] = 1;</code>

Cap the product at four and mark the block used; exact counts beyond four are unnecessary for acceptance.

<a id="l0461"></a>
**L0461** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0462"></a>
**L0462** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0463"></a>
**L0463** — <code>        return total;</code>

Return the conservative, capped certificate rather than enumerating all simple paths.

<a id="l0464"></a>
**L0464** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0465"></a>
**L0465** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Graph rebuilding, debug guard, reset, and spawning

<a id="l0466"></a>
**L0466** — <code>    /// Rebuild adjacency, dead-end depths, and search buffers after terrain edits.</code>

Documentation fragment: “Rebuild adjacency, dead-end depths, and search buffers after terrain edits.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0467"></a>
**L0467** — <code>    void rebuild_graph() {</code>

Rebuild adjacency, dead-end structure, cached fields, the exit chokepoint tree, patrol regions, and the terrain signature after terrain changes.

<a id="l0468"></a>
**L0468** — <code>        const int n = static_cast&lt;int&gt;(grid.size());</code>

Use the full grid size for direct-index arrays.

<a id="l0469"></a>
**L0469** — <code>        adjacency.assign(n, {-1, -1, -1, -1});</code>

Initialize all four neighbor slots to absent; walls therefore remain disconnected by default.

<a id="l0470"></a>
**L0470** — <code>        degrees.assign(n, 0);</code>

Reset every degree counter.

<a id="l0471"></a>
**L0471** — <code>        open_cells.clear();</code>

Discard the previous map's list of traversable cells.

<a id="l0472"></a>
**L0472** — <code>        for (int u = 0; u &lt; n; ++u) {</code>

Inspect every grid index exactly once for graph construction.

<a id="l0473"></a>
**L0473** — <code>            if (!walkable(u)) continue;</code>

Walls have no outgoing graph edges.

<a id="l0474"></a>
**L0474** — <code>            open_cells.push_back(u);</code>

Record this traversable cell for spawning and random waypoint sampling.

<a id="l0475"></a>
**L0475** — <code>            Point p = point(u);</code>

Convert the cell index before applying direction offsets.

<a id="l0476"></a>
**L0476** — <code>            for (int d = 0; d &lt; 4; ++d) {</code>

Process the four fixed cardinal directions in deterministic order.

<a id="l0477"></a>
**L0477** — <code>                int v = id({p.r + directions[d].r, p.c + directions[d].c});</code>

Calculate a bounded neighboring index through id().

<a id="l0478"></a>
**L0478** — <code>                if (walkable(v)) { adjacency[u][d] = v; ++degrees[u]; }</code>

Store traversable neighbors and count them; actor occupancy does not change this terrain graph.

<a id="l0479"></a>
**L0479** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0480"></a>
**L0480** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0481"></a>
**L0481** — <code>        // Prune leaves toward junctions; protect the entrance-to-exit backbone.</code>

Documentation fragment: “Prune leaves toward junctions; protect the entrance-to-exit backbone.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0482"></a>
**L0482** — <code>        std::vector&lt;int&gt; remaining = degrees, parent(n, -1), removed;</code>

Copy degrees into mutable pruning counts and allocate parent/removal records; the original degrees stay intact.

<a id="l0483"></a>
**L0483** — <code>        std::queue&lt;int&gt; leaves;</code>

Use a FIFO queue for iterative leaf pruning.

<a id="l0484"></a>
**L0484** — <code>        for (int u : open_cells)</code>

Inspect all open graph vertices for initial removable leaves.

<a id="l0485"></a>
**L0485** — <code>            if (remaining[u] == 1 &amp;&amp; u != entrance &amp;&amp; u != exit_cell) leaves.push(u);</code>

Queue degree-one cells except the entrance and exit, protecting the escape backbone.

<a id="l0486"></a>
**L0486** — <code>        while (!leaves.empty()) {</code>

Prune outward branches until no eligible leaf remains.

<a id="l0487"></a>
**L0487** — <code>            int u = leaves.front(); leaves.pop();</code>

Remove the next queued cell from the FIFO.

<a id="l0488"></a>
**L0488** — <code>            if (remaining[u] != 1) continue;</code>

Discard a stale entry if its remaining degree is no longer one.

<a id="l0489"></a>
**L0489** — <code>            remaining[u] = 0;</code>

Mark the leaf as removed from the conceptual pruned graph.

<a id="l0490"></a>
**L0490** — <code>            removed.push_back(u);</code>

Remember removal order to compute depths in reverse later.

<a id="l0491"></a>
**L0491** — <code>            for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; remaining[v] &gt; 0) {</code>

Find its remaining live neighbor, the direction toward the backbone/junction.

<a id="l0492"></a>
**L0492** — <code>                parent[u] = v;</code>

Record that neighbor as this pruned cell's parent.

<a id="l0493"></a>
**L0493** — <code>                if (--remaining[v] == 1 &amp;&amp; v != entrance &amp;&amp; v != exit_cell) leaves.push(v);</code>

Reduce the neighbor's remaining degree; if it becomes an unprotected leaf, schedule it too.

<a id="l0494"></a>
**L0494** — <code>                break;</code>

A leaf has only one remaining neighbor, so stop scanning once it is handled.

<a id="l0495"></a>
**L0495** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0496"></a>
**L0496** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0497"></a>
**L0497** — <code>        dead_end_depth.assign(n, 0);</code>

Initialize all dead-end depths to zero, which is appropriate for the retained backbone.

<a id="l0498"></a>
**L0498** — <code>        branch_gate.assign(n, -1);</code>

Initialize branch-mouth indices as absent.

<a id="l0499"></a>
**L0499** — <code>        for (auto it = removed.rbegin(); it != removed.rend(); ++it) {</code>

Traverse removals backward so each parent has its depth/gate resolved before its child.

<a id="l0500"></a>
**L0500** — <code>            int p = parent[*it];</code>

Read the parent toward the unpruned region.

<a id="l0501"></a>
**L0501** — <code>            if (p &gt;= 0) {</code>

Only assign branch metadata when a parent was actually recorded.

<a id="l0502"></a>
**L0502** — <code>                dead_end_depth[*it] = dead_end_depth[p] + 1;</code>

The child's blind-branch depth is one greater than its parent's.

<a id="l0503"></a>
**L0503** — <code>                branch_gate[*it] = branch_gate[p] &gt;= 0 ? branch_gate[p] : p;</code>

Inherit an existing branch mouth; otherwise the parent is the first attachment to the backbone.

<a id="l0504"></a>
**L0504** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0505"></a>
**L0505** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0506"></a>
**L0506** — <code>        fields.clear();</code>

Invalidate every cached terrain distance field because the topology may have changed.

<a id="l0507"></a>
**L0507** — <code>        guide_distances.clear(); guide_occupancy.clear();</code>

Clear occupancy-aware exit guidance and its associated monster-position snapshot.

<a id="l0508"></a>
**L0508** — <code>        overlay_dirty = true;</code>

Mark the displayed path stale so it will be rebuilt when auto-solving needs it.

<a id="l0509"></a>
**L0509** — <code>        exit_distances = distance_field(exit_cell);</code>

Compute and retain a terrain-only reverse BFS field rooted at the exit.

<a id="l0510"></a>
**L0510** — <code>        beam_seen.assign(n, 0); beam_index.resize(n); beam_epoch = 0;</code>

Reset per-depth beam deduplication stamps and candidate indices.

<a id="l0511"></a>
**L0511** — <code>        visits.assign(n, {});</code>

Allocate fresh visit-memory records for every grid cell.

<a id="l0512"></a>
**L0512** — <code>        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();</code>

Discard any recovery goal and its occupancy-specific distance field.

<a id="l0513"></a>
**L0513** — <code>        local_pool.clear();</code>

Discard bounded threat fields computed for the previous terrain.

<a id="l0514"></a>
**L0514** — <code>        build_exit_tree();</code>

Recompute the exit-rooted DFS tree used to find route chokepoints for interceptors.

<a id="l0515"></a>
**L0515** — <code>        region_cols = (cols + PATROL_REGION - 1) / PATROL_REGION;</code>

Number of 8x8 patrol regions per row, rounding up to cover partial blocks.

<a id="l0516"></a>
**L0516** — <code>        region_seen.assign(static_cast&lt;std::size_t&gt;(region_cols) * ((rows + PATROL_REGION - 1) / PATROL_REGION), 0);</code>

Allocate one last-visit time per patrol region, all starting at zero.

<a id="l0517"></a>
**L0517** — <code>        graph_signature = terrain_signature();</code>

Remember the terrain signature that these caches describe.

<a id="l0518"></a>
**L0518** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0519"></a>
**L0519** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0520"></a>
**L0520** — <code>    /// Hash the terrain so debug builds can detect edits made without rebuild_graph().</code>

Documentation fragment: “Hash the terrain so debug builds can detect edits made without rebuild_graph().” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0521"></a>
**L0521** — <code>    std::uint64_t terrain_signature() const {</code>

Hash every terrain cell with 64-bit FNV-1a so stale caches can be detected.

<a id="l0522"></a>
**L0522** — <code>        std::uint64_t hash = 1469598103934665603ull;</code>

FNV-1a offset basis.

<a id="l0523"></a>
**L0523** — <code>        for (char c : grid) { hash ^= static_cast&lt;unsigned char&gt;(c); hash *= 1099511628211ull; }</code>

Fold each cell character into the hash with an XOR and the FNV prime.

<a id="l0524"></a>
**L0524** — <code>        return hash ^ grid.size();</code>

Mix in the grid size so differently sized grids cannot collide trivially.

<a id="l0525"></a>
**L0525** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0526"></a>
**L0526** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0527"></a>
**L0527** — <code>    /// Debug-only guard: cached adjacency and distance fields describe the old map</code>

Documentation fragment: “Debug-only guard: cached adjacency and distance fields describe the old map” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0528"></a>
**L0528** — <code>    /// if grid[] changes without rebuild_graph(). Release builds (NDEBUG) skip it.</code>

Documentation fragment: “if grid[] changes without rebuild_graph(). Release builds (NDEBUG) skip it.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0529"></a>
**L0529** — <code>    void verify_graph() const {</code>

Debug-only guard called at simulation entry points.

<a id="l0530"></a>
**L0530** — <code>#ifndef NDEBUG</code>

Compile the check only when NDEBUG is not defined; release builds skip the O(cells) hash.

<a id="l0531"></a>
**L0531** — <code>        if (graph_signature != terrain_signature())</code>

If the terrain hash differs from the one recorded by rebuild_graph()...

<a id="l0532"></a>
**L0532** — <code>            throw std::logic_error("Maze terrain changed without rebuild_graph().");</code>

...fail loudly instead of running on stale adjacency and distance fields.

<a id="l0533"></a>
**L0533** — <code>#endif</code>

End of the debug-only region.

<a id="l0534"></a>
**L0534** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0535"></a>
**L0535** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0536"></a>
**L0536** — <code>    /// Reset runtime state, movement hints, idle deadlines, and route visualization.</code>

Documentation fragment: “Reset runtime state, movement hints, idle deadlines, and route visualization.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0537"></a>
**L0537** — <code>    void reset_game() {</code>

Reset a run's dynamic state without carving a map or reseeding the RNG; generate() performs those operations separately.

<a id="l0538"></a>
**L0538** — <code>        player = entrance;</code>

Return the player to the fixed entrance.

<a id="l0539"></a>
**L0539** — <code>        monsters.clear(); solution_overlay.clear(); player_history.clear();</code>

Remove enemies, visual route, and short-term player action history.

<a id="l0540"></a>
**L0540** — <code>        outcome = Outcome::Playing; auto_solve = false; timer_started = false;</code>

Restore active manual play with the elapsed gameplay timer stopped.

<a id="l0541"></a>
**L0541** — <code>        manual_steps = automatic_steps = 0;</code>

Clear both displacement counters.

<a id="l0542"></a>
**L0542** — <code>        now = timer_start = elapsed_ms = 0; next_auto = NEVER;</code>

Reset simulation and elapsed-clock values; disable the next automatic action.

<a id="l0543"></a>
**L0543** — <code>        cancel_held_move();</code>

Disarm any scheduled manual repeat when resetting the game state.

<a id="l0544"></a>
**L0544** — <code>        movement_revision = 0; idle_deadline = NEVER; last_movement_hint_at = -NEVER;</code>

Reset clue revisions and scent expiry; an old negative last-report time allows the first new report immediately.

<a id="l0545"></a>
**L0545** — <code>        overlay_dirty = true; overlay_reported = false; overlay_avoids_monsters = true;</code>

Reset all route-cache/report flags and its initial optimistic safety indicator.

<a id="l0546"></a>
**L0546** — <code>        last_auto_wait = false; planned_wait_for_idle = false;</code>

Clear both actual and planned waiting indicators so the next wait can be logged accurately.

<a id="l0547"></a>
**L0547** — <code>        priority_rotation = 0;</code>

Restart rotating monster movement priority at spawn order zero.

<a id="l0548"></a>
**L0548** — <code>        guide_distances.clear(); guide_occupancy.clear();</code>

Forget prior occupancy-aware exit guidance.

<a id="l0549"></a>
**L0549** — <code>        director = {};</code>

Value-initialize all director state, including NEVER-valued event deadlines from its member defaults.

<a id="l0550"></a>
**L0550** — <code>        visits.assign(grid.size(), {}); tried_recovery_goals.clear();</code>

Clear visit memory and the list of recently abandoned/reached recovery goals.

<a id="l0551"></a>
**L0551** — <code>        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();</code>

Remove any committed recovery destination and its cached BFS.

<a id="l0552"></a>
**L0552** — <code>        blocked_since = clear_route_since = NEVER; next_recovery_attempt = 0;</code>

Clear blocked/open-route durations and allow an immediate recovery attempt once conditions warrant one.

<a id="l0553"></a>
**L0553** — <code>        navigation_progress_at = recovery_progress_at = 0;</code>

Reset timestamps used to recognize navigation and detour progress.

<a id="l0554"></a>
**L0554** — <code>        best_exit_distance = recovery_best_distance = INF_DISTANCE;</code>

Reset best-so-far distances to unreachable sentinels.

<a id="l0555"></a>
**L0555** — <code>        recovery_deadline = NEVER;</code>

Clear the current recovery commitment deadline.

<a id="l0556"></a>
**L0556** — <code>        recovery_pause_until = 0;</code>

Remove any post-retreat scent-loss pause.

<a id="l0557"></a>
**L0557** — <code>        latest_hint = {};</code>

Clear the last local movement rectangle so a new run cannot inherit the previous player's clue.

<a id="l0558"></a>
**L0558** — <code>        detour_history.clear(); last_reported_boldness = 0;</code>

Clear detour history and the last boldness value reported in the log.

<a id="l0559"></a>
**L0559** — <code>        std::fill(region_seen.begin(), region_seen.end(), 0);</code>

Mark every patrol region as unvisited for the new run.

<a id="l0560"></a>
**L0560** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0561"></a>
**L0561** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0562"></a>
**L0562** — <code>    /// Place unique monsters, preferring cells at a substantial corridor distance.</code>

Documentation fragment: “Place unique monsters, preferring cells at a substantial corridor distance.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0563"></a>
**L0563** — <code>    void spawn_monsters(int count) {</code>

Populate monsters in unique eligible cells, preferring substantial graph distance from the initial player.

<a id="l0564"></a>
**L0564** — <code>        monsters.clear();</code>

Remove any existing monster set before spawning.

<a id="l0565"></a>
**L0565** — <code>        auto from_player = distance_field(player);</code>

Obtain a cached BFS field measuring actual corridor distance from the player, not Manhattan distance.

<a id="l0566"></a>
**L0566** — <code>        std::vector&lt;int&gt; candidates;</code>

Build a list of cells eligible for spawning.

<a id="l0567"></a>
**L0567** — <code>        for (int u : open_cells) if (u != player &amp;&amp; u != exit_cell) candidates.push_back(u);</code>

Exclude the player and exit; every remaining candidate is a terrain-open cell.

<a id="l0568"></a>
**L0568** — <code>        std::shuffle(candidates.begin(), candidates.end(), rng);</code>

Shuffle once with the game RNG so equal-priority candidates retain random order.

<a id="l0569"></a>
**L0569** — <code>        const int preferred_distance = std::min(rows, cols) / 2;</code>

Set the preferred spawn separation to half the shorter maze dimension.

<a id="l0570"></a>
**L0570** — <code>        std::stable_partition(candidates.begin(), candidates.end(), [&amp;](int u) {</code>

Stably group distant cells before near cells, preserving the shuffle within each group.

<a id="l0571"></a>
**L0571** — <code>            return (*from_player)[u] &gt; preferred_distance;</code>

The preference uses corridor distance strictly greater than the threshold; it is not a hard spawn guarantee.

<a id="l0572"></a>
**L0572** — <code>        });</code>

Close the distance-threshold predicate and finish stable partitioning: preferred distant cells precede nearer cells, while the prior shuffled order is preserved within both groups.

<a id="l0573"></a>
**L0573** — <code>        count = std::min(std::max(0, count), std::min(MAX_MONSTERS, static_cast&lt;int&gt;(candidates.size())));</code>

Clamp count to nonnegative, ten, and the number of available distinct cells; tiny mazes may spawn fewer than requested.

<a id="l0574"></a>
**L0574** — <code>        for (int i = 0; i &lt; count; ++i) {</code>

Create the first count candidates as individually identified monsters.

<a id="l0575"></a>
**L0575** — <code>            Monster m;</code>

Default construction supplies patrol behavior and no clue/target.

<a id="l0576"></a>
**L0576** — <code>            m.id = i; m.cell = candidates[i];</code>

Assign stable zero-based ID and the corresponding unique shuffled candidate cell.

<a id="l0577"></a>
**L0577** — <code>            // Offsets are relative to spawn. Monsters stay frozen until the player's</code>

Documentation fragment: “Offsets are relative to spawn. Monsters stay frozen until the player's” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0578"></a>
**L0578** — <code>            // first input; start_timer() shifts these deadlines to that moment.</code>

Documentation fragment: “first input; start_timer() shifts these deadlines to that moment.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0579"></a>
**L0579** — <code>            m.next_move = now + 1 + static_cast&lt;int&gt;(rng() % settings.monster_move_ms);</code>

Stagger initial movement deadlines randomly from 1 through the profile's movement interval, preventing synchronized initial steps.

<a id="l0580"></a>
**L0580** — <code>            m.last_moved_at = now;</code>

Record spawn as this monster's last movement time.

<a id="l0581"></a>
**L0581** — <code>            monsters.push_back(m);</code>

Append the fully placed monster to the active vector.

<a id="l0582"></a>
**L0582** — <code>            mark_region(m.cell);</code>

Credit the spawn cell's patrol region as recently covered.

<a id="l0583"></a>
**L0583** — <code>            log_event("Monster " + std::to_string(m.id + 1) + " spawned at " + cell_label(m.cell) + " | ROAMING");</code>

Log spawn identity, exact diagnostic coordinates, and initial roaming state; these logs are not AI perception input.

<a id="l0584"></a>
**L0584** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0585"></a>
**L0585** — <code>        overlay_dirty = true;</code>

New enemy occupancy invalidates any previous path overlay.

<a id="l0586"></a>
**L0586** — <code>        log_event("Player spawned at " + cell_label(player) + " | Exit " + cell_label(exit_cell));</code>

Log the player's start and goal for development diagnostics.

<a id="l0587"></a>
**L0587** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0588"></a>
**L0588** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Distance fields and bounded threat fields

<a id="l0589"></a>
**L0589** — <code>    /// Cache reverse BFS fields by destination; shared ownership survives eviction.</code>

Documentation fragment: “Cache reverse BFS fields by destination; shared ownership survives eviction.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0590"></a>
**L0590** — <code>    /// Hits move to the back, so long-lived target fields are not evicted by churn.</code>

Documentation fragment: “Hits move to the back, so long-lived target fields are not evicted by churn.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0591"></a>
**L0591** — <code>    std::shared_ptr&lt;const std::vector&lt;int&gt;&gt; distance_field(int destination) {</code>

Return shared ownership of a reverse BFS field; cached retrieval is cheap, while a cache miss traverses the terrain graph.

<a id="l0592"></a>
**L0592** — <code>        for (auto it = fields.begin(); it != fields.end(); ++it) if (it-&gt;first == destination) {</code>

Search the cache for a field toward this destination.

<a id="l0593"></a>
**L0593** — <code>            auto hit = *it;</code>

Copy the cached entry, keeping its shared field alive during the move.

<a id="l0594"></a>
**L0594** — <code>            if (std::next(it) != fields.end()) { fields.erase(it); fields.push_back(hit); }</code>

Move a hit to the back so frequently used target fields survive eviction (least-recently-used order).

<a id="l0595"></a>
**L0595** — <code>            return hit.second;</code>

Return the cached field without running BFS.

<a id="l0596"></a>
**L0596** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0597"></a>
**L0597** — <code>        auto distances = std::make_shared&lt;std::vector&lt;int&gt;&gt;(grid.size(), INF_DISTANCE);</code>

Create a full-grid array initialized unreachable; shared ownership keeps callers safe when the cache evicts an entry.

<a id="l0598"></a>
**L0598** — <code>        std::vector&lt;int&gt; queue;</code>

Use a vector plus head index as an allocation-efficient FIFO.

<a id="l0599"></a>
**L0599** — <code>        if (walkable(destination)) {</code>

Only a traversable destination can seed the BFS.

<a id="l0600"></a>
**L0600** — <code>            (*distances)[destination] = 0;</code>

The goal's distance to itself is zero.

<a id="l0601"></a>
**L0601** — <code>            queue.push_back(destination);</code>

Enqueue the destination as the BFS root.

<a id="l0602"></a>
**L0602** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0603"></a>
**L0603** — <code>        for (std::size_t head = 0; head &lt; queue.size(); ++head) {</code>

Advance through the vector without removing its front.

<a id="l0604"></a>
**L0604** — <code>            int u = queue[head];</code>

Read the next BFS cell.

<a id="l0605"></a>
**L0605** — <code>            for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; (*distances)[v] == INF_DISTANCE) {</code>

Visit each valid neighbor only once by testing the unreachable sentinel.

<a id="l0606"></a>
**L0606** — <code>                (*distances)[v] = (*distances)[u] + 1;</code>

Unit-cost cardinal movement makes the first visit the shortest distance.

<a id="l0607"></a>
**L0607** — <code>                queue.push_back(v);</code>

Append the newly discovered cell to the FIFO.

<a id="l0608"></a>
**L0608** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0609"></a>
**L0609** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0610"></a>
**L0610** — <code>        if (fields.size() &gt;= 24) fields.pop_front();</code>

Keep at most 24 cached fields; eviction is FIFO insertion order, not true LRU.

<a id="l0611"></a>
**L0611** — <code>        fields.emplace_back(destination, distances);</code>

Associate the destination key with the newly computed field.

<a id="l0612"></a>
**L0612** — <code>        return distances;</code>

Return the same shared object to the caller, preserving it even after future cache eviction.

<a id="l0613"></a>
**L0613** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0614"></a>
**L0614** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0615"></a>
**L0615** — <code>    /// Bounded corridor distances. Unreached cells report radius + 1, a lower</code>

Introduces the bounded corridor-distance field used for both tactical and acoustic queries.

<a id="l0616"></a>
**L0616** — <code>    /// bound for threat estimates and an out-of-range value for hearing.</code>

Explains the sentinel for cells beyond the search radius. Tactical forecasts may use it as a conservative distance bound.

<a id="l0617"></a>
**L0617** — <code>    struct LocalField {</code>

Bounded BFS result from one source cell, reusable across many queries.

<a id="l0618"></a>
**L0618** — <code>        int source = -1;</code>

Cell this field was computed from, or -1 when unused.

<a id="l0619"></a>
**L0619** — <code>        int radius = THREAT_RADIUS;</code>

Remember the radius used to build this field. Threat queries default to THREAT_RADIUS.

<a id="l0620"></a>
**L0620** — <code>        std::uint32_t epoch = 0;</code>

Generation counter: a stamp equal to epoch marks a cell reached in the current search.

<a id="l0621"></a>
**L0621** — <code>        std::vector&lt;std::uint32_t&gt; stamp;</code>

Per-cell generation stamps avoid clearing the whole array between searches.

<a id="l0622"></a>
**L0622** — <code>        std::vector&lt;std::uint8_t&gt; dist;</code>

Store visited distances in one byte per cell. Generation stamps distinguish valid bytes from stale entries.

<a id="l0623"></a>
**L0623** — <code>        std::vector&lt;int&gt; queue;</code>

BFS queue, kept to avoid reallocating on every recomputation.

<a id="l0624"></a>
**L0624** — <code>        int at(int cell) const { return stamp[cell] == epoch ? dist[cell] : radius + 1; }</code>

Return the exact distance for this generation. Other cells receive the integer sentinel one beyond this field's configured radius.

<a id="l0625"></a>
**L0625** — <code>    };</code>

Close the LocalField definition.

<a id="l0626"></a>
**L0626** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0627"></a>
**L0627** — <code>    /// Return a bounded field, reusing pooled buffers. Entries still referenced by</code>

Documentation fragment: “Return a bounded field, reusing pooled buffers. Entries still referenced by” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0628"></a>
**L0628** — <code>    /// a live forecast are never overwritten, because only unshared ones are recycled.</code>

Documentation fragment: “a live forecast are never overwritten, because only unshared ones are recycled.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0629"></a>
**L0629** — <code>    std::shared_ptr&lt;const LocalField&gt; local_field(int source, int radius = THREAT_RADIUS) {</code>

Acquire a pooled immutable distance field for a source/radius pair. Callers doing acoustic queries can override the default tactical radius.

<a id="l0630"></a>
**L0630** — <code>        if (radius &lt; 0 || radius &gt; std::numeric_limits&lt;std::uint8_t&gt;::max())</code>

Reject a radius outside the range representable by the distance storage before allocating or indexing a field.

<a id="l0631"></a>
**L0631** — <code>            throw std::invalid_argument("Local distance radius exceeds the byte field.");</code>

Report an invalid radius explicitly instead of wrapping a distance as an unsigned byte.

<a id="l0632"></a>
**L0632** — <code>        for (std::size_t i = local_pool.size(); i-- &gt; 0;) {</code>

Search pooled fields from newest to oldest. A zero-size pool skips the loop.

<a id="l0633"></a>
**L0633** — <code>            if (local_pool[i]-&gt;source != source || local_pool[i]-&gt;radius != radius) continue;</code>

Match both source and search radius. A shorter tactical field cannot answer a longer hearing query at the same source.

<a id="l0634"></a>
**L0634** — <code>            auto hit = local_pool[i];</code>

Copy the shared pointer for the hit.

<a id="l0635"></a>
**L0635** — <code>            local_pool.erase(local_pool.begin() + static_cast&lt;std::ptrdiff_t&gt;(i));</code>

Remove the hit from its old position...

<a id="l0636"></a>
**L0636** — <code>            local_pool.push_back(hit);</code>

...and append it as most recently used.

<a id="l0637"></a>
**L0637** — <code>            return hit;</code>

Return the cached field.

<a id="l0638"></a>
**L0638** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0639"></a>
**L0639** — <code>        std::shared_ptr&lt;LocalField&gt; field;</code>

Holder for a recycled or new field.

<a id="l0640"></a>
**L0640** — <code>        if (local_pool.size() &gt;= MAX_MONSTERS + 4) {</code>

Recycle only once the pool holds 14 fields (ten monsters plus spare slots).

<a id="l0641"></a>
**L0641** — <code>            for (std::size_t i = 0; i &lt; local_pool.size(); ++i) if (local_pool[i].use_count() == 1) {</code>

Pick the oldest entry that nobody else holds; fields still referenced by a forecast are never overwritten.

<a id="l0642"></a>
**L0642** — <code>                field = local_pool[i];</code>

Take ownership of that entry.

<a id="l0643"></a>
**L0643** — <code>                local_pool.erase(local_pool.begin() + static_cast&lt;std::ptrdiff_t&gt;(i));</code>

Remove it from the pool before reuse.

<a id="l0644"></a>
**L0644** — <code>                break;</code>

Stop at the first recyclable entry.

<a id="l0645"></a>
**L0645** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0646"></a>
**L0646** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0647"></a>
**L0647** — <code>        if (!field) field = std::make_shared&lt;LocalField&gt;();</code>

Allocate a new field when nothing could be recycled.

<a id="l0648"></a>
**L0648** — <code>        if (field-&gt;stamp.size() != grid.size()) {</code>

Size the arrays for this grid on first use or after a size change.

<a id="l0649"></a>
**L0649** — <code>            field-&gt;stamp.assign(grid.size(), 0); field-&gt;dist.assign(grid.size(), 0); field-&gt;epoch = 0;</code>

Allocate stamps and distances and reset the epoch.

<a id="l0650"></a>
**L0650** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0651"></a>
**L0651** — <code>        if (++field-&gt;epoch == 0) { std::fill(field-&gt;stamp.begin(), field-&gt;stamp.end(), 0); field-&gt;epoch = 1; }</code>

Advance the epoch; on 32-bit wraparound, clear stamps once so old marks cannot be mistaken for current ones.

<a id="l0652"></a>
**L0652** — <code>        field-&gt;source = source;</code>

Record the source this field now describes.

<a id="l0653"></a>
**L0653** — <code>        field-&gt;radius = radius;</code>

Store the requested radius with the newly initialized or recycled field.

<a id="l0654"></a>
**L0654** — <code>        field-&gt;queue.clear();</code>

Reset the reusable BFS queue.

<a id="l0655"></a>
**L0655** — <code>        if (walkable(source)) {</code>

Only a walkable source seeds the search.

<a id="l0656"></a>
**L0656** — <code>            field-&gt;stamp[source] = field-&gt;epoch; field-&gt;dist[source] = 0; field-&gt;queue.push_back(source);</code>

Stamp the source at distance zero and enqueue it.

<a id="l0657"></a>
**L0657** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0658"></a>
**L0658** — <code>        for (std::size_t head = 0; head &lt; field-&gt;queue.size(); ++head) {</code>

Breadth-first expansion over the queue.

<a id="l0659"></a>
**L0659** — <code>            int u = field-&gt;queue[head], d = field-&gt;dist[u];</code>

Dequeue a cell and read its distance.

<a id="l0660"></a>
**L0660** — <code>            if (d &gt;= radius) continue;</code>

Keep cells at the inclusive radius, but stop expanding from them. Every explored shortest path therefore stays in budget.

<a id="l0661"></a>
**L0661** — <code>            for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; field-&gt;stamp[v] != field-&gt;epoch) {</code>

Visit open neighbors not yet stamped in this generation.

<a id="l0662"></a>
**L0662** — <code>                field-&gt;stamp[v] = field-&gt;epoch; field-&gt;dist[v] = static_cast&lt;std::uint8_t&gt;(d + 1); field-&gt;queue.push_back(v);</code>

Stamp, record distance plus one, and enqueue.

<a id="l0663"></a>
**L0663** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0664"></a>
**L0664** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0665"></a>
**L0665** — <code>        local_pool.push_back(field);</code>

Append the fresh field as most recently used.

<a id="l0666"></a>
**L0666** — <code>        return field;</code>

Return it with shared ownership.

<a id="l0667"></a>
**L0667** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0668"></a>
**L0668** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Private caches, exit chokepoint tree, and patrol regions

<a id="l0669"></a>
**L0669** — <code>private:</code>

Set following class members to private access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l0670"></a>
**L0670** — <code>    static constexpr std::array&lt;Point, 4&gt; directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};</code>

Fixed direction order is up, down, left, right; deterministic scans and tie behavior depend on this order.

<a id="l0671"></a>
**L0671** — <code>    std::mt19937 rng;</code>

One mt19937 engine owns simulation randomness; rendering deliberately uses no draws from it.

<a id="l0672"></a>
**L0672** — <code>    std::vector&lt;std::array&lt;int, 4&gt;&gt; adjacency;</code>

Four cached neighbor slots per flat cell, with -1 for missing directions.

<a id="l0673"></a>
**L0673** — <code>    std::vector&lt;int&gt; degrees, open_cells, dead_end_depth, branch_gate;</code>

Degree, traversable-cell list, blind-branch depth, and branch-mouth arrays support spawning and navigation scoring.

<a id="l0674"></a>
**L0674** — <code>    std::deque&lt;std::pair&lt;int, std::shared_ptr&lt;const std::vector&lt;int&gt;&gt;&gt;&gt; fields;</code>

Small least-recently-used cache of destination-indexed shared reverse BFS arrays; hits move to the back.

<a id="l0675"></a>
**L0675** — <code>    std::shared_ptr&lt;const std::vector&lt;int&gt;&gt; exit_distances;</code>

Retained terrain-only exit-distance field, surviving cache eviction through shared ownership.

<a id="l0676"></a>
**L0676** — <code>    std::vector&lt;int&gt; beam_index;</code>

Beam deduplication index; the A* scratch arrays were removed when A* moved to the tests.

<a id="l0677"></a>
**L0677** — <code>    std::vector&lt;int&gt; guide_distances, guide_occupancy, guide_next;</code>

Cache weighted exit costs, obstacle classifications and the actual optimal next cell for each square.

<a id="l0678"></a>
**L0678** — <code>    std::vector&lt;std::uint32_t&gt; beam_seen;</code>

Beam generation stamps; the A* stamps were removed with the game-side A*.

<a id="l0679"></a>
**L0679** — <code>    std::uint32_t beam_epoch = 0;</code>

Beam epoch counter.

<a id="l0680"></a>
**L0680** — <code>    std::uint64_t movement_revision = 0;</code>

Monotonic clue revision counter; director precision changes also issue a new revision.

<a id="l0681"></a>
**L0681** — <code>    Millis idle_deadline = NEVER;</code>

Time at which the player's latest displacement stops counting as recent movement, or NEVER while idle. Monster hunts use their own scent deadlines.

<a id="l0682"></a>
**L0682** — <code>    Millis next_manual_repeat = NEVER;</code>

Initialize the next repeat to the disabled deadline. An inactive key cannot generate movement on a later tick.

<a id="l0683"></a>
**L0683** — <code>    Point held_direction{0, 0};</code>

Remember the row/column displacement that the scheduler will apply while a movement key remains held.

<a id="l0684"></a>
**L0684** — <code>    Millis last_movement_hint_at = -NEVER;</code>

Last emitted location-report timestamp, separate from the deadline refreshed on every displacement.

<a id="l0685"></a>
**L0685** — <code>    bool overlay_dirty = true, overlay_reported = false, last_auto_wait = false;</code>

Flags for stale overlay, whether its status was logged, and whether the last auto action was a wait.

<a id="l0686"></a>
**L0686** — <code>    bool planned_wait_for_idle = false;</code>

Marks an intentional scent-loss wait so auto_step can explain that choice separately from generic evasion.

<a id="l0687"></a>
**L0687** — <code>    std::size_t priority_rotation = 0;</code>

Rotating starting index gives monsters changing precedence at contested bottlenecks.

<a id="l0688"></a>
**L0688** — <code>    std::deque&lt;int&gt; player_history;</code>

Recent player action cells, including waits; bounded separately from time-decaying visit heat.

<a id="l0689"></a>
**L0689** — <code>    DirectorState director;</code>

Director observation and uncertainty-control state stays inside Maze.

<a id="l0690"></a>
**L0690** — <code>    std::vector&lt;VisitMemory&gt; visits;</code>

Per-cell long-term revisit information used to discourage local oscillation.

<a id="l0691"></a>
**L0691** — <code>    Millis blocked_since = NEVER, clear_route_since = NEVER, next_recovery_attempt = 0;</code>

Start of continuous blockage, start of sustained open routing, and cooldown before another recovery-goal search.

<a id="l0692"></a>
**L0692** — <code>    Millis navigation_progress_at = 0, recovery_deadline = NEVER, recovery_progress_at = 0;</code>

Last recognized novel/exit progress, hard recovery deadline, and last progress toward the current recovery goal.

<a id="l0693"></a>
**L0693** — <code>    Millis recovery_pause_until = 0;</code>

Deadline for the brief scent-loss opportunity after successfully completing a retreat.

<a id="l0694"></a>
**L0694** — <code>    int best_exit_distance = INF_DISTANCE, recovery_target = -1, recovery_best_distance = INF_DISTANCE;</code>

Best terrain exit distance ever reached, active detour endpoint, and best distance achieved toward that endpoint.

<a id="l0695"></a>
**L0695** — <code>    std::vector&lt;int&gt; recovery_guidance, recovery_occupancy;</code>

Recovery-specific reverse BFS and its occupancy snapshot; kept separate from the exit overlay field.

<a id="l0696"></a>
**L0696** — <code>    std::deque&lt;std::pair&lt;int, Millis&gt;&gt; tried_recovery_goals;</code>

Previously tried recovery endpoints with expiry times, discouraging repeated selection of neighboring retreat goals.

<a id="l0697"></a>
**L0697** — <code>    std::vector&lt;std::shared_ptr&lt;LocalField&gt;&gt; local_pool;</code>

Pool of bounded threat fields, most recently used last.

<a id="l0698"></a>
**L0698** — <code>    // Exit-rooted DFS tree: exit_cut_above[u] means exit_parent[u] separates u's</code>

Documentation fragment: “Exit-rooted DFS tree: exit_cut_above[u] means exit_parent[u] separates u's” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0699"></a>
**L0699** — <code>    // subtree from the exit, i.e. every route from there must pass that parent.</code>

Documentation fragment: “subtree from the exit, i.e. every route from there must pass that parent.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0700"></a>
**L0700** — <code>    std::vector&lt;int&gt; exit_parent;</code>

Parent of each cell in the DFS tree rooted at the exit.

<a id="l0701"></a>
**L0701** — <code>    std::vector&lt;unsigned char&gt; exit_cut_above;</code>

Whether a cell's parent is a chokepoint that separates the cell's subtree from the exit.

<a id="l0702"></a>
**L0702** — <code>    std::vector&lt;Millis&gt; region_seen;</code>

Last monster visit time for each 8x8 patrol region.

<a id="l0703"></a>
**L0703** — <code>    int region_cols = 1;</code>

Number of regions per row, used to flatten region coordinates.

<a id="l0704"></a>
**L0704** — <code>    LocationHint latest_hint;</code>

Most recent published clue, delivered to monsters that hear later steps while reports are throttled.

<a id="l0705"></a>
**L0705** — <code>    std::deque&lt;Millis&gt; detour_history;</code>

Start times of recent auto-solver detours, used for loop detection.

<a id="l0706"></a>
**L0706** — <code>    int last_reported_boldness = 0;</code>

Last boldness written to the log, so escalation is reported once per change.

<a id="l0707"></a>
**L0707** — <code>    std::uint64_t graph_signature = 0;</code>

Terrain hash recorded by the last rebuild_graph() call.

<a id="l0708"></a>
**L0708** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0709"></a>
**L0709** — <code>    /// Iterative Tarjan rooted at the exit. Used to find route chokepoints for</code>

Documentation fragment: “Iterative Tarjan rooted at the exit. Used to find route chokepoints for” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0710"></a>
**L0710** — <code>    /// interceptors from an approximate area only; it never reads the player cell.</code>

Documentation fragment: “interceptors from an approximate area only; it never reads the player cell.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0711"></a>
**L0711** — <code>    void build_exit_tree() {</code>

Build the exit-rooted DFS tree and mark chokepoints with an iterative Tarjan traversal (no recursion on large maps).

<a id="l0712"></a>
**L0712** — <code>        const int n = static_cast&lt;int&gt;(grid.size());</code>

Cell count for array sizing.

<a id="l0713"></a>
**L0713** — <code>        exit_parent.assign(n, -1); exit_cut_above.assign(n, 0);</code>

Reset parent links and chokepoint flags.

<a id="l0714"></a>
**L0714** — <code>        if (!walkable(exit_cell)) return;</code>

Without a walkable exit there is no tree to build.

<a id="l0715"></a>
**L0715** — <code>        std::vector&lt;int&gt; disc(n, -1), low(n, 0), cursor(n, 0), stack{exit_cell};</code>

Discovery order, low-link values, per-cell neighbor cursors, and the explicit DFS stack starting at the exit.

<a id="l0716"></a>
**L0716** — <code>        int sequence = 0;</code>

Discovery counter.

<a id="l0717"></a>
**L0717** — <code>        disc[exit_cell] = low[exit_cell] = sequence++;</code>

The exit is discovered first.

<a id="l0718"></a>
**L0718** — <code>        while (!stack.empty()) {</code>

Continue until the whole connected component is explored.

<a id="l0719"></a>
**L0719** — <code>            int u = stack.back();</code>

Inspect the top of the stack.

<a id="l0720"></a>
**L0720** — <code>            if (cursor[u] &lt; 4) {</code>

Neighbors remain to be examined for this cell.

<a id="l0721"></a>
**L0721** — <code>                int v = adjacency[u][cursor[u]++];</code>

Take the next neighbor and advance the cursor.

<a id="l0722"></a>
**L0722** — <code>                if (v &lt; 0 || v == exit_parent[u]) continue;</code>

Skip absent neighbors and the tree edge back to the parent.

<a id="l0723"></a>
**L0723** — <code>                if (disc[v] &lt; 0) {</code>

An undiscovered neighbor becomes a tree child.

<a id="l0724"></a>
**L0724** — <code>                    exit_parent[v] = u; disc[v] = low[v] = sequence++; stack.push_back(v);</code>

Record its parent, assign discovery and low values, and descend.

<a id="l0725"></a>
**L0725** — <code>                } else low[u] = std::min(low[u], disc[v]);</code>

An already discovered neighbor is a back edge that can lower this cell's low-link.

<a id="l0726"></a>
**L0726** — <code>            } else {</code>

Otherwise every neighbor of this cell has been examined:

<a id="l0727"></a>
**L0727** — <code>                stack.pop_back();</code>

All neighbors done: finish this cell.

<a id="l0728"></a>
**L0728** — <code>                int p = exit_parent[u];</code>

Its parent receives the finished child's low-link.

<a id="l0729"></a>
**L0729** — <code>                if (p &lt; 0) continue;</code>

The root has no parent to update.

<a id="l0730"></a>
**L0730** — <code>                low[p] = std::min(low[p], low[u]);</code>

Propagate the lowest reachable discovery time to the parent.

<a id="l0731"></a>
**L0731** — <code>                exit_cut_above[u] = p != exit_cell &amp;&amp; low[u] &gt;= disc[p];</code>

If the child's subtree cannot reach above the parent, the parent (unless it is the exit) separates that subtree from the exit.

<a id="l0732"></a>
**L0732** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0733"></a>
**L0733** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0734"></a>
**L0734** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0735"></a>
**L0735** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0736"></a>
**L0736** — <code>    /// Every route from `from` to the exit passes these cells. Each entry is the</code>

Documentation fragment: “Every route from `from` to the exit passes these cells. Each entry is the” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0737"></a>
**L0737** — <code>    /// chokepoint and its exact corridor distance from `from`.</code>

Documentation fragment: “chokepoint and its exact corridor distance from `from`.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0738"></a>
**L0738** — <code>    std::vector&lt;std::pair&lt;int, int&gt;&gt; exit_chokepoints(int from, const LocationHint&amp; area) const {</code>

List chokepoints that every route from `from` to the exit must pass, each with its exact corridor distance from `from`.

<a id="l0739"></a>
**L0739** — <code>        std::vector&lt;std::pair&lt;int, int&gt;&gt; result;</code>

Collected (cell, distance) pairs, nearest first.

<a id="l0740"></a>
**L0740** — <code>        int previous = from, c = exit_parent[from];</code>

Start the walk at `from` and its exit-tree parent.

<a id="l0741"></a>
**L0741** — <code>        for (int steps = 0; c &gt;= 0 &amp;&amp; c != exit_cell &amp;&amp; steps &lt; CUTTER_SEARCH_STEPS; ++steps) {</code>

Walk toward the exit for at most 80 links, stopping before the exit itself.

<a id="l0742"></a>
**L0742** — <code>            if (exit_cut_above[previous] &amp;&amp; !area.contains(point(c))) {</code>

A parent that separates the previous subtree is a true chokepoint; skip any inside the clue area.

<a id="l0743"></a>
**L0743** — <code>                int ahead = (*exit_distances)[from] - (*exit_distances)[c];</code>

Because every route passes this cell, its distance from `from` is exactly the difference of exit distances.

<a id="l0744"></a>
**L0744** — <code>                if (ahead &gt;= 4) result.emplace_back(c, ahead);</code>

Ignore chokepoints fewer than four steps ahead; they are too close to intercept.

<a id="l0745"></a>
**L0745** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0746"></a>
**L0746** — <code>            previous = c; c = exit_parent[c];</code>

Step one link closer to the exit.

<a id="l0747"></a>
**L0747** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0748"></a>
**L0748** — <code>        return result;</code>

Return the chokepoints in walking order.

<a id="l0749"></a>
**L0749** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0750"></a>
**L0750** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0751"></a>
**L0751** — <code>    /// Record that a monster has covered a coarse patrol region.</code>

Documentation fragment: “Record that a monster has covered a coarse patrol region.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0752"></a>
**L0752** — <code>    void mark_region(int cell) {</code>

Record that a monster has covered the patrol region containing this cell.

<a id="l0753"></a>
**L0753** — <code>        if (region_seen.empty() || !walkable(cell)) return;</code>

Ignore calls before the region table exists and non-walkable cells.

<a id="l0754"></a>
**L0754** — <code>        Point p = point(cell);</code>

Convert the cell to coordinates.

<a id="l0755"></a>
**L0755** — <code>        region_seen[static_cast&lt;std::size_t&gt;((p.r / PATROL_REGION) * region_cols + p.c / PATROL_REGION)] = now;</code>

Store the current simulation time for the 8x8 region containing it.

<a id="l0756"></a>
**L0756** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0757"></a>
**L0757** — <code>    Millis region_age(int cell) const {</code>

Time since any monster last entered the region containing this cell.

<a id="l0758"></a>
**L0758** — <code>        if (region_seen.empty()) return 0;</code>

Before regions exist, report zero age.

<a id="l0759"></a>
**L0759** — <code>        Point p = point(cell);</code>

Convert the cell to coordinates.

<a id="l0760"></a>
**L0760** — <code>        return now - region_seen[static_cast&lt;std::size_t&gt;((p.r / PATROL_REGION) * region_cols + p.c / PATROL_REGION)];</code>

Subtract the region's last visit time from now.

<a id="l0761"></a>
**L0761** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0762"></a>
**L0762** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0763"></a>
**L0763** — <code>public:</code>

Set following class members to public access until another access label or class end; this changes compile-time accessibility, not runtime state.

## Logging, corridor hearing, director radio, and interceptors

<a id="l0764"></a>
**L0764** — <code>    /// Describe the chosen balance and reproduction inputs in the startup terminal.</code>

Documentation fragment: “Describe the chosen balance and reproduction inputs in the startup terminal.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0765"></a>
**L0765** — <code>    std::string configuration_summary() const {</code>

Compose the configuration summary without modifying gameplay state.

<a id="l0766"></a>
**L0766** — <code>        std::ostringstream text;</code>

Use an in-memory stream to format one coherent summary string.

<a id="l0767"></a>
**L0767** — <code>        text &lt;&lt; MAZERUNNER_TITLE &lt;&lt; " | Difficulty: " &lt;&lt; settings.name &lt;&lt; " | seed=" &lt;&lt; seed</code>

Begin the startup summary with the shared title, difficulty name, seed, size, and monster count.

<a id="l0768"></a>
**L0768** — <code>             &lt;&lt; " | " &lt;&lt; rows &lt;&lt; 'x' &lt;&lt; cols &lt;&lt; " | Monsters: " &lt;&lt; monsters.size()</code>

Append actual normalized dimensions and actual spawned monster count.

<a id="l0769"></a>
**L0769** — <code>             &lt;&lt; "\nMonster interval: " &lt;&lt; settings.monster_move_ms &lt;&lt; " ms | Player interval: "</code>

Begin timing diagnostics with the monster movement period and player-action label.

<a id="l0770"></a>
**L0770** — <code>             &lt;&lt; AUTO_SOLVE_SPEED_MS &lt;&lt; " ms | Scent timeout: " &lt;&lt; settings.scent_ms &lt;&lt; " ms"</code>

Append auto-action period and movement-scent duration in milliseconds.

<a id="l0771"></a>
**L0771** — <code>             &lt;&lt; " | Search sweep: " &lt;&lt; settings.search_ms &lt;&lt; " ms"</code>

Report how long a monster sweeps the last clue area after losing the scent.

<a id="l0772"></a>
**L0772** — <code>             &lt;&lt; "\nNormal hint: up to " &lt;&lt; settings.hint_area_size &lt;&lt; 'x' &lt;&lt; settings.hint_area_size</code>

Describe the unpadded base clue rectangle size.

<a id="l0773"></a>
**L0773** — <code>             &lt;&lt; " | Moving updates: ";</code>

Introduce the report-spacing policy.

<a id="l0774"></a>
**L0774** — <code>        if (settings.hint_interval_ms) text &lt;&lt; "at most every " &lt;&lt; settings.hint_interval_ms &lt;&lt; " ms";</code>

A positive hint interval is reported as a minimum time between moving-location reports.

<a id="l0775"></a>
**L0775** — <code>        else text &lt;&lt; "every successful move";</code>

Zero interval means every successful movement can publish a clue.

<a id="l0776"></a>
**L0776** — <code>        text &lt;&lt; " | Hearing radius: " &lt;&lt; hearing_radius() &lt;&lt; " corridor steps | Interceptors: " &lt;&lt; settings.cutters</code>

Print the acoustic reach in corridor steps and the profile's interceptor cap in the startup summary.

<a id="l0777"></a>
**L0777** — <code>             &lt;&lt; " | Corridor sight: ";</code>

Label the corridor-sight setting.

<a id="l0778"></a>
**L0778** — <code>        if (settings.sight_cells) text &lt;&lt; settings.sight_cells &lt;&lt; " cells"; else text &lt;&lt; "off";</code>

Print the sight range in cells, or "off" for profiles without sight.

<a id="l0779"></a>
**L0779** — <code>        text &lt;&lt; "\nDirector radio: first successful step, then every " &lt;&lt; settings.report_interval_ms</code>

Introduce the map-wide radio channel in the startup diagnostic and report its normal cooldown.

<a id="l0780"></a>
**L0780** — <code>             &lt;&lt; " ms on a successful step | Normal radio area: " &lt;&lt; std::min(rows, radio_sector_size())</code>

Print the actual normal report height after limiting the sector to the number of map rows.

<a id="l0781"></a>
**L0781** — <code>             &lt;&lt; 'x' &lt;&lt; std::min(cols, radio_sector_size()) &lt;&lt; " | Remote monsters investigate without gaining scent.";</code>

Append the independently clipped width and describe what a distant recipient does with the report.

<a id="l0782"></a>
**L0782** — <code>        text &lt;&lt; "\nMonsters wait for the first move or auto-solve | Director confinement: "</code>

State the start grace and begin the director parameters.

<a id="l0783"></a>
**L0783** — <code>             &lt;&lt; settings.confinement_ms &lt;&lt; " ms | Relief hold: " &lt;&lt; settings.relief_ms</code>

Continue with confinement and relief-hold durations.

<a id="l0784"></a>
**L0784** — <code>             &lt;&lt; " ms | Minimum maze solutions: " &lt;&lt; settings.minimum_solutions</code>

Append the guaranteed topology-certificate threshold.

<a id="l0785"></a>
**L0785** — <code>             &lt;&lt; " | Extra-connection budget: " &lt;&lt; settings.loop_density_percent &lt;&lt; "% of base";</code>

Report extra-connection percentage relative to the area-based budget; it is not a passage-density percentage.

<a id="l0786"></a>
**L0786** — <code>        return text.str();</code>

Return the completed configuration text.

<a id="l0787"></a>
**L0787** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0788"></a>
**L0788** — <code>    /// Expose director precision for diagnostics; edge clipping can reduce the area.</code>

Documentation fragment: “Expose director precision for diagnostics; edge clipping can reduce the area.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0789"></a>
**L0789** — <code>    int director_hint_width() const { return settings.hint_area_size + 2 * director.hint_padding; }</code>

Nominal uncertain rectangle width equals base side length plus twice the padding; actual bounds may be clipped by map size.

<a id="l0790"></a>
**L0790** — <code>    const char* director_phase() const { return director.hint_padding ? "RELIEF" : "NORMAL"; }</code>

Expose the director's current intensity phase. Positive clue padding corresponds to RELIEF.

<a id="l0791"></a>
**L0791** — <code>    std::uint64_t director_reports() const { return director.reports; }</code>

Return the cumulative broadcast count for the HUD, terminal and regressions.

<a id="l0792"></a>
**L0792** — <code>    /// Actual radio footprint on this map, including any current relief.</code>

Describes the dimensions the HUD reports for this particular map and current director phase.

<a id="l0793"></a>
**L0793** — <code>    std::string radio_area_label() const {</code>

Format the current strategic footprint for diagnostics without changing simulation state.

<a id="l0794"></a>
**L0794** — <code>        int size = radio_sector_size() + 2 * director.hint_padding;</code>

Add padding to both sides of the normal radio sector.

<a id="l0795"></a>
**L0795** — <code>        return std::to_string(std::min(rows, size)) + "x" + std::to_string(std::min(cols, size));</code>

Clip height and width separately and format them in row-by-column order. A rectangular board can clip only one axis.

<a id="l0796"></a>
**L0796** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0797"></a>
**L0797** — <code>    /// Expose the committed detour endpoint, or -1 during ordinary exit planning.</code>

Documentation fragment: “Expose the committed detour endpoint, or -1 during ordinary exit planning.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0798"></a>
**L0798** — <code>    int auto_recovery_target() const { return recovery_target; }</code>

Expose only the active recovery waypoint ID for testing/diagnostics, or -1 if none.

<a id="l0799"></a>
**L0799** — <code>    /// Walkable distance within which a monster hears a player step. Larger maps</code>

Documents an acoustic reach measured over walkable cells and scaled by the longer dimension.

<a id="l0800"></a>
**L0800** — <code>    /// widen it so difficulty does not collapse as monster density falls.</code>

Documentation fragment: “widen it so difficulty does not collapse as monster density falls.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0801"></a>
**L0801** — <code>    int hearing_radius() const {</code>

Compute acoustic reach in walkable steps using the profile and longer map axis.

<a id="l0802"></a>
**L0802** — <code>        return settings.hearing_cells + settings.hearing_per_100 * std::max(rows, cols) / 100;</code>

Base radius plus the per-100-cells increment, scaled by the larger map dimension.

<a id="l0803"></a>
**L0803** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0804"></a>
**L0804** — <code>    /// Expose solver escalation for diagnostics: recent detours raise it.</code>

Documentation fragment: “Expose solver escalation for diagnostics: recent detours raise it.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0805"></a>
**L0805** — <code>    int auto_boldness() const {</code>

Diagnostic accessor for solver escalation after repeated detours.

<a id="l0806"></a>
**L0806** — <code>        int recent = 0;</code>

Count recent detours.

<a id="l0807"></a>
**L0807** — <code>        for (Millis at : detour_history) recent += now - at &lt; AUTO_LOOP_WINDOW_MS;</code>

A detour counts if it started within the 20-second loop window.

<a id="l0808"></a>
**L0808** — <code>        return std::min(AUTO_MAX_BOLDNESS, std::max(0, recent - 1));</code>

The first recent detour is free; each additional one adds one boldness, capped at four.

<a id="l0809"></a>
**L0809** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0810"></a>
**L0810** — <code>    /// Format one cell for terminal diagnostics without exposing it to monster AI.</code>

Documentation fragment: “Format one cell for terminal diagnostics without exposing it to monster AI.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0811"></a>
**L0811** — <code>    std::string cell_label(int cell) const {</code>

Format a terrain-valid cell for logs; this utility does not grant monsters player-position access.

<a id="l0812"></a>
**L0812** — <code>        if (!walkable(cell)) return "none";</code>

Invalid/wall indices have the diagnostic label none.

<a id="l0813"></a>
**L0813** — <code>        Point p = point(cell);</code>

Convert a valid index to row and column.

<a id="l0814"></a>
**L0814** — <code>        return "(" + std::to_string(p.r) + "," + std::to_string(p.c) + ")";</code>

Return a parenthesized coordinate pair in row,column order.

<a id="l0815"></a>
**L0815** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0816"></a>
**L0816** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0817"></a>
**L0817** — <code>    /// Append a complete timestamped line; avoid overwriting earlier state messages.</code>

Documentation fragment: “Append a complete timestamped line; avoid overwriting earlier state messages.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0818"></a>
**L0818** — <code>    void log_event(const std::string&amp; message) const {</code>

Append one timestamped diagnostic event when a logging destination exists.

<a id="l0819"></a>
**L0819** — <code>        if (!log_output) return;</code>

Quiet mode avoids any event output by supplying a null stream pointer.

<a id="l0820"></a>
**L0820** — <code>        std::ostringstream line;</code>

Construct a complete log line before writing it.

<a id="l0821"></a>
**L0821** — <code>        line &lt;&lt; '[' &lt;&lt; std::fixed &lt;&lt; std::setprecision(3) &lt;&lt; now / 1000.0 &lt;&lt; "s] " &lt;&lt; message &lt;&lt; '\n';</code>

Convert integer simulation milliseconds to seconds and display three fractional digits.

<a id="l0822"></a>
**L0822** — <code>        *log_output &lt;&lt; line.str();</code>

Write the composed line without carriage-return overwriting of older logs.

<a id="l0823"></a>
**L0823** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0824"></a>
**L0824** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0825"></a>
**L0825** — <code>    /// Provide an explicit status snapshot for the terminal while preserving history.</code>

Documentation fragment: “Provide an explicit status snapshot for the terminal while preserving history.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0826"></a>
**L0826** — <code>    void log_status() const {</code>

Emit a snapshot of player state, director status, and hunting/searching counts.

<a id="l0827"></a>
**L0827** — <code>        std::ostringstream text;</code>

Prepare a stream for the multi-field status message.

<a id="l0828"></a>
**L0828** — <code>        int hunting = 0, searching = 0;</code>

Count hunting and searching monsters for the status line.

<a id="l0829"></a>
**L0829** — <code>        for (const auto&amp; m : monsters) {</code>

Visit every monster.

<a id="l0830"></a>
**L0830** — <code>            hunting += m.state == MonsterState::Hunting;</code>

Count hunters.

<a id="l0831"></a>
**L0831** — <code>            searching += m.state == MonsterState::Searching;</code>

Count searchers.

<a id="l0832"></a>
**L0832** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0833"></a>
**L0833** — <code>        text &lt;&lt; "Player " &lt;&lt; cell_label(player) &lt;&lt; " | Manual steps: " &lt;&lt; manual_steps</code>

Include player coordinate and manual displacement count.

<a id="l0834"></a>
**L0834** — <code>             &lt;&lt; " | Auto steps: " &lt;&lt; automatic_steps &lt;&lt; " | Time: " &lt;&lt; std::fixed</code>

Include automatic displacement count and begin fixed-point elapsed time formatting.

<a id="l0835"></a>
**L0835** — <code>             &lt;&lt; std::setprecision(2) &lt;&lt; elapsed_ms / 1000.0 &lt;&lt; " s | Auto-solve: " &lt;&lt; (auto_solve ? "ON" : "OFF")</code>

Report elapsed seconds with two decimals and auto-solve ON/OFF state.

<a id="l0836"></a>
**L0836** — <code>             &lt;&lt; " | Difficulty: " &lt;&lt; settings.name</code>

Include selected difficulty.

<a id="l0837"></a>
**L0837** — <code>             &lt;&lt; " | Director: " &lt;&lt; director_phase() &lt;&lt; " | Radio reports: " &lt;&lt; director.reports</code>

Append the director phase and broadcast count to the terminal state snapshot.

<a id="l0838"></a>
**L0838** — <code>             &lt;&lt; " | Radio area: " &lt;&lt; radio_area_label()</code>

Append the current radio footprint so logs can distinguish a broad relief search from a normal report.

<a id="l0839"></a>
**L0839** — <code>             &lt;&lt; " | Hint width: " &lt;&lt; director_hint_width()</code>

Continue the status line with the director hint width.

<a id="l0840"></a>
**L0840** — <code>             &lt;&lt; " | Hunting: " &lt;&lt; hunting &lt;&lt; " | Searching: " &lt;&lt; searching;</code>

Append the hunting and searching counts.

<a id="l0841"></a>
**L0841** — <code>        log_event(text.str());</code>

Route the assembled status through the standard timestamped logger.

<a id="l0842"></a>
**L0842** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0843"></a>
**L0843** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0844"></a>
**L0844** — <code>private:</code>

Set following class members to private access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l0845"></a>
**L0845** — <code>    static std::string monster_name(const Monster&amp; m) { return "Monster " + std::to_string(m.id + 1); }</code>

Format a monster's one-based display name for log lines.

<a id="l0846"></a>
**L0846** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0847"></a>
**L0847** — <code>    /// On a small board a normal radio sector still identifies part of the maze.</code>

Explains the small-map upper bound on the normal broadcast width.

<a id="l0848"></a>
**L0848** — <code>    int radio_sector_size() const {</code>

Calculate the normal map-wide radio sector before relief padding or clipping by axis.

<a id="l0849"></a>
**L0849** — <code>        return std::min(settings.report_area_size, std::max(settings.hint_area_size, (2 * std::max(rows, cols) + 1) / 3));</code>

Limit the profile's nominal side by about two thirds of the longer dimension. The cap is bounded below by the local clue side.

<a id="l0850"></a>
**L0850** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0851"></a>
**L0851** — <code>[blank line]</code>

A blank separator groups the adjacent declarations or operations.

<a id="l0852"></a>
**L0852** — <code>    /// Grow a remembered rectangle on the map without looking up the player.</code>

Describes widening the evidence already held by a monster or the director.

<a id="l0853"></a>
**L0853** — <code>    LocationHint widen_hint(LocationHint hint, int padding) const {</code>

Work on a copy of a remembered rectangle so callers choose which stored memory to replace.

<a id="l0854"></a>
**L0854** — <code>        if (!hint.revision || !padding) return hint;</code>

Preserve an unpublished or already appropriately sized clue when there is no padding to apply.

<a id="l0855"></a>
**L0855** — <code>        int height = std::min(rows, hint.max_row - hint.min_row + 1 + 2 * padding);</code>

Increase the height at both edges and cap it at the full map height.

<a id="l0856"></a>
**L0856** — <code>        int width = std::min(cols, hint.max_col - hint.min_col + 1 + 2 * padding);</code>

Apply the same expansion independently to the rectangle's width.

<a id="l0857"></a>
**L0857** — <code>        hint.min_row = std::clamp(hint.min_row - padding, 0, rows - height);</code>

Shift the expanded top edge into bounds while preserving its requested height.

<a id="l0858"></a>
**L0858** — <code>        hint.min_col = std::clamp(hint.min_col - padding, 0, cols - width);</code>

Shift the expanded left edge into bounds while preserving its requested width.

<a id="l0859"></a>
**L0859** — <code>        hint.max_row = hint.min_row + height - 1;</code>

Set the inclusive maximum row from the shifted origin and the expanded height.

<a id="l0860"></a>
**L0860** — <code>        hint.max_col = hint.min_col + width - 1;</code>

Set the inclusive maximum column from the shifted origin and the expanded width.

<a id="l0861"></a>
**L0861** — <code>        return hint;</code>

Return the expanded rectangle with its original evidence revision.

<a id="l0862"></a>
**L0862** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0863"></a>
**L0863** — <code>[blank line]</code>

A blank separator groups the adjacent declarations or operations.

<a id="l0864"></a>
**L0864** — <code>    /// Quantize a successful step into a fixed sector before applying relief.</code>

Describes quantizing a successful movement into a fixed sector shared by many possible player cells.

<a id="l0865"></a>
**L0865** — <code>    LocationHint make_location_hint(int cell, int sector, std::uint64_t revision) const {</code>

Build a clue using the selected channel's sector size and the revision of this movement event.

<a id="l0866"></a>
**L0866** — <code>        Point p = point(cell);</code>

Convert the flattened player cell into grid coordinates solely to select its sector.

<a id="l0867"></a>
**L0867** — <code>        int height = std::min(rows, sector), width = std::min(cols, sector);</code>

Compute the usable sector height and width independently for a rectangular or small map.

<a id="l0868"></a>
**L0868** — <code>        int row = std::min(p.r / sector * sector, rows - height);</code>

Snap the top edge to a grid of sectors. The last sector can slide inward to retain its clipped height.

<a id="l0869"></a>
**L0869** — <code>        int col = std::min(p.c / sector * sector, cols - width);</code>

Apply the same quantization and edge rule to the column.

<a id="l0870"></a>
**L0870** — <code>        return widen_hint({row, row + height - 1, col, col + width - 1, revision}, director.hint_padding);</code>

Construct inclusive bounds and then apply the director's current padding to this coarse rectangle.

<a id="l0871"></a>
**L0871** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0872"></a>
**L0872** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0873"></a>
**L0873** — <code>    /// Give a monster the latest area clue and keep a still-valid target.</code>

Documentation fragment: “Give a monster the latest area clue and keep a still-valid target.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0874"></a>
**L0874** — <code>    void deliver_hint(Monster&amp; m, const LocationHint&amp; hint) {</code>

Give one monster the latest area clue and keep a target that is still valid.

<a id="l0875"></a>
**L0875** — <code>        bool was_hunting = m.state == MonsterState::Hunting;</code>

Remember whether this monster was already hunting.

<a id="l0876"></a>
**L0876** — <code>        if (!was_hunting) {</code>

A roaming or searching monster becomes a hunter.

<a id="l0877"></a>
**L0877** — <code>            log_event(monster_name(m) + ": Heard the player, switching " +</code>

Log which state the monster leaves...

<a id="l0878"></a>
**L0878** — <code>                      (m.state == MonsterState::Searching ? "SEARCHING" : "ROAMING") + " -&gt; HUNTING.");</code>

...naming SEARCHING or ROAMING as the previous state.

<a id="l0879"></a>
**L0879** — <code>            m.state = MonsterState::Hunting;</code>

Switch to hunting.

<a id="l0880"></a>
**L0880** — <code>            m.search_until = NEVER;</code>

Cancel any search deadline.

<a id="l0881"></a>
**L0881** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0882"></a>
**L0882** — <code>        if (!was_hunting || m.hint.revision != hint.revision) {</code>

Invalidate the previous plan when entering a hunt or receiving a different clue revision.

<a id="l0883"></a>
**L0883** — <code>            m.hint = hint;</code>

Store the new clue.

<a id="l0884"></a>
**L0884** — <code>            if (!was_hunting || (!m.sighted &amp;&amp; m.role == MonsterRole::Chaser &amp;&amp;</code>

Retarget a new hunter, or a chaser (not sighted)...

<a id="l0885"></a>
**L0885** — <code>                                 (!walkable(m.target) || !hint.contains(point(m.target))))) {</code>

...whose target is invalid or now outside the clue area.

<a id="l0886"></a>
**L0886** — <code>                m.target = -1; m.blocked_turns = 0;</code>

Clear the target and blocked count so the next turn chooses afresh.

<a id="l0887"></a>
**L0887** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0888"></a>
**L0888** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0889"></a>
**L0889** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0890"></a>
**L0890** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0891"></a>
**L0891** — <code>    /// Direct a remote patrol to investigate a sector. A report never grants</code>

Documents that distant monsters investigate strategic information through their search state.

<a id="l0892"></a>
**L0892** — <code>    /// hearing, renews scent, or displaces a live hunt or first-hand sighting.</code>

Specifies the priority of physical evidence and the separation of the scent clock from radio.

<a id="l0893"></a>
**L0893** — <code>    void investigate_report(Monster&amp; m, const LocationHint&amp; report) {</code>

Deliver a coarse report to a monster eligible for remote investigation.

<a id="l0894"></a>
**L0894** — <code>        const bool was_patrolling = m.state == MonsterState::Patrolling;</code>

Record whether this delivery changes the monster from patrol to search.

<a id="l0895"></a>
**L0895** — <code>        if (was_patrolling)</code>

Emit the state-transition message only on the initial transition.

<a id="l0896"></a>
**L0896** — <code>            log_event(monster_name(m) + ": Director radio, switching ROAMING -&gt; SEARCHING.");</code>

Identify the transition as a radio-triggered investigation in the terminal log.

<a id="l0897"></a>
**L0897** — <code>        m.state = MonsterState::Searching;</code>

Enter or continue the search state. Future monster moves choose waypoints using the stored area.

<a id="l0898"></a>
**L0898** — <code>        m.hint = report;</code>

Store only the reported bounds and revision as this monster's current evidence.

<a id="l0899"></a>
**L0899** — <code>        m.search_until = now + settings.search_ms;</code>

Reset the search deadline for this fresh broadcast using the profile's sweep duration.

<a id="l0900"></a>
**L0900** — <code>        if (was_patrolling || !walkable(m.target) || !report.contains(point(m.target))) {</code>

Reconsider a waypoint for a fresh patrol assignment, an invalid target or a target outside the new bounds.

<a id="l0901"></a>
**L0901** — <code>            m.target = -1;</code>

Discard the old target so a later monster step selects a walkable destination inside the sector.

<a id="l0902"></a>
**L0902** — <code>            m.blocked_turns = 0;</code>

Clear congestion history accumulated on the discarded route.

<a id="l0903"></a>
**L0903** — <code>        }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0904"></a>
**L0904** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0905"></a>
**L0905** — <code>[blank line]</code>

A blank separator groups the adjacent declarations or operations.

<a id="l0906"></a>
**L0906** — <code>    /// A real displacement produces local sound and can also trigger an independent</code>

Documents the two information paths that may originate from one successful displacement.

<a id="l0907"></a>
**L0907** — <code>    /// map-wide director report. Waiting, blocked input and clock ticks send neither.</code>

Specifies that inactivity, blocked movement and director sampling produce no new movement evidence.

<a id="l0908"></a>
**L0908** — <code>    void publish_movement_hint() {</code>

Determine acoustic listeners, renew physical contact, publish eligible local clues and independently broadcast a strategic area.

<a id="l0909"></a>
**L0909** — <code>        bool first_movement = idle_deadline == NEVER;</code>

An absent active deadline means this is the first movement after an idle spell or at game start.

<a id="l0910"></a>
**L0910** — <code>        idle_deadline = now + settings.scent_ms;</code>

Mark the player as moving for one scent period; this drives first-movement detection and the director's idle test.

<a id="l0911"></a>
**L0911** — <code>        const int radius = hearing_radius();</code>

This map's hearing radius.

<a id="l0912"></a>
**L0912** — <code>        std::vector&lt;std::size_t&gt; listeners;</code>

Indices of monsters that hear this step.

<a id="l0913"></a>
**L0913** — <code>        std::shared_ptr&lt;const LocalField&gt; sound;</code>

Delay creating the shared acoustic distance field until a geometrically plausible listener is found.

<a id="l0914"></a>
**L0914** — <code>        for (std::size_t i = 0; i &lt; monsters.size(); ++i) {</code>

Examine each monster once to identify which could hear this step.

<a id="l0915"></a>
**L0915** — <code>            const int cell = monsters[i].cell;</code>

Read its position for the geometric prefilter and the corridor-distance lookup.

<a id="l0916"></a>
**L0916** — <code>            if (manhattan(cell, player) &gt; radius) continue;  // Corridor distance is at least this large.</code>

Reject an impossible listener cheaply using Manhattan distance as a lower bound on corridor distance.

<a id="l0917"></a>
**L0917** — <code>            if (!sound) sound = local_field(player, radius);  // One bounded BFS for all potential listeners.</code>

Build or retrieve one bounded corridor search rooted at the player and reuse it for the remaining listeners.

<a id="l0918"></a>
**L0918** — <code>            if (sound-&gt;at(cell) &lt;= radius) listeners.push_back(i);</code>

Record a listener only if its actual walkable distance falls within the inclusive budget.

<a id="l0919"></a>
**L0919** — <code>        }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0920"></a>
**L0920** — <code>        // Scent tracks every audible displacement even when the director withholds</code>

Documentation fragment: “Scent tracks every audible displacement even when the director withholds” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0921"></a>
**L0921** — <code>        // a location update. No queued report can fire after the player stops.</code>

Documentation fragment: “a location update. No queued report can fire after the player stops.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0922"></a>
**L0922** — <code>        const int local_interval = director.hint_padding</code>

Choose the publication interval for local clue revisions from the current director phase.

<a id="l0923"></a>
**L0923** — <code>            ? std::max(settings.hint_interval_ms, 2 * DIRECTOR_TICK_MS) : settings.hint_interval_ms;</code>

During relief enforce a minimum of two 200-ms director ticks between local revisions. Physical hearing can still renew scent between revisions.

<a id="l0924"></a>
**L0924** — <code>        bool publish = first_movement || !latest_hint.revision || now - last_movement_hint_at &gt;= local_interval;</code>

Publish an acoustic rectangle on first movement, if no previous rectangle exists, or once the local interval elapses.

<a id="l0925"></a>
**L0925** — <code>        const int radio_interval = settings.report_interval_ms * (director.hint_padding ? 2 : 1);</code>

Double the strategic broadcast cooldown while relief is active.

<a id="l0926"></a>
**L0926** — <code>        bool radio_due = !monsters.empty() &amp;&amp; now - director.last_report_at &gt;= radio_interval;</code>

A strategic report requires at least one monster and an elapsed radio cooldown. Acoustic distance is irrelevant to this decision.

<a id="l0927"></a>
**L0927** — <code>        if (publish || radio_due) ++movement_revision;</code>

Allocate one new movement revision when either channel publishes on this step.

<a id="l0928"></a>
**L0928** — <code>        if (publish) {</code>

Publish a new clue revision.

<a id="l0929"></a>
**L0929** — <code>            latest_hint = make_location_hint(player, settings.hint_area_size, movement_revision);</code>

Quantize the player's step using the local acoustic sector and store the resulting rectangle.

<a id="l0930"></a>
**L0930** — <code>            last_movement_hint_at = now;</code>

Restart the reporting interval.

<a id="l0931"></a>
**L0931** — <code>            if (!monsters.empty())</code>

Log only when there are monsters to inform.

<a id="l0932"></a>
**L0932** — <code>                log_event("Movement hint #" + std::to_string(latest_hint.revision) + " | Local footstep area: rows " +</code>

Begin the terminal entry identifying a local clue separately from a radio transmission.

<a id="l0933"></a>
**L0933** — <code>                          std::to_string(latest_hint.min_row) + "-" + std::to_string(latest_hint.max_row) + ", cols " +</code>

...the row range and column label...

<a id="l0934"></a>
**L0934** — <code>                          std::to_string(latest_hint.min_col) + "-" + std::to_string(latest_hint.max_col) +</code>

...the column range...

<a id="l0935"></a>
**L0935** — <code>                          " | Heard by " + std::to_string(listeners.size()) + " of " + std::to_string(monsters.size()));</code>

...and how many monsters heard the step.

<a id="l0936"></a>
**L0936** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0937"></a>
**L0937** — <code>        for (std::size_t i : listeners) {</code>

Each listener:

<a id="l0938"></a>
**L0938** — <code>            Monster&amp; m = monsters[i];</code>

Reference the monster.

<a id="l0939"></a>
**L0939** — <code>            m.scent_until = now + settings.scent_ms;</code>

Renew its own scent deadline, even if no new clue was published.

<a id="l0940"></a>
**L0940** — <code>            deliver_hint(m, latest_hint);</code>

Deliver the last published local rectangle. Low normally throttles revisions, and relief can throttle revisions on every difficulty.

<a id="l0941"></a>
**L0941** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0942"></a>
**L0942** — <code>        if (radio_due) {</code>

Start the independent broadcast path when the strategic cooldown is satisfied.

<a id="l0943"></a>
**L0943** — <code>            director.last_report = make_location_hint(player, radio_sector_size(), movement_revision);</code>

Build and remember a radio-sized sector from this successful movement.

<a id="l0944"></a>
**L0944** — <code>            const auto&amp; report = director.last_report;</code>

Reuse the same immutable broadcast rectangle for every recipient and the event log.

<a id="l0945"></a>
**L0945** — <code>            int remote = 0;</code>

Count recipients that immediately investigate the radio clue.

<a id="l0946"></a>
**L0946** — <code>            for (auto&amp; m : monsters) {</code>

Visit all active monsters, including ones outside any possible hearing range.

<a id="l0947"></a>
**L0947** — <code>                if (m.state == MonsterState::Hunting) continue;  // Fresh local evidence takes priority.</code>

Keep a monster with a live hunt on its own higher-priority evidence.

<a id="l0948"></a>
**L0948** — <code>                investigate_report(m, report);</code>

Assign an eligible monster to search the reported area without setting a scent deadline.

<a id="l0949"></a>
**L0949** — <code>                ++remote;</code>

Count the remote investigation for this transmission.

<a id="l0950"></a>
**L0950** — <code>            }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0951"></a>
**L0951** — <code>            director.last_report_at = now;</code>

Start the cooldown for the next report at this simulated instant.

<a id="l0952"></a>
**L0952** — <code>            ++director.reports;</code>

Increment the public radio counter exactly once per transmission.

<a id="l0953"></a>
**L0953** — <code>            log_event("Director: REPORT " + std::to_string(director.reports) + " | " + director_phase() +</code>

Start the terminal broadcast record with its sequence number and intensity phase.

<a id="l0954"></a>
**L0954** — <code>                      " | Estimated player area: rows " + std::to_string(report.min_row) + "-" +</code>

Write the estimated area's lower row boundary.

<a id="l0955"></a>
**L0955** — <code>                      std::to_string(report.max_row) + ", cols " + std::to_string(report.min_col) + "-" +</code>

Complete the row range and begin the independent column bounds.

<a id="l0956"></a>
**L0956** — <code>                      std::to_string(report.max_col) + " | Investigating by radio: " + std::to_string(remote) +</code>

Append the final column boundary and the number of monsters using radio immediately.

<a id="l0957"></a>
**L0957** — <code>                      " | Using local evidence: " + std::to_string(monsters.size() - remote));</code>

Count the remaining monsters as using their own local evidence for this report.

<a id="l0958"></a>
**L0958** — <code>        }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0959"></a>
**L0959** — <code>        if (publish &amp;&amp; !director.hint_padding) assign_interceptors(listeners);</code>

Assign interceptors only for a published local clue in the normal phase.

<a id="l0960"></a>
**L0960** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0961"></a>
**L0961** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l0962"></a>
**L0962** — <code>    /// Send up to settings.cutters hunters to chokepoints that every route from the</code>

Documentation fragment: “Send up to settings.cutters hunters to chokepoints that every route from the” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0963"></a>
**L0963** — <code>    /// clue area to the exit must pass. Uses the area and terrain only: the walk</code>

Documentation fragment: “clue area to the exit must pass. Uses the area and terrain only: the walk” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0964"></a>
**L0964** — <code>    /// starts from the area cell nearest the exit, never from the player cell.</code>

Documentation fragment: “starts from the area cell nearest the exit, never from the player cell.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0965"></a>
**L0965** — <code>    void assign_interceptors(const std::vector&lt;std::size_t&gt;&amp; listeners) {</code>

Reserve existing guards and assign eligible listeners to distinct chokepoints within the global profile cap.

<a id="l0966"></a>
**L0966** — <code>        if (settings.cutters &lt;= 0 || listeners.empty()) return;</code>

Low has no interceptors; nothing to do without listeners.

<a id="l0967"></a>
**L0967** — <code>        int from = -1;</code>

Clue cell nearest the exit, the starting point for the chokepoint walk.

<a id="l0968"></a>
**L0968** — <code>        for (int r = latest_hint.min_row; r &lt;= latest_hint.max_row; ++r)</code>

Scan the clue rectangle's rows...

<a id="l0969"></a>
**L0969** — <code>            for (int c = latest_hint.min_col; c &lt;= latest_hint.max_col; ++c) {</code>

...and columns.

<a id="l0970"></a>
**L0970** — <code>                int v = id({r, c});</code>

Flatten the coordinate.

<a id="l0971"></a>
**L0971** — <code>                if (walkable(v) &amp;&amp; (from &lt; 0 || (*exit_distances)[v] &lt; (*exit_distances)[from])) from = v;</code>

Keep the walkable clue cell with the smallest exit distance; the player cell itself is never used.

<a id="l0972"></a>
**L0972** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0973"></a>
**L0973** — <code>        if (from &lt; 0) return;</code>

No walkable clue cell means no interception.

<a id="l0974"></a>
**L0974** — <code>        auto chokepoints = exit_chokepoints(from, latest_hint);</code>

Find chokepoints from that cell toward the exit.

<a id="l0975"></a>
**L0975** — <code>        std::vector&lt;std::size_t&gt; pool;</code>

Listeners eligible to intercept.

<a id="l0976"></a>
**L0976** — <code>        std::vector&lt;int&gt; reserved_chokes;</code>

Track target cells already consuming interceptor slots across the entire monster population.

<a id="l0977"></a>
**L0977** — <code>        for (std::size_t i = 0; i &lt; monsters.size(); ++i) {</code>

Examine all active monsters to account for existing guards as well as newly eligible listeners.

<a id="l0978"></a>
**L0978** — <code>            const Monster&amp; m = monsters[i];</code>

Read this monster's hunt and role state before updating the candidate pool.

<a id="l0979"></a>
**L0979** — <code>            if (m.sighted || m.state != MonsterState::Hunting) continue;</code>

Monsters charging after a sighting and non-hunters are not eligible.

<a id="l0980"></a>
**L0980** — <code>            if (std::find(listeners.begin(), listeners.end(), i) != listeners.end()) {</code>

Check whether this hunter is one of the listeners for the current local publication.

<a id="l0981"></a>
**L0981** — <code>                pool.push_back(i);</code>

Add an eligible listener to the reassignment pool.

<a id="l0982"></a>
**L0982** — <code>            } else if (m.role == MonsterRole::Cutter) {</code>

Account separately for a cutter outside the new listener group.

<a id="l0983"></a>
**L0983** — <code>                // This hunter heard no new clue. Its assignment still consumes a slot.</code>

Explains why an existing guard remains part of the profile-wide budget.

<a id="l0984"></a>
**L0984** — <code>                reserved_chokes.push_back(m.target);</code>

Reserve that guard's target before offering slots or chokepoints to new candidates.

<a id="l0985"></a>
**L0985** — <code>            }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l0986"></a>
**L0986** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l0987"></a>
**L0987** — <code>        std::vector&lt;std::size_t&gt; cutters;</code>

Indices chosen as interceptors in this pass.

<a id="l0988"></a>
**L0988** — <code>        for (const auto&amp; choke : chokepoints) {</code>

Try chokepoints nearest first.

<a id="l0989"></a>
**L0989** — <code>            if (reserved_chokes.size() &gt;= static_cast&lt;std::size_t&gt;(settings.cutters)) break;</code>

Stop assigning as soon as the global interceptor budget is full.

<a id="l0990"></a>
**L0990** — <code>            if (is_reserved(reserved_chokes, choke.first)) continue;</code>

Skip a chokepoint already reserved by another hunter.

<a id="l0991"></a>
**L0991** — <code>            std::size_t best = monsters.size();</code>

No candidate yet for this chokepoint.

<a id="l0992"></a>
**L0992** — <code>            int best_distance = THREAT_RADIUS + 1;</code>

Best corridor distance found, starting beyond the threat radius.

<a id="l0993"></a>
**L0993** — <code>            for (std::size_t i : pool) {</code>

Consider each eligible monster.

<a id="l0994"></a>
**L0994** — <code>                if (std::find(cutters.begin(), cutters.end(), i) != cutters.end()) continue;</code>

A monster can intercept only one chokepoint.

<a id="l0995"></a>
**L0995** — <code>                int d = local_field(monsters[i].cell)-&gt;at(choke.first);</code>

Corridor distance from the monster to the chokepoint, from its bounded field.

<a id="l0996"></a>
**L0996** — <code>                // Only a monster that can plausibly arrive first is worth sending.</code>

Documentation fragment: “Only a monster that can plausibly arrive first is worth sending.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l0997"></a>
**L0997** — <code>                if (d &gt; THREAT_RADIUS || Millis(d) * settings.monster_move_ms &gt; Millis(choke.second) * CUTTER_PACE_MS) continue;</code>

Skip monsters too far away or unable to arrive before the player at the assumed 80 ms pace.

<a id="l0998"></a>
**L0998** — <code>                if (d &lt; best_distance) { best_distance = d; best = i; }</code>

Prefer the closest qualifying monster.

<a id="l0999"></a>
**L0999** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1000"></a>
**L1000** — <code>            if (best == monsters.size()) continue;</code>

No monster can reach this chokepoint in time; try the next one.

<a id="l1001"></a>
**L1001** — <code>            Monster&amp; m = monsters[best];</code>

Reference the chosen interceptor.

<a id="l1002"></a>
**L1002** — <code>            if (m.role != MonsterRole::Cutter || m.target != choke.first) {</code>

Only a new assignment changes state and logs.

<a id="l1003"></a>
**L1003** — <code>                m.role = MonsterRole::Cutter; m.target = choke.first; m.blocked_turns = 0;</code>

Make it a cutter targeting the chokepoint.

<a id="l1004"></a>
**L1004** — <code>                m.role_until = now + CUTTER_CAMP_MS + Millis(best_distance) * settings.monster_move_ms;</code>

Hold time: the travel time plus about four seconds of camping.

<a id="l1005"></a>
**L1005** — <code>                m.planned_revision = latest_hint.revision; m.next_replan = now + MONSTER_REPLAN_MS;</code>

Mark the clue as planned and schedule the next review.

<a id="l1006"></a>
**L1006** — <code>                log_event(monster_name(m) + " | INTERCEPTING at chokepoint " + cell_label(choke.first) +</code>

Log the interception point...

<a id="l1007"></a>
**L1007** — <code>                          " between the clue area and the exit");</code>

...describing it as between the clue area and the exit.

<a id="l1008"></a>
**L1008** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1009"></a>
**L1009** — <code>            cutters.push_back(best);</code>

Mark the winning candidate as selected for this assignment round.

<a id="l1010"></a>
**L1010** — <code>            reserved_chokes.push_back(choke.first);</code>

Reserve the selected chokepoint so another interceptor cannot claim it in the same pass.

<a id="l1011"></a>
**L1011** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1012"></a>
**L1012** — <code>        // Hunters not chosen this time return to closing in on the area.</code>

Documentation fragment: “Hunters not chosen this time return to closing in on the area.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1013"></a>
**L1013** — <code>        for (std::size_t i : pool) if (monsters[i].role == MonsterRole::Cutter &amp;&amp;</code>

Cutters not chosen in this pass...

<a id="l1014"></a>
**L1014** — <code>                                       std::find(cutters.begin(), cutters.end(), i) == cutters.end()) {</code>

...that are not in the chosen list...

<a id="l1015"></a>
**L1015** — <code>            monsters[i].role = MonsterRole::Chaser; monsters[i].target = -1; monsters[i].role_until = NEVER;</code>

...return to chasing with no target and no hold deadline.

<a id="l1016"></a>
**L1016** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1017"></a>
**L1017** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1018"></a>
**L1018** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Director control

<a id="l1019"></a>
**L1019** — <code>    /// Widen each monster's own evidence, release guards and discard tighter</code>

Describes how escalating relief affects each monster's existing information and held chokepoints.

<a id="l1020"></a>
**L1020** — <code>    /// waypoints. Precision recovery affects future reports only. Neither action</code>

Specifies that returning to normal precision changes the shape of future publications.

<a id="l1021"></a>
**L1021** — <code>    /// supplies a fresh position or extends a scent or search deadline.</code>

Documents the independence of precision changes from movement evidence and expiry timestamps.

<a id="l1022"></a>
**L1022** — <code>    void set_director_precision(int padding, const std::string&amp; reason) {</code>

Widen each stored area on escalation, release interceptors and discard old plans. Normal precision applies to future reports.

<a id="l1023"></a>
**L1023** — <code>        const int increase = padding - director.hint_padding;</code>

Compute only the change in padding. Repeated escalations grow existing memories incrementally.

<a id="l1024"></a>
**L1024** — <code>        director.hint_padding = padding;</code>

Store the new per-side clue padding.

<a id="l1025"></a>
**L1025** — <code>        int redirected = 0, released = 0;</code>

Track the number of active plans discarded and interceptor roles released for the event log.

<a id="l1026"></a>
**L1026** — <code>        if (increase &gt; 0) {</code>

Retask and widen existing evidence only when the director increases uncertainty.

<a id="l1027"></a>
**L1027** — <code>            latest_hint = widen_hint(latest_hint, increase);</code>

Broaden the last published local footstep area using its existing coordinates.

<a id="l1028"></a>
**L1028** — <code>            director.last_report = widen_hint(director.last_report, increase);</code>

Broaden the remembered strategic broadcast without issuing a newer report.

<a id="l1029"></a>
**L1029** — <code>            for (auto&amp; m : monsters) {</code>

Apply relief to each monster's own stored evidence and current role.

<a id="l1030"></a>
**L1030** — <code>                if (m.role == MonsterRole::Cutter) {</code>

Check whether this monster is occupying an interceptor slot.

<a id="l1031"></a>
**L1031** — <code>                    m.role = MonsterRole::Chaser; m.role_until = NEVER;</code>

Switch the guard back to the chaser role and disable its role deadline.

<a id="l1032"></a>
**L1032** — <code>                    m.target = -1; ++released;</code>

Drop the chokepoint waypoint and count the slot freed by this escalation.

<a id="l1033"></a>
**L1033** — <code>                }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l1034"></a>
**L1034** — <code>                if (!m.hint.revision || m.state == MonsterState::Patrolling) continue;</code>

Leave ordinary patrols and monsters with no known clue on their current patrol plans.

<a id="l1035"></a>
**L1035** — <code>                m.hint = widen_hint(m.hint, increase);</code>

Broaden this monster's personal clue from its own stored bounds.

<a id="l1036"></a>
**L1036** — <code>                if (m.sighted &amp;&amp; sees_player(m)) continue;  // Direct High-difficulty sight takes priority.</code>

Preserve a High monster's charge while the player is still genuinely visible down an open corridor.

<a id="l1037"></a>
**L1037** — <code>                m.sighted = false;</code>

Clear a stale sight flag before requesting a new area-based plan.

<a id="l1038"></a>
**L1038** — <code>                m.target = -1; m.planned_revision = 0; m.blocked_turns = 0; m.next_replan = now;</code>

Discard the tighter target, planning revision and blockage count, and make replanning due now.

<a id="l1039"></a>
**L1039** — <code>                ++redirected;</code>

Count this monster as having received a new planning opportunity.

<a id="l1040"></a>
**L1040** — <code>            }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l1041"></a>
**L1041** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1042"></a>
**L1042** — <code>        log_event("Director: " + reason + " | Radio area: " + radio_area_label() + " | Retasked: " + std::to_string(redirected) +</code>

Begin a precision-transition log that identifies the new footprint and number of redirected monsters.

<a id="l1043"></a>
**L1043** — <code>                  " | Interceptors released: " + std::to_string(released));</code>

Finish the diagnostic with the number of interceptor slots released.

<a id="l1044"></a>
**L1044** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1045"></a>
**L1045** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1046"></a>
**L1046** — <code>    /// Sample confinement on a fixed simulation clock, independent of frame rate.</code>

Documentation fragment: “Sample confinement on a fixed simulation clock, independent of frame rate.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1047"></a>
**L1047** — <code>    /// The profile's confinement window starts relief; persistent confinement widens</code>

Documentation fragment: “The profile's confinement window starts relief; persistent confinement widens” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1048"></a>
**L1048** — <code>    /// it again. A minimum hold and sustained escape prevent precision oscillation.</code>

Documentation fragment: “it again. A minimum hold and sustained escape prevent precision oscillation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1049"></a>
**L1049** — <code>    /// Choosing to stand still is not confinement: it already sheds scent, so idle</code>

Documentation fragment: “Choosing to stand still is not confinement: it already sheds scent, so idle” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1050"></a>
**L1050** — <code>    /// time is excluded from the window instead of being rewarded with wider clues.</code>

Documentation fragment: “time is excluded from the window instead of being rewarded with wider clues.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1051"></a>
**L1051** — <code>    /// The solver is trying to escape even when a tactical wait has open terrain.</code>

Explains why active auto-solver waits on open terrain contribute to confinement sampling.

<a id="l1052"></a>
**L1052** — <code>    void update_director() {</code>

Run one fixed-time director sample: confinement, relief escalation, and recovery hysteresis. A manual player standing still is not counted as confined.

<a id="l1053"></a>
**L1053** — <code>        if (director.next_update &gt; now) return;</code>

Ignore calls before the next scheduled sample.

<a id="l1054"></a>
**L1054** — <code>        director.next_update = now + DIRECTOR_TICK_MS;</code>

Schedule the next sample exactly 200 simulation milliseconds later.

<a id="l1055"></a>
**L1055** — <code>        if (idle_deadline == NEVER &amp;&amp; !auto_solve) {</code>

Exclude prolonged manual inactivity from confinement history. An enabled auto-solver keeps accumulating samples while waiting.

<a id="l1056"></a>
**L1056** — <code>            director.samples.clear();</code>

...drop the sample window so idle time cannot accumulate as confinement...

<a id="l1057"></a>
**L1057** — <code>            director.samples.push_back({now, player});</code>

...restart it from the current sample...

<a id="l1058"></a>
**L1058** — <code>            director.recovered_since = NEVER;</code>

...and cancel any recovery hysteresis.

<a id="l1059"></a>
**L1059** — <code>            return;</code>

Skip confinement evaluation this tick.

<a id="l1060"></a>
**L1060** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1061"></a>
**L1061** — <code>        director.samples.push_back({now, player});</code>

Record the current player cell and timestamp.

<a id="l1062"></a>
**L1062** — <code>        while (director.samples.size() &gt; 1 &amp;&amp; director.samples[1].at &lt;= now - settings.confinement_ms)</code>

Keep a sample at or before the confinement-window boundary while dropping older redundant samples.

<a id="l1063"></a>
**L1063** — <code>            director.samples.pop_front();</code>

Remove the oldest sample while preserving enough history to measure a full interval.

<a id="l1064"></a>
**L1064** — <code>        if (monsters.empty() || director.samples.empty()) return;</code>

Skip difficulty adjustment without monsters or usable observations.

<a id="l1065"></a>
**L1065** — <code>        const int padding_limit = settings.max_padding;</code>

Use the profile's configured maximum relief expansion on every supported board size; rectangle construction clips the physical area.

<a id="l1066"></a>
**L1066** — <code>        Point low = point(player), high = low;</code>

Initialize the visited-position bounding box from the current cell.

<a id="l1067"></a>
**L1067** — <code>        std::vector&lt;int&gt; unique;</code>

Collect distinct sampled cells to distinguish exploration from tiny loops.

<a id="l1068"></a>
**L1068** — <code>        for (const auto&amp; sample : director.samples) {</code>

Inspect each retained motion sample.

<a id="l1069"></a>
**L1069** — <code>            Point p = point(sample.cell);</code>

Convert its flat coordinate for bounding-box comparisons.

<a id="l1070"></a>
**L1070** — <code>            low.r = std::min(low.r, p.r); low.c = std::min(low.c, p.c);</code>

Expand the row/column minima.

<a id="l1071"></a>
**L1071** — <code>            high.r = std::max(high.r, p.r); high.c = std::max(high.c, p.c);</code>

Expand the row/column maxima.

<a id="l1072"></a>
**L1072** — <code>            if (std::find(unique.begin(), unique.end(), sample.cell) == unique.end()) unique.push_back(sample.cell);</code>

Append this cell only if it has not appeared in the small sample-window distinct list.

<a id="l1073"></a>
**L1073** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1074"></a>
**L1074** — <code>        bool full_window = now - director.samples.front().at &gt;= settings.confinement_ms;</code>

Do not call the player confined until a full profile-specific observation window exists.

<a id="l1075"></a>
**L1075** — <code>        bool local_loop = unique.size() &lt;= 12 &amp;&amp; high.r - low.r &lt;= 8 &amp;&amp; high.c - low.c &lt;= 8;</code>

A local loop uses <=12 distinct sampled cells and spans <=8 cells in each coordinate difference.

<a id="l1076"></a>
**L1076** — <code>        // Longer repeated detours can span several corridors. Lack of novel cells</code>

Documentation fragment: “Longer repeated detours can span several corridors. Lack of novel cells” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1077"></a>
**L1077** — <code>        // or exit progress catches those loops even when the bounding box is larger.</code>

Documentation fragment: “or exit progress catches those loops even when the bounding box is larger.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1078"></a>
**L1078** — <code>        bool repeated_detours = auto_solve &amp;&amp; now - navigation_progress_at &gt;= settings.confinement_ms;</code>

Auto-solve also counts as stuck after a confinement window with no recognized navigation progress, catching longer repeated detours.

<a id="l1079"></a>
**L1079** — <code>        bool confined = full_window &amp;&amp; (local_loop || repeated_detours);</code>

Combine adequate observation duration with either local confinement or repeated detours.

<a id="l1080"></a>
**L1080** — <code>        if (!director.hint_padding) {</code>

When no relief is active, consider entering relief.

<a id="l1081"></a>
**L1081** — <code>            if (!confined || padding_limit == 0) return;</code>

Stay normal unless confinement is established and some padding can actually be added.

<a id="l1082"></a>
**L1082** — <code>            director.relief_anchor = player;</code>

Record the player cell at relief onset for later manual-movement escape assessment.

<a id="l1083"></a>
**L1083** — <code>            director.relief_exit_distance = (*exit_distances)[player];</code>

Remember the current cell's terrain distance to the exit as the baseline for measuring recovery.

<a id="l1084"></a>
**L1084** — <code>            director.relief_until = now + settings.relief_ms;</code>

Establish the minimum duration relief must be held.

<a id="l1085"></a>
**L1085** — <code>            director.recovered_since = NEVER;</code>

Clear any sustained-recovery streak.

<a id="l1086"></a>
**L1086** — <code>            set_director_precision(std::min(settings.padding_step, padding_limit),</code>

Increase uncertainty by one profile-specific padding step, bounded by the map/profile limit.

<a id="l1087"></a>
**L1087** — <code>                                   "RELIEF started; player confined, widening monster search areas");</code>

Log the reason for the initial widening.

<a id="l1088"></a>
**L1088** — <code>            return;</code>

Finish this sample after entering relief; recovery is assessed on subsequent samples.

<a id="l1089"></a>
**L1089** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1090"></a>
**L1090** — <code>        bool escaped_area = (*exit_distances)[player] &lt;= director.relief_exit_distance - 6 ||</code>

Automatic recovery requires a current terrain-exit distance at least six below the relief baseline.

<a id="l1091"></a>
**L1091** — <code>                            manhattan(player, director.relief_anchor) &gt;= 6;</code>

Movement at least six Manhattan cells from the original relief anchor also qualifies as leaving the area.

<a id="l1092"></a>
**L1092** — <code>        bool exit_route_open = !auto_solve || (!guide_distances.empty() &amp;&amp; guide_distances[player] != INF_DISTANCE);</code>

Automatic recovery additionally needs an exit route not blocked by near or parked monsters; manual mode does not require solver guidance.

<a id="l1093"></a>
**L1093** — <code>        if (!confined &amp;&amp; escaped_area &amp;&amp; exit_route_open) {</code>

Start/continue a recovery streak only if confinement has ended, the area has been escaped, and the route condition holds.

<a id="l1094"></a>
**L1094** — <code>            if (director.recovered_since == NEVER) director.recovered_since = now;</code>

Record the first qualifying sample time rather than resetting it every tick.

<a id="l1095"></a>
**L1095** — <code>        } else director.recovered_since = NEVER;</code>

Any failing condition clears the streak, enforcing continuous rather than intermittent recovery.

<a id="l1096"></a>
**L1096** — <code>        if (now &gt;= director.relief_until &amp;&amp; director.recovered_since != NEVER &amp;&amp;</code>

The minimum relief hold must expire, and a valid recovery streak must exist.

<a id="l1097"></a>
**L1097** — <code>            now - director.recovered_since &gt;= DIRECTOR_RECOVERY_MS) {</code>

The streak must last at least 2,000 ms before normal precision returns.

<a id="l1098"></a>
**L1098** — <code>            set_director_precision(0, "NORMAL precision restored after sustained escape from confinement");</code>

Remove padding and retask active hunters at normal information precision.

<a id="l1099"></a>
**L1099** — <code>            director.relief_until = director.recovered_since = NEVER;</code>

Clear relief/recovery deadlines after restoration.

<a id="l1100"></a>
**L1100** — <code>        } else if (now &gt;= director.relief_until &amp;&amp; confined) {</code>

If the minimum hold expired while the player remains confined, prolong relief instead.

<a id="l1101"></a>
**L1101** — <code>            director.relief_until = now + settings.relief_ms;</code>

Schedule another full profile-specific hold interval.

<a id="l1102"></a>
**L1102** — <code>            if (director.hint_padding &lt; padding_limit)</code>

Only increase padding if more uncertainty is allowed.

<a id="l1103"></a>
**L1103** — <code>                set_director_precision(std::min(director.hint_padding + settings.padding_step, padding_limit),</code>

Add another bounded padding increment.

<a id="l1104"></a>
**L1104** — <code>                                       "RELIEF increased; confinement persists");</code>

Log persistent confinement as the reason for escalation; at maximum padding targets are retained to avoid repeated disruption.

<a id="l1105"></a>
**L1105** — <code>            // At maximum uncertainty, retain the broad clue without repeatedly</code>

Documentation fragment: “At maximum uncertainty, retain the broad clue without repeatedly” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1106"></a>
**L1106** — <code>            // discarding targets that are already taking monsters away.</code>

Documentation fragment: “discarding targets that are already taking monsters away.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1107"></a>
**L1107** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1108"></a>
**L1108** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1109"></a>
**L1109** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Scent expiry, searching, and corridor sight

<a id="l1110"></a>
**L1110** — <code>    /// Earliest deadline for a scent, a search sweep, or an interceptor assignment.</code>

Introduces a deadline query that includes interceptor roles as well as scent and search states.

<a id="l1111"></a>
**L1111** — <code>    Millis next_monster_state_change() const {</code>

Find the next scent, search or interceptor deadline.

<a id="l1112"></a>
**L1112** — <code>        Millis event = NEVER;</code>

Start with no pending event.

<a id="l1113"></a>
**L1113** — <code>        for (const auto&amp; m : monsters) {</code>

Check every monster.

<a id="l1114"></a>
**L1114** — <code>            if (m.state == MonsterState::Hunting) event = std::min(event, m.scent_until);</code>

A hunter's scent deadline.

<a id="l1115"></a>
**L1115** — <code>            else if (m.state == MonsterState::Searching) event = std::min(event, m.search_until);</code>

A searcher's sweep deadline.

<a id="l1116"></a>
**L1116** — <code>            if (m.role == MonsterRole::Cutter) event = std::min(event, m.role_until);</code>

Schedule a guard's release at the precise role deadline even if no monster step is due then.

<a id="l1117"></a>
**L1117** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1118"></a>
**L1118** — <code>        return event;</code>

Return the earliest one, or NEVER.

<a id="l1119"></a>
**L1119** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1120"></a>
**L1120** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1121"></a>
**L1121** — <code>    /// A monster that stops hearing the player sweeps the last clue area for the</code>

Documentation fragment: “A monster that stops hearing the player sweeps the last clue area for the” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1122"></a>
**L1122** — <code>    /// profile's search time before roaming again. Standing still is not immunity.</code>

Documentation fragment: “profile's search time before roaming again. Standing still is not immunity.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1123"></a>
**L1123** — <code>    void expire_monster_states() {</code>

Release expired guards, end scent-driven hunts and complete area searches at their respective deadlines.

<a id="l1124"></a>
**L1124** — <code>        if (idle_deadline != NEVER &amp;&amp; now &gt;= idle_deadline) {</code>

If the player's global movement deadline has passed...

<a id="l1125"></a>
**L1125** — <code>            idle_deadline = NEVER;</code>

...clear it...

<a id="l1126"></a>
**L1126** — <code>            log_event("Player stopped moving for " + std::to_string(settings.scent_ms) + " ms | Scent fading.");</code>

...and log that the scent is fading; monster transitions follow their own deadlines below.

<a id="l1127"></a>
**L1127** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1128"></a>
**L1128** — <code>        for (auto&amp; m : monsters) {</code>

Visit every monster to apply its due scent or search expiry.

<a id="l1129"></a>
**L1129** — <code>            if (m.role == MonsterRole::Cutter &amp;&amp; now &gt;= m.role_until) {</code>

Test whether the current interceptor assignment has elapsed on the simulation clock.

<a id="l1130"></a>
**L1130** — <code>                m.role = MonsterRole::Chaser;</code>

Return an expired guard to ordinary chasing behavior.

<a id="l1131"></a>
**L1131** — <code>                m.role_until = NEVER;</code>

Remove the expired guard deadline from the event queue.

<a id="l1132"></a>
**L1132** — <code>                m.target = -1;</code>

Invalidate the old chokepoint destination so the monster can select a new waypoint.

<a id="l1133"></a>
**L1133** — <code>            }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l1134"></a>
**L1134** — <code>            if (m.state == MonsterState::Hunting &amp;&amp; now &gt;= m.scent_until) {</code>

A hunter whose own scent has expired:

<a id="l1135"></a>
**L1135** — <code>                const Millis last_contact = m.scent_until - settings.scent_ms;</code>

Recover the timestamp of the last genuine acoustic or sight contact from the stored expiry.

<a id="l1136"></a>
**L1136** — <code>                m.scent_until = NEVER; m.sighted = false;</code>

Clear its deadline and any sighting.

<a id="l1137"></a>
**L1137** — <code>                m.role = MonsterRole::Chaser; m.role_until = NEVER;</code>

Drop any interceptor role.

<a id="l1138"></a>
**L1138** — <code>                m.target = -1; m.blocked_turns = 0; m.planned_revision = 0; m.next_replan = now;</code>

Clear the target and force a replan now.

<a id="l1139"></a>
**L1139** — <code>                // A hunter also received the broadcast. Once its scent expires it</code>

Explains the fallback available to a hunter that kept following better first-hand evidence during a broadcast.

<a id="l1140"></a>
**L1140** — <code>                // can use a newer, still-live report; this never restarts a hunt.</code>

Specifies that an eligible fallback is used for searching after scent expiry.

<a id="l1141"></a>
**L1141** — <code>                const bool use_radio = director.last_report.revision &amp;&amp; director.last_report_at &gt; last_contact &amp;&amp;</code>

Require a real broadcast strictly newer than this monster's last physical contact.

<a id="l1142"></a>
**L1142** — <code>                                       now - director.last_report_at &lt; settings.search_ms;</code>

Require that broadcast still to be inside its original search lifetime.

<a id="l1143"></a>
**L1143** — <code>                if (use_radio) m.hint = director.last_report;</code>

Adopt the stored broadcast rectangle only when it passes both freshness checks.

<a id="l1144"></a>
**L1144** — <code>                if (settings.search_ms &gt; 0 &amp;&amp; m.hint.revision != 0) {</code>

With a search duration and a known clue area...

<a id="l1145"></a>
**L1145** — <code>                    m.state = MonsterState::Searching;</code>

...start searching that area...

<a id="l1146"></a>
**L1146** — <code>                    m.search_until = (use_radio ? director.last_report_at : now) + settings.search_ms;</code>

Give a radio fallback its original broadcast-based expiry. A local sweep starts at scent loss.

<a id="l1147"></a>
**L1147** — <code>                    log_event(monster_name(m) + ": Lost the scent, switching HUNTING -&gt; SEARCHING " +</code>

Log the change of state at the precise expiry time.

<a id="l1148"></a>
**L1148** — <code>                              (use_radio ? "the last radio sector." : "the last clue area."));</code>

Identify which memory supplies the area for the new search.

<a id="l1149"></a>
**L1149** — <code>                } else {</code>

Without a search duration or a known clue area:

<a id="l1150"></a>
**L1150** — <code>                    m.state = MonsterState::Patrolling; m.hint = {};</code>

Otherwise return straight to roaming and forget the clue.

<a id="l1151"></a>
**L1151** — <code>                    log_event(monster_name(m) + ": Player idle, switching HUNTING -&gt; ROAMING.");</code>

Log the direct return to roaming.

<a id="l1152"></a>
**L1152** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1153"></a>
**L1153** — <code>            } else if (m.state == MonsterState::Searching &amp;&amp; now &gt;= m.search_until) {</code>

A searcher whose sweep time has elapsed:

<a id="l1154"></a>
**L1154** — <code>                m.state = MonsterState::Patrolling; m.search_until = NEVER;</code>

Return to roaming.

<a id="l1155"></a>
**L1155** — <code>                m.hint = {}; m.target = -1; m.blocked_turns = 0; m.planned_revision = 0; m.next_replan = now;</code>

Forget the clue and force a fresh patrol waypoint.

<a id="l1156"></a>
**L1156** — <code>                log_event(monster_name(m) + ": Search found nothing, switching SEARCHING -&gt; ROAMING.");</code>

Log that the search found nothing.

<a id="l1157"></a>
**L1157** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1158"></a>
**L1158** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1159"></a>
**L1159** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1160"></a>
**L1160** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1161"></a>
**L1161** — <code>    /// High difficulty only: a monster sees the player down a straight open corridor.</code>

Documentation fragment: “High difficulty only: a monster sees the player down a straight open corridor.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1162"></a>
**L1162** — <code>    bool sees_player(const Monster&amp; m) const { return line_of_sight(m.cell, player); }</code>

Whether this monster can see the current player cell.

<a id="l1163"></a>
**L1163** — <code>    bool line_of_sight(int from, int to) const {</code>

Straight-corridor line of sight between two cells, used by the game and by the solver's forecast.

<a id="l1164"></a>
**L1164** — <code>        if (settings.sight_cells &lt;= 0 || !walkable(from) || !walkable(to)) return false;</code>

No sight on profiles without it, or from or to a wall.

<a id="l1165"></a>
**L1165** — <code>        Point a = point(from), b = point(to);</code>

Convert both cells to coordinates.

<a id="l1166"></a>
**L1166** — <code>        if (a.r != b.r &amp;&amp; a.c != b.c) return false;</code>

Sight works only along a shared row or column.

<a id="l1167"></a>
**L1167** — <code>        int distance = std::abs(a.r - b.r) + std::abs(a.c - b.c);</code>

Distance along that line.

<a id="l1168"></a>
**L1168** — <code>        // Spotting works at a distance; at close quarters monsters behave normally,</code>

Documentation fragment: “Spotting works at a distance; at close quarters monsters behave normally,” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1169"></a>
**L1169** — <code>        // otherwise every adjacent monster would become an instant capture.</code>

Documentation fragment: “otherwise every adjacent monster would become an instant capture.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1170"></a>
**L1170** — <code>        if (distance &lt; SIGHT_MIN_CELLS || distance &gt; settings.sight_cells) return false;</code>

Too close (under three cells) or beyond the profile's range means no sighting.

<a id="l1171"></a>
**L1171** — <code>        int dr = (b.r &gt; a.r) - (b.r &lt; a.r), dc = (b.c &gt; a.c) - (b.c &lt; a.c);</code>

Unit step from the monster toward the player.

<a id="l1172"></a>
**L1172** — <code>        for (int k = 1; k &lt; distance; ++k) if (!walkable(id({a.r + dr * k, a.c + dc * k}))) return false;</code>

Every cell strictly between them must be open; any wall blocks sight.

<a id="l1173"></a>
**L1173** — <code>        return true;</code>

A clear straight corridor within range.

<a id="l1174"></a>
**L1174** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1175"></a>
**L1175** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1176"></a>
**L1176** — <code>public:</code>

Set following class members to public access until another access label or class end; this changes compile-time accessibility, not runtime state.

## Event scheduler, monsters, and player controls

<a id="l1177"></a>
**L1177** — <code>    /// Advance all events chronologically. On ties monsters move before the player.</code>

Documentation fragment: “Advance all events chronologically. On ties monsters move before the player.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1178"></a>
**L1178** — <code>    /// Occupancy is checked after every move, so exchanges cannot bypass capture.</code>

Documentation fragment: “Occupancy is checked after every move, so exchanges cannot bypass capture.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1179"></a>
**L1179** — <code>    /// Monsters stay frozen until the player's first move or auto-solve toggle.</code>

Documentation fragment: “Monsters stay frozen until the player's first move or auto-solve toggle.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1180"></a>
**L1180** — <code>    void advance_to(Millis destination) {</code>

Advance the same authoritative discrete-event simulation used in headless and graphical modes; render frame size does not alter event ordering.

<a id="l1181"></a>
**L1181** — <code>        if (destination &lt; now) throw std::runtime_error("Simulation time cannot go backward.");</code>

Reject backward destinations so scheduling and elapsed-time invariants remain monotonic.

<a id="l1182"></a>
**L1182** — <code>        verify_graph();</code>

Debug builds verify that the terrain was not edited without rebuild_graph().

<a id="l1183"></a>
**L1183** — <code>        while (outcome == Outcome::Playing) {</code>

Process pending events until play ends or the requested destination is reached.

<a id="l1184"></a>
**L1184** — <code>            Millis event = std::min(auto_solve ? next_auto : NEVER, idle_deadline);</code>

Start with the earlier automatic-action deadline or the player's movement-deadline expiry; disabled auto-solving contributes NEVER.

<a id="l1185"></a>
**L1185** — <code>            event = std::min(event, next_manual_repeat);</code>

Include held-key repeats when choosing the earliest simulation event.

<a id="l1186"></a>
**L1186** — <code>            event = std::min(event, director.next_update);</code>

Include the director's next fixed sample.

<a id="l1187"></a>
**L1187** — <code>            if (timer_started) {</code>

Monster events exist only after the player's first input.

<a id="l1188"></a>
**L1188** — <code>                event = std::min(event, next_monster_state_change());</code>

Include the next scent or search expiry.

<a id="l1189"></a>
**L1189** — <code>                for (const auto&amp; m : monsters) event = std::min(event, m.next_move);</code>

Include each monster's next move.

<a id="l1190"></a>
**L1190** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1191"></a>
**L1191** — <code>            if (event &gt; destination || event == NEVER) break;</code>

Stop when no event is due before destination or every source is unscheduled.

<a id="l1192"></a>
**L1192** — <code>            now = event;</code>

Move simulation time directly to the next event timestamp.

<a id="l1193"></a>
**L1193** — <code>            expire_monster_states();</code>

Apply due scent and search expiries before the director and monsters act.

<a id="l1194"></a>
**L1194** — <code>            update_director();</code>

Then update director observations and precision.

<a id="l1195"></a>
**L1195** — <code>            update_monsters();</code>

Then move due monsters; on ties this precedes the automatic player action.

<a id="l1196"></a>
**L1196** — <code>            if (outcome != Outcome::Playing) break;</code>

Capture/victory ends event processing immediately.

<a id="l1197"></a>
**L1197** — <code>            if (auto_solve &amp;&amp; next_auto &lt;= now) {</code>

If auto-solving remains active and due, execute one tactical action.

<a id="l1198"></a>
**L1198** — <code>                auto_step();</code>

Plan and commit exactly one movement or wait.

<a id="l1199"></a>
**L1199** — <code>                next_auto = auto_solve ? now + AUTO_SOLVE_SPEED_MS : NEVER;</code>

Schedule the next 40 ms action, unless the action ended/disabled auto-solving.

<a id="l1200"></a>
**L1200** — <code>            } else if (next_manual_repeat &lt;= now) {</code>

When no auto action takes precedence, service a due held-key repeat after monster actions at this time.

<a id="l1201"></a>
**L1201** — <code>                const bool moved = move_player(held_direction.r, held_direction.c);</code>

Attempt the stored row/column step through the ordinary collision and movement logic.

<a id="l1202"></a>
**L1202** — <code>                next_manual_repeat = moved &amp;&amp; outcome == Outcome::Playing ? now + MANUAL_HOLD_REPEAT_MS : NEVER;</code>

Schedule the next repeat after a successful nonterminal move, otherwise disarm the hold.

<a id="l1203"></a>
**L1203** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1204"></a>
**L1204** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1205"></a>
**L1205** — <code>        if (outcome == Outcome::Playing) now = destination;</code>

If still active after all due events, move now to the requested destination; terminal outcomes retain their exact event time.

<a id="l1206"></a>
**L1206** — <code>        update_elapsed();</code>

Refresh elapsed gameplay duration from simulation time.

<a id="l1207"></a>
**L1207** — <code>        refresh_solution_overlay();</code>

Ensure full-route visualization reflects final positions after this advancement.

<a id="l1208"></a>
**L1208** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1209"></a>
**L1209** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1210"></a>
**L1210** — <code>    /// Process only due monsters. Snapshot occupancy and rotating priority reserve</code>

Documentation fragment: “Process only due monsters. Snapshot occupancy and rotating priority reserve” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1211"></a>
**L1211** — <code>    /// destinations, prevent stacking/swaps, and avoid a fixed winner at bottlenecks.</code>

Documentation fragment: “destinations, prevent stacking/swaps, and avoid a fixed winner at bottlenecks.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1212"></a>
**L1212** — <code>    void update_monsters() {</code>

Move only due monsters, reserving source and destination cells to prevent stacking and swaps; also applies High corridor sightings and interceptor hold expiry.

<a id="l1213"></a>
**L1213** — <code>        if (outcome != Outcome::Playing || monsters.empty() || !timer_started) return;</code>

Skip when play has ended, there are no monsters, or the player has not started yet.

<a id="l1214"></a>
**L1214** — <code>        bool any_due = false;</code>

Track whether at least one movement deadline has arrived.

<a id="l1215"></a>
**L1215** — <code>        for (const auto&amp; m : monsters) if (m.next_move &lt;= now) any_due = true;</code>

Scan monsters for due events before allocating occupancy storage.

<a id="l1216"></a>
**L1216** — <code>        if (!any_due) return;</code>

Avoid all subsequent movement work if no enemy is due.

<a id="l1217"></a>
**L1217** — <code>        std::vector&lt;int&gt; reserved;</code>

Reserved cells as a short list: ten monsters reserve at most twenty cells, so no grid-sized array is needed.

<a id="l1218"></a>
**L1218** — <code>        reserved.reserve(monsters.size() * 2);</code>

Reserve capacity for start cells plus destinations.

<a id="l1219"></a>
**L1219** — <code>        for (const auto&amp; m : monsters) reserved.push_back(m.cell);</code>

Every monster's current cell starts reserved.

<a id="l1220"></a>
**L1220** — <code>        const std::size_t n = monsters.size();</code>

Cache monster count for priority rotation.

<a id="l1221"></a>
**L1221** — <code>        for (std::size_t k = 0; k &lt; n; ++k) {</code>

Visit each monster once in rotated rather than permanently fixed order.

<a id="l1222"></a>
**L1222** — <code>            Monster&amp; m = monsters[(priority_rotation + k) % n];</code>

Select the kth monster relative to the current priority offset.

<a id="l1223"></a>
**L1223** — <code>            if (m.next_move &gt; now) continue;</code>

Skip an enemy whose individual deadline is still in the future.

<a id="l1224"></a>
**L1224** — <code>            m.next_move = now + settings.monster_move_ms;</code>

Set its next action time from this event plus the profile's movement period.

<a id="l1225"></a>
**L1225** — <code>            // Sight is a discovery mechanic: a monster that is not already hunting</code>

Documentation fragment: “Sight is a discovery mechanic: a monster that is not already hunting” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1226"></a>
**L1226** — <code>            // (or one mid-charge) can spot the player; hunters rely on area clues.</code>

Documentation fragment: “(or one mid-charge) can spot the player; hunters rely on area clues.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1227"></a>
**L1227** — <code>            if ((m.state != MonsterState::Hunting || m.sighted) &amp;&amp; sees_player(m)) {</code>

A monster that is not already hunting (or is mid-charge) and can see the player:

<a id="l1228"></a>
**L1228** — <code>                if (!m.sighted)</code>

Log only the first sighting of a charge.

<a id="l1229"></a>
**L1229** — <code>                    log_event(monster_name(m) + ": SPOTTED the player down a corridor; charging.");</code>

Announce the sighting.

<a id="l1230"></a>
**L1230** — <code>                if (m.state != MonsterState::Hunting) { m.state = MonsterState::Hunting; m.search_until = NEVER; }</code>

Become a hunter, cancelling any search.

<a id="l1231"></a>
**L1231** — <code>                if (m.hint.revision == 0) m.hint = latest_hint;</code>

Give a monster with no clue the latest one for later retargeting.

<a id="l1232"></a>
**L1232** — <code>                m.sighted = true; m.role = MonsterRole::Chaser; m.role_until = NEVER;</code>

Mark the charge and drop any interceptor role.

<a id="l1233"></a>
**L1233** — <code>                m.scent_until = std::max(m.scent_until == NEVER ? Millis(0) : m.scent_until, now + settings.scent_ms);</code>

Extend its scent to at least one scent period from now.

<a id="l1234"></a>
**L1234** — <code>                m.target = player; m.planned_revision = m.hint.revision; m.next_replan = now + MONSTER_REPLAN_MS;</code>

Target the exact cell where it saw the player and schedule the next review.

<a id="l1235"></a>
**L1235** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1236"></a>
**L1236** — <code>            // Movement hints survive across frames until this particular monster acts.</code>

Documentation fragment: “Movement hints survive across frames until this particular monster acts.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1237"></a>
**L1237** — <code>            if (m.target &lt; 0 || m.planned_revision != m.hint.revision ||</code>

Request target review for an invalid target or a clue revision not yet planned against.

<a id="l1238"></a>
**L1238** — <code>                now &gt;= m.next_replan || m.blocked_turns &gt;= 2 ||</code>

Also review on periodic expiry or after at least two blocked turns.

<a id="l1239"></a>
**L1239** — <code>                m.cell == m.target) {</code>

Reaching the current waypoint triggers selection of another.

<a id="l1240"></a>
**L1240** — <code>                int previous_target = m.target;</code>

Remember the old target solely for deciding whether a target-change log is needed.

<a id="l1241"></a>
**L1241** — <code>                m.target = choose_target(m);</code>

Choose or retain a waypoint through the monster policy: charge, interception, clue area, search area, or patrol.

<a id="l1242"></a>
**L1242** — <code>                m.planned_revision = m.hint.revision;</code>

Stamp which clue revision has now been considered.

<a id="l1243"></a>
**L1243** — <code>                m.next_replan = now + MONSTER_REPLAN_MS;</code>

Schedule the next periodic target review.

<a id="l1244"></a>
**L1244** — <code>                if (m.target != previous_target)</code>

Log only an actual target change, limiting repetitive output.

<a id="l1245"></a>
**L1245** — <code>                    log_event(monster_name(m) + " | " +</code>

Log a target change with its reason:

<a id="l1246"></a>
**L1246** — <code>                              (m.sighted ? "CHASING last seen " :</code>

a sighting charge,

<a id="l1247"></a>
**L1247** — <code>                               m.role == MonsterRole::Cutter ? "INTERCEPT point " :</code>

an interception point,

<a id="l1248"></a>
**L1248** — <code>                               m.state == MonsterState::Hunting ? "HUNTING area target " :</code>

an area target for a hunter,

<a id="l1249"></a>
**L1249** — <code>                               m.state == MonsterState::Searching ? "SEARCHING area target " : "ROAMING waypoint ") +</code>

an area target for a searcher, or a roaming waypoint, followed by the target cell.

<a id="l1250"></a>
**L1250** — <code>                              cell_label(m.target) + " | From " + cell_label(m.cell));</code>

Append target and current-cell labels.

<a id="l1251"></a>
**L1251** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1252"></a>
**L1252** — <code>            int next = choose_monster_step(m, reserved);</code>

Select a terrain-valid next step while respecting all reserved cells.

<a id="l1253"></a>
**L1253** — <code>            if (next != m.cell) {</code>

A different chosen cell represents a successful movement.

<a id="l1254"></a>
**L1254** — <code>                if (verbose_logs)</code>

Detailed per-step monster logs are optional.

<a id="l1255"></a>
**L1255** — <code>                    log_event(monster_name(m) + ": " + cell_label(m.cell) + " -&gt; " + cell_label(next));</code>

Verbose log of the individual step, using the shared monster name format.

<a id="l1256"></a>
**L1256** — <code>                remember(m.recent_cells, m.cell, 12);</code>

Remember the departed cell in the monster's 12-cell history.

<a id="l1257"></a>
**L1257** — <code>                m.cell = next; reserved.push_back(next); m.blocked_turns = 0;</code>

Move the monster, reserve its destination in the short list, and clear its blocked count.

<a id="l1258"></a>
**L1258** — <code>                m.last_moved_at = now;</code>

Record the step time; the solver treats long-unmoving monsters as parked.

<a id="l1259"></a>
**L1259** — <code>                mark_region(next);</code>

Credit the destination's patrol region as recently covered.

<a id="l1260"></a>
**L1260** — <code>                overlay_dirty = true;</code>

Enemy displacement invalidates the displayed occupancy-aware path.

<a id="l1261"></a>
**L1261** — <code>                if (m.sighted &amp;&amp; m.cell == m.target) m.sighted = false;</code>

A charging monster that reaches the last seen cell ends its charge and returns to clue-based hunting.

<a id="l1262"></a>
**L1262** — <code>            } else if (m.cell != m.target) {</code>

A stationary enemy away from its goal has been blocked.

<a id="l1263"></a>
**L1263** — <code>                ++m.blocked_turns;</code>

Increment consecutive blocked attempts, which affects future step and target choices.

<a id="l1264"></a>
**L1264** — <code>                if (m.blocked_turns == 1 || verbose_logs)</code>

Log the first blocked turn, or every blocked turn in verbose mode.

<a id="l1265"></a>
**L1265** — <code>                    log_event(monster_name(m) + ": Movement blocked at " + cell_label(m.cell));</code>

Log the first blocked turn (or every one in verbose mode) with the monster's name.

<a id="l1266"></a>
**L1266** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1267"></a>
**L1267** — <code>            check_outcome();</code>

Check player/enemy overlap immediately after each enemy action.

<a id="l1268"></a>
**L1268** — <code>            if (outcome != Outcome::Playing) break;</code>

Stop moving further monsters after the game ends.

<a id="l1269"></a>
**L1269** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1270"></a>
**L1270** — <code>        priority_rotation = (priority_rotation + 1) % n;</code>

Rotate the next batch's priority so one spawn ID does not always win bottleneck reservations.

<a id="l1271"></a>
**L1271** — <code>        // advance_to() refreshes the overlay once per frame; doing it per monster</code>

Documentation fragment: “advance_to() refreshes the overlay once per frame; doing it per monster” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1272"></a>
**L1272** — <code>        // event rebuilt whole-grid guidance several times per player action.</code>

Documentation fragment: “event rebuilt whole-grid guidance several times per player action.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1273"></a>
**L1273** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1274"></a>
**L1274** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1275"></a>
**L1275** — <code>    /// Toggle assistance; include waiting time even when the initial route is blocked.</code>

Documentation fragment: “Toggle assistance; include waiting time even when the initial route is blocked.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1276"></a>
**L1276** — <code>    void set_auto_solve(bool enabled) {</code>

Toggle automatic assistance, reset its tactical memory, and update the overlay while preserving the existing map and actor positions.

<a id="l1277"></a>
**L1277** — <code>        if (outcome != Outcome::Playing) return;</code>

Ignore toggles after capture or escape.

<a id="l1278"></a>
**L1278** — <code>        cancel_held_move();</code>

Cancel a pending manual repeat on this control-state transition.

<a id="l1279"></a>
**L1279** — <code>        auto_solve = enabled;</code>

Store the new assistance state.

<a id="l1280"></a>
**L1280** — <code>        solution_overlay.clear();</code>

Discard the previously displayed route.

<a id="l1281"></a>
**L1281** — <code>        overlay_dirty = true; overlay_reported = false; last_auto_wait = false; planned_wait_for_idle = false;</code>

Reset overlay and waiting-report state for the new mode.

<a id="l1282"></a>
**L1282** — <code>        next_auto = enabled ? now + AUTO_SOLVE_SPEED_MS : NEVER;</code>

Schedule the first auto action 40 ms ahead when enabling; disable the event when turning assistance off.

<a id="l1283"></a>
**L1283** — <code>        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();</code>

Drop any committed detour and its cached field.

<a id="l1284"></a>
**L1284** — <code>        blocked_since = clear_route_since = NEVER;</code>

Reset continuous blocked/open-route observation intervals.

<a id="l1285"></a>
**L1285** — <code>        navigation_progress_at = now; next_recovery_attempt = now;</code>

Restart the progress clock and allow recovery searches when their conditions become true.

<a id="l1286"></a>
**L1286** — <code>        recovery_pause_until = 0;</code>

Clear any old post-retreat pause.

<a id="l1287"></a>
**L1287** — <code>        if (enabled) start_timer();</code>

Starting auto-solving begins elapsed gameplay time even if its first actions must be waits.

<a id="l1288"></a>
**L1288** — <code>        log_event(std::string("Auto-solve ") + (enabled ? "activated." : "deactivated."));</code>

Log the explicit assistance mode transition.

<a id="l1289"></a>
**L1289** — <code>        refresh_solution_overlay();</code>

Immediately construct or clear the full exit-path display as appropriate.

<a id="l1290"></a>
**L1290** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1291"></a>
**L1291** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1292"></a>
**L1292** — <code>    /// Apply one cardinal manual move through the same rules as automatic movement.</code>

Documentation fragment: “Apply one cardinal manual move through the same rules as automatic movement.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1293"></a>
**L1293** — <code>    bool move_player(int dr, int dc) {</code>

Manual movement wrapper; both manual and automatic displacements eventually use commit_player_move.

<a id="l1294"></a>
**L1294** — <code>        if (auto_solve || std::abs(dr) + std::abs(dc) != 1) return false;</code>

Reject manual input while auto-solving and reject zero/diagonal/multicell vectors.

<a id="l1295"></a>
**L1295** — <code>        verify_graph();</code>

Debug builds verify that the terrain was not edited without rebuild_graph().

<a id="l1296"></a>
**L1296** — <code>        Point p = point(player);</code>

Read the current player coordinate.

<a id="l1297"></a>
**L1297** — <code>        solution_overlay.clear();</code>

Clear an assistance path when processing manual movement.

<a id="l1298"></a>
**L1298** — <code>        return commit_player_move(id({p.r + dr, p.c + dc}), false);</code>

Convert the requested neighbor and delegate validation, scent, counters, and outcome handling to the common commit function.

<a id="l1299"></a>
**L1299** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1300"></a>
**L1300** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1301"></a>
**L1301** — <code>    /// Apply the first keypress immediately. Later repeats share the monster and auto-solver clock.</code>

Describes the two stages of a key hold: an immediate step followed by timed simulation events.

<a id="l1302"></a>
**L1302** — <code>    bool begin_held_move(int dr, int dc) {</code>

Start held movement with a direction supplied by the graphical input layer.

<a id="l1303"></a>
**L1303** — <code>        cancel_held_move();</code>

Remove an older repeat before attempting the newly pressed key's first step.

<a id="l1304"></a>
**L1304** — <code>        if (!move_player(dr, dc) || outcome != Outcome::Playing) return false;</code>

Report failure for a blocked or terminal first move, leaving no repeat scheduled.

<a id="l1305"></a>
**L1305** — <code>        held_direction = {dr, dc};</code>

Remember the valid direction for future scheduled steps.

<a id="l1306"></a>
**L1306** — <code>        next_manual_repeat = now + MANUAL_HOLD_DELAY_MS;</code>

Arm the first simulated repeat 220 ms after this successful press.

<a id="l1307"></a>
**L1307** — <code>        return true;</code>

Report that a live run accepted the held direction.

<a id="l1308"></a>
**L1308** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l1309"></a>
**L1309** — <code>[blank line]</code>

A blank separator groups the adjacent declarations or operations.

<a id="l1310"></a>
**L1310** — <code>    /// Disarm the repeat when a key is released, a menu opens, or the input mode changes.</code>

Lists the release and mode-change events that disable pending repeats.

<a id="l1311"></a>
**L1311** — <code>    void cancel_held_move() { next_manual_repeat = NEVER; }</code>

Set the next repeat to the inactive sentinel without advancing time.

<a id="l1312"></a>
**L1312** — <code>[blank line]</code>

A blank separator groups the adjacent declarations or operations.

<a id="l1313"></a>
**L1313** — <code>    /// Resolve capture before victory, including occupied exits and manual movement.</code>

Documentation fragment: “Resolve capture before victory, including occupied exits and manual movement.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1314"></a>
**L1314** — <code>    void check_outcome() {</code>

Resolve outcomes atomically with capture precedence over reaching an occupied exit.

<a id="l1315"></a>
**L1315** — <code>        if (outcome != Outcome::Playing) return;</code>

A terminal result cannot be changed by later checks.

<a id="l1316"></a>
**L1316** — <code>        for (const auto&amp; m : monsters) if (m.cell == player) {</code>

Search actual monster occupancy, independent of patrol/hunt state and visibility.

<a id="l1317"></a>
**L1317** — <code>            outcome = Outcome::Caught;</code>

Overlap produces capture.

<a id="l1318"></a>
**L1318** — <code>            break;</code>

Stop searching after the first capturing monster.

<a id="l1319"></a>
**L1319** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1320"></a>
**L1320** — <code>        if (outcome == Outcome::Playing &amp;&amp; player == exit_cell) outcome = Outcome::Won;</code>

Award escape only if no capture was found and the player occupies the exit.

<a id="l1321"></a>
**L1321** — <code>        if (outcome != Outcome::Playing) {</code>

Finalize timers and scheduled activities when an outcome becomes terminal.

<a id="l1322"></a>
**L1322** — <code>            update_elapsed();</code>

Capture the exact elapsed simulation duration before freezing future events.

<a id="l1323"></a>
**L1323** — <code>            cancel_held_move();</code>

Cancel scheduled repeats when the run reaches a terminal outcome.

<a id="l1324"></a>
**L1324** — <code>            auto_solve = false; next_auto = NEVER; solution_overlay.clear();</code>

Disable auto-solving, unschedule its actions, and remove the assistance trail.

<a id="l1325"></a>
**L1325** — <code>            idle_deadline = NEVER;</code>

Disable pending scent expiry.

<a id="l1326"></a>
**L1326** — <code>            director.next_update = NEVER;</code>

Disable director sampling after play ends.

<a id="l1327"></a>
**L1327** — <code>            for (auto&amp; m : monsters) { m.scent_until = m.search_until = NEVER; }</code>

Clear every monster's scent and search deadlines so no further state events are scheduled after the game ends.

<a id="l1328"></a>
**L1328** — <code>            log_event(std::string("Player ") + (outcome == Outcome::Caught ? "CAUGHT BY A MONSTER" : "REACHED THE EXIT") +</code>

Begin a terminal event identifying capture or successful exit.

<a id="l1329"></a>
**L1329** — <code>                      " at " + cell_label(player));</code>

Append the final cell coordinate.

<a id="l1330"></a>
**L1330** — <code>            log_status();</code>

Emit a final full player/director status snapshot.

<a id="l1331"></a>
**L1331** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1332"></a>
**L1332** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1333"></a>
**L1333** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Private simulation bookkeeping and movement commits

<a id="l1334"></a>
**L1334** — <code>private:</code>

Set following class members to private access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l1335"></a>
**L1335** — <code>    /// Maintain a bounded history without unbounded per-entity memory growth.</code>

Documentation fragment: “Maintain a bounded history without unbounded per-entity memory growth.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1336"></a>
**L1336** — <code>    static void remember(std::deque&lt;int&gt;&amp; history, int cell, std::size_t limit) {</code>

Append an action cell to a bounded history deque; shared by player history and monster recent-cell memory.

<a id="l1337"></a>
**L1337** — <code>        history.push_back(cell);</code>

Place the newest observation at the back.

<a id="l1338"></a>
**L1338** — <code>        if (history.size() &gt; limit) history.pop_front();</code>

Remove the oldest entry when the requested capacity is exceeded, keeping per-entity memory bounded.

<a id="l1339"></a>
**L1339** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1340"></a>
**L1340** — <code>    /// Start the gameplay clock once, regardless of which input mode starts play.</code>

Documentation fragment: “Start the gameplay clock once, regardless of which input mode starts play.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1341"></a>
**L1341** — <code>    void start_timer() {</code>

Start the gameplay clock once, seed director and progress tracking, and release the frozen monsters.

<a id="l1342"></a>
**L1342** — <code>        if (!timer_started) {</code>

Do nothing if gameplay timing has already begun.

<a id="l1343"></a>
**L1343** — <code>            timer_started = true; timer_start = now;</code>

Mark the timer active and store the current simulation time as its origin.

<a id="l1344"></a>
**L1344** — <code>            // Release the frozen monsters with their spawn-relative staggering intact.</code>

Documentation fragment: “Release the frozen monsters with their spawn-relative staggering intact.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1345"></a>
**L1345** — <code>            for (auto&amp; m : monsters) {</code>

Release the frozen monsters at the player's first input:

<a id="l1346"></a>
**L1346** — <code>                if (m.next_move &lt; NEVER) m.next_move += now;</code>

shift each spawn-relative deadline to start now, keeping the original staggering (NEVER stays NEVER)...

<a id="l1347"></a>
**L1347** — <code>                m.last_moved_at = now;</code>

...and treat release time as each monster's last movement, so none appears parked.

<a id="l1348"></a>
**L1348** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1349"></a>
**L1349** — <code>            if (!monsters.empty()) log_event("Player started | Monsters released.");</code>

Log the start when there are monsters to release.

<a id="l1350"></a>
**L1350** — <code>            director.samples.push_back({now, player});</code>

Seed the director's observation window with the player's starting location at actual play onset.

<a id="l1351"></a>
**L1351** — <code>            director.next_update = now + DIRECTOR_TICK_MS;</code>

Schedule the first director sample 200 ms later.

<a id="l1352"></a>
**L1352** — <code>            best_exit_distance = (*exit_distances)[player];</code>

Initialize best-ever progress using terrain distance from the current cell to exit.

<a id="l1353"></a>
**L1353** — <code>            navigation_progress_at = now;</code>

Start the no-progress clock at the same simulation time.

<a id="l1354"></a>
**L1354** — <code>            visits[player] = {now, 1.0};</code>

Mark the initial cell visited once so returning there can acquire a revisit penalty.

<a id="l1355"></a>
**L1355** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1356"></a>
**L1356** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1357"></a>
**L1357** — <code>    /// Sample elapsed simulation time before final state is frozen by advance_to.</code>

Documentation fragment: “Sample elapsed simulation time before final state is frozen by advance_to.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1358"></a>
**L1358** — <code>    void update_elapsed() { if (timer_started) elapsed_ms = now - timer_start; }</code>

Elapsed gameplay time is now minus timer_start when active; waiting before the first input is excluded, and monsters are frozen during it.

<a id="l1359"></a>
**L1359** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1360"></a>
**L1360** — <code>    /// Decay revisit cost by elapsed time. Waiting does not add visits or erase</code>

Documentation fragment: “Decay revisit cost by elapsed time. Waiting does not add visits or erase” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1361"></a>
**L1361** — <code>    /// the evidence of a loop, unlike a history truncated after a few actions.</code>

Documentation fragment: “the evidence of a loop, unlike a history truncated after a few actions.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1362"></a>
**L1362** — <code>    double visit_heat(int cell) const {</code>

Return a linearly time-decayed revisit penalty without mutating stored memory.

<a id="l1363"></a>
**L1363** — <code>        const auto&amp; visit = visits[cell];</code>

Read this cell's latest visit record.

<a id="l1364"></a>
**L1364** — <code>        return visit.heat * std::max(0.0, 1.0 - double(now - visit.at) / AUTO_VISIT_MEMORY_MS);</code>

Multiply recorded heat by max(0,1-age/12000); a long-idle cell becomes inexpensive, but its stored heat is not erased for novelty detection.

<a id="l1365"></a>
**L1365** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1366"></a>
**L1366** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1367"></a>
**L1367** — <code>    /// Commit one move or wait and resolve collision/victory atomically. Only a</code>

Documentation fragment: “Commit one move or wait and resolve collision/victory atomically. Only a” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1368"></a>
**L1368** — <code>    /// successful displacement publishes a movement hint; waits do not prolong hunting.</code>

Documentation fragment: “successful displacement publishes a movement hint; waits do not prolong hunting.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1369"></a>
**L1369** — <code>    bool commit_player_move(int destination, bool automatic) {</code>

Common transaction for one cardinal move or wait: validate, update clocks/memory, publish movement information, and resolve capture/escape.

<a id="l1370"></a>
**L1370** — <code>        if (outcome != Outcome::Playing || !walkable(destination) ||</code>

Reject terminal games and non-walkable destinations before performing any movement side effects.

<a id="l1371"></a>
**L1371** — <code>            manhattan(player, destination) &gt; 1) {</code>

Reject destinations farther than one Manhattan cell; remaining legal cases are a cardinal step or remaining still.

<a id="l1372"></a>
**L1372** — <code>            if (!automatic &amp;&amp; outcome == Outcome::Playing)</code>

Manual invalid moves during active play receive a blocked diagnostic.

<a id="l1373"></a>
**L1373** — <code>                log_event("Player move blocked at " + cell_label(player));</code>

Log the player's present location, without publishing a movement clue.

<a id="l1374"></a>
**L1374** — <code>            return false;</code>

Return false after rejection; an invalid input does not start the gameplay timer.

<a id="l1375"></a>
**L1375** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1376"></a>
**L1376** — <code>        start_timer();</code>

A valid move or automatic wait starts timing if needed.

<a id="l1377"></a>
**L1377** — <code>        if (destination != player) {</code>

Only actual displacement increments counters, visit heat, and scent.

<a id="l1378"></a>
**L1378** — <code>            int previous = player;</code>

Remember the departed cell for a movement log.

<a id="l1379"></a>
**L1379** — <code>            player = destination;</code>

Commit the new player cell before checking resulting collision/victory.

<a id="l1380"></a>
**L1380** — <code>            if (automatic) ++automatic_steps; else ++manual_steps;</code>

Increment the counter corresponding to the input mode.

<a id="l1381"></a>
**L1381** — <code>            overlay_dirty = true;</code>

The full-route display now starts at a new position and must be recomputed.

<a id="l1382"></a>
**L1382** — <code>            last_auto_wait = false;</code>

Reset the repeated-wait log suppressor after moving.

<a id="l1383"></a>
**L1383** — <code>            double heat = visit_heat(player);</code>

Read decayed revisit heat before recording the new arrival.

<a id="l1384"></a>
**L1384** — <code>            if (visits[player].heat == 0 || (*exit_distances)[player] &lt; best_exit_distance) navigation_progress_at = now;</code>

Recognize progress on a never-visited cell or a new all-time best exit distance. Decayed heat alone does not make an old cell novel again.

<a id="l1385"></a>
**L1385** — <code>            best_exit_distance = std::min(best_exit_distance, (*exit_distances)[player]);</code>

Maintain the smallest terrain-only exit distance achieved during the run.

<a id="l1386"></a>
**L1386** — <code>            visits[player] = {now, std::min(6.0, heat + 1.0)};</code>

Record this arrival's timestamp and heat, adding one and capping at six.

<a id="l1387"></a>
**L1387** — <code>            log_event(std::string("Player ") + (automatic ? "AUTO" : "MANUAL") + ": " +</code>

Begin a MANUAL/AUTO movement log with the departed coordinate.

<a id="l1388"></a>
**L1388** — <code>                      cell_label(previous) + " -&gt; " + cell_label(player));</code>

Append the arrived coordinate.

<a id="l1389"></a>
**L1389** — <code>            publish_movement_hint();</code>

An actual displacement renews scent for monsters within hearing range and may publish a coarse location report.

<a id="l1390"></a>
**L1390** — <code>        } else if (automatic &amp;&amp; !last_auto_wait) {</code>

A stationary automatic action logs once at the start of a consecutive waiting sequence.

<a id="l1391"></a>
**L1391** — <code>            log_event("Auto-solve: WAITING at " + cell_label(player) + " for a safer move.");</code>

Describe the generic safer-move wait at the current cell.

<a id="l1392"></a>
**L1392** — <code>            last_auto_wait = true;</code>

Suppress repeated identical wait messages until movement or mode reset.

<a id="l1393"></a>
**L1393** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1394"></a>
**L1394** — <code>        remember(player_history, player, 32);</code>

Record the resulting cell even for waits, with a 32-action cap; this is separate from time-decaying visit memory.

<a id="l1395"></a>
**L1395** — <code>        update_elapsed();</code>

Update elapsed simulation time.

<a id="l1396"></a>
**L1396** — <code>        check_outcome();</code>

Resolve overlap before escape, including after a manual step into an occupied cell.

<a id="l1397"></a>
**L1397** — <code>        refresh_solution_overlay();</code>

Refresh the complete current-to-exit overlay if enabled and dirty.

<a id="l1398"></a>
**L1398** — <code>        return true;</code>

Return true for an accepted action, including a legal wait; success here does not mean the player survived it.

<a id="l1399"></a>
**L1399** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1400"></a>
**L1400** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Monster targets, reservations, and forecasting

<a id="l1401"></a>
**L1401** — <code>    /// Avoid concentrating every monster on one candidate inside the hinted area.</code>

Documentation fragment: “Avoid concentrating every monster on one candidate inside the hinted area.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1402"></a>
**L1402** — <code>    bool target_claimed(const Monster&amp; m, int cell) const {</code>

Determine whether another monster of the same behavior state has already selected a candidate waypoint.

<a id="l1403"></a>
**L1403** — <code>        for (const auto&amp; other : monsters)</code>

Inspect the active monster set.

<a id="l1404"></a>
**L1404** — <code>            if (other.id != m.id &amp;&amp; other.state == m.state &amp;&amp; other.target == cell) return true;</code>

Ignore self and different states; any matching target counts as claimed.

<a id="l1405"></a>
**L1405** — <code>        return false;</code>

No matching competitor means this target receives no crowding penalty.

<a id="l1406"></a>
**L1406** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1407"></a>
**L1407** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1408"></a>
**L1408** — <code>    /// Hunt or search a reachable cell inside the approximate region, never an exact</code>

Documentation fragment: “Hunt or search a reachable cell inside the approximate region, never an exact” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1409"></a>
**L1409** — <code>    /// player coordinate (except a High-difficulty corridor sighting). Keep a valid</code>

Documentation fragment: “player coordinate (except a High-difficulty corridor sighting). Keep a valid” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1410"></a>
**L1410** — <code>    /// target until reached to avoid zigzagging. Interceptors hold their chokepoint.</code>

Documentation fragment: “target until reached to avoid zigzagging. Interceptors hold their chokepoint.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1411"></a>
**L1411** — <code>    /// Roaming prefers regions no monster has covered recently and avoids crowding.</code>

Documentation fragment: “Roaming prefers regions no monster has covered recently and avoids crowding.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1412"></a>
**L1412** — <code>    int choose_target(const Monster&amp; m) {</code>

Select a waypoint: an exact cell only for a High sighting, otherwise an interceptor chokepoint, a clue-area cell, a search cell, or a patrol waypoint.

<a id="l1413"></a>
**L1413** — <code>        if (m.sighted &amp;&amp; walkable(m.target) &amp;&amp; m.target != m.cell) return m.target;</code>

A charging monster keeps the exact last seen cell until it gets there.

<a id="l1414"></a>
**L1414** — <code>        if (m.role == MonsterRole::Cutter &amp;&amp; walkable(m.target)) return m.target;</code>

An interceptor keeps its chokepoint, even while standing on it.

<a id="l1415"></a>
**L1415** — <code>        bool area_mode = (m.state == MonsterState::Hunting || m.state == MonsterState::Searching) &amp;&amp; m.hint.revision != 0;</code>

Hunters and searchers with a clue pick cells inside the clue area; others roam.

<a id="l1416"></a>
**L1416** — <code>        if (walkable(m.target) &amp;&amp; m.target != m.cell &amp;&amp; m.blocked_turns &lt; 2 &amp;&amp;</code>

Prefer retaining a valid unreached target while blockage remains below two turns.

<a id="l1417"></a>
**L1417** — <code>            (!area_mode || m.hint.contains(point(m.target)))) return m.target;</code>

Keep an existing valid target that still lies inside the clue area (any target for a roaming monster).

<a id="l1418"></a>
**L1418** — <code>        auto from_self = local_field(m.cell);</code>

Bounded distances from this monster, computed once for all candidates.

<a id="l1419"></a>
**L1419** — <code>        if (area_mode) {</code>

Hunting or searching: choose a cell inside the clue rectangle.

<a id="l1420"></a>
**L1420** — <code>            const bool searching = m.state == MonsterState::Searching;</code>

Searchers sweep rather than converge, so they weigh distance less and repetition more.

<a id="l1421"></a>
**L1421** — <code>            std::vector&lt;int&gt; candidates;</code>

Store eligible cells within the hinted rectangle.

<a id="l1422"></a>
**L1422** — <code>            std::vector&lt;double&gt; weights;</code>

Store a parallel weight for each eligible destination.

<a id="l1423"></a>
**L1423** — <code>            for (int r = m.hint.min_row; r &lt;= m.hint.max_row; ++r) {</code>

Iterate inclusive hinted rows.

<a id="l1424"></a>
**L1424** — <code>                for (int c = m.hint.min_col; c &lt;= m.hint.max_col; ++c) {</code>

Iterate inclusive hinted columns.

<a id="l1425"></a>
**L1425** — <code>                    int v = id({r, c});</code>

Convert the candidate coordinate to a flat cell ID.

<a id="l1426"></a>
**L1426** — <code>                    if (!walkable(v) || v == m.cell) continue;</code>

Skip walls and the monster's own cell; a cell beyond the bounded radius still counts, with distance 65.

<a id="l1427"></a>
**L1427** — <code>                    int distance = from_self-&gt;at(v);</code>

Corridor distance from the monster, or 65 beyond the radius.

<a id="l1428"></a>
**L1428** — <code>                    int recent = static_cast&lt;int&gt;(std::count(m.recent_cells.begin(), m.recent_cells.end(), v));</code>

Count how often this candidate appears in the monster's recent-cell history.

<a id="l1429"></a>
**L1429** — <code>                    // Hunters converge on nearby cells; searchers sweep, so distance matters less.</code>

Documentation fragment: “Hunters converge on nearby cells; searchers sweep, so distance matters less.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1430"></a>
**L1430** — <code>                    double weight = 1.0 / (1.0 + (searching ? 0.01 : 0.03) * distance + (searching ? 4.0 : 2.0) * recent);</code>

Hunters prefer nearby cells (0.03 per step); searchers barely care about distance (0.01) and avoid recent cells twice as strongly.

<a id="l1431"></a>
**L1431** — <code>                    if (target_claimed(m, v)) weight *= 0.15;</code>

Multiply weight by 0.15 if another same-state monster claims this target, encouraging distributed search without forbidding overlap of intentions.

<a id="l1432"></a>
**L1432** — <code>                    candidates.push_back(v); weights.push_back(weight);</code>

Append the candidate and matching probability weight.

<a id="l1433"></a>
**L1433** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1434"></a>
**L1434** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1435"></a>
**L1435** — <code>            if (!candidates.empty()) {</code>

If the approximate area contains any eligible destination, sample one.

<a id="l1436"></a>
**L1436** — <code>                std::discrete_distribution&lt;std::size_t&gt; pick(weights.begin(), weights.end());</code>

Construct a weighted discrete distribution; its exact algorithm can differ between standard-library implementations.

<a id="l1437"></a>
**L1437** — <code>                return candidates[pick(rng)];</code>

Consume RNG to choose the hunt or search waypoint by weight, not by exact player proximity.

<a id="l1438"></a>
**L1438** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1439"></a>
**L1439** — <code>            return m.cell;</code>

If no eligible hinted destination exists, stay at the current cell rather than leaking an exact target.

<a id="l1440"></a>
**L1440** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1441"></a>
**L1441** — <code>        int best = m.cell;</code>

Initialize patrol choice to staying put as a safe fallback.

<a id="l1442"></a>
**L1442** — <code>        double best_score = -std::numeric_limits&lt;double&gt;::infinity();</code>

Start with negative infinity so any usable sampled candidate can win.

<a id="l1443"></a>
**L1443** — <code>        for (int i = 0; i &lt; 32 &amp;&amp; !open_cells.empty(); ++i) {</code>

Sample up to 32 random open cells, bounding patrol selection cost after the bounded field exists.

<a id="l1444"></a>
**L1444** — <code>            int v = open_cells[rng() % open_cells.size()];</code>

Choose a cell index using the game RNG and modulo the open-cell list length.

<a id="l1445"></a>
**L1445** — <code>            if (v == m.cell) continue;</code>

Skip the monster's own cell.

<a id="l1446"></a>
**L1446** — <code>            int distance = from_self-&gt;at(v);</code>

Bounded corridor distance to the sampled cell.

<a id="l1447"></a>
**L1447** — <code>            int recent = static_cast&lt;int&gt;(std::count(m.recent_cells.begin(), m.recent_cells.end(), v));</code>

Count prior visits to this sampled waypoint.

<a id="l1448"></a>
**L1448** — <code>            double staleness = double(std::min&lt;Millis&gt;(region_age(v), 20000)) / 1000.0;</code>

Seconds since any monster entered this cell's 8x8 region, capped at 20.

<a id="l1449"></a>
**L1449** — <code>            int crowd = 0;</code>

Count other monsters already near this cell or heading near it.

<a id="l1450"></a>
**L1450** — <code>            for (const auto&amp; other : monsters)</code>

Check every other monster.

<a id="l1451"></a>
**L1451** — <code>                if (other.id != m.id &amp;&amp; (manhattan(other.cell, v) &lt;= 6 || (walkable(other.target) &amp;&amp; manhattan(other.target, v) &lt;= 6))) ++crowd;</code>

Another monster within six cells of the candidate, or with a target within six cells, counts as crowding.

<a id="l1452"></a>
**L1452** — <code>            double score = std::min(distance, 20) * 0.3 + staleness * 0.8 - recent * 8</code>

Score distance (capped at 20) lightly and favor stale regions strongly...

<a id="l1453"></a>
**L1453** — <code>                         - (target_claimed(m, v) ? 8 : 0) - crowd * 4.0;</code>

...and penalize recent cells, a target another monster already claimed, and crowding.

<a id="l1454"></a>
**L1454** — <code>            if (score &gt; best_score) { best_score = score; best = v; }</code>

Keep the highest-scoring patrol destination; ties preserve the earlier sampled choice.

<a id="l1455"></a>
**L1455** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1456"></a>
**L1456** — <code>        return best;</code>

Return the best patrol waypoint, or the current cell if no candidate was useful.

<a id="l1457"></a>
**L1457** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1458"></a>
**L1458** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1459"></a>
**L1459** — <code>    static bool is_reserved(const std::vector&lt;int&gt;&amp; reserved, int cell) {</code>

Membership test for the short reservation list.

<a id="l1460"></a>
**L1460** — <code>        return std::find(reserved.begin(), reserved.end(), cell) != reserved.end();</code>

Linear search is cheap because at most about twenty cells are reserved.

<a id="l1461"></a>
**L1461** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1462"></a>
**L1462** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1463"></a>
**L1463** — <code>    /// Follow a shared reverse-distance field while respecting reserved cells.</code>

Documentation fragment: “Follow a shared reverse-distance field while respecting reserved cells.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1464"></a>
**L1464** — <code>    /// If blocked, allow a temporary side/back step to make room at bottlenecks.</code>

Documentation fragment: “If blocked, allow a temporary side/back step to make room at bottlenecks.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1465"></a>
**L1465** — <code>    int choose_monster_step(const Monster&amp; m, const std::vector&lt;int&gt;&amp; reserved) {</code>

Choose the next step toward this monster's target while respecting the reserved-cell list.

<a id="l1466"></a>
**L1466** — <code>        if (!walkable(m.target) || m.cell == m.target) return m.cell;</code>

An invalid or already reached target produces a wait.

<a id="l1467"></a>
**L1467** — <code>        auto distance = distance_field(m.target);</code>

Get terrain distance-to-target for every cell; cached fields can be shared across monsters.

<a id="l1468"></a>
**L1468** — <code>        int best = m.cell;</code>

Default to remaining in place.

<a id="l1469"></a>
**L1469** — <code>        double best_score = (*distance)[m.cell];</code>

Use current target distance as the score to beat.

<a id="l1470"></a>
**L1470** — <code>        if (m.blocked_turns &gt;= 1) best_score += 3.0;</code>

After one blocked turn, permit up to three extra distance units of sidestep/backtrack cost to relieve bottlenecks.

<a id="l1471"></a>
**L1471** — <code>        for (int v : adjacency[m.cell]) {</code>

Consider every cardinal neighbor.

<a id="l1472"></a>
**L1472** — <code>            if (v &lt; 0 || is_reserved(reserved, v) || (*distance)[v] == INF_DISTANCE) continue;</code>

Skip absent, reserved, or unreachable neighbors.

<a id="l1473"></a>
**L1473** — <code>            double score = (*distance)[v] + 0.15 * std::count(m.recent_cells.begin(), m.recent_cells.end(), v);</code>

Score remaining target distance plus a small 0.15 penalty per recent visit.

<a id="l1474"></a>
**L1474** — <code>            if (score &lt; best_score) { best_score = score; best = v; }</code>

Choose strictly lower score; deterministic neighbor order resolves exact ties.

<a id="l1475"></a>
**L1475** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1476"></a>
**L1476** — <code>        return best;</code>

Return one legal step or the original cell.

<a id="l1477"></a>
**L1477** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1478"></a>
**L1478** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1479"></a>
**L1479** — <code>    // Intent predictions are estimates: new movement hints/replanning can change them.</code>

Documentation fragment: “Intent predictions are estimates: new movement hints/replanning can change them.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1480"></a>
**L1480** — <code>    struct Forecast {</code>

Snapshot forecast container; predictions estimate retained intentions rather than guarantee actual future choices.

<a id="l1481"></a>
**L1481** — <code>        int horizon = AUTO_HORIZON;</code>

Lookahead depth used to build this forecast.

<a id="l1482"></a>
**L1482** — <code>        std::vector&lt;std::vector&lt;int&gt;&gt; positions;</code>

For each tactical depth, store one predicted cell per monster.

<a id="l1483"></a>
**L1483** — <code>        std::vector&lt;std::shared_ptr&lt;const LocalField&gt;&gt; distances;</code>

Bounded per-monster distance fields, shared with the pool so they stay valid while the forecast exists.

<a id="l1484"></a>
**L1484** — <code>        std::vector&lt;Millis&gt; first_move_delay;</code>

Record each enemy's delay until its next scheduled move.

<a id="l1485"></a>
**L1485** — <code>    };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l1486"></a>
**L1486** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1487"></a>
**L1487** — <code>    /// Roll out cached intentions at actual deadlines using the same reservation</code>

Documentation fragment: “Roll out cached intentions at actual deadlines using the same reservation” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1488"></a>
**L1488** — <code>    /// rules. Also retain conservative reachability fields for uncertain behavior.</code>

Documentation fragment: “rules. Also retain conservative reachability fields for uncertain behavior.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1489"></a>
**L1489** — <code>    /// Distances are bounded around each monster; far cells get a lower bound.</code>

Documentation fragment: “Distances are bounded around each monster; far cells get a lower bound.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1490"></a>
**L1490** — <code>    Forecast forecast_monsters(int horizon = AUTO_HORIZON) {</code>

Build a forecast for the requested lookahead, defaulting to 12 actions.

<a id="l1491"></a>
**L1491** — <code>        Forecast forecast;</code>

Initialize a new forecast result.

<a id="l1492"></a>
**L1492** — <code>        forecast.horizon = std::clamp(horizon, 1, AUTO_HORIZON_MAX);</code>

Clamp the horizon between one action and the 18-action maximum.

<a id="l1493"></a>
**L1493** — <code>        forecast.positions.resize(static_cast&lt;std::size_t&gt;(forecast.horizon) + 1);</code>

Allocate monster positions for each depth from zero through the horizon.

<a id="l1494"></a>
**L1494** — <code>        std::vector&lt;Monster&gt; predicted = monsters;</code>

Copy monsters so rollout updates do not mutate live positions, schedules, blockage, or memory.

<a id="l1495"></a>
**L1495** — <code>        for (const auto&amp; m : monsters) {</code>

Initialize each monster's conservative reachability information.

<a id="l1496"></a>
**L1496** — <code>            forecast.distances.push_back(local_field(m.cell));</code>

Use a bounded threat field per monster rather than a whole-grid BFS.

<a id="l1497"></a>
**L1497** — <code>            forecast.first_move_delay.push_back(std::max&lt;Millis&gt;(0, m.next_move - now));</code>

Clamp overdue first-move delays to zero rather than predicting a negative arrival time.

<a id="l1498"></a>
**L1498** — <code>            forecast.positions[0].push_back(m.cell);</code>

Store live occupancy at the initial forecast layer.

<a id="l1499"></a>
**L1499** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1500"></a>
**L1500** — <code>        std::size_t rotation = priority_rotation;</code>

Copy rotating reservation priority for the prediction.

<a id="l1501"></a>
**L1501** — <code>        std::vector&lt;int&gt; reserved;</code>

Reusable reservation list for the rollout.

<a id="l1502"></a>
**L1502** — <code>        for (int depth = 1; depth &lt;= forecast.horizon; ++depth) {</code>

Roll the prediction forward to the chosen horizon.

<a id="l1503"></a>
**L1503** — <code>            Millis boundary = now + depth * AUTO_SOLVE_SPEED_MS;</code>

Convert depth into an absolute future simulation timestamp.

<a id="l1504"></a>
**L1504** — <code>            while (!predicted.empty()) {</code>

Process predicted monster events due before this boundary; no iteration occurs with zero enemies.

<a id="l1505"></a>
**L1505** — <code>                Millis event = NEVER;</code>

Initialize earliest predicted event as unscheduled.

<a id="l1506"></a>
**L1506** — <code>                for (const auto&amp; m : predicted) event = std::min(event, m.next_move);</code>

Find the minimum next_move among copied monsters.

<a id="l1507"></a>
**L1507** — <code>                if (event &gt; boundary) break;</code>

Stop this layer's rollout once the next event lies after its boundary.

<a id="l1508"></a>
**L1508** — <code>                reserved.clear();</code>

Clear the reservation list for this event.

<a id="l1509"></a>
**L1509** — <code>                for (const auto&amp; m : predicted) reserved.push_back(m.cell);</code>

Reserve every predicted monster's current cell.

<a id="l1510"></a>
**L1510** — <code>                for (std::size_t k = 0; k &lt; predicted.size(); ++k) {</code>

Apply the same rotated order as the live movement system.

<a id="l1511"></a>
**L1511** — <code>                    auto&amp; m = predicted[(rotation + k) % predicted.size()];</code>

Select the copied monster in that rotated order.

<a id="l1512"></a>
**L1512** — <code>                    if (m.next_move != event) continue;</code>

Only monsters due at this exact event act.

<a id="l1513"></a>
**L1513** — <code>                    // Apply the same corridor-sight rule as the game: a monster that</code>

Documentation fragment: “Apply the same corridor-sight rule as the game: a monster that” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1514"></a>
**L1514** — <code>                    // can see the player's current cell is predicted to charge it.</code>

Documentation fragment: “can see the player's current cell is predicted to charge it.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1515"></a>
**L1515** — <code>                    if ((m.state != MonsterState::Hunting || m.sighted) &amp;&amp; line_of_sight(m.cell, player)) m.target = player;</code>

Apply the game's sight rule: an unalerted or charging monster that can see the player's current cell is predicted to charge it.

<a id="l1516"></a>
**L1516** — <code>                    int next = choose_monster_step(m, reserved);</code>

Predict its next step from the existing waypoint; future hint-driven target changes are intentionally not simulated.

<a id="l1517"></a>
**L1517** — <code>                    if (next != m.cell) {</code>

Handle successful predicted displacement.

<a id="l1518"></a>
**L1518** — <code>                        remember(m.recent_cells, m.cell, 12);</code>

Update copied recent-cell memory with the departed cell.

<a id="l1519"></a>
**L1519** — <code>                        m.cell = next; reserved.push_back(next); m.blocked_turns = 0;</code>

Move the predicted monster and reserve its destination.

<a id="l1520"></a>
**L1520** — <code>                    } else if (m.cell != m.target) ++m.blocked_turns;</code>

Count predicted blockage when a stationary monster has not reached its waypoint.

<a id="l1521"></a>
**L1521** — <code>                    m.next_move += settings.monster_move_ms;</code>

Advance the copied deadline by one profile-specific monster period.

<a id="l1522"></a>
**L1522** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1523"></a>
**L1523** — <code>                rotation = (rotation + 1) % predicted.size();</code>

Rotate prediction priority after the event batch, matching live fairness behavior.

<a id="l1524"></a>
**L1524** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1525"></a>
**L1525** — <code>            for (const auto&amp; m : predicted) forecast.positions[depth].push_back(m.cell);</code>

Store the predicted occupancy vector for this player-action depth.

<a id="l1526"></a>
**L1526** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1527"></a>
**L1527** — <code>        return forecast;</code>

Return both intended trajectories and conservative reachability fields.

<a id="l1528"></a>
**L1528** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1529"></a>
**L1529** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1530"></a>
**L1530** — <code>    /// Earliest possible arrival using corridor distance and the next due move.</code>

Documentation fragment: “Earliest possible arrival using corridor distance and the next due move.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1531"></a>
**L1531** — <code>    /// A monster can wait, so this bounds possibility, not capture probability.</code>

Documentation fragment: “A monster can wait, so this bounds possibility, not capture probability.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1532"></a>
**L1532** — <code>    Millis earliest_arrival(const Forecast&amp; forecast, std::size_t monster, int cell) const {</code>

Compute a lower bound on possible arrival at a cell from graph distance and the enemy's current schedule; not a probability estimate.

<a id="l1533"></a>
**L1533** — <code>        int distance = forecast.distances[monster]-&gt;at(cell);</code>

Corridor distance from the bounded field; cells beyond 64 steps report 65, a lower bound.

<a id="l1534"></a>
**L1534** — <code>        if (distance == 0) return 0;</code>

Already occupying the cell means possible contact at time zero.

<a id="l1535"></a>
**L1535** — <code>        return forecast.first_move_delay[monster] + (distance - 1) * settings.monster_move_ms;</code>

First movement uses its actual remaining delay; each of the other distance-1 steps costs the monster period.

<a id="l1536"></a>
**L1536** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1537"></a>
**L1537** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1538"></a>
**L1538** — <code>    /// Score time-indexed actions. For the first interval, prefer excluding all</code>

Documentation fragment: “Score time-indexed actions. For the first interval, prefer excluding all” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1539"></a>
**L1539** — <code>    /// reachable threats; later intervals use soft risk to avoid total paralysis.</code>

Documentation fragment: “reachable threats; later intervals use soft risk to avoid total paralysis.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1540"></a>
**L1540** — <code>    bool evaluate_action(const Forecast&amp; forecast, int cell, int depth, bool conservative,</code>

Evaluate one time-indexed player action under forecast occupancy and conservative/soft uncertainty rules.

<a id="l1541"></a>
**L1541** — <code>                         double&amp; risk) const {</code>

Output risk is accumulated by reference; boolean return indicates whether hard constraints allow the action.

<a id="l1542"></a>
**L1542** — <code>        risk = 0;</code>

Reset the action's soft-risk score before considering enemies.

<a id="l1543"></a>
**L1543** — <code>        const Millis end = (depth + 1) * AUTO_SOLVE_SPEED_MS;</code>

Define the end of this occupied player interval relative to the current planning time.

<a id="l1544"></a>
**L1544** — <code>        for (std::size_t i = 0; i &lt; monsters.size(); ++i) {</code>

Check each monster independently and accumulate risks.

<a id="l1545"></a>
**L1545** — <code>            // Arrival at the exit terminates play immediately after occupancy checks.</code>

Documentation fragment: “Arrival at the exit terminates play immediately after occupancy checks.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1546"></a>
**L1546** — <code>            if (forecast.positions[depth][i] == cell) return false;</code>

Reject occupying a cell where a monster is predicted at the action's starting boundary.

<a id="l1547"></a>
**L1547** — <code>            if (cell != exit_cell &amp;&amp; forecast.positions[depth + 1][i] == cell) return false;</code>

Reject predicted enemy occupancy at the next boundary, except that reaching the exit ends play immediately after arrival checks.

<a id="l1548"></a>
**L1548** — <code>            Millis arrival = earliest_arrival(forecast, i, cell);</code>

Get the fastest plausible arrival bound, independent of the retained-target forecast.

<a id="l1549"></a>
**L1549** — <code>            if (depth == 0 &amp;&amp; conservative &amp;&amp; cell != exit_cell &amp;&amp; arrival &lt;= end) return false;</code>

The first conservative action rejects any non-exit cell an enemy could reach before the interval ends.

<a id="l1550"></a>
**L1550** — <code>            Millis margin = arrival - (cell == exit_cell ? depth * AUTO_SOLVE_SPEED_MS : end);</code>

Compute temporal clearance relative to immediate goal arrival or the end of a non-goal occupancy interval.

<a id="l1551"></a>
**L1551** — <code>            if (margin &lt;= 0) risk += 5.0;</code>

An arrival bound already inside the interval contributes a substantial soft risk of five.

<a id="l1552"></a>
**L1552** — <code>            else if (margin &lt; settings.monster_move_ms) risk += 2.0 * (1.0 - double(margin) / settings.monster_move_ms);</code>

Smaller positive margins within one monster period contribute a linearly decaying risk up to two.

<a id="l1553"></a>
**L1553** — <code>            // A deep branch can look safe locally while its only mouth is cut off.</code>

Documentation fragment: “A deep branch can look safe locally while its only mouth is cut off.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1554"></a>
**L1554** — <code>            // Compare the fastest return to that mouth with a monster's arrival.</code>

Documentation fragment: “Compare the fastest return to that mouth with a monster's arrival.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1555"></a>
**L1555** — <code>            if (branch_gate[cell] &gt;= 0 &amp;&amp; cell != exit_cell) {</code>

Blind branches need an extra gate-risk calculation; exit cells do not suffer this branch penalty.

<a id="l1556"></a>
**L1556** — <code>                Millis return_time = (depth + dead_end_depth[cell]) * AUTO_SOLVE_SPEED_MS;</code>

Estimate the player's earliest return to the branch mouth using depth plus dead-end distance at 40 ms per step.

<a id="l1557"></a>
**L1557** — <code>                if (earliest_arrival(forecast, i, branch_gate[cell]) &lt;= return_time + settings.monster_move_ms)</code>

If a monster can reach that mouth before the player's return plus one monster interval, treat the branch as potentially trap-prone.

<a id="l1558"></a>
**L1558** — <code>                    risk += 3.0 + std::min(dead_end_depth[cell], 12) * 0.2;</code>

Add risk three plus 0.2 times branch depth, capped at depth twelve.

<a id="l1559"></a>
**L1559** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1560"></a>
**L1560** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1561"></a>
**L1561** — <code>        return true;</code>

No hard rejection occurred; retain the accumulated soft risk for ranking.

<a id="l1562"></a>
**L1562** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1563"></a>
**L1563** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1564"></a>
**L1564** — <code>    // Each surviving node stores a parent in the bounded search tree for recovery.</code>

Documentation fragment: “Each surviving node stores a parent in the bounded search tree for recovery.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1565"></a>
**L1565** — <code>    struct PlanNode {</code>

One node in the bounded tactical search tree, with enough parent information to reconstruct the first action.

<a id="l1566"></a>
**L1566** — <code>        int cell, parent, depth;</code>

Store occupied cell, parent tree index, and action depth.

<a id="l1567"></a>
**L1567** — <code>        double cost, rank;</code>

cost accumulates movement/risk/revisit terms; rank adds goal-distance guidance.

<a id="l1568"></a>
**L1568** — <code>    };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l1569"></a>
**L1569** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Timing-aware guidance and recovery navigation

<a id="l1570"></a>
**L1570** — <code>    /// Guide the short temporal search around occupied bottlenecks. A monster is a</code>

Documentation fragment: “Guide the short temporal search around occupied bottlenecks. A monster is a” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1571"></a>
**L1571** — <code>    /// wall only if the player could reach it before it is likely to move (near, or</code>

Documentation fragment: “wall only if the player could reach it before it is likely to move (near, or” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1572"></a>
**L1572** — <code>    /// parked); a distant moving monster adds a soft detour cost instead, so it</code>

Documentation fragment: “parked); a distant moving monster adds a soft detour cost instead, so it” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1573"></a>
**L1573** — <code>    /// cannot freeze the whole plan from across the map.</code>

Documentation fragment: “cannot freeze the whole plan from across the map.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1574"></a>
**L1574** — <code>    const std::vector&lt;int&gt;&amp; route_guidance() {</code>

Return weighted exit guidance in which near or parked monsters are walls and distant moving monsters add a soft cost, with terrain-only fallback when walls cut every route.

<a id="l1575"></a>
**L1575** — <code>        auto from_player = local_field(player);</code>

Bounded distances from the player decide which monsters are near.

<a id="l1576"></a>
**L1576** — <code>        std::vector&lt;int&gt; hard, soft;</code>

Monster cells split into hard walls and soft costs.

<a id="l1577"></a>
**L1577** — <code>        for (const auto&amp; m : monsters) {</code>

Classify each monster.

<a id="l1578"></a>
**L1578** — <code>            bool parked = now - m.last_moved_at &gt;= AUTO_STATIONARY_MS;</code>

A monster that has not moved for 1.5 s is parked.

<a id="l1579"></a>
**L1579** — <code>            if (from_player-&gt;at(m.cell) &lt;= AUTO_NEAR_THREAT_CELLS || parked) hard.push_back(m.cell);</code>

Near monsters (14 steps or fewer) and parked ones block outright.

<a id="l1580"></a>
**L1580** — <code>            else soft.push_back(m.cell);</code>

Distant moving monsters only add cost.

<a id="l1581"></a>
**L1581** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1582"></a>
**L1582** — <code>        std::sort(hard.begin(), hard.end()); std::sort(soft.begin(), soft.end());</code>

Sort both lists for binary search and a stable cache key.

<a id="l1583"></a>
**L1583** — <code>        std::vector&lt;int&gt; key = hard;</code>

The cache key starts with the hard cells...

<a id="l1584"></a>
**L1584** — <code>        key.push_back(-1);</code>

...a separator...

<a id="l1585"></a>
**L1585** — <code>        key.insert(key.end(), soft.begin(), soft.end());</code>

...then the soft cells, so a change in classification also rebuilds guidance.

<a id="l1586"></a>
**L1586** — <code>        if (guide_distances.empty() || key != guide_occupancy) {</code>

Recompute only when occupancy or classification changed.

<a id="l1587"></a>
**L1587** — <code>            overlay_dirty = true;</code>

Mark the rendered overlay dirty when its underlying weighted guidance changes.

<a id="l1588"></a>
**L1588** — <code>            guide_occupancy = std::move(key);</code>

Store the new key.

<a id="l1589"></a>
**L1589** — <code>            const int n = static_cast&lt;int&gt;(grid.size());</code>

Cell count.

<a id="l1590"></a>
**L1590** — <code>            guide_distances.assign(n, INF_DISTANCE);</code>

Start with every cell unreachable.

<a id="l1591"></a>
**L1591** — <code>            guide_next.assign(n, -1);</code>

Reset every stored successor to the no-route sentinel before computing the new field.

<a id="l1592"></a>
**L1592** — <code>            const auto is_hard = [&amp;](int u) { return std::binary_search(hard.begin(), hard.end(), u); };</code>

Hard-cell test by binary search.

<a id="l1593"></a>
**L1593** — <code>            const auto entry_cost = [&amp;](int u) {</code>

Cost of stepping onto a cell:

<a id="l1594"></a>
**L1594** — <code>                return 1 + (std::binary_search(soft.begin(), soft.end(), u) ? AUTO_FAR_THREAT_COST : 0);</code>

one step, plus 24 if a distant monster stands there.

<a id="l1595"></a>
**L1595** — <code>            };</code>

Close the entry-cost lambda.

<a id="l1596"></a>
**L1596** — <code>            // Dial's algorithm: integer edge costs in [1, AUTO_FAR_THREAT_COST + 1].</code>

Documentation fragment: “Dial's algorithm: integer edge costs in [1, AUTO_FAR_THREAT_COST + 1].” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1597"></a>
**L1597** — <code>            std::vector&lt;std::vector&lt;int&gt;&gt; buckets(AUTO_FAR_THREAT_COST + 2);</code>

Dial's bucket queue: costs range from 1 to 25, so 26 circular buckets suffice.

<a id="l1598"></a>
**L1598** — <code>            std::size_t pending = 0;</code>

Number of queued entries.

<a id="l1599"></a>
**L1599** — <code>            if (!is_hard(exit_cell)) {</code>

An exit occupied by a near or parked monster leaves every cell unreachable.

<a id="l1600"></a>
**L1600** — <code>                guide_distances[exit_cell] = 0; buckets[0].push_back(exit_cell); pending = 1;</code>

Seed the exit at cost zero.

<a id="l1601"></a>
**L1601** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1602"></a>
**L1602** — <code>            for (int current = 0; pending &gt; 0; ++current) {</code>

Process cost levels in increasing order until the queue is empty.

<a id="l1603"></a>
**L1603** — <code>                auto&amp; bucket = buckets[static_cast&lt;std::size_t&gt;(current) % buckets.size()];</code>

Bucket for this cost level.

<a id="l1604"></a>
**L1604** — <code>                while (!bucket.empty()) {</code>

Drain it.

<a id="l1605"></a>
**L1605** — <code>                    int u = bucket.back(); bucket.pop_back(); --pending;</code>

Pop a cell.

<a id="l1606"></a>
**L1606** — <code>                    if (guide_distances[u] != current) continue;</code>

Skip stale entries superseded by a cheaper cost.

<a id="l1607"></a>
**L1607** — <code>                    // Entering u costs entry_cost(u); its neighbors pay that to step onto u.</code>

Documentation fragment: “Entering u costs entry_cost(u); its neighbors pay that to step onto u.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1608"></a>
**L1608** — <code>                    for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; !is_hard(v)) {</code>

Relax each open, non-hard neighbor.

<a id="l1609"></a>
**L1609** — <code>                        int candidate = current + entry_cost(u);</code>

A neighbor reaches the exit through u by paying u's entry cost.

<a id="l1610"></a>
**L1610** — <code>                        if (candidate &lt; guide_distances[v]) {</code>

Only improvements are recorded.

<a id="l1611"></a>
**L1611** — <code>                            guide_distances[v] = candidate;</code>

Store the cheaper cost.

<a id="l1612"></a>
**L1612** — <code>                            guide_next[v] = u;  // The cost of entering u is already in candidate.</code>

Store u as v's next step on the winning relaxation. The cost includes any penalty for entering u.

<a id="l1613"></a>
**L1613** — <code>                            buckets[static_cast&lt;std::size_t&gt;(candidate) % buckets.size()].push_back(v);</code>

Queue the neighbor in its cost bucket.

<a id="l1614"></a>
**L1614** — <code>                            ++pending;</code>

Count the queued entry.

<a id="l1615"></a>
**L1615** — <code>                        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1616"></a>
**L1616** — <code>                    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1617"></a>
**L1617** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1618"></a>
**L1618** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1619"></a>
**L1619** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1620"></a>
**L1620** — <code>        return guide_distances[player] == INF_DISTANCE ? *exit_distances : guide_distances;</code>

Return the weighted guidance when the player still has a route past near and parked monsters; otherwise use the terrain field so local evasion still has an exit direction.

<a id="l1621"></a>
**L1621** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1622"></a>
**L1622** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1623"></a>
**L1623** — <code>    /// On unit-cost terrain the first lower neighbor is a shortest step to the exit.</code>

Describes why an unweighted distance gradient suffices on terrain-only routes.

<a id="l1624"></a>
**L1624** — <code>    int terrain_exit_step(int cell) const {</code>

Select a shortest-path successor using the cached terrain field.

<a id="l1625"></a>
**L1625** — <code>        for (int v : adjacency[cell])</code>

Check every neighbor.

<a id="l1626"></a>
**L1626** — <code>            if (v &gt;= 0 &amp;&amp; (*exit_distances)[v] &lt; (*exit_distances)[cell]) return v;</code>

Return the first valid four-neighbor whose exact terrain distance decreases.

<a id="l1627"></a>
**L1627** — <code>        return -1;</code>

Indicate that no downhill terrain successor exists.

<a id="l1628"></a>
**L1628** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1629"></a>
**L1629** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1630"></a>
**L1630** — <code>    /// Construct a complete distance field around the current occupied cells.</code>

Documentation fragment: “Construct a complete distance field around the current occupied cells.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1631"></a>
**L1631** — <code>    /// Recovery uses its own field so the highlighted route always targets the exit.</code>

Documentation fragment: “Recovery uses its own field so the highlighted route always targets the exit.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1632"></a>
**L1632** — <code>    std::vector&lt;int&gt; recovery_distance_field(int goal) const {</code>

Compute an independent occupancy-aware BFS toward a recovery waypoint; this avoids redirecting the visual exit overlay toward a temporary detour.

<a id="l1633"></a>
**L1633** — <code>        std::vector&lt;int&gt; distance(grid.size(), INF_DISTANCE), queue, blocked;</code>

Distances, BFS queue, and the short list of monster cells to avoid.

<a id="l1634"></a>
**L1634** — <code>        for (const auto&amp; m : monsters) blocked.push_back(m.cell);</code>

Every monster cell blocks the recovery route.

<a id="l1635"></a>
**L1635** — <code>        if (walkable(goal) &amp;&amp; !is_reserved(blocked, goal)) { distance[goal] = 0; queue.push_back(goal); }</code>

Seed the goal unless a monster occupies it.

<a id="l1636"></a>
**L1636** — <code>        for (std::size_t head = 0; head &lt; queue.size(); ++head) {</code>

Traverse the queue in BFS order.

<a id="l1637"></a>
**L1637** — <code>            int u = queue[head];</code>

Read the active cell.

<a id="l1638"></a>
**L1638** — <code>            for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; distance[v] == INF_DISTANCE &amp;&amp; !is_reserved(blocked, v)) {</code>

Expand through open neighbors not occupied by monsters.

<a id="l1639"></a>
**L1639** — <code>                distance[v] = distance[u] + 1; queue.push_back(v);</code>

Assign one-more-than-parent distance and enqueue it.

<a id="l1640"></a>
**L1640** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1641"></a>
**L1641** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1642"></a>
**L1642** — <code>        return distance;</code>

Return the complete recovery distance field, including unreachable sentinels.

<a id="l1643"></a>
**L1643** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1644"></a>
**L1644** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1645"></a>
**L1645** — <code>    /// Retire a waypoint and remember its neighborhood long enough to avoid</code>

Documentation fragment: “Retire a waypoint and remember its neighborhood long enough to avoid” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1646"></a>
**L1646** — <code>    /// alternating between the same two retreat destinations on successive plans.</code>

Documentation fragment: “alternating between the same two retreat destinations on successive plans.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1647"></a>
**L1647** — <code>    void finish_recovery(const std::string&amp; reason) {</code>

Retire a committed detour and remember its neighborhood so the next attempt avoids oscillating between the same choices.

<a id="l1648"></a>
**L1648** — <code>        if (recovery_target &lt; 0) return;</code>

No-op when no recovery waypoint is active.

<a id="l1649"></a>
**L1649** — <code>        log_event("Auto-solve: Detour " + reason + " at " + cell_label(player) +</code>

Begin a diagnostic explaining why the detour ended and where the player is now.

<a id="l1650"></a>
**L1650** — <code>                  " | Previous waypoint " + cell_label(recovery_target));</code>

Append the retired waypoint coordinate.

<a id="l1651"></a>
**L1651** — <code>        tried_recovery_goals.emplace_back(recovery_target, now + AUTO_VISIT_MEMORY_MS);</code>

Remember the goal until now plus twelve seconds.

<a id="l1652"></a>
**L1652** — <code>        if (tried_recovery_goals.size() &gt; 16) tried_recovery_goals.pop_front();</code>

Cap remembered recovery endpoints at sixteen.

<a id="l1653"></a>
**L1653** — <code>        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();</code>

Remove the goal and its path/occupancy cache.

<a id="l1654"></a>
**L1654** — <code>        next_recovery_attempt = now + 400;</code>

Wait at least 400 ms before searching for another recovery target.

<a id="l1655"></a>
**L1655** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1656"></a>
**L1656** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1657"></a>
**L1657** — <code>    /// Search reachable alternatives, including cells farther from the exit.</code>

Documentation fragment: “Search reachable alternatives, including cells farther from the exit.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1658"></a>
**L1658** — <code>    /// Favor unvisited junctions and room to maneuver; penalize recently tried</code>

Documentation fragment: “Favor unvisited junctions and room to maneuver; penalize recently tried” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1659"></a>
**L1659** — <code>    /// neighborhoods and blind branches. Work is bounded on maximum-sized mazes.</code>

Documentation fragment: “neighborhoods and blind branches. Work is bounded on maximum-sized mazes.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1660"></a>
**L1660** — <code>    /// Each recent detour raises boldness, which weights exit progress more.</code>

Documentation fragment: “Each recent detour raises boldness, which weights exit progress more.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1661"></a>
**L1661** — <code>    bool begin_recovery(const Forecast&amp; forecast) {</code>

Search reachable alternative waypoints, allowing temporary movement away from the exit; exploration is bounded to 4,096 expansions and depth 32.

<a id="l1662"></a>
**L1662** — <code>        next_recovery_attempt = now + 400;</code>

Throttle recovery-goal searches to once per 400 ms.

<a id="l1663"></a>
**L1663** — <code>        while (!tried_recovery_goals.empty() &amp;&amp; tried_recovery_goals.front().second &lt;= now)</code>

Discard remembered goals whose time-to-live expired.

<a id="l1664"></a>
**L1664** — <code>            tried_recovery_goals.pop_front();</code>

Pop the oldest expired endpoint.

<a id="l1665"></a>
**L1665** — <code>        const double progress_weight = 1.5 + 1.0 * auto_boldness();</code>

Each recent detour raises how strongly recovery prefers cells closer to the exit.

<a id="l1666"></a>
**L1666** — <code>        std::vector&lt;int&gt; blocked;</code>

Short list of monster cells to avoid in the recovery search.

<a id="l1667"></a>
**L1667** — <code>        for (const auto&amp; m : monsters) blocked.push_back(m.cell);</code>

Fill it with every monster cell.

<a id="l1668"></a>
**L1668** — <code>        std::vector&lt;int&gt; distance(grid.size(), INF_DISTANCE), queue{player};</code>

Initialize a player-rooted BFS distance vector and queue.

<a id="l1669"></a>
**L1669** — <code>        distance[player] = 0;</code>

The player's current cell has recovery-search distance zero.

<a id="l1670"></a>
**L1670** — <code>        int best = -1;</code>

No useful recovery goal has been selected yet.

<a id="l1671"></a>
**L1671** — <code>        double best_score = -std::numeric_limits&lt;double&gt;::infinity();</code>

Begin candidate maximization at negative infinity.

<a id="l1672"></a>
**L1672** — <code>        for (std::size_t head = 0; head &lt; queue.size() &amp;&amp; head &lt; 4096; ++head) {</code>

Traverse at most 4,096 queued cells, bounding search cost on very large mazes.

<a id="l1673"></a>
**L1673** — <code>            int u = queue[head], d = distance[u];</code>

Read a candidate and its player-rooted distance.

<a id="l1674"></a>
**L1674** — <code>            if (d &gt; 0) {</code>

The starting cell itself is not considered an alternative waypoint.

<a id="l1675"></a>
**L1675** — <code>                Millis clearance = NEVER;</code>

Initialize earliest enemy clearance as effectively infinite.

<a id="l1676"></a>
**L1676** — <code>                for (std::size_t i = 0; i &lt; monsters.size(); ++i)</code>

Inspect all monsters' conservative arrival bounds at the candidate.

<a id="l1677"></a>
**L1677** — <code>                    clearance = std::min(clearance, earliest_arrival(forecast, i, u));</code>

Use the earliest of those arrivals as available clearance.

<a id="l1678"></a>
**L1678** — <code>                // A static occupancy gap alone is insufficient if a monster can</code>

Documentation fragment: “A static occupancy gap alone is insufficient if a monster can” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1679"></a>
**L1679** — <code>                // close it before arrival. The next action is checked separately.</code>

Documentation fragment: “close it before arrival. The next action is checked separately.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1680"></a>
**L1680** — <code>                if (d &gt;= 4 &amp;&amp; clearance &gt; d * AUTO_SOLVE_SPEED_MS + settings.monster_move_ms) {</code>

Require at least four player steps of displacement and a clearance exceeding travel time plus one monster action interval.

<a id="l1681"></a>
**L1681** — <code>                    double score = 2.0 * std::min(d, 16) + double(std::min&lt;Millis&gt;(clearance, 20 * settings.monster_move_ms)) / settings.monster_move_ms</code>

Reward travel into another area up to sixteen cells and longer clearance up to twenty monster periods.

<a id="l1682"></a>
**L1682** — <code>                                   - 4.0 * visit_heat(u) - 0.6 * dead_end_depth[u]</code>

Subtract accumulated visit heat and blind-branch depth penalties.

<a id="l1683"></a>
**L1683** — <code>                                   - progress_weight * ((*exit_distances)[u] - (*exit_distances)[player]);</code>

Penalize distance away from the exit using the boldness-adjusted weight.

<a id="l1684"></a>
**L1684** — <code>                    if (degrees[u] &gt;= 3) score += 6.0;</code>

Add six for a junction with at least three exits, favoring room to maneuver.

<a id="l1685"></a>
**L1685** — <code>                    if (u == entrance) score -= 12.0;</code>

Discourage retreat to the boundary entrance with a twelve-point penalty.

<a id="l1686"></a>
**L1686** — <code>                    for (const auto&amp; tried : tried_recovery_goals)</code>

Compare against recently tried recovery destinations.

<a id="l1687"></a>
**L1687** — <code>                        if (manhattan(u, tried.first) &lt;= 3) score -= 24.0;</code>

Subtract twenty-four near a tried goal within Manhattan distance three, preventing neighboring-goal oscillation.

<a id="l1688"></a>
**L1688** — <code>                    if (score &gt; best_score) { best_score = score; best = u; }</code>

Retain the highest-scoring eligible goal; ties preserve traversal order.

<a id="l1689"></a>
**L1689** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1690"></a>
**L1690** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1691"></a>
**L1691** — <code>            if (d &gt;= 32) continue;</code>

Do not expand beyond distance 32, although this depth can still be evaluated as a candidate.

<a id="l1692"></a>
**L1692** — <code>            for (int v : adjacency[u]) if (v &gt;= 0 &amp;&amp; distance[v] == INF_DISTANCE &amp;&amp; !is_reserved(blocked, v)) {</code>

Expand unvisited neighbors that are not monster cells.

<a id="l1693"></a>
**L1693** — <code>                if (d == 0) {</code>

For neighbors directly beside the player, apply an immediate conservative safety check.

<a id="l1694"></a>
**L1694** — <code>                    double risk;</code>

Allocate an output variable for the action risk.

<a id="l1695"></a>
**L1695** — <code>                    if (!evaluate_action(forecast, v, 0, true, risk)) continue;</code>

Reject an unsafe first step so a remote attractive goal cannot justify an immediately lethal opening move.

<a id="l1696"></a>
**L1696** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1697"></a>
**L1697** — <code>                distance[v] = d + 1; queue.push_back(v);</code>

Set shortest recovery distance and enqueue the accepted neighbor.

<a id="l1698"></a>
**L1698** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1699"></a>
**L1699** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1700"></a>
**L1700** — <code>        if (best &lt; 0) return false;</code>

No eligible goal means the tactical planner must continue with waiting/evasion fallback.

<a id="l1701"></a>
**L1701** — <code>        recovery_target = best;</code>

Commit the selected recovery cell across subsequent actions.

<a id="l1702"></a>
**L1702** — <code>        recovery_guidance = recovery_distance_field(best);</code>

Build its independent occupancy-aware reverse guidance field.

<a id="l1703"></a>
**L1703** — <code>        recovery_occupancy.clear();</code>

Clear the previous recovery occupancy record.

<a id="l1704"></a>
**L1704** — <code>        for (const auto&amp; m : monsters) recovery_occupancy.push_back(m.cell);</code>

Save current positions to detect future guidance invalidation.

<a id="l1705"></a>
**L1705** — <code>        recovery_best_distance = recovery_guidance[player];</code>

Initialize best remaining distance to the recovery goal.

<a id="l1706"></a>
**L1706** — <code>        recovery_progress_at = now; recovery_deadline = now + AUTO_RECOVERY_COMMIT_MS;</code>

Start recovery progress tracking and its four-second hard commitment limit.

<a id="l1707"></a>
**L1707** — <code>        detour_history.push_back(now);</code>

Record this detour for loop detection.

<a id="l1708"></a>
**L1708** — <code>        while (!detour_history.empty() &amp;&amp; now - detour_history.front() &gt;= AUTO_LOOP_WINDOW_MS) detour_history.pop_front();</code>

Drop detours older than the 20-second window.

<a id="l1709"></a>
**L1709** — <code>        log_event(std::string("Auto-solve: ") +</code>

Begin the recovery diagnostic and classify direction relative to the exit.

<a id="l1710"></a>
**L1710** — <code>                  ((*exit_distances)[best] &gt; (*exit_distances)[player] ? "BACKTRACK" : "DETOUR") +</code>

Call it BACKTRACK when the waypoint's terrain exit distance exceeds the player's, otherwise DETOUR.

<a id="l1711"></a>
**L1711** — <code>                  " committed toward " + cell_label(best) + " | " +</code>

Append the destination coordinate and route-length separator.

<a id="l1712"></a>
**L1712** — <code>                  std::to_string(recovery_best_distance) + " steps | Rechecking safety every action.");</code>

State waypoint distance and the fact that safety will be reconsidered for each action.

<a id="l1713"></a>
**L1713** — <code>        return true;</code>

Report that a recovery waypoint was successfully committed.

<a id="l1714"></a>
**L1714** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1715"></a>
**L1715** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1716"></a>
**L1716** — <code>    /// Keep a chosen escape goal across actions. Rebuild only when occupancy</code>

Documentation fragment: “Keep a chosen escape goal across actions. Rebuild only when occupancy” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1717"></a>
**L1717** — <code>    /// changes; retire goals that are reached, cut off, or no longer making progress.</code>

Documentation fragment: “changes; retire goals that are reached, cut off, or no longer making progress.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1718"></a>
**L1718** — <code>    void update_recovery() {</code>

Maintain an existing recovery commitment, reacting to progress, changed enemy occupancy, arrival, cutoff, or timeout.

<a id="l1719"></a>
**L1719** — <code>        if (recovery_target &lt; 0) return;</code>

Ordinary exit planning has no recovery state to update.

<a id="l1720"></a>
**L1720** — <code>        if (player == recovery_target) {</code>

Detect successful arrival at the committed waypoint.

<a id="l1721"></a>
**L1721** — <code>            finish_recovery("reached");</code>

Retire the reached goal and add it to the recently tried list.

<a id="l1722"></a>
**L1722** — <code>            // A retreat should create a chance to lose the scent, rather than</code>

Documentation fragment: “A retreat should create a chance to lose the scent, rather than” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1723"></a>
**L1723** — <code>            // immediately reversing and continuously re-alerting every monster.</code>

Documentation fragment: “immediately reversing and continuously re-alerting every monster.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1724"></a>
**L1724** — <code>            recovery_pause_until = now + settings.scent_ms + settings.monster_move_ms;</code>

Allow a post-retreat pause lasting one scent period plus one monster move interval; safety can still override waiting.

<a id="l1725"></a>
**L1725** — <code>            next_recovery_attempt = recovery_pause_until;</code>

Prevent another recovery search from immediately undoing that pause.

<a id="l1726"></a>
**L1726** — <code>            return;</code>

Stop processing the completed goal.

<a id="l1727"></a>
**L1727** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1728"></a>
**L1728** — <code>        std::vector&lt;int&gt; occupancy;</code>

Prepare the live occupancy snapshot.

<a id="l1729"></a>
**L1729** — <code>        for (const auto&amp; m : monsters) occupancy.push_back(m.cell);</code>

Collect current monster cells in stable order.

<a id="l1730"></a>
**L1730** — <code>        if (occupancy != recovery_occupancy) {</code>

A changed snapshot invalidates the recovery field.

<a id="l1731"></a>
**L1731** — <code>            recovery_occupancy = occupancy;</code>

Record the new snapshot.

<a id="l1732"></a>
**L1732** — <code>            recovery_guidance = recovery_distance_field(recovery_target);</code>

Recompute the route to the same committed target, rather than choosing a new target on every action.

<a id="l1733"></a>
**L1733** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1734"></a>
**L1734** — <code>        if (recovery_guidance[player] == INF_DISTANCE) { finish_recovery("cut off"); return; }</code>

Retire the target immediately if current occupancy cuts off every route to it.

<a id="l1735"></a>
**L1735** — <code>        if (recovery_guidance[player] &lt; recovery_best_distance) {</code>

Recognize a strictly shorter remaining recovery distance as progress.

<a id="l1736"></a>
**L1736** — <code>            recovery_best_distance = recovery_guidance[player]; recovery_progress_at = now;</code>

Update the best distance and the time of that improvement.

<a id="l1737"></a>
**L1737** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1738"></a>
**L1738** — <code>        if (now &gt;= recovery_deadline || now - recovery_progress_at &gt;= (2 * settings.scent_ms))</code>

End commitment after its hard deadline or after two scent periods without improvement.

<a id="l1739"></a>
**L1739** — <code>            finish_recovery("timed out; selecting another branch");</code>

Record timeout so a different neighborhood can be considered after cooldown.

<a id="l1740"></a>
**L1740** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1741"></a>
**L1741** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1742"></a>
**L1742** — <code>    /// Follow the committed global detour one verified action at a time. A safe</code>

Documentation fragment: “Follow the committed global detour one verified action at a time. A safe” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1743"></a>
**L1743** — <code>    /// retreat may temporarily increase exit distance. If no step toward the goal</code>

Documentation fragment: “retreat may temporarily increase exit distance. If no step toward the goal” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1744"></a>
**L1744** — <code>    /// is safe, the temporal planner remains free to wait or evade nearby threats.</code>

Documentation fragment: “is safe, the temporal planner remains free to wait or evade nearby threats.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1745"></a>
**L1745** — <code>    int recovery_step(const Forecast&amp; forecast) const {</code>

Select a safe adjacent step that makes progress toward the committed recovery goal, even when that step moves away from the maze exit.

<a id="l1746"></a>
**L1746** — <code>        int best = -1;</code>

No acceptable step has been found yet.

<a id="l1747"></a>
**L1747** — <code>        double best_score = std::numeric_limits&lt;double&gt;::infinity();</code>

Minimize immediate risk plus revisit cost.

<a id="l1748"></a>
**L1748** — <code>        for (int v : adjacency[player]) {</code>

Inspect each cardinal neighbor of the player's current cell.

<a id="l1749"></a>
**L1749** — <code>            if (v &lt; 0 || recovery_guidance[v] &gt;= recovery_guidance[player]) continue;</code>

Require a valid neighbor with strictly smaller recovery distance; sideways evasions are left to the temporal fallback.

<a id="l1750"></a>
**L1750** — <code>            double risk;</code>

Receive the candidate's soft-risk score.

<a id="l1751"></a>
**L1751** — <code>            if (!evaluate_action(forecast, v, 0, true, risk)) continue;</code>

Reject candidates violating the conservative first-interval safety rules.

<a id="l1752"></a>
**L1752** — <code>            double score = risk + 0.2 * visit_heat(v);</code>

Rank safe progress by risk plus 0.2 times time-decayed visit heat.

<a id="l1753"></a>
**L1753** — <code>            if (score &lt; best_score) { best_score = score; best = v; }</code>

Keep the lowest score, retaining deterministic adjacency order for exact ties.

<a id="l1754"></a>
**L1754** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1755"></a>
**L1755** — <code>        return best;</code>

Return a chosen cell or -1 to request tactical fallback.

<a id="l1756"></a>
**L1756** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1757"></a>
**L1757** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Beam planning, adaptive horizon, and complete-route overlay

<a id="l1758"></a>
**L1758** — <code>    /// Search (cell, action time) with a fixed beam and depth budget. Keep the</code>

Documentation fragment: “Search (cell, action time) with a fixed beam and depth budget. Keep the” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1759"></a>
**L1759** — <code>    /// deepest survivable frontier, then prefer lower risk, fewer repeats, and exit</code>

Documentation fragment: “deepest survivable frontier, then prefer lower risk, fewer repeats, and exit” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1760"></a>
**L1760** — <code>    /// progress. This bounded search is deliberately not an optimality guarantee.</code>

Documentation fragment: “progress. This bounded search is deliberately not an optimality guarantee.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1761"></a>
**L1761** — <code>    std::vector&lt;int&gt; beam_plan(const Forecast&amp; forecast, bool conservative, const std::vector&lt;int&gt;&amp; guidance) {</code>

Bounded time-expanded beam search over cell/action depth; accepts a choice of conservative first-step policy and global guidance field.

<a id="l1762"></a>
**L1762** — <code>        std::vector&lt;PlanNode&gt; tree{{player, -1, 0, 0, double(guidance[player])}};</code>

Create a root tree node for the player's current cell with no parent and zero accumulated action cost.

<a id="l1763"></a>
**L1763** — <code>        std::vector&lt;int&gt; frontier{0};</code>

The initial frontier contains only that root index.

<a id="l1764"></a>
**L1764** — <code>        int chosen = -1;</code>

No survivable non-root frontier or goal has been chosen yet.

<a id="l1765"></a>
**L1765** — <code>        const auto better = [](const PlanNode&amp; a, const PlanNode&amp; b) {</code>

Define deterministic ordering shared by candidate deduplication and beam truncation.

<a id="l1766"></a>
**L1766** — <code>            if (a.rank != b.rank) return a.rank &lt; b.rank;</code>

Lower rank, combining accumulated cost and remaining-distance guidance, is preferable.

<a id="l1767"></a>
**L1767** — <code>            if (a.cell != b.cell) return a.cell &lt; b.cell;</code>

Break rank ties by smaller cell index.

<a id="l1768"></a>
**L1768** — <code>            return a.parent &lt; b.parent;</code>

Break remaining ties by smaller parent index, ensuring stable reconstruction.

<a id="l1769"></a>
**L1769** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l1770"></a>
**L1770** — <code>        for (int depth = 0; depth &lt; forecast.horizon; ++depth) {</code>

Search to the forecast's horizon (12 or 18 actions).

<a id="l1771"></a>
**L1771** — <code>            if (++beam_epoch == 0) {</code>

Advance the deduplication epoch for this depth, so identical cells at different times remain distinct states.

<a id="l1772"></a>
**L1772** — <code>                std::fill(beam_seen.begin(), beam_seen.end(), 0); ++beam_epoch;</code>

Handle unsigned wrap by clearing stamps and resuming at a nonzero epoch.

<a id="l1773"></a>
**L1773** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1774"></a>
**L1774** — <code>            std::vector&lt;PlanNode&gt; candidates;</code>

Collect candidate states for the next depth.

<a id="l1775"></a>
**L1775** — <code>            candidates.reserve(frontier.size() * 5);</code>

Each frontier node has at most five actions: stay plus four cardinal moves.

<a id="l1776"></a>
**L1776** — <code>            for (int parent : frontier) {</code>

Expand every retained parent from the current frontier.

<a id="l1777"></a>
**L1777** — <code>                const PlanNode&amp; previous = tree[parent];</code>

Read the parent's full search state from the persistent tree.

<a id="l1778"></a>
**L1778** — <code>                std::array&lt;int, 5&gt; moves{previous.cell, -1, -1, -1, -1};</code>

Place waiting first in the candidate action array; initialize remaining slots as missing.

<a id="l1779"></a>
**L1779** — <code>                std::copy(adjacency[previous.cell].begin(), adjacency[previous.cell].end(), moves.begin() + 1);</code>

Fill the four movement slots with terrain neighbors.

<a id="l1780"></a>
**L1780** — <code>                for (int v : moves) {</code>

Evaluate each waiting/movement destination.

<a id="l1781"></a>
**L1781** — <code>                    if (v &lt; 0 || guidance[v] == INF_DISTANCE) continue;</code>

Reject absent cells and cells unreachable under the selected guidance field.

<a id="l1782"></a>
**L1782** — <code>                    double danger;</code>

Allocate the soft danger score returned by action evaluation.

<a id="l1783"></a>
**L1783** — <code>                    if (!evaluate_action(forecast, v, depth, conservative, danger)) continue;</code>

Reject collisions or conservative first-step reachability violations before assigning a ranking cost.

<a id="l1784"></a>
**L1784** — <code>                    int repeats = static_cast&lt;int&gt;(std::count(player_history.begin(), player_history.end(), v));</code>

Count occurrences in the player's bounded recent action history, including prior waits.

<a id="l1785"></a>
**L1785** — <code>                    double cost = previous.cost + 1.0 + 2.0 * danger + 0.1 * repeats + 0.5 * visit_heat(v);</code>

Accumulate base cost one, twice predicted danger, 0.1 per recent repeat, and 0.5 times decayed visit heat.

<a id="l1786"></a>
**L1786** — <code>                    if (v == previous.cell) cost += 0.35;</code>

Waiting adds a small 0.35 penalty; it remains available when safer than movement.

<a id="l1787"></a>
**L1787** — <code>                    if (previous.parent &gt;= 0 &amp;&amp; tree[previous.parent].cell == v) cost += 0.8;</code>

Immediate reversal to the grandparent cell adds 0.8, discouraging oscillation without forbidding backtracking.

<a id="l1788"></a>
**L1788** — <code>                    // Penalize entering deep branches only while a threat is nearby.</code>

Documentation fragment: “Penalize entering deep branches only while a threat is nearby.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1789"></a>
**L1789** — <code>                    if (danger &gt; 0) cost += std::min(dead_end_depth[v], 12) * 0.2;</code>

With nonzero danger, deeper blind branches become less attractive by up to 2.4 extra cost.

<a id="l1790"></a>
**L1790** — <code>                    // Prefer a junction over a blind branch when danger is comparable.</code>

Documentation fragment: “Prefer a junction over a blind branch when danger is comparable.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1791"></a>
**L1791** — <code>                    if (danger &gt; 0 &amp;&amp; degrees[v] &lt; 2 &amp;&amp; v != exit_cell) cost += 2.0;</code>

With danger, a non-exit cell with fewer than two neighbors receives another two-point trap penalty.

<a id="l1792"></a>
**L1792** — <code>                    PlanNode candidate{v, parent, depth + 1, cost, cost + guidance[v]};</code>

Construct the child node and rank it using cost plus guidance distance.

<a id="l1793"></a>
**L1793** — <code>                    if (beam_seen[v] == beam_epoch) {</code>

If this destination already has a candidate at this same depth, deduplicate it.

<a id="l1794"></a>
**L1794** — <code>                        auto&amp; old = candidates[beam_index[v]];</code>

Retrieve the previous candidate's slot directly through beam_index.

<a id="l1795"></a>
**L1795** — <code>                        if (better(candidate, old)) old = candidate;</code>

Keep the better-ranked predecessor for that cell/time state.

<a id="l1796"></a>
**L1796** — <code>                    } else {</code>

This cell has no candidate at the current beam depth yet, so create its first per-layer entry.

<a id="l1797"></a>
**L1797** — <code>                        beam_seen[v] = beam_epoch; beam_index[v] = static_cast&lt;int&gt;(candidates.size());</code>

Stamp a newly seen destination and record its index before appending.

<a id="l1798"></a>
**L1798** — <code>                        candidates.push_back(candidate);</code>

Append the new unique candidate.

<a id="l1799"></a>
**L1799** — <code>                    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1800"></a>
**L1800** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1801"></a>
**L1801** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1802"></a>
**L1802** — <code>            if (candidates.empty()) break;</code>

If no candidates survive, stop and retain the deepest earlier survivable choice if one exists.

<a id="l1803"></a>
**L1803** — <code>            // Preserve a feasible goal even if many low-cost partial paths fill the beam.</code>

Documentation fragment: “Preserve a feasible goal even if many low-cost partial paths fill the beam.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1804"></a>
**L1804** — <code>            auto goal = std::find_if(candidates.begin(), candidates.end(), [&amp;](const PlanNode&amp; n) {</code>

Search surviving candidates for an immediate exit goal before the beam is truncated.

<a id="l1805"></a>
**L1805** — <code>                return n.cell == exit_cell;</code>

A candidate cell equals the exit when the goal has been reached.

<a id="l1806"></a>
**L1806** — <code>            });</code>

Close the goal predicate and complete the search for a candidate already at the exit.

<a id="l1807"></a>
**L1807** — <code>            if (goal != candidates.end()) {</code>

Preserve a feasible goal even when it would otherwise fall outside the top 64 ranks.

<a id="l1808"></a>
**L1808** — <code>                chosen = static_cast&lt;int&gt;(tree.size()); tree.push_back(*goal); break;</code>

Append the goal to the reconstruction tree, choose it, and finish the search.

<a id="l1809"></a>
**L1809** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1810"></a>
**L1810** — <code>            int keep = std::min(AUTO_BEAM_WIDTH, static_cast&lt;int&gt;(candidates.size()));</code>

Retain no more than AUTO_BEAM_WIDTH=64 candidates.

<a id="l1811"></a>
**L1811** — <code>            std::partial_sort(candidates.begin(), candidates.begin() + keep, candidates.end(), better);</code>

Partially sort only the prefix that will survive, avoiding a full ordering of discarded candidates.

<a id="l1812"></a>
**L1812** — <code>            frontier.clear();</code>

Replace the old frontier with this depth's retained candidates.

<a id="l1813"></a>
**L1813** — <code>            for (int i = 0; i &lt; keep; ++i) {</code>

Append each retained candidate to the reconstruction tree.

<a id="l1814"></a>
**L1814** — <code>                frontier.push_back(static_cast&lt;int&gt;(tree.size())); tree.push_back(candidates[i]);</code>

Store its final tree index in the next frontier before appending the node.

<a id="l1815"></a>
**L1815** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1816"></a>
**L1816** — <code>            chosen = frontier.front();</code>

Choose the best state of the newest survivable depth; depth survival takes precedence over earlier shallower ranks.

<a id="l1817"></a>
**L1817** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1818"></a>
**L1818** — <code>        std::vector&lt;int&gt; result;</code>

Prepare the reconstructed sequence of cells.

<a id="l1819"></a>
**L1819** — <code>        for (int node = chosen; node &gt;= 0; node = tree[node].parent) result.push_back(tree[node].cell);</code>

Walk parent indices from chosen state to root; chosen=-1 yields an empty route.

<a id="l1820"></a>
**L1820** — <code>        std::reverse(result.begin(), result.end());</code>

Reverse to player-first action order.

<a id="l1821"></a>
**L1821** — <code>        return result;</code>

Return the bounded tactical route; this is not the full displayed route or an optimality proof.

<a id="l1822"></a>
**L1822** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1823"></a>
**L1823** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1824"></a>
**L1824** — <code>    /// Look further ahead while a threat is at mid range: close enough to shape the</code>

Documentation fragment: “Look further ahead while a threat is at mid range: close enough to shape the” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1825"></a>
**L1825** — <code>    /// route, far enough that the default window cannot see how it will arrive.</code>

Documentation fragment: “route, far enough that the default window cannot see how it will arrive.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1826"></a>
**L1826** — <code>    int choose_horizon() {</code>

Pick the lookahead depth from the nearest threat's distance.

<a id="l1827"></a>
**L1827** — <code>        auto from_player = local_field(player);</code>

Bounded distances from the player.

<a id="l1828"></a>
**L1828** — <code>        int nearest = THREAT_RADIUS + 1;</code>

Start beyond the radius.

<a id="l1829"></a>
**L1829** — <code>        for (const auto&amp; m : monsters) nearest = std::min(nearest, from_player-&gt;at(m.cell));</code>

Find the nearest monster by corridor distance.

<a id="l1830"></a>
**L1830** — <code>        return nearest &gt;= 10 &amp;&amp; nearest &lt;= 36 ? AUTO_HORIZON_MAX : AUTO_HORIZON;</code>

Use 18 actions when the nearest threat is 10 to 36 steps away; otherwise 12.

<a id="l1831"></a>
**L1831** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1832"></a>
**L1832** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1833"></a>
**L1833** — <code>public:</code>

Set following class members to public access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l1834"></a>
**L1834** — <code>    /// Show the complete current-to-exit route independently of tactical lookahead.</code>

Documentation fragment: “Show the complete current-to-exit route independently of tactical lookahead.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1835"></a>
**L1835** — <code>    /// Trace the chosen weighted successors. Amber indicates an occupied cell on</code>

Documents the correspondence between the weighted planner's stored decisions and the path shown on screen.

<a id="l1836"></a>
**L1836** — <code>    /// the displayed route. A fully blocked field falls back to the terrain route.</code>

Explains amber route occupancy and the use of a terrain route when the weighted field is disconnected.

<a id="l1837"></a>
**L1837** — <code>    void refresh_solution_overlay() {</code>

Trace weighted successors to the exit or follow terrain when blocked. Mark occupied routes amber.

<a id="l1838"></a>
**L1838** — <code>        if (!auto_solve || outcome != Outcome::Playing || !overlay_dirty) return;</code>

Skip work when assistance is off, play ended, or neither player nor monster positions invalidated the cache.

<a id="l1839"></a>
**L1839** — <code>        const auto&amp; guidance = route_guidance();</code>

Refresh weighted guidance before deciding which full path to display.

<a id="l1840"></a>
**L1840** — <code>        overlay_dirty = false;</code>

Consume the dirty flag before rebuilding.

<a id="l1841"></a>
**L1841** — <code>        std::vector&lt;Point&gt; route;</code>

Route cells for the overlay.

<a id="l1842"></a>
**L1842** — <code>        const bool guided = guide_distances[player] != INF_DISTANCE;</code>

Test whether the current player cell is reachable in the weighted exit field.

<a id="l1843"></a>
**L1843** — <code>        bool avoids_monsters = guided;</code>

Start the clean-route flag from reachability; walking the actual route checks its occupied cells as well.

<a id="l1844"></a>
**L1844** — <code>        int current = player;</code>

Begin at the actual current player position.

<a id="l1845"></a>
**L1845** — <code>        if (guidance[current] != INF_DISTANCE) {</code>

Only reconstruct if the selected guidance has a finite path to the exit.

<a id="l1846"></a>
**L1846** — <code>            route.push_back(point(current));</code>

Include the player cell as the route's first coordinate.

<a id="l1847"></a>
**L1847** — <code>            while (current != exit_cell) {</code>

Continue gradient descent all the way to the exit, with no twelve-step display limit.

<a id="l1848"></a>
**L1848** — <code>                int next = guided &amp;&amp; !monsters.empty() ? guide_next[current] : terrain_exit_step(current);</code>

Follow the recorded weighted successor with active monsters, otherwise use a downhill terrain step.

<a id="l1849"></a>
**L1849** — <code>                if (next &lt; 0) { route.clear(); break; }</code>

Defensively discard the route if an expected descending edge is missing.

<a id="l1850"></a>
**L1850** — <code>                if (avoids_monsters)</code>

While the route still looks monster-free...

<a id="l1851"></a>
**L1851** — <code>                    for (const auto&amp; m : monsters) if (m.cell == next) avoids_monsters = false;</code>

...mark it as crossing a monster if the next cell is occupied.

<a id="l1852"></a>
**L1852** — <code>                route.push_back(point(next)); current = next;</code>

Append the next coordinate and advance reconstruction.

<a id="l1853"></a>
**L1853** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1854"></a>
**L1854** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1855"></a>
**L1855** — <code>        if (route.empty()) avoids_monsters = false;</code>

An empty route never avoids monsters.

<a id="l1856"></a>
**L1856** — <code>        bool changed = route != solution_overlay || avoids_monsters != overlay_avoids_monsters;</code>

Detect either geometric changes or a change between monster-free and monster-crossing status.

<a id="l1857"></a>
**L1857** — <code>        if (!overlay_reported || avoids_monsters != overlay_avoids_monsters || (verbose_logs &amp;&amp; changed)) {</code>

Log first availability and status changes; verbose mode additionally logs every changed route.

<a id="l1858"></a>
**L1858** — <code>            if (route.empty()) log_event("Auto-solve: No terrain route to the exit.");</code>

An empty reconstructed path is reported as no terrain route, distinct from temporary monster blockage.

<a id="l1859"></a>
**L1859** — <code>            else if (avoids_monsters)</code>

Choose the clear-route diagnostic when current monster cells were avoided.

<a id="l1860"></a>
**L1860** — <code>                log_event("Auto-solve: Full path to exit updated | " + std::to_string(route.size() - 1) +</code>

Begin a message with total route edges rather than number of stored endpoints.

<a id="l1861"></a>
**L1861** — <code>                          " steps | Current monster cells avoided.");</code>

Clarify that safety concerns current occupancy, not guaranteed future positions.

<a id="l1862"></a>
**L1862** — <code>            else</code>

The remaining branch covers routes that cross a monster: a distant monster's soft cost was cheaper than any detour, or near/parked monsters block every route and terrain guidance is shown.

<a id="l1863"></a>
**L1863** — <code>                log_event("Auto-solve: Path crosses monster(s) | Full route shown in AMBER | Evasion active.");</code>

Log that the route crosses at least one monster and is drawn in amber while evasion stays active.

<a id="l1864"></a>
**L1864** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1865"></a>
**L1865** — <code>        solution_overlay = std::move(route);</code>

Move the completed temporary vector into the public overlay.

<a id="l1866"></a>
**L1866** — <code>        overlay_avoids_monsters = avoids_monsters;</code>

Store its current-occupancy safety classification for coloring and future change detection.

<a id="l1867"></a>
**L1867** — <code>        overlay_reported = true;</code>

Mark the current status as already reported.

<a id="l1868"></a>
**L1868** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1869"></a>
**L1869** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Automatic action selection and execution

<a id="l1870"></a>
**L1870** — <code>    /// Plan a short safe route, including waits. If conservative avoidance has no</code>

Documentation fragment: “Plan a short safe route, including waits. If conservative avoidance has no” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1871"></a>
**L1871** — <code>    /// action, relax uncertain threats; if every prediction fails, maximize time</code>

Documentation fragment: “action, relax uncertain threats; if every prediction fails, maximize time” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1872"></a>
**L1872** — <code>    /// before contact among currently unoccupied neighbors. No survival guarantee.</code>

Documentation fragment: “before contact among currently unoccupied neighbors. No survival guarantee.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1873"></a>
**L1873** — <code>    std::vector&lt;int&gt; plan_auto_route() {</code>

Top-level automatic policy: no-threat shortest path, loop-aware scent wait (never beside a monster), committed recovery, conservative beam, relaxed beam, then emergency clearance.

<a id="l1874"></a>
**L1874** — <code>        planned_wait_for_idle = false;</code>

Clear the intentional-scent-wait marker before deciding the next action.

<a id="l1875"></a>
**L1875** — <code>        if (monsters.empty()) {</code>

Use an exact shortest-path special case when there are no monsters.

<a id="l1876"></a>
**L1876** — <code>            // Without threats, a reverse BFS gradient gives exact shortest movement.</code>

Documentation fragment: “Without threats, a reverse BFS gradient gives exact shortest movement.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1877"></a>
**L1877** — <code>            std::vector&lt;int&gt; route{player};</code>

Start a route with the player's current cell.

<a id="l1878"></a>
**L1878** — <code>            int u = player;</code>

Track the cell being extended along the exit-distance gradient.

<a id="l1879"></a>
**L1879** — <code>            for (int step = 0; step &lt; AUTO_HORIZON &amp;&amp; u != exit_cell; ++step) {</code>

Extend up to twelve actions or until the exit is reached.

<a id="l1880"></a>
**L1880** — <code>                int best = terrain_exit_step(u);</code>

Look up the next step along the unit-cost exit route when examining terrain progress in the solver.

<a id="l1881"></a>
**L1881** — <code>                if (best &lt; 0) break;</code>

Stop tracing when the current cell has no lower-distance neighbor.

<a id="l1882"></a>
**L1882** — <code>                route.push_back(best); u = best;</code>

Append the chosen step and continue from it.

<a id="l1883"></a>
**L1883** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1884"></a>
**L1884** — <code>            return route;</code>

Return the no-threat route without invoking risk forecasting or recovery heuristics.

<a id="l1885"></a>
**L1885** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1886"></a>
**L1886** — <code>        // Loop detection: several detours in a short window mean waiting is not</code>

Documentation fragment: “Loop detection: several detours in a short window mean waiting is not” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1887"></a>
**L1887** — <code>        // working, so wait less (and begin_recovery favors exit progress more).</code>

Documentation fragment: “working, so wait less (and begin_recovery favors exit progress more).” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1888"></a>
**L1888** — <code>        // An all-out "press on" mode was measured and rejected: it turned</code>

Documentation fragment: “An all-out "press on" mode was measured and rejected: it turned” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1889"></a>
**L1889** — <code>        // timeouts into captures rather than escapes.</code>

Documentation fragment: “timeouts into captures rather than escapes.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1890"></a>
**L1890** — <code>        const int boldness = auto_boldness();</code>

Loop detection: boldness from recent detours.

<a id="l1891"></a>
**L1891** — <code>        if (boldness &gt; 0 &amp;&amp; boldness != last_reported_boldness)</code>

Log only when boldness changes to a nonzero value.

<a id="l1892"></a>
**L1892** — <code>            log_event("Auto-solve: " + std::to_string(boldness + 1) + " detours in " +</code>

Report the number of recent detours...

<a id="l1893"></a>
**L1893** — <code>                      std::to_string(AUTO_LOOP_WINDOW_MS / 1000) + " s | Boldness " + std::to_string(boldness) +</code>

...the window length and boldness level...

<a id="l1894"></a>
**L1894** — <code>                      " | Shorter waits, detours favor exit progress.");</code>

...and the consequences.

<a id="l1895"></a>
**L1895** — <code>        last_reported_boldness = boldness;</code>

Remember what was reported.

<a id="l1896"></a>
**L1896** — <code>        const Millis wait_budget = std::max&lt;Millis&gt;(settings.scent_ms,</code>

Wait budget for scent loss: twice the scent duration...

<a id="l1897"></a>
**L1897** — <code>                                                    2 * settings.scent_ms - boldness * settings.scent_ms / 2);</code>

...minus half a scent per boldness level, never below one scent duration.

<a id="l1898"></a>
**L1898** — <code>        Forecast forecast = forecast_monsters(choose_horizon());</code>

Forecast to the lookahead chosen from the nearest threat's distance.

<a id="l1899"></a>
**L1899** — <code>        const auto&amp; guidance = route_guidance();</code>

Obtain whole-maze weighted exit guidance under the current near/far monster classification.

<a id="l1900"></a>
**L1900** — <code>        bool blocked = guide_distances[player] == INF_DISTANCE;</code>

Determine whether near or parked monsters block every exit route.

<a id="l1901"></a>
**L1901** — <code>        if (blocked) {</code>

Track continuous blockage as its own state.

<a id="l1902"></a>
**L1902** — <code>            clear_route_since = NEVER;</code>

A blocked snapshot interrupts any streak of clear routing.

<a id="l1903"></a>
**L1903** — <code>            if (blocked_since == NEVER) blocked_since = now;</code>

Set the blockage origin only once, preventing each auto tick from restarting its waiting allowance.

<a id="l1904"></a>
**L1904** — <code>        } else {</code>

When a route past near and parked monsters exists, track how long the opening persists before resetting the blocked-wait budget.

<a id="l1905"></a>
**L1905** — <code>            if (clear_route_since == NEVER) clear_route_since = now;</code>

When clear, begin a clear-route streak only on its first clear action.

<a id="l1906"></a>
**L1906** — <code>            // Brief gaps at a bottleneck do not repeatedly grant a fresh wait budget.</code>

Documentation fragment: “Brief gaps at a bottleneck do not repeatedly grant a fresh wait budget.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1907"></a>
**L1907** — <code>            if (now - clear_route_since &gt;= settings.scent_ms) blocked_since = NEVER;</code>

Clear the old blockage budget only after the route remains open for a full scent interval, resisting flickering bottleneck gaps.

<a id="l1908"></a>
**L1908** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1909"></a>
**L1909** — <code>        update_recovery();</code>

Maintain or retire the existing recovery waypoint before selecting another action.

<a id="l1910"></a>
**L1910** — <code>        if (recovery_target &gt;= 0 &amp;&amp; !blocked &amp;&amp; now - clear_route_since &gt;= DIRECTOR_TICK_MS) {</code>

Consider abandoning recovery after at least 200 ms of open exit routing.

<a id="l1911"></a>
**L1911** — <code>            for (int v : adjacency[player]) {</code>

Look for a safe first step along the restored exit gradient.

<a id="l1912"></a>
**L1912** — <code>                if (v &lt; 0 || guidance[v] &gt;= guidance[player]) continue;</code>

Any neighbor lower on the guidance field counts as an exit step.

<a id="l1913"></a>
**L1913** — <code>                double risk;</code>

Receive that step's risk estimate.

<a id="l1914"></a>
**L1914** — <code>                if (evaluate_action(forecast, v, 0, true, risk) &amp;&amp; risk &lt; 1.0) {</code>

Require conservative feasibility and risk below one before canceling recovery.

<a id="l1915"></a>
**L1915** — <code>                    finish_recovery("cancelled; exit route reopened");</code>

Retire the detour with an explicit reopened-route reason.

<a id="l1916"></a>
**L1916** — <code>                    break;</code>

One safe exit-directed neighbor is enough to cancel the commitment.

<a id="l1917"></a>
**L1917** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1918"></a>
**L1918** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1919"></a>
**L1919** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1920"></a>
**L1920** — <code>        // Stand still long enough to shed the movement scent, but never restart</code>

Documentation fragment: “Stand still long enough to shed the movement scent, but never restart” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1921"></a>
**L1921** — <code>        // this allowance on each auto tick. Imminent danger still overrides waiting.</code>

Documentation fragment: “this allowance on each auto tick. Imminent danger still overrides waiting.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1922"></a>
**L1922** — <code>        // Never idle for scent loss while a monster is within one step of arrival.</code>

Documentation fragment: “Never idle for scent loss while a monster is within one step of arrival.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1923"></a>
**L1923** — <code>        Millis nearest_arrival = NEVER;</code>

Earliest time any monster could reach the player's cell.

<a id="l1924"></a>
**L1924** — <code>        for (std::size_t i = 0; i &lt; monsters.size(); ++i)</code>

Check every monster.

<a id="l1925"></a>
**L1925** — <code>            nearest_arrival = std::min(nearest_arrival, earliest_arrival(forecast, i, player));</code>

Keep the minimum.

<a id="l1926"></a>
**L1926** — <code>        if (blocked &amp;&amp; recovery_target &lt; 0 &amp;&amp;</code>

Waiting for scent loss is considered only while near or parked monsters block every route and no recovery waypoint is active.

<a id="l1927"></a>
**L1927** — <code>            nearest_arrival &gt; AUTO_SOLVE_SPEED_MS + settings.monster_move_ms &amp;&amp;</code>

Only wait if no monster can arrive within one action plus one monster step...

<a id="l1928"></a>
**L1928** — <code>            (now - blocked_since &lt; wait_budget || now &lt; recovery_pause_until)) {</code>

...while the wait budget lasts or a post-retreat pause is active.

<a id="l1929"></a>
**L1929** — <code>            double risk = 0;</code>

Prepare a risk output for staying at the current cell.

<a id="l1930"></a>
**L1930** — <code>            if (evaluate_action(forecast, player, 0, true, risk)) {</code>

Stay only if conservative immediate safety allows it; imminent danger overrides scent-loss waiting.

<a id="l1931"></a>
**L1931** — <code>                planned_wait_for_idle = true;</code>

Mark this as an intentional scent-loss action for logging.

<a id="l1932"></a>
**L1932** — <code>                return {player, player};</code>

Return a two-cell route containing the same position twice, encoding one wait.

<a id="l1933"></a>
**L1933** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1934"></a>
**L1934** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1935"></a>
**L1935** — <code>        bool stalled = now - navigation_progress_at &gt;= AUTO_STALL_MS;</code>

Recognize 2,400 ms without novel or best-ever exit progress as a navigation stall.

<a id="l1936"></a>
**L1936** — <code>        if (recovery_target &lt; 0 &amp;&amp; now &gt;= next_recovery_attempt &amp;&amp;</code>

Search for a recovery goal only when none is active and the 400 ms retry cooldown has elapsed.

<a id="l1937"></a>
**L1937** — <code>            ((blocked &amp;&amp; now - blocked_since &gt;= wait_budget) || stalled))</code>

Start recovery after the boldness-adjusted wait budget, or when navigation has stalled.

<a id="l1938"></a>
**L1938** — <code>            begin_recovery(forecast);</code>

Try committing to a safer reachable alternative branch, including retreat.

<a id="l1939"></a>
**L1939** — <code>        if (recovery_target &gt;= 0) {</code>

If recovery exists, first attempt direct safe progress toward that stable goal.

<a id="l1940"></a>
**L1940** — <code>            int next = recovery_step(forecast);</code>

Get the best conservative recovery-gradient neighbor.

<a id="l1941"></a>
**L1941** — <code>            if (next &gt;= 0) return {player, next};</code>

Execute that one step immediately when available.

<a id="l1942"></a>
**L1942** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1943"></a>
**L1943** — <code>        const auto&amp; tactical_guidance = recovery_target &gt;= 0 ? recovery_guidance : guidance;</code>

Otherwise choose recovery guidance while committed, or ordinary exit guidance when not.

<a id="l1944"></a>
**L1944** — <code>        auto route = beam_plan(forecast, true, tactical_guidance);</code>

Run the first beam pass with conservative first-interval reachability exclusions.

<a id="l1945"></a>
**L1945** — <code>        if (route.size() &gt; 1) return route;</code>

A route with root plus at least one action is executable.

<a id="l1946"></a>
**L1946** — <code>        route = beam_plan(forecast, false, tactical_guidance);</code>

If necessary, run a second pass relaxing uncertain first-step threats while retaining predicted collision checks.

<a id="l1947"></a>
**L1947** — <code>        if (route.size() &gt; 1) return route;</code>

Accept the relaxed route if it contains an action.

<a id="l1948"></a>
**L1948** — <code>        int best = player;</code>

Emergency fallback defaults to remaining at the current cell.

<a id="l1949"></a>
**L1949** — <code>        double best_score = -std::numeric_limits&lt;double&gt;::infinity();</code>

Maximize time clearance rather than minimize beam cost in this last resort.

<a id="l1950"></a>
**L1950** — <code>        std::array&lt;int, 5&gt; candidates{player, -1, -1, -1, -1};</code>

Create stay plus four-neighbor candidate slots.

<a id="l1951"></a>
**L1951** — <code>        std::copy(adjacency[player].begin(), adjacency[player].end(), candidates.begin() + 1);</code>

Fill the movement slots from cached adjacency.

<a id="l1952"></a>
**L1952** — <code>        for (int v : candidates) {</code>

Evaluate every candidate independently.

<a id="l1953"></a>
**L1953** — <code>            if (v &lt; 0) continue;</code>

Ignore nonexistent terrain neighbors.

<a id="l1954"></a>
**L1954** — <code>            bool occupied = false;</code>

Track actual live occupancy as a hard prohibition.

<a id="l1955"></a>
**L1955** — <code>            Millis margin = NEVER;</code>

Initialize the nearest possible enemy arrival as infinite.

<a id="l1956"></a>
**L1956** — <code>            for (std::size_t i = 0; i &lt; monsters.size(); ++i) {</code>

Compare against each monster.

<a id="l1957"></a>
**L1957** — <code>                if (monsters[i].cell == v) occupied = true;</code>

Mark a candidate occupied if any enemy is currently there.

<a id="l1958"></a>
**L1958** — <code>                margin = std::min(margin, earliest_arrival(forecast, i, v));</code>

Use the earliest conservative arrival as this candidate's margin.

<a id="l1959"></a>
**L1959** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1960"></a>
**L1960** — <code>            if (occupied) continue;</code>

Never intentionally choose a currently occupied emergency destination.

<a id="l1961"></a>
**L1961** — <code>            double score = std::min&lt;Millis&gt;(margin, 2000) - (*exit_distances)[v] * 0.3 + degrees[v] * 2;</code>

Reward clearance capped at 2,000 ms and room to maneuver; modestly penalize distance from exit.

<a id="l1962"></a>
**L1962** — <code>            if (v == exit_cell) score += 10000;</code>

A currently unoccupied exit gets a decisive bonus because arrival wins immediately after collision checks.

<a id="l1963"></a>
**L1963** — <code>            if (score &gt; best_score) { best_score = score; best = v; }</code>

Keep the highest-scoring remaining emergency choice.

<a id="l1964"></a>
**L1964** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1965"></a>
**L1965** — <code>        return {player, best};</code>

Return root and emergency destination, including a wait if no better movement is available; survival is not guaranteed.

<a id="l1966"></a>
**L1966** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1967"></a>
**L1967** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1968"></a>
**L1968** — <code>    /// Execute one tactical action, then independently refresh the full exit route.</code>

Documentation fragment: “Execute one tactical action, then independently refresh the full exit route.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1969"></a>
**L1969** — <code>    void auto_step() {</code>

Execute one tactical action and let the common movement transaction update state, scent, outcomes, and the complete overlay.

<a id="l1970"></a>
**L1970** — <code>        if (!auto_solve || outcome != Outcome::Playing) return;</code>

Ignore calls when assistance is disabled or the run is terminal.

<a id="l1971"></a>
**L1971** — <code>        auto route = plan_auto_route();</code>

Replan from current state rather than blindly following a stale route.

<a id="l1972"></a>
**L1972** — <code>        if (planned_wait_for_idle &amp;&amp; !last_auto_wait)</code>

Explain the beginning of an intentional scent-loss wait, suppressing repeated identical messages.

<a id="l1973"></a>
**L1973** — <code>            log_event("Auto-solve: All exit routes blocked; standing still so monsters lose the movement hint. Bounded wait; then try a detour or backtrack.");</code>

The message explicitly describes bounded waiting followed by detour/backtracking, matching the policy above.

<a id="l1974"></a>
**L1974** — <code>        if (route.size() &lt; 2) { commit_player_move(player, true); return; }</code>

If planning yields no usable next action, commit a legal wait and finish.

<a id="l1975"></a>
**L1975** — <code>        int next = route[1];</code>

Use only the first planned action; future actions are reconsidered at the next 40 ms tick.

<a id="l1976"></a>
**L1976** — <code>        if (next != player &amp;&amp; !solution_overlay.empty() &amp;&amp; solution_overlay.size() &gt; 1 &amp;&amp;</code>

Check whether an actual movement differs from a nonempty displayed full-path suggestion.

<a id="l1977"></a>
**L1977** — <code>            point(next) != solution_overlay[1]) {</code>

Compare the chosen next cell to the overlay's second coordinate.

<a id="l1978"></a>
**L1978** — <code>            log_event("Auto-solve: EVADING toward " + cell_label(next) + "; replanning full exit path.");</code>

Log the local evasion and upcoming full-route refresh when tactical and visual choices differ.

<a id="l1979"></a>
**L1979** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1980"></a>
**L1980** — <code>        commit_player_move(next, true);</code>

Commit the selected step or wait through the common authoritative rules.

<a id="l1981"></a>
**L1981** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1982"></a>
**L1982** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l1983"></a>
**L1983** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Simulation CLI and parsing

<a id="l1984"></a>
**L1984** — <code>/// Describe the current result consistently for graphical and headless sessions.</code>

Documentation fragment: “Describe the current result consistently for graphical and headless sessions.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1985"></a>
**L1985** — <code>const char* outcome_name(Outcome outcome) {</code>

Map the outcome enum to consistent labels shared by HUD-adjacent diagnostics and simulation output.

<a id="l1986"></a>
**L1986** — <code>    switch (outcome) {</code>

Select by terminal/active state.

<a id="l1987"></a>
**L1987** — <code>        case Outcome::Won: return "ESCAPED";</code>

Successful exit is named ESCAPED.

<a id="l1988"></a>
**L1988** — <code>        case Outcome::Caught: return "CAUGHT";</code>

Enemy collision is named CAUGHT.

<a id="l1989"></a>
**L1989** — <code>        default: return "RUNNING";</code>

Remaining cases are presented as RUNNING.

<a id="l1990"></a>
**L1990** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1991"></a>
**L1991** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l1992"></a>
**L1992** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l1993"></a>
**L1993** — <code>/// Parse a nonnegative integer completely, rejecting signs and trailing characters.</code>

Documentation fragment: “Parse a nonnegative integer completely, rejecting signs and trailing characters.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l1994"></a>
**L1994** — <code>std::uint64_t parse_number(const std::string&amp; text, std::uint64_t maximum) {</code>

Parse an entire nonnegative decimal CLI argument into uint64, with a caller-selected maximum.

<a id="l1995"></a>
**L1995** — <code>    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)</code>

Reject empty strings, signs, whitespace, decimal points, and any non-digit before conversion.

<a id="l1996"></a>
**L1996** — <code>        throw std::runtime_error("Expected an unsigned integer: " + text);</code>

Report the offending token in a readable numeric-format error.

<a id="l1997"></a>
**L1997** — <code>    std::size_t consumed = 0;</code>

Track the count of characters consumed by the library conversion.

<a id="l1998"></a>
**L1998** — <code>    unsigned long long value = 0;</code>

Parsed value, initialized before the conversion attempt.

<a id="l1999"></a>
**L1999** — <code>    try { value = std::stoull(text, &amp;consumed); }</code>

Convert the validated digit string.

<a id="l2000"></a>
**L2000** — <code>    catch (const std::out_of_range&amp;) { throw std::runtime_error("Number out of range: " + text); }</code>

A value too large for unsigned long long becomes a readable range error instead of the bare "stoull" message.

<a id="l2001"></a>
**L2001** — <code>    if (consumed != text.size() || value &gt; maximum) throw std::runtime_error("Number out of range: " + text);</code>

Reject incomplete conversion or a value exceeding the argument-specific limit.

<a id="l2002"></a>
**L2002** — <code>    return value;</code>

Return the validated unsigned integer.

<a id="l2003"></a>
**L2003** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2004"></a>
**L2004** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2005"></a>
**L2005** — <code>/// Run the exact game simulation without graphics for reproducible AI experiments.</code>

Documentation fragment: “Run the exact game simulation without graphics for reproducible AI experiments.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2006"></a>
**L2006** — <code>int run_simulation(int argc, char** argv) {</code>

Headless CLI entry point using exactly the same Maze implementation; no OpenGL context or desktop is needed.

<a id="l2007"></a>
**L2007** — <code>    if (argc &lt; 6)</code>

The simulation invocation requires executable, --simulate, rows, columns, monster count, and seed.

<a id="l2008"></a>
**L2008** — <code>        throw std::runtime_error("Usage: --simulate ROWS COLS MONSTERS SEED [LIMIT_MS] [--difficulty low|medium|high] [--quiet] [--verbose]");</code>

Provide the complete positional/optional syntax when required arguments are missing.

<a id="l2009"></a>
**L2009** — <code>    int rows = static_cast&lt;int&gt;(parse_number(argv[2], MAX_MAZE_DIMENSION));</code>

Read rows with the global 501 upper bound; the Maze constructor enforces the lower bound.

<a id="l2010"></a>
**L2010** — <code>    int cols = static_cast&lt;int&gt;(parse_number(argv[3], MAX_MAZE_DIMENSION));</code>

Read columns with the same upper bound.

<a id="l2011"></a>
**L2011** — <code>    int count = static_cast&lt;int&gt;(parse_number(argv[4], MAX_MONSTERS));</code>

Read count with the ten-monster bound.

<a id="l2012"></a>
**L2012** — <code>    auto seed = static_cast&lt;std::uint32_t&gt;(parse_number(argv[5], std::numeric_limits&lt;std::uint32_t&gt;::max()));</code>

Accept any seed in the full uint32 range, including zero.

<a id="l2013"></a>
**L2013** — <code>    Millis limit = 120000;</code>

Default simulation deadline is 120,000 ms, or two simulated minutes.

<a id="l2014"></a>
**L2014** — <code>    Difficulty difficulty = Difficulty::Medium;</code>

Default profile is Medium when --difficulty is absent.

<a id="l2015"></a>
**L2015** — <code>    bool quiet = false, verbose = false, limit_set = false;</code>

Track output modes and whether an optional numeric time limit was already supplied.

<a id="l2016"></a>
**L2016** — <code>    for (int i = 6; i &lt; argc; ++i) {</code>

Scan optional arguments after the four required simulation values.

<a id="l2017"></a>
**L2017** — <code>        std::string arg = argv[i];</code>

Wrap the current argument as a string for comparisons.

<a id="l2018"></a>
**L2018** — <code>        if (arg == "--quiet") quiet = true;</code>

--quiet suppresses detailed event/configuration output, but the final experiment summary is still printed.

<a id="l2019"></a>
**L2019** — <code>        else if (arg == "--verbose") verbose = true;</code>

--verbose enables additional per-step diagnostics.

<a id="l2020"></a>
**L2020** — <code>        else if (arg == "--difficulty") {</code>

Handle an explicit difficulty option.

<a id="l2021"></a>
**L2021** — <code>            if (i + 1 &gt;= argc) throw std::runtime_error("--difficulty requires low, medium, or high.");</code>

Reject a missing value after --difficulty.

<a id="l2022"></a>
**L2022** — <code>            difficulty = parse_difficulty(argv[++i]);</code>

Consume and normalize the following difficulty token.

<a id="l2023"></a>
**L2023** — <code>        } else if (!limit_set &amp;&amp; arg.find_first_not_of("0123456789") == std::string::npos) {</code>

Accept at most one all-decimal positional time-limit argument among the options.

<a id="l2024"></a>
**L2024** — <code>            limit = static_cast&lt;Millis&gt;(parse_number(arg, 3600000)); limit_set = true;</code>

Cap the requested limit at 3,600,000 ms and mark it supplied.

<a id="l2025"></a>
**L2025** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2026"></a>
**L2026** — <code>        else throw std::runtime_error("Unrecognized simulation argument: " + arg);</code>

Reject unknown flags, duplicate numeric limits, and malformed optional values.

<a id="l2027"></a>
**L2027** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2028"></a>
**L2028** — <code>    Maze maze(rows, cols, seed, quiet ? nullptr : &amp;std::cout, difficulty);</code>

Construct the simulation with chosen dimensions/seed/profile and a null log pointer in quiet mode.

<a id="l2029"></a>
**L2029** — <code>    maze.verbose_logs = verbose;</code>

Set verbose logging before generation begins.

<a id="l2030"></a>
**L2030** — <code>    maze.generate(count);</code>

Generate and validate terrain, then spawn monsters.

<a id="l2031"></a>
**L2031** — <code>    if (!quiet) std::cout &lt;&lt; maze.configuration_summary() &lt;&lt; '\n';</code>

Print actual normalized setup when not quiet.

<a id="l2032"></a>
**L2032** — <code>    maze.set_auto_solve(true);</code>

Enable auto-solving and start its elapsed gameplay timer at simulation time zero.

<a id="l2033"></a>
**L2033** — <code>    auto start = std::chrono::steady_clock::now();</code>

Start a monotonic host-time measurement for computation cost.

<a id="l2034"></a>
**L2034** — <code>    maze.advance_to(limit);</code>

Advance directly to the simulation deadline, processing intermediate events in chronological order.

<a id="l2035"></a>
**L2035** — <code>    double compute_ms = std::chrono::duration&lt;double, std::milli&gt;(std::chrono::steady_clock::now() - start).count();</code>

Measure elapsed host milliseconds; this benchmark value varies across executions and is excluded from deterministic state comparisons.

<a id="l2036"></a>
**L2036** — <code>    std::cout &lt;&lt; MAZERUNNER_TITLE " | difficulty=" &lt;&lt; maze.settings.name &lt;&lt; " | seed=" &lt;&lt; seed &lt;&lt; " | " &lt;&lt; maze.rows &lt;&lt; 'x' &lt;&lt; maze.cols</code>

Print the one-line simulation summary starting with the shared title.

<a id="l2037"></a>
**L2037** — <code>              &lt;&lt; " | monsters=" &lt;&lt; maze.monsters.size() &lt;&lt; " | " &lt;&lt; outcome_name(maze.outcome)</code>

Append spawned count and current outcome; RUNNING means the time limit expired without a terminal result.

<a id="l2038"></a>
**L2038** — <code>              &lt;&lt; " | simulation_ms=" &lt;&lt; maze.elapsed_ms &lt;&lt; " | auto_steps=" &lt;&lt; maze.automatic_steps</code>

Append elapsed simulation time and actual auto displacement count.

<a id="l2039"></a>
**L2039** — <code>              &lt;&lt; " | compute_ms=" &lt;&lt; std::fixed &lt;&lt; std::setprecision(2) &lt;&lt; compute_ms &lt;&lt; '\n';</code>

Append measured host compute time with two decimals and terminate the record.

<a id="l2040"></a>
**L2040** — <code>    // A caught player is a valid experiment result, not a process failure.</code>

Documentation fragment: “A caught player is a valid experiment result, not a process failure.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2041"></a>
**L2041** — <code>    return 0;</code>

Return process success even when the player was caught or the time budget ended; those are valid experiment outcomes.

<a id="l2042"></a>
**L2042** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2043"></a>
**L2043** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Vertex types and embedded artwork

<a id="l2044"></a>
**L2044** — <code>// Presentation geometry uses world coordinates (column, row, height). It never</code>

Documentation fragment: “Presentation geometry uses world coordinates (column, row, height). It never” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2045"></a>
**L2045** — <code>// consumes the simulation RNG or changes gameplay state.</code>

Documentation fragment: “consumes the simulation RNG or changes gameplay state.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2046"></a>
**L2046** — <code>struct Vertex { float x, y, z, r, g, b; };</code>

GPU vertex contains world position and RGB color as six interleaved floats; the shader layout uses this stride.

<a id="l2047"></a>
**L2047** — <code>struct RGB { float r, g, b; };</code>

Simple material/color record using normalized floating-point components.

<a id="l2048"></a>
**L2048** — <code>struct Vec3 { float x, y, z; };</code>

Three-dimensional geometry point; x is column, y is row, and z is vertical height.

<a id="l2049"></a>
**L2049** — <code>constexpr float WALL_HEIGHT = 0.38f;</code>

Walls rise 0.38 grid units above the floor plane.

<a id="l2050"></a>
**L2050** — <code>constexpr float VIEW_COS = 0.906307787f, VIEW_SIN = 0.422618262f;</code>

Cosine and sine of the fixed 25-degree orthographic viewing tilt; projection and billboard construction must use compatible values.

<a id="l2051"></a>
**L2051** — <code>using PixelSprite = std::array&lt;std::string_view, 20&gt;;</code>

Each sprite is exactly twenty rows represented as string_views; a compile-time check later requires each row to contain twenty palette characters.

<a id="l2052"></a>
**L2052** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2053"></a>
**L2053** — <code>// Original twenty-pixel silhouettes are embedded so no texture files are needed.</code>

Documentation fragment: “Original twenty-pixel silhouettes are embedded so no texture files are needed.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2054"></a>
**L2054** — <code>constexpr PixelSprite KNIGHT_SPRITE{{</code>

Begin KNIGHT_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2055"></a>
**L2055** — <code>    "........RRRR........", ".......RRrrRR.......",</code>

KNIGHT_SPRITE row 0 (top-based): 4 opaque pixels spanning columns 8–11, palette counts R=4; row 1 (top-based): 6 opaque pixels spanning columns 7–12, palette counts R=4, r=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2056"></a>
**L2056** — <code>    "........SSSS........", ".......SLLLLS.......",</code>

KNIGHT_SPRITE row 2 (top-based): 4 opaque pixels spanning columns 8–11, palette counts S=4; row 3 (top-based): 6 opaque pixels spanning columns 7–12, palette counts L=4, S=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2057"></a>
**L2057** — <code>    "......SLLLLLLS......", "......SLKKKKLS......",</code>

KNIGHT_SPRITE row 4 (top-based): 8 opaque pixels spanning columns 6–13, palette counts L=6, S=2; row 5 (top-based): 8 opaque pixels spanning columns 6–13, palette counts K=4, L=2, S=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2058"></a>
**L2058** — <code>    "......SLKYYKLS......", ".......SLLLLS.......",</code>

KNIGHT_SPRITE row 6 (top-based): 8 opaque pixels spanning columns 6–13, palette counts K=2, L=2, S=2, Y=2; row 7 (top-based): 6 opaque pixels spanning columns 7–12, palette counts L=4, S=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2059"></a>
**L2059** — <code>    "...A....SSSS....L...", "..AABBBSLLLS....L...",</code>

KNIGHT_SPRITE row 8 (top-based): 6 opaque pixels spanning columns 3–16, palette counts A=1, L=1, S=4; row 9 (top-based): 11 opaque pixels spanning columns 2–16, palette counts A=2, B=3, L=4, S=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2060"></a>
**L2060** — <code>    "..ABBAASLLLS...LLL..", "..ABBBASTTTS....L...",</code>

KNIGHT_SPRITE row 10 (top-based): 13 opaque pixels spanning columns 2–17, palette counts A=3, B=2, L=6, S=2; row 11 (top-based): 11 opaque pixels spanning columns 2–16, palette counts A=2, B=3, L=1, S=2, T=3. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2061"></a>
**L2061** — <code>    "..ABBAASBBBS...AAA..", "...AA..SBBBS....S...",</code>

KNIGHT_SPRITE row 12 (top-based): 13 opaque pixels spanning columns 2–17, palette counts A=6, B=5, S=2; row 13 (top-based): 8 opaque pixels spanning columns 3–16, palette counts A=2, B=3, S=3. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2062"></a>
**L2062** — <code>    ".......SBBBS........", ".......SSSSS........",</code>

KNIGHT_SPRITE row 14 (top-based): 5 opaque pixels spanning columns 7–11, palette counts B=3, S=2; row 15 (top-based): 5 opaque pixels spanning columns 7–11, palette counts S=5. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2063"></a>
**L2063** — <code>    ".......SL.SL........", ".......SL.SL........",</code>

KNIGHT_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 7–11, palette counts L=2, S=2; row 17 (top-based): 4 opaque pixels spanning columns 7–11, palette counts L=2, S=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2064"></a>
**L2064** — <code>    "......SSS.SSS.......", "......KKK.KKK......."</code>

KNIGHT_SPRITE row 18 (top-based): 6 opaque pixels spanning columns 6–12, palette counts S=6; row 19 (top-based): 6 opaque pixels spanning columns 6–12, palette counts K=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2065"></a>
**L2065** — <code>}};</code>

Close KNIGHT_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2066"></a>
**L2066** — <code>constexpr PixelSprite GOBLIN_SPRITE{{</code>

Begin GOBLIN_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2067"></a>
**L2067** — <code>    "....................", "...G............G...",</code>

GOBLIN_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 2 opaque pixels spanning columns 3–16, palette counts G=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2068"></a>
**L2068** — <code>    "...GG..........GG...", "...gGG..GGGG..GGg...",</code>

GOBLIN_SPRITE row 2 (top-based): 4 opaque pixels spanning columns 3–16, palette counts G=4; row 3 (top-based): 10 opaque pixels spanning columns 3–16, palette counts G=8, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2069"></a>
**L2069** — <code>    "....gGGGGGGGGGGg....", ".....GGGGGGGGGG.....",</code>

GOBLIN_SPRITE row 4 (top-based): 12 opaque pixels spanning columns 4–15, palette counts G=10, g=2; row 5 (top-based): 10 opaque pixels spanning columns 5–14, palette counts G=10. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2070"></a>
**L2070** — <code>    ".....GEEGGGGEEG.....", ".....GKKGGGGKKG.....",</code>

GOBLIN_SPRITE row 6 (top-based): 10 opaque pixels spanning columns 5–14, palette counts E=4, G=6; row 7 (top-based): 10 opaque pixels spanning columns 5–14, palette counts G=6, K=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2071"></a>
**L2071** — <code>    "......GGWWWWGG......", "......gGKKKKGg......",</code>

GOBLIN_SPRITE row 8 (top-based): 8 opaque pixels spanning columns 6–13, palette counts G=4, W=4; row 9 (top-based): 8 opaque pixels spanning columns 6–13, palette counts G=2, K=4, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2072"></a>
**L2072** — <code>    "....GGggGGGGggGG....", "...GGGTTTTTTTTGGG...",</code>

GOBLIN_SPRITE row 10 (top-based): 12 opaque pixels spanning columns 4–15, palette counts G=8, g=4; row 11 (top-based): 14 opaque pixels spanning columns 3–16, palette counts G=6, T=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2073"></a>
**L2073** — <code>    "...gGGTTTTTTTTGGg...", "....ggTTAAAATTgg....",</code>

GOBLIN_SPRITE row 12 (top-based): 14 opaque pixels spanning columns 3–16, palette counts G=4, T=8, g=2; row 13 (top-based): 12 opaque pixels spanning columns 4–15, palette counts A=4, T=4, g=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2074"></a>
**L2074** — <code>    "......TTTTTTTT......", "......gggggggg......",</code>

GOBLIN_SPRITE row 14 (top-based): 8 opaque pixels spanning columns 6–13, palette counts T=8; row 15 (top-based): 8 opaque pixels spanning columns 6–13, palette counts g=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2075"></a>
**L2075** — <code>    "......GGG..GGG......", "......gGG..GGg......",</code>

GOBLIN_SPRITE row 16 (top-based): 6 opaque pixels spanning columns 6–13, palette counts G=6; row 17 (top-based): 6 opaque pixels spanning columns 6–13, palette counts G=4, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2076"></a>
**L2076** — <code>    ".....WWWW..WWWW.....", "...................."</code>

GOBLIN_SPRITE row 18 (top-based): 8 opaque pixels spanning columns 5–14, palette counts W=8; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2077"></a>
**L2077** — <code>}};</code>

Close GOBLIN_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2078"></a>
**L2078** — <code>constexpr PixelSprite DEMON_SPRITE{{</code>

Begin DEMON_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2079"></a>
**L2079** — <code>    "....W..........W....", "...WW..........WW...",</code>

DEMON_SPRITE row 0 (top-based): 2 opaque pixels spanning columns 4–15, palette counts W=2; row 1 (top-based): 4 opaque pixels spanning columns 3–16, palette counts W=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2080"></a>
**L2080** — <code>    "...WPP........PPW...", "...pPPPPPPPPPPPPp...",</code>

DEMON_SPRITE row 2 (top-based): 6 opaque pixels spanning columns 3–16, palette counts P=4, W=2; row 3 (top-based): 14 opaque pixels spanning columns 3–16, palette counts P=12, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2081"></a>
**L2081** — <code>    "....PPPPPPPPPPPP....", ".....PEEPPPPEEP.....",</code>

DEMON_SPRITE row 4 (top-based): 12 opaque pixels spanning columns 4–15, palette counts P=12; row 5 (top-based): 10 opaque pixels spanning columns 5–14, palette counts E=4, P=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2082"></a>
**L2082** — <code>    ".....PKKPPPPKKP.....", "......PPWWWWPP......",</code>

DEMON_SPRITE row 6 (top-based): 10 opaque pixels spanning columns 5–14, palette counts K=4, P=6; row 7 (top-based): 8 opaque pixels spanning columns 6–13, palette counts P=4, W=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2083"></a>
**L2083** — <code>    "......PPKRRKPP......", "....PPppWWWWppPP....",</code>

DEMON_SPRITE row 8 (top-based): 8 opaque pixels spanning columns 6–13, palette counts K=2, P=4, R=2; row 9 (top-based): 12 opaque pixels spanning columns 4–15, palette counts P=4, W=4, p=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2084"></a>
**L2084** — <code>    "...PPPppPPPPppPPP...", "..pPPPppPPPPppPPPp..",</code>

DEMON_SPRITE row 10 (top-based): 14 opaque pixels spanning columns 3–16, palette counts P=10, p=4; row 11 (top-based): 16 opaque pixels spanning columns 2–17, palette counts P=10, p=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2085"></a>
**L2085** — <code>    "..pPP.ppPPPPpp.PPp..", "...WW.ppPPPPpp.WW...",</code>

DEMON_SPRITE row 12 (top-based): 14 opaque pixels spanning columns 2–17, palette counts P=8, p=6; row 13 (top-based): 12 opaque pixels spanning columns 3–16, palette counts P=4, W=4, p=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2086"></a>
**L2086** — <code>    "......pppppppp......", "......PPP..PPP......",</code>

DEMON_SPRITE row 14 (top-based): 8 opaque pixels spanning columns 6–13, palette counts p=8; row 15 (top-based): 6 opaque pixels spanning columns 6–13, palette counts P=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2087"></a>
**L2087** — <code>    "......pPP..PPp......", ".....WWW....WWW.....",</code>

DEMON_SPRITE row 16 (top-based): 6 opaque pixels spanning columns 6–13, palette counts P=4, p=2; row 17 (top-based): 6 opaque pixels spanning columns 5–14, palette counts W=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2088"></a>
**L2088** — <code>    "....................", "...................."</code>

DEMON_SPRITE row 18 (top-based): fully transparent, emitting no triangles; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2089"></a>
**L2089** — <code>}};</code>

Close DEMON_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2090"></a>
**L2090** — <code>constexpr PixelSprite WRAITH_SPRITE{{</code>

Begin WRAITH_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2091"></a>
**L2091** — <code>    "....................", "........CCCC........",</code>

WRAITH_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 4 opaque pixels spanning columns 8–11, palette counts C=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2092"></a>
**L2092** — <code>    ".......CCCCCC.......", "......CCLLLLCC......",</code>

WRAITH_SPRITE row 2 (top-based): 6 opaque pixels spanning columns 7–12, palette counts C=6; row 3 (top-based): 8 opaque pixels spanning columns 6–13, palette counts C=4, L=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2093"></a>
**L2093** — <code>    ".....CCLLLLLLCC.....", ".....CCEECCEECC.....",</code>

WRAITH_SPRITE row 4 (top-based): 10 opaque pixels spanning columns 5–14, palette counts C=4, L=6; row 5 (top-based): 10 opaque pixels spanning columns 5–14, palette counts C=6, E=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2094"></a>
**L2094** — <code>    ".....CCKKCCKKCC.....", "......CCCCCCCC......",</code>

WRAITH_SPRITE row 6 (top-based): 10 opaque pixels spanning columns 5–14, palette counts C=6, K=4; row 7 (top-based): 8 opaque pixels spanning columns 6–13, palette counts C=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2095"></a>
**L2095** — <code>    "......CCCKKCCC......", "....CCCCCKKCCCCC....",</code>

WRAITH_SPRITE row 8 (top-based): 8 opaque pixels spanning columns 6–13, palette counts C=6, K=2; row 9 (top-based): 12 opaque pixels spanning columns 4–15, palette counts C=10, K=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2096"></a>
**L2096** — <code>    "...CCCDCCCCCCDCCC...", "...CCDCCCCCCCCDCC...",</code>

WRAITH_SPRITE row 10 (top-based): 14 opaque pixels spanning columns 3–16, palette counts C=12, D=2; row 11 (top-based): 14 opaque pixels spanning columns 3–16, palette counts C=12, D=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2097"></a>
**L2097** — <code>    "....CDCCCCCCCCDC....", ".....DCCCCCCCCD.....",</code>

WRAITH_SPRITE row 12 (top-based): 12 opaque pixels spanning columns 4–15, palette counts C=10, D=2; row 13 (top-based): 10 opaque pixels spanning columns 5–14, palette counts C=8, D=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2098"></a>
**L2098** — <code>    ".....DCCCCCCCCD.....", ".....DCCC..CCCD.....",</code>

WRAITH_SPRITE row 14 (top-based): 10 opaque pixels spanning columns 5–14, palette counts C=8, D=2; row 15 (top-based): 8 opaque pixels spanning columns 5–14, palette counts C=6, D=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2099"></a>
**L2099** — <code>    "......CC....CC......", "......C......C......",</code>

WRAITH_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 6–13, palette counts C=4; row 17 (top-based): 2 opaque pixels spanning columns 6–13, palette counts C=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2100"></a>
**L2100** — <code>    "....................", "...................."</code>

WRAITH_SPRITE row 18 (top-based): fully transparent, emitting no triangles; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2101"></a>
**L2101** — <code>}};</code>

Close WRAITH_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2102"></a>
**L2102** — <code>constexpr PixelSprite SKELETON_SPRITE{{</code>

Begin SKELETON_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2103"></a>
**L2103** — <code>    "....................", ".......WWWWWW.......",</code>

SKELETON_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 6 opaque pixels spanning columns 7–12, palette counts W=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2104"></a>
**L2104** — <code>    "......WLLLLLLW......", ".....WLLLLLLLLW.....",</code>

SKELETON_SPRITE row 2 (top-based): 8 opaque pixels spanning columns 6–13, palette counts L=6, W=2; row 3 (top-based): 10 opaque pixels spanning columns 5–14, palette counts L=8, W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2105"></a>
**L2105** — <code>    "....WLKEELLEEKLW....", "....WLKKKLLKKKLW....",</code>

SKELETON_SPRITE row 4 (top-based): 12 opaque pixels spanning columns 4–15, palette counts E=4, K=2, L=4, W=2; row 5 (top-based): 12 opaque pixels spanning columns 4–15, palette counts K=6, L=4, W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2106"></a>
**L2106** — <code>    ".....WLLLLLLLLW.....", ".....WWLKLLKLWW.....",</code>

SKELETON_SPRITE row 6 (top-based): 10 opaque pixels spanning columns 5–14, palette counts L=8, W=2; row 7 (top-based): 10 opaque pixels spanning columns 5–14, palette counts K=2, L=4, W=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2107"></a>
**L2107** — <code>    "......WLWLWLW.......", ".........WW.........",</code>

SKELETON_SPRITE row 8 (top-based): 7 opaque pixels spanning columns 6–12, palette counts L=3, W=4; row 9 (top-based): 2 opaque pixels spanning columns 9–10, palette counts W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2108"></a>
**L2108** — <code>    "....WW..LWWL..WW....", "...WW..WWWWWW..WW...",</code>

SKELETON_SPRITE row 10 (top-based): 8 opaque pixels spanning columns 4–15, palette counts L=2, W=6; row 11 (top-based): 10 opaque pixels spanning columns 3–16, palette counts W=10. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2109"></a>
**L2109** — <code>    "....W...LWWL...W....", "...WW..WWWWWW..WW...",</code>

SKELETON_SPRITE row 12 (top-based): 6 opaque pixels spanning columns 4–15, palette counts L=2, W=4; row 13 (top-based): 10 opaque pixels spanning columns 3–16, palette counts W=10. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2110"></a>
**L2110** — <code>    "....W...LWWL...W....", ".......WWWWWW.......",</code>

SKELETON_SPRITE row 14 (top-based): 6 opaque pixels spanning columns 4–15, palette counts L=2, W=4; row 15 (top-based): 6 opaque pixels spanning columns 7–12, palette counts W=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2111"></a>
**L2111** — <code>    ".......WL..LW.......", ".......WL..LW.......",</code>

SKELETON_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 7–12, palette counts L=2, W=2; row 17 (top-based): 4 opaque pixels spanning columns 7–12, palette counts L=2, W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2112"></a>
**L2112** — <code>    ".......WL..LW.......", ".....WWWW..WWWW....."</code>

SKELETON_SPRITE row 18 (top-based): 4 opaque pixels spanning columns 7–12, palette counts L=2, W=2; row 19 (top-based): 8 opaque pixels spanning columns 5–14, palette counts W=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2113"></a>
**L2113** — <code>}};</code>

Close SKELETON_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2114"></a>
**L2114** — <code>constexpr PixelSprite SLIME_SPRITE{{</code>

Begin SLIME_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2115"></a>
**L2115** — <code>    "....................", "....................",</code>

SLIME_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2116"></a>
**L2116** — <code>    "....................", "....................",</code>

SLIME_SPRITE row 2 (top-based): fully transparent, emitting no triangles; row 3 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2117"></a>
**L2117** — <code>    "....................", "....................",</code>

SLIME_SPRITE row 4 (top-based): fully transparent, emitting no triangles; row 5 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2118"></a>
**L2118** — <code>    ".......GGGGGG.......", ".....GGLLLGGGGG.....",</code>

SLIME_SPRITE row 6 (top-based): 6 opaque pixels spanning columns 7–12, palette counts G=6; row 7 (top-based): 10 opaque pixels spanning columns 5–14, palette counts G=7, L=3. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2119"></a>
**L2119** — <code>    "....GLLLLGGGGGGG....", "...GGLLGGGGGGGGGG...",</code>

SLIME_SPRITE row 8 (top-based): 12 opaque pixels spanning columns 4–15, palette counts G=8, L=4; row 9 (top-based): 14 opaque pixels spanning columns 3–16, palette counts G=12, L=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2120"></a>
**L2120** — <code>    "...GGGGGGGGGGGGGG...", "..GGGEEGGGGEEGGGGG..",</code>

SLIME_SPRITE row 10 (top-based): 14 opaque pixels spanning columns 3–16, palette counts G=14; row 11 (top-based): 16 opaque pixels spanning columns 2–17, palette counts E=4, G=12. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2121"></a>
**L2121** — <code>    "..GGGKKGGGGKKGGGGG..", ".GGGGGGGGGGGGGGGGGG.",</code>

SLIME_SPRITE row 12 (top-based): 16 opaque pixels spanning columns 2–17, palette counts G=12, K=4; row 13 (top-based): 18 opaque pixels spanning columns 1–18, palette counts G=18. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2122"></a>
**L2122** — <code>    ".GGGGGGGKKGGGGGGGGG.", ".gGGGGGGGGGGGGGGGGg.",</code>

SLIME_SPRITE row 14 (top-based): 18 opaque pixels spanning columns 1–18, palette counts G=16, K=2; row 15 (top-based): 18 opaque pixels spanning columns 1–18, palette counts G=16, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2123"></a>
**L2123** — <code>    ".ggGGGGGGGGGGGGGGgg.", ".gggGGGGGGGGGGGGggg.",</code>

SLIME_SPRITE row 16 (top-based): 18 opaque pixels spanning columns 1–18, palette counts G=14, g=4; row 17 (top-based): 18 opaque pixels spanning columns 1–18, palette counts G=12, g=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2124"></a>
**L2124** — <code>    "..gggggggggggggggg..", ".gggggggggggggggggg."</code>

SLIME_SPRITE row 18 (top-based): 16 opaque pixels spanning columns 2–17, palette counts g=16; row 19 (top-based): 18 opaque pixels spanning columns 1–18, palette counts g=18. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2125"></a>
**L2125** — <code>}};</code>

Close SLIME_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2126"></a>
**L2126** — <code>constexpr PixelSprite BAT_SPRITE{{</code>

Begin BAT_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2127"></a>
**L2127** — <code>    "....................", "....................",</code>

BAT_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2128"></a>
**L2128** — <code>    "...P............P...", "...PP..........PP...",</code>

BAT_SPRITE row 2 (top-based): 2 opaque pixels spanning columns 3–16, palette counts P=2; row 3 (top-based): 4 opaque pixels spanning columns 3–16, palette counts P=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2129"></a>
**L2129** — <code>    "P.PP..PP....PP..PP.P", "PP.PPPPP....PPPPP.PP",</code>

BAT_SPRITE row 4 (top-based): 10 opaque pixels spanning columns 0–19, palette counts P=10; row 5 (top-based): 14 opaque pixels spanning columns 0–19, palette counts P=14. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2130"></a>
**L2130** — <code>    "PPP.PPPPPPPPPPPP.PPP", "PPPPPppPEEPPppPPPPP.",</code>

BAT_SPRITE row 6 (top-based): 18 opaque pixels spanning columns 0–19, palette counts P=18; row 7 (top-based): 19 opaque pixels spanning columns 0–18, palette counts E=2, P=13, p=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2131"></a>
**L2131** — <code>    "PPPPpppPKKPPpppPPPP.", "PPPppppPPWWPppppPPP.",</code>

BAT_SPRITE row 8 (top-based): 19 opaque pixels spanning columns 0–18, palette counts K=2, P=11, p=6; row 9 (top-based): 19 opaque pixels spanning columns 0–18, palette counts P=9, W=2, p=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2132"></a>
**L2132** — <code>    "PPpppppPPPPppppppPP.", "PppppppPPPPpppppppP.",</code>

BAT_SPRITE row 10 (top-based): 19 opaque pixels spanning columns 0–18, palette counts P=8, p=11; row 11 (top-based): 19 opaque pixels spanning columns 0–18, palette counts P=6, p=13. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2133"></a>
**L2133** — <code>    "Ppp.PppPPPPppP.pppP.", ".P....PpPPPPpP....P.",</code>

BAT_SPRITE row 12 (top-based): 17 opaque pixels spanning columns 0–18, palette counts P=8, p=9; row 13 (top-based): 10 opaque pixels spanning columns 1–18, palette counts P=8, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2134"></a>
**L2134** — <code>    ".....P..pppp..P.....", "........pppp........",</code>

BAT_SPRITE row 14 (top-based): 6 opaque pixels spanning columns 5–14, palette counts P=2, p=4; row 15 (top-based): 4 opaque pixels spanning columns 8–11, palette counts p=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2135"></a>
**L2135** — <code>    "........pppp........", "........W..W........",</code>

BAT_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 8–11, palette counts p=4; row 17 (top-based): 2 opaque pixels spanning columns 8–11, palette counts W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2136"></a>
**L2136** — <code>    "....................", "...................."</code>

BAT_SPRITE row 18 (top-based): fully transparent, emitting no triangles; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2137"></a>
**L2137** — <code>}};</code>

Close BAT_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2138"></a>
**L2138** — <code>constexpr PixelSprite SPIDER_SPRITE{{</code>

Begin SPIDER_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2139"></a>
**L2139** — <code>    "....................", "...A............A...",</code>

SPIDER_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 2 opaque pixels spanning columns 3–16, palette counts A=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2140"></a>
**L2140** — <code>    "...AA..........AA...", "....AA..rrrr..AA....",</code>

SPIDER_SPRITE row 2 (top-based): 4 opaque pixels spanning columns 3–16, palette counts A=4; row 3 (top-based): 8 opaque pixels spanning columns 4–15, palette counts A=4, r=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2141"></a>
**L2141** — <code>    ".....AArrrrrrAA.....", ".A....rrRRRRrr....A.",</code>

SPIDER_SPRITE row 4 (top-based): 10 opaque pixels spanning columns 5–14, palette counts A=4, r=6; row 5 (top-based): 10 opaque pixels spanning columns 1–18, palette counts A=2, R=4, r=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2142"></a>
**L2142** — <code>    ".AA...rRRRRRRr...AA.", "..AAA.rRRRRRRr.AAA..",</code>

SPIDER_SPRITE row 6 (top-based): 12 opaque pixels spanning columns 1–18, palette counts A=4, R=6, r=2; row 7 (top-based): 14 opaque pixels spanning columns 2–17, palette counts A=6, R=6, r=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2143"></a>
**L2143** — <code>    "....AArrRRRRrrAA....", "......rrrrrrrr......",</code>

SPIDER_SPRITE row 8 (top-based): 12 opaque pixels spanning columns 4–15, palette counts A=4, R=4, r=4; row 9 (top-based): 8 opaque pixels spanning columns 6–13, palette counts r=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2144"></a>
**L2144** — <code>    "...AAAArREErRAAAA...", "..AA..rREEEEr..AA...",</code>

SPIDER_SPRITE row 10 (top-based): 14 opaque pixels spanning columns 3–16, palette counts A=8, E=2, R=2, r=2; row 11 (top-based): 11 opaque pixels spanning columns 2–16, palette counts A=4, E=4, R=1, r=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2145"></a>
**L2145** — <code>    "..AA...rRKKRr...AA..", "..A....rrrrrr....A..",</code>

SPIDER_SPRITE row 12 (top-based): 10 opaque pixels spanning columns 2–17, palette counts A=4, K=2, R=2, r=2; row 13 (top-based): 8 opaque pixels spanning columns 2–17, palette counts A=2, r=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2146"></a>
**L2146** — <code>    "....AAT..TT..TAA....", "....AA...TT...AA....",</code>

SPIDER_SPRITE row 14 (top-based): 8 opaque pixels spanning columns 4–15, palette counts A=4, T=4; row 15 (top-based): 6 opaque pixels spanning columns 4–15, palette counts A=4, T=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2147"></a>
**L2147** — <code>    "....AA........AA....", "....A..........A....",</code>

SPIDER_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 4–15, palette counts A=4; row 17 (top-based): 2 opaque pixels spanning columns 4–15, palette counts A=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2148"></a>
**L2148** — <code>    "....................", "...................."</code>

SPIDER_SPRITE row 18 (top-based): fully transparent, emitting no triangles; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2149"></a>
**L2149** — <code>}};</code>

Close SPIDER_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2150"></a>
**L2150** — <code>constexpr PixelSprite GOLEM_SPRITE{{</code>

Begin GOLEM_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2151"></a>
**L2151** — <code>    "....................", "......SSSSSSSS......",</code>

GOLEM_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 8 opaque pixels spanning columns 6–13, palette counts S=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2152"></a>
**L2152** — <code>    "......SLLLLLLS......", "......SSESSESS......",</code>

GOLEM_SPRITE row 2 (top-based): 8 opaque pixels spanning columns 6–13, palette counts L=6, S=2; row 3 (top-based): 8 opaque pixels spanning columns 6–13, palette counts E=2, S=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2153"></a>
**L2153** — <code>    "......SSESSESS......", "......SSSSSSSS......",</code>

GOLEM_SPRITE row 4 (top-based): 8 opaque pixels spanning columns 6–13, palette counts E=2, S=6; row 5 (top-based): 8 opaque pixels spanning columns 6–13, palette counts S=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2154"></a>
**L2154** — <code>    "......SSSLLSSS......", "..SSSSSSSSSSSSSSSS..",</code>

GOLEM_SPRITE row 6 (top-based): 8 opaque pixels spanning columns 6–13, palette counts L=2, S=6; row 7 (top-based): 16 opaque pixels spanning columns 2–17, palette counts S=16. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2155"></a>
**L2155** — <code>    "..SLLLSSLLLLSSLLLS..", "..SLLLSSSCCSSSLLLS..",</code>

GOLEM_SPRITE row 8 (top-based): 16 opaque pixels spanning columns 2–17, palette counts L=10, S=6; row 9 (top-based): 16 opaque pixels spanning columns 2–17, palette counts C=2, L=6, S=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2156"></a>
**L2156** — <code>    "..SLLLSSCCCCSSLLLS..", "..SSSSSSSCCSSSSSSS..",</code>

GOLEM_SPRITE row 10 (top-based): 16 opaque pixels spanning columns 2–17, palette counts C=4, L=6, S=6; row 11 (top-based): 16 opaque pixels spanning columns 2–17, palette counts C=2, S=14. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2157"></a>
**L2157** — <code>    "..SSSSSSLLLLSSSSSS..", "..SSSSSSSSSSSSSSSS..",</code>

GOLEM_SPRITE row 12 (top-based): 16 opaque pixels spanning columns 2–17, palette counts L=4, S=12; row 13 (top-based): 16 opaque pixels spanning columns 2–17, palette counts S=16. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2158"></a>
**L2158** — <code>    ".SSSS..SSSSSS..SSSS.", ".....SSSSSSSSSS.....",</code>

GOLEM_SPRITE row 14 (top-based): 14 opaque pixels spanning columns 1–18, palette counts S=14; row 15 (top-based): 10 opaque pixels spanning columns 5–14, palette counts S=10. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2159"></a>
**L2159** — <code>    ".....SLLS..SLLS.....", ".....SLLS..SLLS.....",</code>

GOLEM_SPRITE row 16 (top-based): 8 opaque pixels spanning columns 5–14, palette counts L=4, S=4; row 17 (top-based): 8 opaque pixels spanning columns 5–14, palette counts L=4, S=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2160"></a>
**L2160** — <code>    ".....SSSS..SSSS.....", "...SSSSSS..SSSSSS..."</code>

GOLEM_SPRITE row 18 (top-based): 8 opaque pixels spanning columns 5–14, palette counts S=8; row 19 (top-based): 12 opaque pixels spanning columns 3–16, palette counts S=12. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2161"></a>
**L2161** — <code>}};</code>

Close GOLEM_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2162"></a>
**L2162** — <code>constexpr PixelSprite EVIL_EYE_SPRITE{{</code>

Begin EVIL_EYE_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2163"></a>
**L2163** — <code>    "....................", "......pppppppp......",</code>

EVIL_EYE_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 8 opaque pixels spanning columns 6–13, palette counts p=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2164"></a>
**L2164** — <code>    ".....ppWWWWWWpp.....", "....pWWRWWWWRWp.....",</code>

EVIL_EYE_SPRITE row 2 (top-based): 10 opaque pixels spanning columns 5–14, palette counts W=6, p=4; row 3 (top-based): 11 opaque pixels spanning columns 4–14, palette counts R=2, W=7, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2165"></a>
**L2165** — <code>    "....pWWWWWWWWWWp....", "...pWRWWEEEEWWRWp...",</code>

EVIL_EYE_SPRITE row 4 (top-based): 12 opaque pixels spanning columns 4–15, palette counts W=10, p=2; row 5 (top-based): 14 opaque pixels spanning columns 3–16, palette counts E=4, R=2, W=6, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2166"></a>
**L2166** — <code>    "...pWWWEEEEEEWWWp...", "...pWWWEEKKKKWWWp...",</code>

EVIL_EYE_SPRITE row 6 (top-based): 14 opaque pixels spanning columns 3–16, palette counts E=6, W=6, p=2; row 7 (top-based): 14 opaque pixels spanning columns 3–16, palette counts E=2, K=4, W=6, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2167"></a>
**L2167** — <code>    "...pWWWEEKKKKWWWp...", "...pWRWWEEEEWWRWp...",</code>

EVIL_EYE_SPRITE row 8 (top-based): 14 opaque pixels spanning columns 3–16, palette counts E=2, K=4, W=6, p=2; row 9 (top-based): 14 opaque pixels spanning columns 3–16, palette counts E=4, R=2, W=6, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2168"></a>
**L2168** — <code>    "....pWWWWWWWWWWp....", "....pWWRWWWWRWp.....",</code>

EVIL_EYE_SPRITE row 10 (top-based): 12 opaque pixels spanning columns 4–15, palette counts W=10, p=2; row 11 (top-based): 11 opaque pixels spanning columns 4–14, palette counts R=2, W=7, p=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2169"></a>
**L2169** — <code>    ".....ppWWWWWWpp.....", "......pppppppp......",</code>

EVIL_EYE_SPRITE row 12 (top-based): 10 opaque pixels spanning columns 5–14, palette counts W=6, p=4; row 13 (top-based): 8 opaque pixels spanning columns 6–13, palette counts p=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2170"></a>
**L2170** — <code>    ".....P..P..P..P.....", ".....P..P..P..P.....",</code>

EVIL_EYE_SPRITE row 14 (top-based): 4 opaque pixels spanning columns 5–14, palette counts P=4; row 15 (top-based): 4 opaque pixels spanning columns 5–14, palette counts P=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2171"></a>
**L2171** — <code>    "....P...P..P...P....", "...P....P..P....P...",</code>

EVIL_EYE_SPRITE row 16 (top-based): 4 opaque pixels spanning columns 4–15, palette counts P=4; row 17 (top-based): 4 opaque pixels spanning columns 3–16, palette counts P=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2172"></a>
**L2172** — <code>    "...PP...P..P...PP...", ".......PP..PP......."</code>

EVIL_EYE_SPRITE row 18 (top-based): 6 opaque pixels spanning columns 3–16, palette counts P=6; row 19 (top-based): 4 opaque pixels spanning columns 7–12, palette counts P=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2173"></a>
**L2173** — <code>}};</code>

Close EVIL_EYE_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2174"></a>
**L2174** — <code>constexpr PixelSprite MUSHROOM_SPRITE{{</code>

Begin MUSHROOM_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2175"></a>
**L2175** — <code>    "....................", ".......RRRRRR.......",</code>

MUSHROOM_SPRITE row 0 (top-based): fully transparent, emitting no triangles; row 1 (top-based): 6 opaque pixels spanning columns 7–12, palette counts R=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2176"></a>
**L2176** — <code>    ".....RRRWWRRRRR.....", "....RRRWWRRRRRRR....",</code>

MUSHROOM_SPRITE row 2 (top-based): 10 opaque pixels spanning columns 5–14, palette counts R=8, W=2; row 3 (top-based): 12 opaque pixels spanning columns 4–15, palette counts R=10, W=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2177"></a>
**L2177** — <code>    "...RRRRRRRRRWWRRR...", "..RRRWWRRRRWWRRRRR..",</code>

MUSHROOM_SPRITE row 4 (top-based): 14 opaque pixels spanning columns 3–16, palette counts R=12, W=2; row 5 (top-based): 16 opaque pixels spanning columns 2–17, palette counts R=12, W=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2178"></a>
**L2178** — <code>    "..RRRWWRRRRRRRRRRR..", ".rRRRRRRRWWRRRRRRRr.",</code>

MUSHROOM_SPRITE row 6 (top-based): 16 opaque pixels spanning columns 2–17, palette counts R=14, W=2; row 7 (top-based): 18 opaque pixels spanning columns 1–18, palette counts R=14, W=2, r=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2179"></a>
**L2179** — <code>    ".rrRRRRRRWWRRRRRRrr.", ".rrrrrrrrrrrrrrrrrr.",</code>

MUSHROOM_SPRITE row 8 (top-based): 18 opaque pixels spanning columns 1–18, palette counts R=12, W=2, r=4; row 9 (top-based): 18 opaque pixels spanning columns 1–18, palette counts r=18. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2180"></a>
**L2180** — <code>    ".....TTWWWWWWTT.....", "......WWWWWWWW......",</code>

MUSHROOM_SPRITE row 10 (top-based): 10 opaque pixels spanning columns 5–14, palette counts T=4, W=6; row 11 (top-based): 8 opaque pixels spanning columns 6–13, palette counts W=8. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2181"></a>
**L2181** — <code>    "......WEEWWEEW......", "......WKKWWKKW......",</code>

MUSHROOM_SPRITE row 12 (top-based): 8 opaque pixels spanning columns 6–13, palette counts E=4, W=4; row 13 (top-based): 8 opaque pixels spanning columns 6–13, palette counts K=4, W=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2182"></a>
**L2182** — <code>    "......WWWWWWWW......", "......WWKWWKWW......",</code>

MUSHROOM_SPRITE row 14 (top-based): 8 opaque pixels spanning columns 6–13, palette counts W=8; row 15 (top-based): 8 opaque pixels spanning columns 6–13, palette counts K=2, W=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2183"></a>
**L2183** — <code>    ".......WWWWWW.......", "......TWWWWWWT......",</code>

MUSHROOM_SPRITE row 16 (top-based): 6 opaque pixels spanning columns 7–12, palette counts W=6; row 17 (top-based): 8 opaque pixels spanning columns 6–13, palette counts T=2, W=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2184"></a>
**L2184** — <code>    ".....TTWWWWWWTT.....", "....TTTTTTTTTTTT...."</code>

MUSHROOM_SPRITE row 18 (top-based): 10 opaque pixels spanning columns 5–14, palette counts T=4, W=6; row 19 (top-based): 12 opaque pixels spanning columns 4–15, palette counts T=12. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2185"></a>
**L2185** — <code>}};</code>

Close MUSHROOM_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2186"></a>
**L2186** — <code>constexpr PixelSprite EXIT_SPRITE{{</code>

Begin EXIT_SPRITE, a compile-time 20×20 image. Each following string is one top-to-bottom row; each character is a left-to-right palette index. Dots are transparent, and the pixel art is geometry data rather than an external asset.

<a id="l2187"></a>
**L2187** — <code>    ".......AAAAAA.......", ".....AAGGGGGGAA.....",</code>

EXIT_SPRITE row 0 (top-based): 6 opaque pixels spanning columns 7–12, palette counts A=6; row 1 (top-based): 10 opaque pixels spanning columns 5–14, palette counts A=4, G=6. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2188"></a>
**L2188** — <code>    "....AGggggggggGA....", "...AGgg......ggGA...",</code>

EXIT_SPRITE row 2 (top-based): 12 opaque pixels spanning columns 4–15, palette counts A=2, G=2, g=8; row 3 (top-based): 8 opaque pixels spanning columns 3–16, palette counts A=2, G=2, g=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2189"></a>
**L2189** — <code>    "...AGg........gGA...", "..AGg..........gGA..",</code>

EXIT_SPRITE row 4 (top-based): 6 opaque pixels spanning columns 3–16, palette counts A=2, G=2, g=2; row 5 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2190"></a>
**L2190** — <code>    "..AGg..........gGA..", "..AGg..........gGA..",</code>

EXIT_SPRITE row 6 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2; row 7 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2191"></a>
**L2191** — <code>    "..AGg....GG....gGA..", "..AGg...GggG...gGA..",</code>

EXIT_SPRITE row 8 (top-based): 8 opaque pixels spanning columns 2–17, palette counts A=2, G=4, g=2; row 9 (top-based): 10 opaque pixels spanning columns 2–17, palette counts A=2, G=4, g=4. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2192"></a>
**L2192** — <code>    "..AGg....GG....gGA..", "..AGg..........gGA..",</code>

EXIT_SPRITE row 10 (top-based): 8 opaque pixels spanning columns 2–17, palette counts A=2, G=4, g=2; row 11 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2193"></a>
**L2193** — <code>    "..AGg..........gGA..", "..AGg..........gGA..",</code>

EXIT_SPRITE row 12 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2; row 13 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2194"></a>
**L2194** — <code>    "..AGg..........gGA..", "..AGg..........gGA..",</code>

EXIT_SPRITE row 14 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2; row 15 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2195"></a>
**L2195** — <code>    "..AGg..........gGA..", "..AGGGGGGGGGGGGGGA..",</code>

EXIT_SPRITE row 16 (top-based): 6 opaque pixels spanning columns 2–17, palette counts A=2, G=2, g=2; row 17 (top-based): 16 opaque pixels spanning columns 2–17, palette counts A=2, G=14. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2196"></a>
**L2196** — <code>    "..AAAAAAAAAAAAAAAA..", "...................."</code>

EXIT_SPRITE row 18 (top-based): 16 opaque pixels spanning columns 2–17, palette counts A=16; row 19 (top-based): fully transparent, emitting no triangles. These are two distinct image rows stored on one C++ source line; unlisted columns are transparent.

<a id="l2197"></a>
**L2197** — <code>}};</code>

Close EXIT_SPRITE after exactly twenty rows. valid_sprite later checks every row width and every palette symbol at compile time.

<a id="l2198"></a>
**L2198** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Appearance registry and art validation

<a id="l2199"></a>
**L2199** — <code>// Stable spawn IDs select distinct silhouettes; appearance never changes AI rules.</code>

Documentation fragment: “Stable spawn IDs select distinct silhouettes; appearance never changes AI rules.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2200"></a>
**L2200** — <code>struct MonsterAppearance {</code>

Appearance is a rendering-only record. There is no speed, damage, sensing, or pathfinding parameter here, which keeps art variants independent of monster behavior.

<a id="l2201"></a>
**L2201** — <code>    std::string_view name;</code>

Store a non-owning display name referring to a static string literal.

<a id="l2202"></a>
**L2202** — <code>    const PixelSprite* sprite;</code>

Point to one immutable 20×20 sprite; the registry does not copy all pixel strings for each monster.

<a id="l2203"></a>
**L2203** — <code>    bool hovering;</code>

Enable a small sinusoidal visual hover. This flag has no effect on grid occupancy or movement timing.

<a id="l2204"></a>
**L2204** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2205"></a>
**L2205** — <code>constexpr std::array&lt;MonsterAppearance, 10&gt; MONSTER_APPEARANCES{{</code>

Declare ten compile-time appearance records in the order used by stable spawn IDs.

<a id="l2206"></a>
**L2206** — <code>    {"Goblin", &amp;GOBLIN_SPRITE, false},</code>

Register spawn ID 0 as Goblin, backed by GOBLIN_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2207"></a>
**L2207** — <code>    {"Demon", &amp;DEMON_SPRITE, false},</code>

Register spawn ID 1 as Demon, backed by DEMON_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2208"></a>
**L2208** — <code>    {"Wraith", &amp;WRAITH_SPRITE, true},</code>

Register spawn ID 2 as Wraith, backed by WRAITH_SPRITE. Enable cosmetic hovering. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2209"></a>
**L2209** — <code>    {"Skeleton", &amp;SKELETON_SPRITE, false},</code>

Register spawn ID 3 as Skeleton, backed by SKELETON_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2210"></a>
**L2210** — <code>    {"Slime", &amp;SLIME_SPRITE, false},</code>

Register spawn ID 4 as Slime, backed by SLIME_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2211"></a>
**L2211** — <code>    {"Bat", &amp;BAT_SPRITE, true},</code>

Register spawn ID 5 as Bat, backed by BAT_SPRITE. Enable cosmetic hovering. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2212"></a>
**L2212** — <code>    {"Spider", &amp;SPIDER_SPRITE, false},</code>

Register spawn ID 6 as Spider, backed by SPIDER_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2213"></a>
**L2213** — <code>    {"Stone golem", &amp;GOLEM_SPRITE, false},</code>

Register spawn ID 7 as Stone golem, backed by GOLEM_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2214"></a>
**L2214** — <code>    {"Evil eye", &amp;EVIL_EYE_SPRITE, true},</code>

Register spawn ID 8 as Evil eye, backed by EVIL_EYE_SPRITE. Enable cosmetic hovering. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2215"></a>
**L2215** — <code>    {"Mushroom", &amp;MUSHROOM_SPRITE, false}</code>

Register spawn ID 9 as Mushroom, backed by MUSHROOM_SPRITE. Keep the sprite grounded. This entry does not assign a distinct AI role or alter pursuit parameters.

<a id="l2216"></a>
**L2216** — <code>}};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2217"></a>
**L2217** — <code>static_assert(MONSTER_APPEARANCES.size() &gt;= MAX_MONSTERS,</code>

Assert at compile time that every possible simultaneous spawn has a registered appearance; increasing MAX_MONSTERS without extending the registry fails compilation.

<a id="l2218"></a>
**L2218** — <code>              "Every supported monster needs its own appearance.");</code>

Diagnostic string for the preceding registry-size assertion.

<a id="l2219"></a>
**L2219** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2220"></a>
**L2220** — <code>/// Resolve a spawn's appearance without consuming any simulation randomness.</code>

Documentation fragment: “Resolve a spawn's appearance without consuming any simulation randomness.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2221"></a>
**L2221** — <code>const MonsterAppearance&amp; monster_appearance(int id) {</code>

Resolve an integer spawn ID to an immutable appearance record.

<a id="l2222"></a>
**L2222** — <code>    return MONSTER_APPEARANCES[static_cast&lt;unsigned&gt;(id) % MONSTER_APPEARANCES.size()];</code>

Convert to unsigned and wrap by the registry size. Normal IDs 0–9 map one-to-one; no random draw is consumed, preserving simulation reproducibility.

<a id="l2223"></a>
**L2223** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2224"></a>
**L2224** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2225"></a>
**L2225** — <code>/// Reject malformed sprite dimensions or unknown palette entries at compile time.</code>

Documentation fragment: “Reject malformed sprite dimensions or unknown palette entries at compile time.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2226"></a>
**L2226** — <code>constexpr bool valid_sprite(const PixelSprite&amp; sprite) {</code>

Compile-time-capable validation of one sprite's dimensions and palette membership.

<a id="l2227"></a>
**L2227** — <code>    constexpr std::string_view palette = ".KSLWYABRrTGgPpCDE";</code>

Whitelist transparent dots and all supported color symbols. Adding a new color requires extending both this string and sprite_color.

<a id="l2228"></a>
**L2228** — <code>    for (auto row : sprite) {</code>

Inspect each of the twenty row string_views without allocating copies of their text.

<a id="l2229"></a>
**L2229** — <code>        if (row.size() != sprite.size()) return false;</code>

Require row width to equal sprite height, currently twenty; malformed rows fail validation immediately.

<a id="l2230"></a>
**L2230** — <code>        for (char pixel : row) if (palette.find(pixel) == std::string_view::npos) return false;</code>

Reject any pixel character absent from the palette whitelist, including accidental spaces.

<a id="l2231"></a>
**L2231** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2232"></a>
**L2232** — <code>    return true;</code>

All row lengths and symbols passed, so this sprite is well formed.

<a id="l2233"></a>
**L2233** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2234"></a>
**L2234** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2235"></a>
**L2235** — <code>/// Validate all registered creature art, including future additions.</code>

Documentation fragment: “Validate all registered creature art, including future additions.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2236"></a>
**L2236** — <code>constexpr bool valid_monster_sprites() {</code>

Validate the entire monster registry, including entries added in future revisions.

<a id="l2237"></a>
**L2237** — <code>    for (const auto&amp; appearance : MONSTER_APPEARANCES)</code>

Visit each appearance by const reference.

<a id="l2238"></a>
**L2238** — <code>        if (!appearance.sprite || !valid_sprite(*appearance.sprite)) return false;</code>

Short-circuit on null sprite pointers before dereferencing, then apply the same dimensions/palette test used for the knight and exit.

<a id="l2239"></a>
**L2239** — <code>    return true;</code>

Every registered appearance has valid art.

<a id="l2240"></a>
**L2240** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2241"></a>
**L2241** — <code>static_assert(valid_sprite(KNIGHT_SPRITE) &amp;&amp; valid_sprite(EXIT_SPRITE) &amp;&amp;</code>

Combine knight, exit, and monster-art validation in a compile-time assertion.

<a id="l2242"></a>
**L2242** — <code>              valid_monster_sprites(), "Sprites must use twenty-pixel rows and valid palette entries.");</code>

Complete the assertion and provide a useful compiler diagnostic for invalid pixel data.

<a id="l2243"></a>
**L2243** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Wall, sprite, camera, and route geometry

<a id="l2244"></a>
**L2244** — <code>/// Scale an opaque material color without introducing texture dependencies.</code>

Documentation fragment: “Scale an opaque material color without introducing texture dependencies.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2245"></a>
**L2245** — <code>RGB shade(RGB color, float amount) { return {color.r * amount, color.g * amount, color.b * amount}; }</code>

Multiply all RGB channels by a brightness factor. This is baked vertex-color shading, not a light calculation; the function itself does not clamp values.

<a id="l2246"></a>
**L2246** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2247"></a>
**L2247** — <code>/// Expand one planar quadrilateral into the shared triangle vertex format.</code>

Documentation fragment: “Expand one planar quadrilateral into the shared triangle vertex format.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2248"></a>
**L2248** — <code>void append_quad(std::vector&lt;Vertex&gt;&amp; vertices, Vec3 a, Vec3 b, Vec3 c, Vec3 d, RGB color) {</code>

Accept four quad corners and a constant color, appending to a caller-owned vertex batch.

<a id="l2249"></a>
**L2249** — <code>    for (Vec3 p : {a, b, c, a, c, d}) vertices.push_back({p.x, p.y, p.z, color.r, color.g, color.b});</code>

Triangulate as (a,b,c) and (a,c,d), emitting six position-plus-color vertices. There is no index buffer or normal vector.

<a id="l2250"></a>
**L2250** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2251"></a>
**L2251** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2252"></a>
**L2252** — <code>/// Append a horizontal rectangle in grid space, for caps and ground markings.</code>

Documentation fragment: “Append a horizontal rectangle in grid space, for caps and ground markings.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2253"></a>
**L2253** — <code>void append_floor_rect(std::vector&lt;Vertex&gt;&amp; vertices, float x0, float y0, float x1, float y1,</code>

Start a horizontal rectangle helper using opposite x/y corners.

<a id="l2254"></a>
**L2254** — <code>                       float height, RGB color) {</code>

Specify the shared z height and RGB material for that rectangle.

<a id="l2255"></a>
**L2255** — <code>    append_quad(vertices, {x0,y0,height}, {x1,y0,height}, {x1,y1,height}, {x0,y1,height}, color);</code>

Construct four coplanar corners and reuse the quad triangulator; floor markings and wall caps share this primitive.

<a id="l2256"></a>
**L2256** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2257"></a>
**L2257** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2258"></a>
**L2258** — <code>/// Build raised gray masonry once. Exposed vertical faces and beveled caps have</code>

Documentation fragment: “Build raised gray masonry once. Exposed vertical faces and beveled caps have” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2259"></a>
**L2259** — <code>/// directional shading; hidden interior faces are omitted. Large maps omit small</code>

Documentation fragment: “directional shading; hidden interior faces are omitted. Large maps omit small” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2260"></a>
**L2260** — <code>/// cap bevels to bound mesh memory while retaining real three-dimensional walls.</code>

Documentation fragment: “cap bevels to bound mesh memory while retaining real three-dimensional walls.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2261"></a>
**L2261** — <code>std::vector&lt;Vertex&gt; build_wall_mesh(const Maze&amp; maze) {</code>

Build the static wall mesh from the generated terrain, returning CPU-side vertices for one GPU upload.

<a id="l2262"></a>
**L2262** — <code>    std::vector&lt;Vertex&gt; vertices;</code>

Create an initially empty triangle stream.

<a id="l2263"></a>
**L2263** — <code>    const bool detailed = maze.grid.size() &lt;= 15000;</code>

Use beveled caps only when total grid area is at most 15,000 cells. All three square presets qualify; a maximum-size map does not.

<a id="l2264"></a>
**L2264** — <code>    // Count exposed faces first so the reservation matches the mesh instead of</code>

Documentation fragment: “Count exposed faces first so the reservation matches the mesh instead of” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2265"></a>
**L2265** — <code>    // over-allocating by a third on maximum-size maps.</code>

Documentation fragment: “over-allocating by a third on maximum-size maps.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2266"></a>
**L2266** — <code>    std::size_t quads = 0;</code>

Count the quads the mesh will emit before building it.

<a id="l2267"></a>
**L2267** — <code>    for (int u = 0; u &lt; static_cast&lt;int&gt;(maze.grid.size()); ++u) {</code>

Visit every grid cell.

<a id="l2268"></a>
**L2268** — <code>        if (maze.grid[u] != WALL) continue;</code>

Only wall cells produce geometry.

<a id="l2269"></a>
**L2269** — <code>        Point p = maze.point(u);</code>

Convert to coordinates.

<a id="l2270"></a>
**L2270** — <code>        quads += detailed ? 5 : 1;</code>

Each wall has a top face, plus four bevel faces in detailed mode.

<a id="l2271"></a>
**L2271** — <code>        for (Point q : {Point{p.r-1,p.c}, Point{p.r+1,p.c}, Point{p.r,p.c-1}, Point{p.r,p.c+1}}) {</code>

Check the four orthogonal neighbors...

<a id="l2272"></a>
**L2272** — <code>            int v = maze.id(q);</code>

...by flattened index.

<a id="l2273"></a>
**L2273** — <code>            quads += v &lt; 0 || maze.walkable(v);</code>

An outside or open neighbor exposes a vertical face.

<a id="l2274"></a>
**L2274** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2275"></a>
**L2275** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2276"></a>
**L2276** — <code>    vertices.reserve(quads * 6);</code>

Reserve exactly six vertices per quad instead of a per-cell estimate.

<a id="l2277"></a>
**L2277** — <code>    for (int u = 0; u &lt; static_cast&lt;int&gt;(maze.grid.size()); ++u) {</code>

Visit every flattened grid cell in deterministic row-major order.

<a id="l2278"></a>
**L2278** — <code>        if (maze.grid[u] != WALL) continue;</code>

Emit masonry only for WALL cells; walkable floor remains the black background with optional markings.

<a id="l2279"></a>
**L2279** — <code>        Point p = maze.point(u);</code>

Convert the flattened index into integer row and column coordinates.

<a id="l2280"></a>
**L2280** — <code>        float x = static_cast&lt;float&gt;(p.c), y = static_cast&lt;float&gt;(p.r);</code>

Promote column and row to floating-point world x and y coordinates.

<a id="l2281"></a>
**L2281** — <code>        // Coordinate hashing changes only stone coloration, never gameplay RNG.</code>

Documentation fragment: “Coordinate hashing changes only stone coloration, never gameplay RNG.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2282"></a>
**L2282** — <code>        std::uint32_t grain = std::uint32_t(u) * 2654435761u;</code>

Hash the cell index using unsigned wraparound arithmetic. Stone variation is deterministic and does not consume the gameplay RNG.

<a id="l2283"></a>
**L2283** — <code>        float value = 0.36f + float((grain &gt;&gt; 24) &amp; 15u) * 0.004f;</code>

Choose one of sixteen gray base intensities from 0.360 through 0.420.

<a id="l2284"></a>
**L2284** — <code>        RGB stone{value, value, value};</code>

Use equal RGB channels so stone remains neutral gray before directional brightness factors.

<a id="l2285"></a>
**L2285** — <code>        float bevel = detailed ? 0.035f : 0.0f;</code>

Inset detailed wall caps by 0.035 cells; large-map caps cover the entire cell.

<a id="l2286"></a>
**L2286** — <code>        float shoulder = WALL_HEIGHT - (detailed ? 0.045f : 0.0f);</code>

Set the vertical side-wall shoulder 0.045 below the cap when beveled, otherwise at the full 0.38-cell height.

<a id="l2287"></a>
**L2287** — <code>        append_floor_rect(vertices, x+bevel, y+bevel, x+1-bevel, y+1-bevel, WALL_HEIGHT, shade(stone,1.18f));</code>

Emit the top cap at WALL_HEIGHT with brightness 1.18; coordinates shrink it when bevels are enabled.

<a id="l2288"></a>
**L2288** — <code>        if (detailed) {</code>

Only the detailed mesh emits the following four narrow sloping cap faces.

<a id="l2289"></a>
**L2289** — <code>            append_quad(vertices,{x,y,shoulder},{x+1,y,shoulder},{x+1-bevel,y+bevel,WALL_HEIGHT},{x+bevel,y+bevel,WALL_HEIGHT},shade(stone,1.42f));</code>

Emit the north cap bevel with brightness 1.42, giving that edge the strongest highlight.

<a id="l2290"></a>
**L2290** — <code>            append_quad(vertices,{x+1,y,shoulder},{x+1,y+1,shoulder},{x+1-bevel,y+1-bevel,WALL_HEIGHT},{x+1-bevel,y+bevel,WALL_HEIGHT},shade(stone,0.85f));</code>

Emit the east cap bevel with brightness 0.85.

<a id="l2291"></a>
**L2291** — <code>            append_quad(vertices,{x+1,y+1,shoulder},{x,y+1,shoulder},{x+bevel,y+1-bevel,WALL_HEIGHT},{x+1-bevel,y+1-bevel,WALL_HEIGHT},shade(stone,0.78f));</code>

Emit the south cap bevel with brightness 0.78.

<a id="l2292"></a>
**L2292** — <code>            append_quad(vertices,{x,y+1,shoulder},{x,y,shoulder},{x+bevel,y+bevel,WALL_HEIGHT},{x+bevel,y+1-bevel,WALL_HEIGHT},shade(stone,1.25f));</code>

Emit the west cap bevel with brightness 1.25, completing the directional bevel shading.

<a id="l2293"></a>
**L2293** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2294"></a>
**L2294** — <code>        const auto exposed = [&amp;](Point q) { int v=maze.id(q); return v&lt;0 || maze.walkable(v); };</code>

A neighboring position exposes a face when it lies outside the grid or is walkable. Wall-to-wall interior faces are omitted.

<a id="l2295"></a>
**L2295** — <code>        if (exposed({p.r-1,p.c})) append_quad(vertices,{x,y,0},{x+1,y,0},{x+1,y,shoulder},{x,y,shoulder},shade(stone,0.85f));</code>

Emit the north vertical face from floor to shoulder only if exposed, shaded by 0.85.

<a id="l2296"></a>
**L2296** — <code>        if (exposed({p.r+1,p.c})) append_quad(vertices,{x+1,y+1,0},{x,y+1,0},{x,y+1,shoulder},{x+1,y+1,shoulder},shade(stone,0.57f));</code>

Emit the south vertical face only if exposed, using the darker 0.57 factor.

<a id="l2297"></a>
**L2297** — <code>        if (exposed({p.r,p.c-1})) append_quad(vertices,{x,y+1,0},{x,y,0},{x,y,shoulder},{x,y+1,shoulder},shade(stone,0.76f));</code>

Emit the west vertical face only if exposed, shaded by 0.76.

<a id="l2298"></a>
**L2298** — <code>        if (exposed({p.r,p.c+1})) append_quad(vertices,{x+1,y,0},{x+1,y+1,0},{x+1,y+1,shoulder},{x+1,y,shoulder},shade(stone,0.52f));</code>

Emit the east vertical face only if exposed, shaded by 0.52.

<a id="l2299"></a>
**L2299** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2300"></a>
**L2300** — <code>    return vertices;</code>

Return all wall triangles; the Renderer uploads this mesh once per generated maze.

<a id="l2301"></a>
**L2301** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2302"></a>
**L2302** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2303"></a>
**L2303** — <code>/// Resolve a pixel's material. Hunting changes monster eyes from gold to red;</code>

Documentation fragment: “Resolve a pixel's material. Hunting changes monster eyes from gold to red;” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2304"></a>
**L2304** — <code>/// creature appearance does not assign new AI roles or alter their abilities.</code>

Documentation fragment: “creature appearance does not assign new AI roles or alter their abilities.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2305"></a>
**L2305** — <code>RGB sprite_color(char pixel, bool hunting, bool searching = false) {</code>

Resolve a sprite pixel's color; hunting makes eyes red and searching makes them orange.

<a id="l2306"></a>
**L2306** — <code>    switch (pixel) {</code>

Dispatch on the exact case-sensitive palette symbol; uppercase and lowercase colors are distinct.

<a id="l2307"></a>
**L2307** — <code>        case 'K': return {0.045f,0.05f,0.065f};</code>

Map case-sensitive symbol K to normalized RGB (0.045,0.05,0.065) for near-black outlines. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2308"></a>
**L2308** — <code>        case 'S': return {0.29f,0.35f,0.43f};</code>

Map case-sensitive symbol S to normalized RGB (0.29,0.35,0.43) for dark blue-gray steel. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2309"></a>
**L2309** — <code>        case 'L': return {0.86f,0.92f,0.97f};</code>

Map case-sensitive symbol L to normalized RGB (0.86,0.92,0.97) for pale steel highlights. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2310"></a>
**L2310** — <code>        case 'W': return {0.94f,0.87f,0.67f};</code>

Map case-sensitive symbol W to normalized RGB (0.94,0.87,0.67) for warm bone/cream. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2311"></a>
**L2311** — <code>        case 'Y': return {1.0f,0.83f,0.43f};</code>

Map case-sensitive symbol Y to normalized RGB (1.0,0.83,0.43) for bright warm yellow. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2312"></a>
**L2312** — <code>        case 'A': return {0.82f,0.58f,0.19f};</code>

Map case-sensitive symbol A to normalized RGB (0.82,0.58,0.19) for ochre/gold. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2313"></a>
**L2313** — <code>        case 'B': return {0.19f,0.40f,0.87f};</code>

Map case-sensitive symbol B to normalized RGB (0.19,0.40,0.87) for royal blue. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2314"></a>
**L2314** — <code>        case 'R': return {0.96f,0.19f,0.21f};</code>

Map case-sensitive symbol R to normalized RGB (0.96,0.19,0.21) for bright red. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2315"></a>
**L2315** — <code>        case 'r': return {0.55f,0.06f,0.12f};</code>

Map case-sensitive symbol r to normalized RGB (0.55,0.06,0.12) for dark red. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2316"></a>
**L2316** — <code>        case 'T': return {0.45f,0.23f,0.12f};</code>

Map case-sensitive symbol T to normalized RGB (0.45,0.23,0.12) for brown. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2317"></a>
**L2317** — <code>        case 'G': return {0.40f,0.78f,0.27f};</code>

Map case-sensitive symbol G to normalized RGB (0.40,0.78,0.27) for bright green. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2318"></a>
**L2318** — <code>        case 'g': return {0.17f,0.39f,0.13f};</code>

Map case-sensitive symbol g to normalized RGB (0.17,0.39,0.13) for dark green. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2319"></a>
**L2319** — <code>        case 'P': return {0.71f,0.29f,0.77f};</code>

Map case-sensitive symbol P to normalized RGB (0.71,0.29,0.77) for bright purple. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2320"></a>
**L2320** — <code>        case 'p': return {0.36f,0.11f,0.43f};</code>

Map case-sensitive symbol p to normalized RGB (0.36,0.11,0.43) for dark purple. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2321"></a>
**L2321** — <code>        case 'C': return {0.32f,0.73f,0.84f};</code>

Map case-sensitive symbol C to normalized RGB (0.32,0.73,0.84) for cyan. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2322"></a>
**L2322** — <code>        case 'D': return {0.14f,0.34f,0.51f};</code>

Map case-sensitive symbol D to normalized RGB (0.14,0.34,0.51) for dark cyan. Each opaque pixel uses this constant vertex color in both the world and menu.

<a id="l2323"></a>
**L2323** — <code>        case 'E': return hunting ? RGB{1.0f,0.12f,0.08f} : searching ? RGB{1.0f,0.52f,0.10f} : RGB{1.0f,0.83f,0.23f};</code>

Eyes are red while hunting, orange while searching, and gold otherwise.

<a id="l2324"></a>
**L2324** — <code>        default: return {1,0,1};</code>

Return bright magenta for an unrecognized symbol. Compile-time sprite validation normally prevents this diagnostic color from appearing.

<a id="l2325"></a>
**L2325** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2326"></a>
**L2326** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2327"></a>
**L2327** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2328"></a>
**L2328** — <code>/// Place a crisp, camera-facing sprite within its grid cell. Transparent pixels</code>

Documentation fragment: “Place a crisp, camera-facing sprite within its grid cell. Transparent pixels” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2329"></a>
**L2329** — <code>/// emit no geometry, so depth testing preserves proper gaps and wall occlusion.</code>

Documentation fragment: “emit no geometry, so depth testing preserves proper gaps and wall occlusion.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2330"></a>
**L2330** — <code>void append_sprite(std::vector&lt;Vertex&gt;&amp; vertices, Point cell, const PixelSprite&amp; sprite,</code>

Begin emitting an actor or exit as colored pixel quads positioned within one grid cell.

<a id="l2331"></a>
**L2331** — <code>                   bool mirror = false, bool hunting = false, float lift = 0.0f, bool searching = false) {</code>

Optional final parameter selects the orange searching eye color.

<a id="l2332"></a>
**L2332** — <code>    constexpr float pixel_size = 0.044f;</code>

One sprite pixel spans 0.044 world units, making the full twenty-pixel art 0.88 cells wide.

<a id="l2333"></a>
**L2333** — <code>    const float left = cell.c + 0.5f - 10 * pixel_size;</code>

Center the twenty-pixel row around the cell's horizontal midpoint.

<a id="l2334"></a>
**L2334** — <code>    const float foot = cell.r + 0.82f;</code>

Place the sprite's ground anchor near the lower part of the cell at row + 0.82.

<a id="l2335"></a>
**L2335** — <code>    for (int r=0;r&lt;20;++r) for (int c=0;c&lt;20;++c) {</code>

Traverse every pixel of the fixed 20×20 image, so art changes must preserve these dimensions or update the renderer too.

<a id="l2336"></a>
**L2336** — <code>        char pixel = sprite[r][mirror ? 19-c : c];</code>

Mirror by selecting source column 19−c while leaving destination columns in their usual order.

<a id="l2337"></a>
**L2337** — <code>        if (pixel == '.') continue;</code>

Treat a dot as transparency by emitting no geometry. No alpha-blending or texture transparency is required.

<a id="l2338"></a>
**L2338** — <code>        float x = left + c*pixel_size, bottom = (19-r)*pixel_size;</code>

Compute the pixel's left edge and upward height; source row zero is at the top, so the vertical index is reversed.

<a id="l2339"></a>
**L2339** — <code>        const auto at = [&amp;](float px, float up) {</code>

Define a local conversion from image-plane coordinates to three-dimensional world coordinates.

<a id="l2340"></a>
**L2340** — <code>            return Vec3{px,foot-VIEW_COS*up,0.035f+lift+VIEW_SIN*up};</code>

Tilt the sprite plane to face the fixed camera. A 0.035 base offset plus optional lift separates the art from the floor and supports bobbing.

<a id="l2341"></a>
**L2341** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2342"></a>
**L2342** — <code>        append_quad(vertices,at(x,bottom),at(x+pixel_size,bottom),</code>

Emit the lower two corners of this opaque pixel's quad using the image-to-world conversion.

<a id="l2343"></a>
**L2343** — <code>                    at(x+pixel_size,bottom+pixel_size),at(x,bottom+pixel_size),sprite_color(pixel,hunting,searching));</code>

Pass both hunting and searching states to the palette.

<a id="l2344"></a>
**L2344** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2345"></a>
**L2345** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2346"></a>
**L2346** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2347"></a>
**L2347** — <code>/// Draw a small octagonal ground ring so the knight stays identifiable when tiny.</code>

Documentation fragment: “Draw a small octagonal ground ring so the knight stays identifiable when tiny.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2348"></a>
**L2348** — <code>void append_actor_ring(std::vector&lt;Vertex&gt;&amp; vertices, Point cell, RGB color) {</code>

Create a ground marker around the knight so its position remains recognizable when the sprite becomes small.

<a id="l2349"></a>
**L2349** — <code>    constexpr float pi = 3.14159265359f;</code>

Provide a local single-precision approximation to π for ring construction.

<a id="l2350"></a>
**L2350** — <code>    for (int i=0;i&lt;8;++i) {</code>

Use eight angular sectors, producing an octagonal ring rather than a smooth circle.

<a id="l2351"></a>
**L2351** — <code>        float a=i*pi/4, b=(i+1)*pi/4;</code>

Compute the start and end angle of this 45-degree sector.

<a id="l2352"></a>
**L2352** — <code>        const auto at=[&amp;](float angle,float radius) {</code>

Define polar-to-world conversion around the cell's marker center.

<a id="l2353"></a>
**L2353** — <code>            return Vec3{cell.c+0.5f+radius*std::cos(angle),cell.r+0.55f+radius*std::sin(angle),0.022f};</code>

Place ring vertices at column + 0.5, row + 0.55, z = 0.022; that height keeps the marker above the floor.

<a id="l2354"></a>
**L2354** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2355"></a>
**L2355** — <code>        append_quad(vertices,at(a,0.30f),at(b,0.30f),at(b,0.265f),at(a,0.265f),color);</code>

Emit one annular quad between outer radius 0.30 and inner radius 0.265.

<a id="l2356"></a>
**L2356** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2357"></a>
**L2357** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2358"></a>
**L2358** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2359"></a>
**L2359** — <code>/// Keep cells proportionate on any framebuffer. A 25-degree orthographic tilt</code>

Documentation fragment: “Keep cells proportionate on any framebuffer. A 25-degree orthographic tilt” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2360"></a>
**L2360** — <code>/// exposes wall height without rotating WASD directions or hiding maze junctions.</code>

Documentation fragment: “exposes wall height without rotating WASD directions or hiding maze junctions.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2361"></a>
**L2361** — <code>/// Zoom follows the knight and clamps at map edges; zoom=1 shows the entire maze.</code>

Documentation fragment: “Zoom follows the knight and clamps at map edges; zoom=1 shows the entire maze.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2362"></a>
**L2362** — <code>std::array&lt;float,16&gt; scene_projection(const Maze&amp; maze, int width, int height, float zoom=1.0f) {</code>

Calculate a column-major 4×4 orthographic projection with fixed tilt, aspect preservation, and optional player-following zoom.

<a id="l2363"></a>
**L2363** — <code>    if (width&lt;=0 || height&lt;=0) throw std::invalid_argument("Framebuffer dimensions must be positive.");</code>

Reject zero or negative framebuffer dimensions before division. The renderer normally handles minimized windows before calling this helper.

<a id="l2364"></a>
**L2364** — <code>    zoom=std::clamp(zoom,1.0f,64.0f);</code>

Limit zoom to the supported 1×–64× range.

<a id="l2365"></a>
**L2365** — <code>    float x_extent=float(maze.cols), y_min=-maze.rows*VIEW_COS, y_max=WALL_HEIGHT*VIEW_SIN;</code>

Determine horizontal map extent and vertically projected bounds, including the height of the wall tops.

<a id="l2366"></a>
**L2366** — <code>    float fit=std::min(width/(x_extent+1.4f),height/(y_max-y_min+1.4f))*zoom;</code>

Choose the smaller pixel-per-world-unit fit of the two axes, add 1.4 units of framing margin, then apply zoom.

<a id="l2367"></a>
**L2367** — <code>    float sx=2*fit/width, sy=2*fit/height;</code>

Convert the fitted scale to OpenGL normalized-device coordinates separately for framebuffer width and height.

<a id="l2368"></a>
**L2368** — <code>    float center_x=x_extent/2, center_y=(y_min+y_max)/2;</code>

Default the view center to the center of the whole projected map.

<a id="l2369"></a>
**L2369** — <code>    if (zoom&gt;1.0f) {</code>

Only zoomed views follow the player; the 1× view stays centered to show the entire maze.

<a id="l2370"></a>
**L2370** — <code>        Point p=maze.point(maze.player);</code>

Read the player's current grid coordinates for camera tracking.

<a id="l2371"></a>
**L2371** — <code>        float half_x=width/(2*fit), half_y=height/(2*fit);</code>

Convert half the framebuffer width/height into world-space viewing extents.

<a id="l2372"></a>
**L2372** — <code>        if (2*half_x&lt;x_extent) center_x=std::clamp(p.c+0.5f,half_x,x_extent-half_x);</code>

If the map is wider than the viewport, clamp horizontal tracking so the viewport does not drift beyond the map edges.

<a id="l2373"></a>
**L2373** — <code>        if (2*half_y&lt;y_max-y_min) center_y=std::clamp(-(p.r+0.5f)*VIEW_COS,y_min+half_y,y_max-half_y);</code>

Apply the corresponding clamp to the tilted vertical coordinate, preserving map-edge framing.

<a id="l2374"></a>
**L2374** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2375"></a>
**L2375** — <code>    float depth=1.8f/(maze.rows*VIEW_SIN+2.0f);</code>

Scale depth so the full tilted maze and wall height fit inside the clip-space depth interval.

<a id="l2376"></a>
**L2376** — <code>    return {{sx,0,0,0, 0,-VIEW_COS*sy,-VIEW_SIN*depth,0,</code>

Begin the column-major matrix: x scales directly, while world y contributes negative screen y and depth through the fixed tilt.

<a id="l2377"></a>
**L2377** — <code>             0,VIEW_SIN*sy,-VIEW_COS*depth,0,</code>

World z contributes upward screen displacement and depth; this is what makes wall height visible in orthographic projection.

<a id="l2378"></a>
**L2378** — <code>             -center_x*sx,-center_y*sy,maze.rows*0.5f*VIEW_SIN*depth,1}};</code>

Finish with translations that center the camera and depth range, plus homogeneous w = 1.

<a id="l2379"></a>
**L2379** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2380"></a>
**L2380** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2381"></a>
**L2381** — <code>/// Convert a wrapped hue to a fully saturated RGB color for the complete route.</code>

Documentation fragment: “Convert a wrapped hue to a fully saturated RGB color for the complete route.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2382"></a>
**L2382** — <code>RGB hue_color(float hue) {</code>

Convert a hue parameter into full-saturation, full-value RGB for the route highlight.

<a id="l2383"></a>
**L2383** — <code>    hue-=std::floor(hue);</code>

Wrap hue into [0,1), including inputs outside a single color cycle.

<a id="l2384"></a>
**L2384** — <code>    float sector=hue*6.0f, f=sector-std::floor(sector);</code>

Split the hue wheel into six sectors and calculate the fractional interpolation within the selected sector.

<a id="l2385"></a>
**L2385** — <code>    switch (static_cast&lt;int&gt;(sector)) {</code>

Select the sector's pair of changing RGB channels.

<a id="l2386"></a>
**L2386** — <code>        case 0:return {1,f,0}; case 1:return {1-f,1,0}; case 2:return {0,1,f};</code>

Cover red→yellow, yellow→green, and green→cyan transitions with linear channel interpolation.

<a id="l2387"></a>
**L2387** — <code>        case 3:return {0,1-f,1}; case 4:return {f,0,1}; default:return {1,0,1-f};</code>

Cover cyan→blue, blue→magenta, and magenta→red, completing the wrapped color wheel.

<a id="l2388"></a>
**L2388** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2389"></a>
**L2389** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2390"></a>
**L2390** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2391"></a>
**L2391** — <code>/// Render every segment of the current-to-exit route as a narrow luminous trail.</code>

Documentation fragment: “Render every segment of the current-to-exit route as a narrow luminous trail.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2392"></a>
**L2392** — <code>/// An amber trail retains the existing warning that all snapshot routes are blocked.</code>

Documentation fragment: “An amber trail retains the existing warning that all snapshot routes are blocked.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2393"></a>
**L2393** — <code>void append_route(std::vector&lt;Vertex&gt;&amp; vertices, const Maze&amp; maze) {</code>

Render the already-computed complete player-to-exit overlay; this function performs no path search.

<a id="l2394"></a>
**L2394** — <code>    if (!maze.auto_solve) return;</code>

Suppress the route when automatic solving is disabled.

<a id="l2395"></a>
**L2395** — <code>    RGB color=maze.overlay_avoids_monsters ? hue_color(float(maze.now%100000)/1000.0f*0.3f) : RGB{1,0.65f,0};</code>

Use animated rainbow color for a route that crosses no monster cell; use fixed amber when the route crosses a monster.

<a id="l2396"></a>
**L2396** — <code>    for (std::size_t i=1;i&lt;maze.solution_overlay.size();++i) {</code>

Visit every consecutive pair of overlay points, starting with segment 0→1 and continuing all the way to the exit.

<a id="l2397"></a>
**L2397** — <code>        Point a=maze.solution_overlay[i-1], b=maze.solution_overlay[i];</code>

Copy the endpoints of the current orthogonal grid segment.

<a id="l2398"></a>
**L2398** — <code>        for (int layer=0;layer&lt;2;++layer) {</code>

Draw two opaque layers per segment to suggest a luminous trail without a postprocessing pass.

<a id="l2399"></a>
**L2399** — <code>            float half=layer ? 0.055f : 0.135f;</code>

Use a broad 0.135-cell half-width underlay and a narrow 0.055-cell half-width inner line.

<a id="l2400"></a>
**L2400** — <code>            append_floor_rect(vertices,std::min(a.c,b.c)+0.5f-half,std::min(a.r,b.r)+0.5f-half,</code>

Compute the segment rectangle's minimum x/y including its thickness.

<a id="l2401"></a>
**L2401** — <code>                              std::max(a.c,b.c)+0.5f+half,std::max(a.r,b.r)+0.5f+half,</code>

Compute the maximum x/y; the same formula covers both horizontal and vertical segments.

<a id="l2402"></a>
**L2402** — <code>                              layer ? 0.014f : 0.008f,layer ? color : shade(color,0.22f));</code>

Raise the bright inner line slightly above the darker underlay to avoid depth conflict; both remain below actors and walls.

<a id="l2403"></a>
**L2403** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2404"></a>
**L2404** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2405"></a>
**L2405** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2406"></a>
**L2406** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## UI model and editable setup

<a id="l2407"></a>
**L2407** — <code>// The front end is independent of simulation time and uses a fixed logical canvas.</code>

Documentation fragment: “The front end is independent of simulation time and uses a fixed logical canvas.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2408"></a>
**L2408** — <code>// The same widget rectangles drive drawing, pointer hit testing, and keyboard focus.</code>

Documentation fragment: “The same widget rectangles drive drawing, pointer hit testing, and keyboard focus.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2409"></a>
**L2409** — <code>constexpr float UI_WIDTH=1000, UI_HEIGHT=760;</code>

The UI uses a 1000×760 logical canvas, independent of the physical framebuffer size.

<a id="l2410"></a>
**L2410** — <code>enum class Screen { Setup, Loading, Playing, Paused, Result };</code>

Declare the five application screens; transitions determine input behavior and whether simulation time advances.

<a id="l2411"></a>
**L2411** — <code>enum class Control {</code>

Begin stable logical IDs for widgets, separating interaction semantics from labels and rectangles.

<a id="l2412"></a>
**L2412** — <code>    None, Low, Medium, High, Small, Standard, Large,</code>

IDs for no selection, the three difficulty cards, and the three size presets.

<a id="l2413"></a>
**L2413** — <code>    WidthMinus, Width, WidthPlus, HeightMinus, Height, HeightPlus,</code>

IDs for width/height decrement buttons, numeric fields, and increment buttons.

<a id="l2414"></a>
**L2414** — <code>    CountMinus, Count, CountPlus, Start, Quit, AutoSolve, Pause,</code>

IDs for monster-count controls and the main setup/game buttons.

<a id="l2415"></a>
**L2415** — <code>    Resume, Setup, Again</code>

IDs for resuming, changing setup, and replaying; their presence depends on the active screen.

<a id="l2416"></a>
**L2416** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2417"></a>
**L2417** — <code>struct UiRect {</code>

Store one UI-space axis-aligned rectangle shared by drawing and hit testing.

<a id="l2418"></a>
**L2418** — <code>    float x,y,w,h;</code>

Rectangle origin and dimensions are floats in the 1000×760 logical coordinate system.

<a id="l2419"></a>
**L2419** — <code>    /// Use half-open bounds so touching edges never activate two controls.</code>

Documentation fragment: “Use half-open bounds so touching edges never activate two controls.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2420"></a>
**L2420** — <code>    bool contains(float px,float py) const { return px&gt;=x&amp;&amp;py&gt;=y&amp;&amp;px&lt;x+w&amp;&amp;py&lt;y+h; }</code>

Use half-open bounds: left/top edges are included, right/bottom excluded. Adjacent buttons therefore do not both claim a shared boundary.

<a id="l2421"></a>
**L2421** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2422"></a>
**L2422** — <code>struct UiWidget { Control id; UiRect box; std::string label; bool selected=false, enabled=true; };</code>

A widget combines its stable ID, rectangle, label, selection state, and enabled state; selected defaults false and enabled defaults true.

<a id="l2423"></a>
**L2423** — <code>struct UiTransform {</code>

Represent the uniform scale and letterbox offsets mapping logical UI coordinates to framebuffer pixels.

<a id="l2424"></a>
**L2424** — <code>    float scale=1, x=0, y=0;</code>

Initialize a neutral transform before computing the window-specific fit.

<a id="l2425"></a>
**L2425** — <code>    /// Fit all controls inside the framebuffer, preserving proportions on resize.</code>

Documentation fragment: “Fit all controls inside the framebuffer, preserving proportions on resize.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2426"></a>
**L2426** — <code>    UiTransform(int width,int height) {</code>

Construct the transform from framebuffer width and height, not GLFW window-coordinate dimensions.

<a id="l2427"></a>
**L2427** — <code>        scale=std::max(0.001f,std::min(width/UI_WIDTH,height/UI_HEIGHT));</code>

Fit both UI axes using the smaller scale and avoid a zero scale in minimized/tiny dimensions.

<a id="l2428"></a>
**L2428** — <code>        x=(width-UI_WIDTH*scale)/2; y=(height-UI_HEIGHT*scale)/2;</code>

Center the logical canvas in any unused horizontal or vertical space.

<a id="l2429"></a>
**L2429** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2430"></a>
**L2430** — <code>    /// Translate GLFW's window coordinates through framebuffer scaling to the UI.</code>

Documentation fragment: “Translate GLFW's window coordinates through framebuffer scaling to the UI.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2431"></a>
**L2431** — <code>    std::pair&lt;float,float&gt; pointer(double px,double py,int ww,int wh,int fw,int fh) const {</code>

Convert a GLFW cursor position to UI coordinates, taking both window size and framebuffer size to account for high-DPI displays.

<a id="l2432"></a>
**L2432** — <code>        if(ww&lt;=0||wh&lt;=0||fw&lt;=0||fh&lt;=0) return {-1,-1};</code>

Return an off-canvas sentinel when any dimension is invalid, avoiding division by zero and accidental activation.

<a id="l2433"></a>
**L2433** — <code>        return {float((px*fw/ww-x)/scale),float((py*fh/wh-y)/scale)};</code>

First convert window coordinates to framebuffer pixels, then undo letterboxing and scale to recover logical UI coordinates.

<a id="l2434"></a>
**L2434** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2435"></a>
**L2435** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2436"></a>
**L2436** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2437"></a>
**L2437** — <code>struct SetupForm {</code>

Hold editable setup values as strings so temporary empty/invalid typing states can exist without constructing a game.

<a id="l2438"></a>
**L2438** — <code>    Difficulty difficulty=Difficulty::Medium;</code>

Preselect Medium difficulty unless main passes a different command-line choice.

<a id="l2439"></a>
**L2439** — <code>    std::string width="31",height="21",count="5";</code>

Default to width 31, height 21, and five monsters. These defaults are custom dimensions and do not select the 63×63 Standard preset.

<a id="l2440"></a>
**L2440** — <code>    /// Accept a complete bounded decimal field; empty or partial edits stay invalid.</code>

Documentation fragment: “Accept a complete bounded decimal field; empty or partial edits stay invalid.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2441"></a>
**L2441** — <code>    static bool number(const std::string&amp; text,int minimum,int maximum,int&amp; result) {</code>

Parse a bounded decimal field into an output integer, reporting validity without throwing.

<a id="l2442"></a>
**L2442** — <code>        if(text.empty()||text.size()&gt;3) return false;</code>

Reject empty strings and values requiring more than three characters, matching the largest supported dimension 501.

<a id="l2443"></a>
**L2443** — <code>        int value=0;</code>

Initialize the accumulator used by the digit-by-digit parser.

<a id="l2444"></a>
**L2444** — <code>        for(char c:text) { if(c&lt;'0'||c&gt;'9') return false; value=value*10+c-'0'; }</code>

Require ASCII digits only and accumulate their decimal value; signs, whitespace, and separators are rejected.

<a id="l2445"></a>
**L2445** — <code>        if(value&lt;minimum||value&gt;maximum) return false;</code>

Reject values outside the caller-supplied inclusive range.

<a id="l2446"></a>
**L2446** — <code>        result=value; return true;</code>

Assign the output only after complete validation and report success.

<a id="l2447"></a>
**L2447** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2448"></a>
**L2448** — <code>    /// Return normalized game dimensions only after every field validates.</code>

Documentation fragment: “Return normalized game dimensions only after every field validates.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2449"></a>
**L2449** — <code>    bool values(int&amp; rows,int&amp; cols,int&amp; monsters) const {</code>

Validate the three setup fields and obtain normalized integer game settings.

<a id="l2450"></a>
**L2450** — <code>        if(!number(height,5,501,rows)||!number(width,5,501,cols)||!number(count,0,10,monsters)) return false;</code>

Require height/width in 5–501 and monster count in 0–10; short-circuit on the first invalid field.

<a id="l2451"></a>
**L2451** — <code>        rows+=rows%2==0; cols+=cols%2==0; return true;</code>

Round even dimensions upward to the next odd value, preserving the maze generator's room/wall lattice, then report success.

<a id="l2452"></a>
**L2452** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2453"></a>
**L2453** — <code>    /// Explain the first invalid field directly in the setup screen.</code>

Documentation fragment: “Explain the first invalid field directly in the setup screen.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2454"></a>
**L2454** — <code>    std::string error() const {</code>

Return the most relevant validation message for the current edit state.

<a id="l2455"></a>
**L2455** — <code>        int ignored=0;</code>

Provide a scratch integer for validations whose parsed value is not otherwise needed.

<a id="l2456"></a>
**L2456** — <code>        if(!number(width,5,501,ignored)) return "WIDTH MUST BE BETWEEN 5 AND 501.";</code>

Prioritize an invalid width and return its explicit range requirement.

<a id="l2457"></a>
**L2457** — <code>        if(!number(height,5,501,ignored)) return "HEIGHT MUST BE BETWEEN 5 AND 501.";</code>

Next report an invalid height with the same supported range.

<a id="l2458"></a>
**L2458** — <code>        if(!number(count,0,10,ignored)) return "MONSTERS MUST BE BETWEEN 0 AND 10.";</code>

Report an invalid monster count with the 0–10 limit.

<a id="l2459"></a>
**L2459** — <code>        return {};</code>

An empty string means the fields are valid and no range error needs displaying.

<a id="l2460"></a>
**L2460** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2461"></a>
**L2461** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2462"></a>
**L2462** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2463"></a>
**L2463** — <code>// Input state is separate from Maze, so menu actions cannot move actors or emit scent.</code>

Documentation fragment: “Input state is separate from Maze, so menu actions cannot move actors or emit scent.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

## UI state, widgets, and interaction

<a id="l2464"></a>
**L2464** — <code>struct UiState {</code>

Contain UI interaction state without owning the maze simulation or advancing its clock.

<a id="l2465"></a>
**L2465** — <code>    Screen screen=Screen::Setup;</code>

Start on the setup screen.

<a id="l2466"></a>
**L2466** — <code>    SetupForm form;</code>

Keep the editable setup form across screen transitions and replay requests.

<a id="l2467"></a>
**L2467** — <code>    Control focus=Control::Start,hover=Control::None,pressed=Control::None;</code>

Start keyboard focus on Enter the Maze; pointer hover and pressed-control state begin empty.

<a id="l2468"></a>
**L2468** — <code>    bool replace_field=true, quit=false;</code>

Initial numeric focus replaces the whole field; quit starts false, and the message field holds recoverable setup errors.

<a id="l2469"></a>
**L2469** — <code>    std::string message;</code>

Store user-visible setup/generation feedback independently of the editable form fields.

<a id="l2470"></a>
**L2470** — <code>    /// Identify controls which accept decimal text input.</code>

Documentation fragment: “Identify controls which accept decimal text input.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2471"></a>
**L2471** — <code>    static bool is_field(Control id) { return id==Control::Width||id==Control::Height||id==Control::Count; }</code>

Identify the three numeric fields, used consistently by typing, selection, and keyboard handling.

<a id="l2472"></a>
**L2472** — <code>    /// Obtain the selected edit buffer without changing the other fields.</code>

Documentation fragment: “Obtain the selected edit buffer without changing the other fields.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2473"></a>
**L2473** — <code>    std::string&amp; field(Control id) {</code>

Return a mutable reference to the string associated with a numeric control.

<a id="l2474"></a>
**L2474** — <code>        if(id==Control::Width) return form.width;</code>

Width selects the width string.

<a id="l2475"></a>
**L2475** — <code>        if(id==Control::Height) return form.height;</code>

Height selects the height string.

<a id="l2476"></a>
**L2476** — <code>        return form.count;</code>

The remaining supported field is monster count; callers must first ensure the ID denotes a field.

<a id="l2477"></a>
**L2477** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2478"></a>
**L2478** — <code>    /// Change screens with no stale held mouse button or text edit state.</code>

Documentation fragment: “Change screens with no stale held mouse button or text edit state.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2479"></a>
**L2479** — <code>    void show(Screen next) {</code>

Transition to a new screen while resetting transient pointer/focus state.

<a id="l2480"></a>
**L2480** — <code>        screen=next; hover=pressed=Control::None; replace_field=true; message.clear();</code>

Assign the screen, clear hover/pressed, restore replace-on-first-digit, and clear stale error text.

<a id="l2481"></a>
**L2481** — <code>        focus=next==Screen::Setup?Control::Start:next==Screen::Paused?Control::Resume:</code>

Choose a useful default action for Setup and Paused instead of retaining focus on a now-invisible widget.

<a id="l2482"></a>
**L2482** — <code>              next==Screen::Result?Control::Again:Control::None;</code>

Choose Play Again for Result and no keyboard widget for Loading/Playing.

<a id="l2483"></a>
**L2483** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2484"></a>
**L2484** — <code>    /// Describe every interactive control in visual and keyboard navigation order.</code>

Documentation fragment: “Describe every interactive control in visual and keyboard navigation order.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2485"></a>
**L2485** — <code>    std::vector&lt;UiWidget&gt; widgets(const Maze* maze=nullptr) const {</code>

Construct the current screen's widget list from live UI and optional maze state; the same list governs drawing and interaction.

<a id="l2486"></a>
**L2486** — <code>        if(screen==Screen::Setup) {</code>

Build setup controls when the form is visible.

<a id="l2487"></a>
**L2487** — <code>            int r=0,c=0,n=0; bool valid=form.values(r,c,n);</code>

Validate a temporary parsed configuration so Start can be disabled while input is incomplete or invalid.

<a id="l2488"></a>
**L2488** — <code>            return {</code>

Return the setup list in keyboard-tab order; later list entries are not a separate retained widget tree.

<a id="l2489"></a>
**L2489** — <code>                {Control::Low,{50,158,206,116},"LOW",form.difficulty==Difficulty::Low},</code>

Define the Low control with logical rectangle (x,y,width,height) = (50,158,206,116), selected exactly when the form difficulty is Low. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2490"></a>
**L2490** — <code>                {Control::Medium,{272,158,206,116},"MEDIUM",form.difficulty==Difficulty::Medium},</code>

Define the Medium control with logical rectangle (x,y,width,height) = (272,158,206,116), selected exactly when the form difficulty is Medium. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2491"></a>
**L2491** — <code>                {Control::High,{494,158,206,116},"HIGH",form.difficulty==Difficulty::High},</code>

Define the High control with logical rectangle (x,y,width,height) = (494,158,206,116), selected exactly when the form difficulty is High. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2492"></a>
**L2492** — <code>                {Control::Small,{70,342,142,42},"SMALL",r==33&amp;&amp;c==33},</code>

Define the Small control with logical rectangle (x,y,width,height) = (70,342,142,42), selected only for valid normalized 33×33 dimensions. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2493"></a>
**L2493** — <code>                {Control::Standard,{224,342,142,42},"STANDARD",r==63&amp;&amp;c==63},</code>

Define the Standard control with logical rectangle (x,y,width,height) = (224,342,142,42), selected only for valid normalized 63×63 dimensions. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2494"></a>
**L2494** — <code>                {Control::Large,{378,342,142,42},"LARGE",r==101&amp;&amp;c==101},</code>

Define the Large control with logical rectangle (x,y,width,height) = (378,342,142,42), selected only for valid normalized 101×101 dimensions. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2495"></a>
**L2495** — <code>                {Control::WidthMinus,{70,422,42,44},"-"},</code>

Define the WidthMinus control with logical rectangle (x,y,width,height) = (70,422,42,44), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2496"></a>
**L2496** — <code>                {Control::Width,{118,422,116,44},form.width},</code>

Define the Width control with logical rectangle (x,y,width,height) = (118,422,116,44), displaying the editable width string. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2497"></a>
**L2497** — <code>                {Control::WidthPlus,{240,422,42,44},"+"},</code>

Define the WidthPlus control with logical rectangle (x,y,width,height) = (240,422,42,44), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2498"></a>
**L2498** — <code>                {Control::HeightMinus,{370,422,42,44},"-"},</code>

Define the HeightMinus control with logical rectangle (x,y,width,height) = (370,422,42,44), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2499"></a>
**L2499** — <code>                {Control::Height,{418,422,116,44},form.height},</code>

Define the Height control with logical rectangle (x,y,width,height) = (418,422,116,44), displaying the editable height string. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2500"></a>
**L2500** — <code>                {Control::HeightPlus,{540,422,42,44},"+"},</code>

Define the HeightPlus control with logical rectangle (x,y,width,height) = (540,422,42,44), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2501"></a>
**L2501** — <code>                {Control::CountMinus,{70,554,44,46},"-"},</code>

Define the CountMinus control with logical rectangle (x,y,width,height) = (70,554,44,46), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2502"></a>
**L2502** — <code>                {Control::Count,{122,554,90,46},form.count},</code>

Define the Count control with logical rectangle (x,y,width,height) = (122,554,90,46), displaying the editable monster-count string. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2503"></a>
**L2503** — <code>                {Control::CountPlus,{220,554,44,46},"+"},</code>

Define the CountPlus control with logical rectangle (x,y,width,height) = (220,554,44,46), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2504"></a>
**L2504** — <code>                {Control::Start,{50,658,500,56},"ENTER THE MAZE",false,valid},</code>

Define the Start control with logical rectangle (x,y,width,height) = (50,658,500,56), enabled only when all fields validate. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2505"></a>
**L2505** — <code>                {Control::Quit,{568,658,132,56},"QUIT"}</code>

Define the Quit control with logical rectangle (x,y,width,height) = (568,658,132,56), using the displayed literal label and default enabled state. This same geometry is used for drawing, hit testing, and focus traversal; changing its rectangle updates all three.

<a id="l2506"></a>
**L2506** — <code>            };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2507"></a>
**L2507** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2508"></a>
**L2508** — <code>        if(screen==Screen::Playing) return {</code>

Playing exposes a small HUD control set rather than the setup form.

<a id="l2509"></a>
**L2509** — <code>            {Control::AutoSolve,{560,22,260,48},maze&amp;&amp;maze-&gt;auto_solve?"AUTO-SOLVE: ON":"AUTO-SOLVE: OFF",maze&amp;&amp;maze-&gt;auto_solve},</code>

Create the auto-solve toggle; its label and selected style reflect the live maze's automatic-solving flag.

<a id="l2510"></a>
**L2510** — <code>            {Control::Pause,{836,22,130,48},"PAUSE"}</code>

Create the Pause button in the upper HUD.

<a id="l2511"></a>
**L2511** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2512"></a>
**L2512** — <code>        if(screen==Screen::Paused) return {</code>

Paused exposes a modal menu with three enabled actions.

<a id="l2513"></a>
**L2513** — <code>            {Control::Resume,{325,354,350,48},"RESUME"},</code>

Resume continues the existing maze and its frozen simulation clock.

<a id="l2514"></a>
**L2514** — <code>            {Control::Setup,{325,416,350,48},"NEW GAME"},</code>

Change Setup returns to configuration; the outer application discards the prior maze before starting another one.

<a id="l2515"></a>
**L2515** — <code>            {Control::Quit,{325,478,350,48},"QUIT"}</code>

Quit requests application shutdown.

<a id="l2516"></a>
**L2516** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2517"></a>
**L2517** — <code>        if(screen==Screen::Result) return {</code>

Result exposes replay/setup/quit controls rather than movement controls.

<a id="l2518"></a>
**L2518** — <code>            {Control::Again,{325,414,350,48},"PLAY AGAIN"},</code>

Play Again reuses the stored form; whether it uses the same seed depends on fixed-seed mode.

<a id="l2519"></a>
**L2519** — <code>            {Control::Setup,{325,476,350,48},"CHANGE SETUP"},</code>

Change Setup allows editing difficulty, dimensions, and monster count before another generation.

<a id="l2520"></a>
**L2520** — <code>            {Control::Quit,{325,538,350,48},"QUIT"}</code>

Quit is also available from the result screen.

<a id="l2521"></a>
**L2521** — <code>        };</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2522"></a>
**L2522** — <code>        return {};</code>

Loading has no interactive widgets, preventing a second Start request during generation.

<a id="l2523"></a>
**L2523** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2524"></a>
**L2524** — <code>    /// Focus a control and select its numeric text for convenient replacement.</code>

Documentation fragment: “Focus a control and select its numeric text for convenient replacement.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2525"></a>
**L2525** — <code>    void select(Control id) { focus=id; replace_field=true; }</code>

Move keyboard focus and arm whole-field replacement for the next digit typed.

<a id="l2526"></a>
**L2526** — <code>    /// Move keyboard focus through enabled controls, wrapping at both ends.</code>

Documentation fragment: “Move keyboard focus through enabled controls, wrapping at both ends.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2527"></a>
**L2527** — <code>    void tab(bool backwards,const Maze* maze=nullptr) {</code>

Advance or reverse keyboard focus through enabled widgets only.

<a id="l2528"></a>
**L2528** — <code>        auto all=widgets(maze);std::vector&lt;Control&gt; enabled;</code>

Build the current widget list and a temporary list of focusable IDs.

<a id="l2529"></a>
**L2529** — <code>        for(const auto&amp; w:all) if(w.enabled) enabled.push_back(w.id);</code>

Skip disabled controls, notably Start when form values are invalid.

<a id="l2530"></a>
**L2530** — <code>        if(enabled.empty()) return;</code>

Do nothing if the active screen has no enabled widgets.

<a id="l2531"></a>
**L2531** — <code>        auto found=std::find(enabled.begin(),enabled.end(),focus);</code>

Find the currently focused ID in the enabled sequence.

<a id="l2532"></a>
**L2532** — <code>        int at=found==enabled.end()?(backwards?0:-1):int(found-enabled.begin());</code>

Choose an initial index so Tab from an absent focus lands on the first item and reverse Tab on the last.

<a id="l2533"></a>
**L2533** — <code>        select(enabled[(at+(backwards?-1:1)+int(enabled.size()))%enabled.size()]);</code>

Apply direction, wrap modulo item count, and select the destination.

<a id="l2534"></a>
**L2534** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2535"></a>
**L2535** — <code>    /// Accept only ASCII decimal digits, with a three-digit bound on each buffer.</code>

Documentation fragment: “Accept only ASCII decimal digits, with a three-digit bound on each buffer.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2536"></a>
**L2536** — <code>    void digit(unsigned int codepoint) {</code>

Handle one text-input codepoint for numeric editing.

<a id="l2537"></a>
**L2537** — <code>        if(screen!=Screen::Setup||!is_field(focus)||codepoint&lt;'0'||codepoint&gt;'9') return;</code>

Accept only ASCII decimal digits while Setup is visible and focus is on a numeric field; other Unicode characters are ignored.

<a id="l2538"></a>
**L2538** — <code>        auto&amp; text=field(focus);</code>

Obtain the active field by reference.

<a id="l2539"></a>
**L2539** — <code>        if(replace_field) { text.clear();replace_field=false; }</code>

On the first digit after field selection, clear the old value and end replacement mode.

<a id="l2540"></a>
**L2540** — <code>        if(text.size()&lt;3) text.push_back(char(codepoint));</code>

Append only while fewer than three characters are present; further digits are ignored rather than overflowing a parser.

<a id="l2541"></a>
**L2541** — <code>        message.clear();</code>

Clear prior generation/error messages because the user has begun a new edit.

<a id="l2542"></a>
**L2542** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2543"></a>
**L2543** — <code>    /// Clear the selected text or remove the last digit from the active field.</code>

Documentation fragment: “Clear the selected text or remove the last digit from the active field.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2544"></a>
**L2544** — <code>    void erase(bool all=false) {</code>

Implement Backspace or whole-field deletion without leaving the setup model.

<a id="l2545"></a>
**L2545** — <code>        if(screen!=Screen::Setup||!is_field(focus)) return;</code>

Ignore deletion outside an active setup numeric field.

<a id="l2546"></a>
**L2546** — <code>        auto&amp; text=field(focus);</code>

Obtain the selected numeric string.

<a id="l2547"></a>
**L2547** — <code>        if(all||replace_field) text.clear(); else if(!text.empty()) text.pop_back();</code>

Delete everything for explicit Delete or pending selection; otherwise remove one trailing character if present.

<a id="l2548"></a>
**L2548** — <code>        replace_field=false; message.clear();</code>

End whole-field replacement and clear stale message text.

<a id="l2549"></a>
**L2549** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2550"></a>
**L2550** — <code>    /// Step dimensions through odd sizes and keep the monster count inside 0..10.</code>

Documentation fragment: “Step dimensions through odd sizes and keep the monster count inside 0..10.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2551"></a>
**L2551** — <code>    void step(Control id,int direction) {</code>

Adjust a numeric field using its +/- button or Up/Down while editing.

<a id="l2552"></a>
**L2552** — <code>        int value=0;const bool count=id==Control::Count;</code>

Distinguish monster count from maze dimensions because their bounds and step sizes differ.

<a id="l2553"></a>
**L2553** — <code>        int minimum=count?0:5,maximum=count?10:501;</code>

Use 0–10 for count and 5–501 for dimensions.

<a id="l2554"></a>
**L2554** — <code>        if(!SetupForm::number(field(id),minimum,maximum,value)) value=count?5:31;</code>

If the current text is invalid, start adjustment from count 5 or dimension 31 instead of failing the interaction.

<a id="l2555"></a>
**L2555** — <code>        if(!count&amp;&amp;value%2==0) value+=direction&gt;0?1:-1; else value+=direction*(count?1:2);</code>

Count moves by one; an even dimension moves to an adjacent odd value, and an already-odd dimension moves by two.

<a id="l2556"></a>
**L2556** — <code>        field(id)=std::to_string(std::clamp(value,minimum,maximum));</code>

Clamp the adjusted value to supported bounds and replace the text with its decimal representation.

<a id="l2557"></a>
**L2557** — <code>        replace_field=true;message.clear();</code>

Rearm replacement on subsequent typing and clear stale error text.

<a id="l2558"></a>
**L2558** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2559"></a>
**L2559** — <code>    /// Commit settings and defer generation until the loading screen can be drawn.</code>

Documentation fragment: “Commit settings and defer generation until the loading screen can be drawn.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2560"></a>
**L2560** — <code>    void request_start() {</code>

Validate a request to generate a game before entering Loading.

<a id="l2561"></a>
**L2561** — <code>        int r=0,c=0,n=0;</code>

Allocate temporary parsed settings; no Maze is created here.

<a id="l2562"></a>
**L2562** — <code>        if(!form.values(r,c,n)) { message=form.error();return; }</code>

If validation fails, keep the screen and show the form error.

<a id="l2563"></a>
**L2563** — <code>        form.width=std::to_string(c);form.height=std::to_string(r);form.count=std::to_string(n);</code>

Write the normalized odd dimensions and parsed count back into the visible text fields.

<a id="l2564"></a>
**L2564** — <code>        show(Screen::Loading);</code>

Enter Loading; GameApplication schedules generation after that screen has been drawn.

<a id="l2565"></a>
**L2565** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2566"></a>
**L2566** — <code>    /// Apply one enabled button action; gameplay receives only explicit auto toggles.</code>

Documentation fragment: “Apply one enabled button action; gameplay receives only explicit auto toggles.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2567"></a>
**L2567** — <code>    void activate(Control id,Maze* maze=nullptr) {</code>

Dispatch one enabled visible control, optionally operating on the current maze.

<a id="l2568"></a>
**L2568** — <code>        auto all=widgets(maze);</code>

Rebuild the widget list to check the request against current screen state.

<a id="l2569"></a>
**L2569** — <code>        auto found=std::find_if(all.begin(),all.end(),[&amp;](const UiWidget&amp; w){return w.id==id&amp;&amp;w.enabled;});</code>

Search for the requested ID only among enabled widgets.

<a id="l2570"></a>
**L2570** — <code>        if(found==all.end()) return;</code>

Ignore absent/disabled IDs, preventing hidden-button activation and invalid Start requests.

<a id="l2571"></a>
**L2571** — <code>        select(id);</code>

Set focus to the activated control before applying its action.

<a id="l2572"></a>
**L2572** — <code>        switch(id) {</code>

Dispatch the stable Control enum rather than comparing user-facing strings.

<a id="l2573"></a>
**L2573** — <code>            case Control::Low:form.difficulty=Difficulty::Low;break;</code>

Select Low; the next generated Maze receives that profile.

<a id="l2574"></a>
**L2574** — <code>            case Control::Medium:form.difficulty=Difficulty::Medium;break;</code>

Select Medium, the reference profile.

<a id="l2575"></a>
**L2575** — <code>            case Control::High:form.difficulty=Difficulty::High;break;</code>

Select High, the strongest pursuit profile.

<a id="l2576"></a>
**L2576** — <code>            case Control::Small:form.width=form.height="33";break;</code>

Set both dimension strings to 33 for the Small preset.

<a id="l2577"></a>
**L2577** — <code>            case Control::Standard:form.width=form.height="63";break;</code>

Set both dimension strings to 63 for the Standard preset.

<a id="l2578"></a>
**L2578** — <code>            case Control::Large:form.width=form.height="101";break;</code>

Set both dimension strings to 101 for the Large preset.

<a id="l2579"></a>
**L2579** — <code>            case Control::WidthMinus:step(Control::Width,-1);break;</code>

Decrease width using the validated odd-dimension stepping rule.

<a id="l2580"></a>
**L2580** — <code>            case Control::WidthPlus:step(Control::Width,1);break;</code>

Increase width using the same stepping rule.

<a id="l2581"></a>
**L2581** — <code>            case Control::HeightMinus:step(Control::Height,-1);break;</code>

Decrease height independently of width, permitting custom rectangles.

<a id="l2582"></a>
**L2582** — <code>            case Control::HeightPlus:step(Control::Height,1);break;</code>

Increase height independently of width.

<a id="l2583"></a>
**L2583** — <code>            case Control::CountMinus:step(Control::Count,-1);break;</code>

Decrease monster count, clamped at zero.

<a id="l2584"></a>
**L2584** — <code>            case Control::CountPlus:step(Control::Count,1);break;</code>

Increase monster count, clamped at ten.

<a id="l2585"></a>
**L2585** — <code>            case Control::Start:case Control::Again:request_start();break;</code>

Start and Play Again share form validation and the transition to Loading.

<a id="l2586"></a>
**L2586** — <code>            case Control::Pause:</code>

Handle the HUD's Pause control as an explicit input-state transition.

<a id="l2587"></a>
**L2587** — <code>                if (maze) maze-&gt;cancel_held_move();</code>

Disarm scheduled manual movement before leaving the Playing screen.

<a id="l2588"></a>
**L2588** — <code>                show(Screen::Paused);break;</code>

Switch to the pause UI and finish processing this control.

<a id="l2589"></a>
**L2589** — <code>            case Control::Resume:show(Screen::Playing);break;</code>

Resume returns to Playing without resetting maze progress.

<a id="l2590"></a>
**L2590** — <code>            case Control::Setup:show(Screen::Setup);break;</code>

Change Setup returns to configuration; actual game-resource disposal belongs to the application loop.

<a id="l2591"></a>
**L2591** — <code>            case Control::AutoSolve:if(maze)maze-&gt;set_auto_solve(!maze-&gt;auto_solve);break;</code>

Toggle automatic solving only when a Maze pointer is available, using the simulation's own state-transition method.

<a id="l2592"></a>
**L2592** — <code>            case Control::Quit:quit=true;break;</code>

Set the quit request consumed by the outer window loop.

<a id="l2593"></a>
**L2593** — <code>            default:break;</code>

Numeric fields and unhandled IDs have no additional action beyond focus selection.

<a id="l2594"></a>
**L2594** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2595"></a>
**L2595** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2596"></a>
**L2596** — <code>    /// Hit test against exactly the rectangles used by the renderer.</code>

Documentation fragment: “Hit test against exactly the rectangles used by the renderer.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2597"></a>
**L2597** — <code>    Control hit(float x,float y,const Maze* maze=nullptr) const {</code>

Find the control under a logical UI point.

<a id="l2598"></a>
**L2598** — <code>        for(const auto&amp; w:widgets(maze)) if(w.enabled&amp;&amp;w.box.contains(x,y)) return w.id;</code>

Return the first enabled widget whose rectangle contains the point; disabled controls cannot be clicked.

<a id="l2599"></a>
**L2599** — <code>        return Control::None;</code>

Return None when the pointer is outside every enabled widget.

<a id="l2600"></a>
**L2600** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2601"></a>
**L2601** — <code>    /// Trigger a click only when press and release both land on the same control.</code>

Documentation fragment: “Trigger a click only when press and release both land on the same control.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2602"></a>
**L2602** — <code>    void pointer_button(bool down,float x,float y,Maze* maze=nullptr) {</code>

Track a press/release pair for one logical pointer location.

<a id="l2603"></a>
**L2603** — <code>        Control target=hit(x,y,maze);</code>

Hit-test against the current screen and maze state.

<a id="l2604"></a>
**L2604** — <code>        if(down) { pressed=target; if(target!=Control::None)select(target); }</code>

On press, remember the target and focus it if present; activation waits for release.

<a id="l2605"></a>
**L2605** — <code>        else { Control origin=pressed;pressed=Control::None;if(origin!=Control::None&amp;&amp;origin==target)activate(target,maze); }</code>

On release, clear the pressed state and activate only if press and release hit the same nonempty control, avoiding drag-out activation.

<a id="l2606"></a>
**L2606** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2607"></a>
**L2607** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2608"></a>
**L2608** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Embedded bitmap font

<a id="l2609"></a>
**L2609** — <code>/// Resolve a small embedded 5x7 font. UI text uses uppercase, digits, and punctuation.</code>

Documentation fragment: “Resolve a small embedded 5x7 font. UI text uses uppercase, digits, and punctuation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2610"></a>
**L2610** — <code>std::array&lt;unsigned char,7&gt; ui_glyph(char c) {</code>

Resolve a character to seven five-bit font rows; this embedded bitmap font needs no font files or text library.

<a id="l2611"></a>
**L2611** — <code>    switch(c) {</code>

Select the glyph by character. UiRenderer uppercases labels before calling this function.

<a id="l2612"></a>
**L2612** — <code>        case 'a':return {0,0,14,1,15,17,15};</code>

Define the seven bitmap rows of the lowercase a needed to display the exact release suffix.

<a id="l2613"></a>
**L2613** — <code>        case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};</code>

Define embedded font data: 'A' uses top-to-bottom five-bit rows 01110, 10001, 10001, 11111, 10001, 10001, 10001; 'B' uses top-to-bottom five-bit rows 11110, 10001, 10001, 11110, 10001, 10001, 11110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2614"></a>
**L2614** — <code>        case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};</code>

Define embedded font data: 'C' uses top-to-bottom five-bit rows 01110, 10001, 10000, 10000, 10000, 10001, 01110; 'D' uses top-to-bottom five-bit rows 11110, 10001, 10001, 10001, 10001, 10001, 11110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2615"></a>
**L2615** — <code>        case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};</code>

Define embedded font data: 'E' uses top-to-bottom five-bit rows 11111, 10000, 10000, 11110, 10000, 10000, 11111; 'F' uses top-to-bottom five-bit rows 11111, 10000, 10000, 11110, 10000, 10000, 10000. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2616"></a>
**L2616** — <code>        case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};</code>

Define embedded font data: 'G' uses top-to-bottom five-bit rows 01110, 10001, 10000, 10111, 10001, 10001, 01111; 'H' uses top-to-bottom five-bit rows 10001, 10001, 10001, 11111, 10001, 10001, 10001. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2617"></a>
**L2617** — <code>        case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};</code>

Define embedded font data: 'I' uses top-to-bottom five-bit rows 01110, 00100, 00100, 00100, 00100, 00100, 01110; 'J' uses top-to-bottom five-bit rows 00111, 00010, 00010, 00010, 10010, 10010, 01100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2618"></a>
**L2618** — <code>        case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};</code>

Define embedded font data: 'K' uses top-to-bottom five-bit rows 10001, 10010, 10100, 11000, 10100, 10010, 10001; 'L' uses top-to-bottom five-bit rows 10000, 10000, 10000, 10000, 10000, 10000, 11111. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2619"></a>
**L2619** — <code>        case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};</code>

Define embedded font data: 'M' uses top-to-bottom five-bit rows 10001, 11011, 10101, 10101, 10001, 10001, 10001; 'N' uses top-to-bottom five-bit rows 10001, 11001, 10101, 10011, 10001, 10001, 10001. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2620"></a>
**L2620** — <code>        case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};</code>

Define embedded font data: 'O' uses top-to-bottom five-bit rows 01110, 10001, 10001, 10001, 10001, 10001, 01110; 'P' uses top-to-bottom five-bit rows 11110, 10001, 10001, 11110, 10000, 10000, 10000. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2621"></a>
**L2621** — <code>        case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};</code>

Define embedded font data: 'Q' uses top-to-bottom five-bit rows 01110, 10001, 10001, 10001, 10101, 10010, 01101; 'R' uses top-to-bottom five-bit rows 11110, 10001, 10001, 11110, 10100, 10010, 10001. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2622"></a>
**L2622** — <code>        case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};</code>

Define embedded font data: 'S' uses top-to-bottom five-bit rows 01111, 10000, 10000, 01110, 00001, 00001, 11110; 'T' uses top-to-bottom five-bit rows 11111, 00100, 00100, 00100, 00100, 00100, 00100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2623"></a>
**L2623** — <code>        case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};</code>

Define embedded font data: 'U' uses top-to-bottom five-bit rows 10001, 10001, 10001, 10001, 10001, 10001, 01110; 'V' uses top-to-bottom five-bit rows 10001, 10001, 10001, 10001, 10001, 01010, 00100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2624"></a>
**L2624** — <code>        case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};</code>

Define embedded font data: 'W' uses top-to-bottom five-bit rows 10001, 10001, 10001, 10101, 10101, 10101, 01010; 'X' uses top-to-bottom five-bit rows 10001, 10001, 01010, 00100, 01010, 10001, 10001. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2625"></a>
**L2625** — <code>        case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};</code>

Define embedded font data: 'Y' uses top-to-bottom five-bit rows 10001, 10001, 01010, 00100, 00100, 00100, 00100; 'Z' uses top-to-bottom five-bit rows 11111, 00001, 00010, 00100, 01000, 10000, 11111. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2626"></a>
**L2626** — <code>        case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};</code>

Define embedded font data: '0' uses top-to-bottom five-bit rows 01110, 10001, 10011, 10101, 11001, 10001, 01110; '1' uses top-to-bottom five-bit rows 00100, 01100, 00100, 00100, 00100, 00100, 01110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2627"></a>
**L2627** — <code>        case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};</code>

Define embedded font data: '2' uses top-to-bottom five-bit rows 01110, 10001, 00001, 00010, 00100, 01000, 11111; '3' uses top-to-bottom five-bit rows 11110, 00001, 00001, 01110, 00001, 00001, 11110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2628"></a>
**L2628** — <code>        case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};</code>

Define embedded font data: '4' uses top-to-bottom five-bit rows 00010, 00110, 01010, 10010, 11111, 00010, 00010; '5' uses top-to-bottom five-bit rows 11111, 10000, 10000, 11110, 00001, 00001, 11110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2629"></a>
**L2629** — <code>        case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};</code>

Define embedded font data: '6' uses top-to-bottom five-bit rows 01110, 10000, 10000, 11110, 10001, 10001, 01110; '7' uses top-to-bottom five-bit rows 11111, 00001, 00010, 00100, 01000, 01000, 01000. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2630"></a>
**L2630** — <code>        case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};</code>

Define embedded font data: '8' uses top-to-bottom five-bit rows 01110, 10001, 10001, 01110, 10001, 10001, 01110; '9' uses top-to-bottom five-bit rows 01110, 10001, 10001, 01111, 00001, 00001, 01110. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2631"></a>
**L2631** — <code>        case ':':return {0,4,4,0,4,4,0};case '.':return {0,0,0,0,0,12,12};</code>

Define embedded font data: ':' uses top-to-bottom five-bit rows 00000, 00100, 00100, 00000, 00100, 00100, 00000; '.' uses top-to-bottom five-bit rows 00000, 00000, 00000, 00000, 00000, 01100, 01100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2632"></a>
**L2632** — <code>        case '-':return {0,0,0,31,0,0,0};case '+':return {0,4,4,31,4,4,0};</code>

Define embedded font data: '-' uses top-to-bottom five-bit rows 00000, 00000, 00000, 11111, 00000, 00000, 00000; '+' uses top-to-bottom five-bit rows 00000, 00100, 00100, 11111, 00100, 00100, 00000. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2633"></a>
**L2633** — <code>        case '/':return {1,2,2,4,8,8,16};case '|':return {4,4,4,4,4,4,4};</code>

Define embedded font data: '/' uses top-to-bottom five-bit rows 00001, 00010, 00010, 00100, 01000, 01000, 10000; '|' uses top-to-bottom five-bit rows 00100, 00100, 00100, 00100, 00100, 00100, 00100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2634"></a>
**L2634** — <code>        case '&gt;':return {16,8,4,2,4,8,16};case '&lt;':return {1,2,4,8,4,2,1};</code>

Define embedded font data: '>' uses top-to-bottom five-bit rows 10000, 01000, 00100, 00010, 00100, 01000, 10000; '<' uses top-to-bottom five-bit rows 00001, 00010, 00100, 01000, 00100, 00010, 00001. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2635"></a>
**L2635** — <code>        case '!':return {4,4,4,4,4,0,4};</code>

Define embedded font data: '!' uses top-to-bottom five-bit rows 00100, 00100, 00100, 00100, 00100, 00000, 00100. A 1 emits a foreground square; a 0 leaves the background visible. Bit 4 is the leftmost column.

<a id="l2636"></a>
**L2636** — <code>        case '(':return {2,4,8,8,8,4,2};case ')':return {8,4,2,2,2,4,8};</code>

Parenthesis glyphs, added so the HUD legend can show "(HOLD)".

<a id="l2637"></a>
**L2637** — <code>        case ' ':return {};default:return {14,17,1,2,4,0,4};</code>

A space has seven zero rows; unrecognized characters render a question-mark pattern rather than reading outside font data.

<a id="l2638"></a>
**L2638** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2639"></a>
**L2639** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2640"></a>
**L2640** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## OpenGL shader compilation and linking

<a id="l2641"></a>
**L2641** — <code>#ifndef MAZERUNNER_HEADLESS</code>

Exclude all OpenGL resource management and drawing code from MAZERUNNER_HEADLESS builds. Geometry construction and UI state above remain available to portable tests.

<a id="l2642"></a>
**L2642** — <code>/// Compile a shader and throw its diagnostic after releasing a failed object.</code>

Documentation fragment: “Compile a shader and throw its diagnostic after releasing a failed object.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2643"></a>
**L2643** — <code>GLuint compile_shader(GLenum type, const char* source) {</code>

Compile one GLSL shader stage in the current OpenGL context and return its owned object handle.

<a id="l2644"></a>
**L2644** — <code>    GLuint shader = glCreateShader(type);</code>

Allocate a shader object of the caller-supplied vertex or fragment type.

<a id="l2645"></a>
**L2645** — <code>    glShaderSource(shader, 1, &amp;source, nullptr); glCompileShader(shader);</code>

Supply one null-terminated source string and ask the driver to compile it.

<a id="l2646"></a>
**L2646** — <code>    GLint success = 0;</code>

Initialize the compile-status result before querying the driver.

<a id="l2647"></a>
**L2647** — <code>    glGetShaderiv(shader, GL_COMPILE_STATUS, &amp;success);</code>

Read GL_COMPILE_STATUS rather than assuming successful compilation.

<a id="l2648"></a>
**L2648** — <code>    if (!success) {</code>

Handle a rejected shader locally so callers do not inherit a leaked shader object.

<a id="l2649"></a>
**L2649** — <code>        char log[2048]{}; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);</code>

Retrieve up to 2,048 bytes of driver diagnostic text into a zero-initialized buffer.

<a id="l2650"></a>
**L2650** — <code>        glDeleteShader(shader);</code>

Delete the failed shader before propagating an exception.

<a id="l2651"></a>
**L2651** — <code>        throw std::runtime_error(std::string("Shader compilation failed: ") + log);</code>

Include the driver log in the runtime error, making GLSL incompatibilities diagnosable from the terminal.

<a id="l2652"></a>
**L2652** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2653"></a>
**L2653** — <code>    return shader;</code>

Transfer the successful shader handle to the caller, which must eventually delete it.

<a id="l2654"></a>
**L2654** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2655"></a>
**L2655** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2656"></a>
**L2656** — <code>/// Link the renderer's GLSL stages; all intermediate shaders are released.</code>

Documentation fragment: “Link the renderer's GLSL stages; all intermediate shaders are released.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2657"></a>
**L2657** — <code>GLuint create_shader_program() {</code>

Create the shared position/color shader program used by the world and UI renderers.

<a id="l2658"></a>
**L2658** — <code>    const char* vertex_source = R"glsl(</code>

Begin a C++ raw string containing the vertex shader; its text is compiled by the GPU driver, not the C++ compiler.

<a id="l2659"></a>
**L2659** — <code>#version 330 core</code>

Request GLSL 3.30 core syntax. This line is inside the raw string and is not a C++ preprocessor directive.

<a id="l2660"></a>
**L2660** — <code>layout(location = 0) in vec3 position;</code>

Bind incoming three-component positions to vertex attribute location zero.

<a id="l2661"></a>
**L2661** — <code>uniform mat4 viewProjection;</code>

Declare the camera/UI transform supplied as one uniform 4×4 matrix.

<a id="l2662"></a>
**L2662** — <code>layout(location = 1) in vec3 color;</code>

Bind incoming RGB colors to attribute location one, matching Vertex's color fields.

<a id="l2663"></a>
**L2663** — <code>out vec3 vertexColor;</code>

Export interpolated RGB to the fragment shader.

<a id="l2664"></a>
**L2664** — <code>// Project real wall height and billboard pixels through the same camera.</code>

Documentation fragment: “Project real wall height and billboard pixels through the same camera.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2665"></a>
**L2665** — <code>void main() { gl_Position = viewProjection * vec4(position, 1.0); vertexColor = color; }</code>

Transform each position to homogeneous clip space and forward its color. There are no normals, UVs, skeletal transforms, or lighting calculations.

<a id="l2666"></a>
**L2666** — <code>)glsl";</code>

Terminate the raw vertex-shader string.

<a id="l2667"></a>
**L2667** — <code>    const char* fragment_source = R"glsl(</code>

Begin the fragment-shader raw string.

<a id="l2668"></a>
**L2668** — <code>#version 330 core</code>

Use the same GLSL 3.30 core language version for the fragment stage.

<a id="l2669"></a>
**L2669** — <code>in vec3 vertexColor;</code>

Receive the vertex shader's interpolated RGB value.

<a id="l2670"></a>
**L2670** — <code>out vec4 fragmentColor;</code>

Declare the fragment's four-component color output.

<a id="l2671"></a>
**L2671** — <code>// Render opaque cell and entity colors without texture or lighting lookups.</code>

Documentation fragment: “Render opaque cell and entity colors without texture or lighting lookups.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2672"></a>
**L2672** — <code>void main() { fragmentColor = vec4(vertexColor, 1.0); }</code>

Write opaque RGB with alpha one. Pixel-art holes are omitted geometry rather than sampled alpha.

<a id="l2673"></a>
**L2673** — <code>)glsl";</code>

Terminate the raw fragment-shader string.

<a id="l2674"></a>
**L2674** — <code>    GLuint vs = 0, fs = 0, program = 0;</code>

Initialize all owned GL handles to zero so partial-construction cleanup can be conditional.

<a id="l2675"></a>
**L2675** — <code>    try {</code>

Start an exception-safe allocation/linking region.

<a id="l2676"></a>
**L2676** — <code>        vs = compile_shader(GL_VERTEX_SHADER, vertex_source);</code>

Compile the vertex stage, taking ownership of its returned handle.

<a id="l2677"></a>
**L2677** — <code>        fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);</code>

Compile the fragment stage; failure here still permits cleanup of the already-created vertex shader.

<a id="l2678"></a>
**L2678** — <code>        program = glCreateProgram();</code>

Allocate the program object.

<a id="l2679"></a>
**L2679** — <code>        glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);</code>

Attach both compiled stages and link their interfaces into an executable program.

<a id="l2680"></a>
**L2680** — <code>        GLint success = 0; glGetProgramiv(program, GL_LINK_STATUS, &amp;success);</code>

Query link status separately from individual shader compile status.

<a id="l2681"></a>
**L2681** — <code>        if (!success) {</code>

Treat an unsuccessful link as an error even when both stages compiled individually.

<a id="l2682"></a>
**L2682** — <code>            char log[2048]{}; glGetProgramInfoLog(program, sizeof(log), nullptr, log);</code>

Read the program-linker diagnostic into a bounded buffer.

<a id="l2683"></a>
**L2683** — <code>            throw std::runtime_error(std::string("Shader link failed: ") + log);</code>

Raise an exception carrying the linker log; the surrounding catch performs cleanup.

<a id="l2684"></a>
**L2684** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2685"></a>
**L2685** — <code>    } catch (...) {</code>

Catch any exception during allocation, compilation, or linking, regardless of exception type.

<a id="l2686"></a>
**L2686** — <code>        if (vs) glDeleteShader(vs);</code>

Delete a successfully created vertex shader on failure.

<a id="l2687"></a>
**L2687** — <code>        if (fs) glDeleteShader(fs);</code>

Delete a successfully created fragment shader on failure.

<a id="l2688"></a>
**L2688** — <code>        if (program) glDeleteProgram(program);</code>

Delete a partially created or failed program on failure.

<a id="l2689"></a>
**L2689** — <code>        throw;</code>

Rethrow the original exception after cleanup, preserving its diagnostic.

<a id="l2690"></a>
**L2690** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2691"></a>
**L2691** — <code>    glDeleteShader(vs); glDeleteShader(fs);</code>

Release the intermediate shader objects after successful linking; the linked program retains its executable stages.

<a id="l2692"></a>
**L2692** — <code>    return program;</code>

Return the program handle to its owning renderer.

<a id="l2693"></a>
**L2693** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2694"></a>
**L2694** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## World renderer and GPU ownership

<a id="l2695"></a>
**L2695** — <code>// Own GPU resources only while their OpenGL context remains current. All camera,</code>

Documentation fragment: “Own GPU resources only while their OpenGL context remains current. All camera,” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2696"></a>
**L2696** — <code>// facing and animation state belongs to the renderer, independent of Maze.</code>

Documentation fragment: “facing and animation state belongs to the renderer, independent of Maze.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2697"></a>
**L2697** — <code>class Renderer {</code>

Own the world-rendering GPU objects and purely visual camera/animation history.

<a id="l2698"></a>
**L2698** — <code>    GLuint program = 0;</code>

Store the linked world shader handle, initially unallocated.

<a id="l2699"></a>
**L2699** — <code>    GLint projection_location = -1;</code>

Cache the projection uniform location; −1 denotes an unavailable location until initialization.

<a id="l2700"></a>
**L2700** — <code>    std::array&lt;GLuint,2&gt; vaos{}, buffers{};</code>

Keep two VAO/VBO pairs: one static terrain batch and one per-frame dynamic batch.

<a id="l2701"></a>
**L2701** — <code>    GLsizei terrain_vertices = 0;</code>

Remember the static mesh's vertex count for glDrawArrays.

<a id="l2702"></a>
**L2702** — <code>    std::vector&lt;Vertex&gt; dynamic_vertices;</code>

Reuse CPU vector storage while rebuilding route, exit, monsters, and knight geometry each frame.

<a id="l2703"></a>
**L2703** — <code>    float zoom = 1.0f;</code>

Start with a whole-maze 1× camera fit.

<a id="l2704"></a>
**L2704** — <code>    int previous_player = -1;</code>

Use −1 to indicate that no previous rendered player position exists yet.

<a id="l2705"></a>
**L2705** — <code>    bool player_mirrored = false;</code>

Track the knight's last horizontal facing direction independently of simulation state.

<a id="l2706"></a>
**L2706** — <code>    Millis player_moved_at = -NEVER;</code>

Initialize the last-move timestamp far in the past so the knight is not initially treated as stepping.

<a id="l2707"></a>
**L2707** — <code>    std::vector&lt;int&gt; previous_monsters;</code>

Remember each monster's previous rendered grid cell to infer facing.

<a id="l2708"></a>
**L2708** — <code>    std::vector&lt;unsigned char&gt; monster_mirrored;</code>

Store one byte per monster for horizontal mirroring; this is visual history, not an AI field.

<a id="l2709"></a>
**L2709** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2710"></a>
**L2710** — <code>    /// Release partially initialized handles as well as a complete renderer.</code>

Documentation fragment: “Release partially initialized handles as well as a complete renderer.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2711"></a>
**L2711** — <code>    void release() {</code>

Centralize cleanup for normal destruction and partially completed construction.

<a id="l2712"></a>
**L2712** — <code>        glDeleteVertexArrays(2,vaos.data()); glDeleteBuffers(2,buffers.data());</code>

Delete both VAO/VBO pairs; zero handles are harmless for these deletion APIs.

<a id="l2713"></a>
**L2713** — <code>        if (program) glDeleteProgram(program);</code>

Delete the shader program if allocated.

<a id="l2714"></a>
**L2714** — <code>        vaos={}; buffers={}; program=0;</code>

Zero the resource handles so repeated cleanup cannot delete the same owned objects again.

<a id="l2715"></a>
**L2715** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2716"></a>
**L2716** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2717"></a>
**L2717** — <code>public:</code>

Set following class members to public access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l2718"></a>
**L2718** — <code>    /// Upload immutable wall geometry once; reserve the second batch for sprites</code>

Documentation fragment: “Upload immutable wall geometry once; reserve the second batch for sprites” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2719"></a>
**L2719** — <code>    /// and the complete route. No image loading libraries or asset paths are used.</code>

Documentation fragment: “and the complete route. No image loading libraries or asset paths are used.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2720"></a>
**L2720** — <code>    explicit Renderer(const Maze&amp; maze) {</code>

Construct the world renderer from immutable maze terrain while an OpenGL context is current.

<a id="l2721"></a>
**L2721** — <code>        auto terrain=build_wall_mesh(maze);</code>

Build the terrain triangles on the CPU before starting GPU allocation.

<a id="l2722"></a>
**L2722** — <code>        dynamic_vertices.reserve((MAX_MONSTERS+2)*20*20*6);</code>

Reserve enough dynamic vertices for fully opaque sprites for ten monsters, the knight, and exit; route/ring geometry may grow the vector further.

<a id="l2723"></a>
**L2723** — <code>        try {</code>

Begin GPU initialization with exception cleanup through release().

<a id="l2724"></a>
**L2724** — <code>            program=create_shader_program();</code>

Create the position/color shader program.

<a id="l2725"></a>
**L2725** — <code>            projection_location=glGetUniformLocation(program,"viewProjection");</code>

Find its viewProjection uniform once rather than looking it up every frame.

<a id="l2726"></a>
**L2726** — <code>            if (projection_location&lt;0) throw std::runtime_error("Renderer projection uniform is unavailable.");</code>

Reject a missing projection uniform because drawing without the required transform would be invalid.

<a id="l2727"></a>
**L2727** — <code>            glGenVertexArrays(2,vaos.data()); glGenBuffers(2,buffers.data());</code>

Generate both vertex-array and buffer-object pairs.

<a id="l2728"></a>
**L2728** — <code>            for (int i=0;i&lt;2;++i) {</code>

Configure the same Vertex layout for the static and dynamic batches.

<a id="l2729"></a>
**L2729** — <code>                glBindVertexArray(vaos[i]); glBindBuffer(GL_ARRAY_BUFFER,buffers[i]);</code>

Bind this VAO and its corresponding array buffer so attribute bindings are recorded together.

<a id="l2730"></a>
**L2730** — <code>                glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);</code>

Describe position as three non-normalized floats at byte offset zero with stride sizeof(Vertex).

<a id="l2731"></a>
**L2731** — <code>                glEnableVertexAttribArray(0);</code>

Enable the position attribute stream.

<a id="l2732"></a>
**L2732** — <code>                glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast&lt;void*&gt;(offsetof(Vertex,r)));</code>

Describe RGB as three floats beginning at offsetof(Vertex,r), avoiding an assumption about member offset.

<a id="l2733"></a>
**L2733** — <code>                glEnableVertexAttribArray(1);</code>

Enable the color attribute stream.

<a id="l2734"></a>
**L2734** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2735"></a>
**L2735** — <code>            glBindBuffer(GL_ARRAY_BUFFER,buffers[0]);</code>

Select the terrain VBO for its one-time upload.

<a id="l2736"></a>
**L2736** — <code>            glBufferData(GL_ARRAY_BUFFER,static_cast&lt;GLsizeiptr&gt;(terrain.size()*sizeof(Vertex)),terrain.data(),GL_STATIC_DRAW);</code>

Allocate/upload all terrain bytes with a static-use hint; the mesh is not re-uploaded on every frame.

<a id="l2737"></a>
**L2737** — <code>            terrain_vertices=static_cast&lt;GLsizei&gt;(terrain.size()); glBindVertexArray(0);</code>

Store the vertex count and unbind the VAO after configuration.

<a id="l2738"></a>
**L2738** — <code>            if (glGetError()!=GL_NO_ERROR) throw std::runtime_error("Unable to allocate maze graphics buffers.");</code>

Check for GL allocation/setup errors and fail with a useful diagnostic.

<a id="l2739"></a>
**L2739** — <code>        } catch (...) { release(); throw; }</code>

Release every partially initialized GPU object before rethrowing constructor failures.

<a id="l2740"></a>
**L2740** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2741"></a>
**L2741** — <code>    /// GPU object ownership cannot be copied.</code>

Documentation fragment: “GPU object ownership cannot be copied.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2742"></a>
**L2742** — <code>    Renderer(const Renderer&amp;)=delete;</code>

Delete the copy constructor to prevent two Renderers from owning and deleting the same GL handles.

<a id="l2743"></a>
**L2743** — <code>    /// GPU object ownership cannot be assigned.</code>

Documentation fragment: “GPU object ownership cannot be assigned.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2744"></a>
**L2744** — <code>    Renderer&amp; operator=(const Renderer&amp;)=delete;</code>

Delete copy assignment for the same exclusive-ownership reason.

<a id="l2745"></a>
**L2745** — <code>    /// Delete graphics resources before the window's context is destroyed.</code>

Documentation fragment: “Delete graphics resources before the window's context is destroyed.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2746"></a>
**L2746** — <code>    ~Renderer() { release(); }</code>

Release GL objects in the destructor; the application must destroy the renderer before destroying the context.

<a id="l2747"></a>
**L2747** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2748"></a>
**L2748** — <code>    /// Adjust only the camera; zoom follows the knight and never pauses gameplay.</code>

Documentation fragment: “Adjust only the camera; zoom follows the knight and never pauses gameplay.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2749"></a>
**L2749** — <code>    void change_zoom(double wheel_steps) {</code>

Apply a scroll/key zoom request without changing any Maze field or simulation clock.

<a id="l2750"></a>
**L2750** — <code>        zoom=std::clamp(zoom*std::pow(1.25f,float(std::clamp(wheel_steps,-4.0,4.0))),1.0f,64.0f);</code>

Scale zoom exponentially by 1.25 per step, cap one input to ±4 steps, and clamp the final zoom to 1–64.

<a id="l2751"></a>
**L2751** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2752"></a>
**L2752** — <code>    /// Return to the complete-maze view, including entrance and exit.</code>

Documentation fragment: “Return to the complete-maze view, including entrance and exit.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2753"></a>
**L2753** — <code>    void reset_view() { zoom=1.0f; }</code>

Reset the visual camera to a full-maze fit.

<a id="l2754"></a>
**L2754** — <code>    /// Expose the current visual zoom for rendering regression tests.</code>

Documentation fragment: “Expose the current visual zoom for rendering regression tests.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2755"></a>
**L2755** — <code>    float zoom_factor() const { return zoom; }</code>

Expose the current zoom as a const accessor for verification and diagnostics.

<a id="l2756"></a>
**L2756** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2757"></a>
**L2757** — <code>    /// Draw a depth-tested scene into any current OpenGL framebuffer. Keeping</code>

Documentation fragment: “Draw a depth-tested scene into any current OpenGL framebuffer. Keeping” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2758"></a>
**L2758** — <code>    /// window access outside this function also permits real offscreen rendering.</code>

Documentation fragment: “window access outside this function also permits real offscreen rendering.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2759"></a>
**L2759** — <code>    void draw(int width, int height, const Maze&amp; maze, int viewport_y = 0) {</code>

Render the world into a caller-selected vertical viewport within the current framebuffer.

<a id="l2760"></a>
**L2760** — <code>        if (width&lt;=0 || height&lt;=0) return;</code>

Skip drawing minimized or zero-sized targets, avoiding invalid projection divisions.

<a id="l2761"></a>
**L2761** — <code>        auto projection=scene_projection(maze,width,height,zoom);</code>

Compute an aspect-correct tilted camera using current zoom and player position.

<a id="l2762"></a>
**L2762** — <code>        glViewport(0,viewport_y,width,height);</code>

Set the world viewport, including the HUD-reserving vertical offset supplied by GameApplication.

<a id="l2763"></a>
**L2763** — <code>        glClearColor(0,0,0,1); glClearDepth(1.0);</code>

Set an opaque black clear color and maximum clear depth, producing the requested black maze background.

<a id="l2764"></a>
**L2764** — <code>        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_TRUE);</code>

Enable depth testing/writes and accept equal depths, so geometry ordering and wall occlusion are resolved consistently.

<a id="l2765"></a>
**L2765** — <code>        glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDisable(GL_MULTISAMPLE); glDisable(GL_DITHER);</code>

Disable blending, culling, multisampling, and dithering for opaque, crisp, deterministic pixel geometry.

<a id="l2766"></a>
**L2766** — <code>        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);</code>

Clear color and depth buffers. A viewport alone does not restrict glClear; no scissor test is introduced here.

<a id="l2767"></a>
**L2767** — <code>        glUseProgram(program); glUniformMatrix4fv(projection_location,1,GL_FALSE,projection.data());</code>

Bind the world shader and upload the column-major camera matrix without transposing.

<a id="l2768"></a>
**L2768** — <code>        glBindVertexArray(vaos[0]); glDrawArrays(GL_TRIANGLES,0,terrain_vertices);</code>

Draw the static terrain triangle batch using the previously uploaded buffer.

<a id="l2769"></a>
**L2769** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2770"></a>
**L2770** — <code>        dynamic_vertices.clear(); append_route(dynamic_vertices,maze);</code>

Clear the dynamic CPU batch while retaining capacity, then append the full route overlay if enabled.

<a id="l2771"></a>
**L2771** — <code>        append_sprite(dynamic_vertices,maze.point(maze.exit_cell),EXIT_SPRITE);</code>

Place the exit's embedded sprite at the actual exit cell.

<a id="l2772"></a>
**L2772** — <code>        if (previous_monsters.size()!=maze.monsters.size()) {</code>

Reinitialize visual monster history if the number of monsters has changed.

<a id="l2773"></a>
**L2773** — <code>            previous_monsters.assign(maze.monsters.size(),-1);</code>

Mark every monster's prior position as unknown.

<a id="l2774"></a>
**L2774** — <code>            monster_mirrored.assign(maze.monsters.size(),0);</code>

Start each appearance unmirrored until horizontal movement establishes facing.

<a id="l2775"></a>
**L2775** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2776"></a>
**L2776** — <code>        for (std::size_t i=0;i&lt;maze.monsters.size();++i) {</code>

Append every current monster to the shared dynamic batch.

<a id="l2777"></a>
**L2777** — <code>            const auto&amp; monster=maze.monsters[i]; Point p=maze.point(monster.cell);</code>

Read the monster by const reference and convert its grid index to a Point.

<a id="l2778"></a>
**L2778** — <code>            if (previous_monsters[i]&gt;=0 &amp;&amp; previous_monsters[i]!=monster.cell) {</code>

Only infer motion when a previous valid position differs from the current one.

<a id="l2779"></a>
**L2779** — <code>                int dc=p.c-maze.point(previous_monsters[i]).c;</code>

Compute horizontal displacement since the previous rendered frame.

<a id="l2780"></a>
**L2780** — <code>                if (dc) monster_mirrored[i]=dc&lt;0;</code>

Mirror on leftward movement; purely vertical movement preserves the previous facing.

<a id="l2781"></a>
**L2781** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2782"></a>
**L2782** — <code>            previous_monsters[i]=monster.cell;</code>

Record the current cell as the next frame's reference.

<a id="l2783"></a>
**L2783** — <code>            const auto&amp; appearance=monster_appearance(monster.id);</code>

Resolve the stable appearance from the monster's spawn ID.

<a id="l2784"></a>
**L2784** — <code>            const PixelSprite&amp; sprite=*appearance.sprite;</code>

Dereference the already-validated static sprite pointer.

<a id="l2785"></a>
**L2785** — <code>            float lift=appearance.hovering ? 0.014f*(1.0f+std::sin(float(maze.now%100000)/300.0f+float(monster.id))) : 0.0f;</code>

For hovering variants, use an ID-offset sine wave producing lift from 0 to 0.028 cells; non-hovering variants remain grounded.

<a id="l2786"></a>
**L2786** — <code>            append_sprite(dynamic_vertices,p,sprite,monster_mirrored[i]!=0,monster.state==MonsterState::Hunting,lift,</code>

Draw the monster sprite with red eyes when hunting...

<a id="l2787"></a>
**L2787** — <code>                          monster.state==MonsterState::Searching);</code>

...or orange eyes when searching.

<a id="l2788"></a>
**L2788** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2789"></a>
**L2789** — <code>        Point player=maze.point(maze.player);</code>

Read the knight's current grid position.

<a id="l2790"></a>
**L2790** — <code>        if (previous_player&gt;=0 &amp;&amp; previous_player!=maze.player) {</code>

Recognize a visual movement only after a prior frame has established a different player cell.

<a id="l2791"></a>
**L2791** — <code>            int dc=player.c-maze.point(previous_player).c;</code>

Calculate horizontal displacement for knight facing.

<a id="l2792"></a>
**L2792** — <code>            if (dc) player_mirrored=dc&lt;0;</code>

Mirror leftward and preserve facing during vertical movement.

<a id="l2793"></a>
**L2793** — <code>            player_moved_at=maze.now;</code>

Remember the simulation timestamp of the observed step for a brief bob animation.

<a id="l2794"></a>
**L2794** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2795"></a>
**L2795** — <code>        previous_player=maze.player;</code>

Store the current player cell for the next frame.

<a id="l2796"></a>
**L2796** — <code>        append_actor_ring(dynamic_vertices,player,{0.31f,0.53f,0.95f});</code>

Append a blue octagonal ground ring to distinguish the knight at small scales.

<a id="l2797"></a>
**L2797** — <code>        float step_lift=maze.now-player_moved_at&lt;160 &amp;&amp; (maze.now/80)%2 ? 0.015f : 0.0f;</code>

Within 160 ms of a step, alternate a 0.015-cell lift on 80-ms phases; otherwise use zero lift.

<a id="l2798"></a>
**L2798** — <code>        append_sprite(dynamic_vertices,player,KNIGHT_SPRITE,player_mirrored,false,step_lift);</code>

Draw the knight with the computed facing and step lift; hunting eye coloration is never applied to the player.

<a id="l2799"></a>
**L2799** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2800"></a>
**L2800** — <code>        glBindVertexArray(vaos[1]); glBindBuffer(GL_ARRAY_BUFFER,buffers[1]);</code>

Bind the dynamic VAO and VBO.

<a id="l2801"></a>
**L2801** — <code>        glBufferData(GL_ARRAY_BUFFER,static_cast&lt;GLsizeiptr&gt;(dynamic_vertices.size()*sizeof(Vertex)),dynamic_vertices.data(),GL_STREAM_DRAW);</code>

Upload this frame's complete route/actor triangle stream with a streaming-use hint.

<a id="l2802"></a>
**L2802** — <code>        glDrawArrays(GL_TRIANGLES,0,static_cast&lt;GLsizei&gt;(dynamic_vertices.size()));</code>

Draw the entire dynamic batch in one triangle call, letting depth testing resolve overlap with terrain.

<a id="l2803"></a>
**L2803** — <code>        glBindVertexArray(0);</code>

Unbind the VAO after drawing.

<a id="l2804"></a>
**L2804** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2805"></a>
**L2805** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2806"></a>
**L2806** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## UI renderer and screen composition

<a id="l2807"></a>
**L2807** — <code>// Opaque UI quads share the game's vertex format and shader. The font and icons</code>

Documentation fragment: “Opaque UI quads share the game's vertex format and shader. The font and icons” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2808"></a>
**L2808** — <code>// are embedded, so menus require neither a font package nor image asset files.</code>

Documentation fragment: “are embedded, so menus require neither a font package nor image asset files.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2809"></a>
**L2809** — <code>class UiRenderer {</code>

Own a separate overlay batch using the same Vertex format and shader logic as the world.

<a id="l2810"></a>
**L2810** — <code>    GLuint program=0,vao=0,buffer=0;</code>

Initialize UI program, vertex-array, and buffer handles as unallocated.

<a id="l2811"></a>
**L2811** — <code>    GLint projection=-1;</code>

Reserve a cached projection-uniform location.

<a id="l2812"></a>
**L2812** — <code>    std::vector&lt;Vertex&gt; vertices;</code>

Reuse a CPU triangle vector for all labels, panels, buttons, and icons in a UI frame.

<a id="l2813"></a>
**L2813** — <code>    const RGB ink{0.055f,0.07f,0.085f}, panel{0.085f,0.105f,0.125f};</code>

Define dark ink and panel colors in normalized RGB, central points for a menu theme change.

<a id="l2814"></a>
**L2814** — <code>    const RGB white{0.92f,0.92f,0.86f},muted{0.56f,0.62f,0.66f},gold{0.87f,0.71f,0.39f};</code>

Define light text, muted secondary text, and gold accent colors shared throughout the UI.

<a id="l2815"></a>
**L2815** — <code>    /// Add an opaque rectangle in logical UI pixels.</code>

Documentation fragment: “Add an opaque rectangle in logical UI pixels.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2816"></a>
**L2816** — <code>    void rect(UiRect box,RGB color) { append_floor_rect(vertices,box.x,box.y,box.x+box.w,box.y+box.h,0,color); }</code>

Convert a logical UI rectangle into a flat colored quad at z = 0, appending to the overlay batch.

<a id="l2817"></a>
**L2817** — <code>    /// Add a one- or two-pixel outline without covering its interior.</code>

Documentation fragment: “Add a one- or two-pixel outline without covering its interior.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2818"></a>
**L2818** — <code>    void border(UiRect b,RGB color,float thickness=2) {</code>

Draw an outline from four filled rectangles, with configurable thickness defaulting to two logical pixels.

<a id="l2819"></a>
**L2819** — <code>        rect({b.x,b.y,b.w,thickness},color);rect({b.x,b.y+b.h-thickness,b.w,thickness},color);</code>

Emit top and bottom edges without filling the rectangle's interior.

<a id="l2820"></a>
**L2820** — <code>        rect({b.x,b.y,thickness,b.h},color);rect({b.x+b.w-thickness,b.y,thickness,b.h},color);</code>

Emit left and right edges; corner overlap is harmless with opaque colors.

<a id="l2821"></a>
**L2821** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2822"></a>
**L2822** — <code>    /// Emit the embedded bitmap font, keeping pixel edges aligned within a glyph.</code>

Documentation fragment: “Emit the embedded bitmap font, keeping pixel edges aligned within a glyph.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2823"></a>
**L2823** — <code>    void text(float x,float y,std::string value,float size,RGB color,bool preserve_case=false) {</code>

Add an opt-in case-preservation flag to the bitmap text renderer. Existing UI labels retain their default uppercase treatment.

<a id="l2824"></a>
**L2824** — <code>        for(unsigned char c:value) {</code>

Iterate unsigned bytes so std::toupper is called with a valid nonnegative character value.

<a id="l2825"></a>
**L2825** — <code>            auto glyph=ui_glyph(preserve_case ? static_cast&lt;char&gt;(c) : static_cast&lt;char&gt;(std::toupper(c)));</code>

Use the original character for case-sensitive labels, otherwise normalize it before looking up its glyph.

<a id="l2826"></a>
**L2826** — <code>            for(int r=0;r&lt;7;++r)for(int col=0;col&lt;5;++col)</code>

Inspect all seven rows and five columns of the glyph.

<a id="l2827"></a>
**L2827** — <code>                if(glyph[r]&amp;(1u&lt;&lt;(4-col)))rect({x+col*size,y+r*size,size,size},color);</code>

Read bit 4 as the leftmost pixel and bit 0 as the rightmost, emitting a square only for a set bit.

<a id="l2828"></a>
**L2828** — <code>            x+=6*size;</code>

Advance six scaled pixels per character: five drawing columns plus one column of spacing.

<a id="l2829"></a>
**L2829** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2830"></a>
**L2830** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2831"></a>
**L2831** — <code>    /// Center a single-line label in its rectangle.</code>

Documentation fragment: “Center a single-line label in its rectangle.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2832"></a>
**L2832** — <code>    void centered(UiRect b,const std::string&amp; label,float size,RGB color) {</code>

Center one line of bitmap text within a rectangle.

<a id="l2833"></a>
**L2833** — <code>        float length=label.empty()?0.0f:float(label.size()*6-1)*size;</code>

Measure the glyph string as 6n−1 pixels, excluding trailing inter-character spacing; empty text has zero width.

<a id="l2834"></a>
**L2834** — <code>        text(b.x+(b.w-length)/2,b.y+(b.h-7*size)/2,label,size,color);</code>

Offset by half the unused width/height and reuse text(), whose glyph height is seven pixels.

<a id="l2835"></a>
**L2835** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2836"></a>
**L2836** — <code>    /// Reuse the actual game sprites as flat menu illustrations.</code>

Documentation fragment: “Reuse the actual game sprites as flat menu illustrations.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2837"></a>
**L2837** — <code>    void icon(float x,float y,const PixelSprite&amp; sprite,float size,bool hunting=false) {</code>

Draw one embedded sprite in flat UI coordinates; no world tilt or camera transform is used for this illustration.

<a id="l2838"></a>
**L2838** — <code>        for(int r=0;r&lt;20;++r)for(int c=0;c&lt;20;++c)if(sprite[r][c]!='.')</code>

Traverse all sprite pixels and skip transparency dots.

<a id="l2839"></a>
**L2839** — <code>            rect({x+c*size,y+r*size,size,size},sprite_color(sprite[r][c],hunting));</code>

Draw each opaque pixel as a logical UI square with its palette color and optional hunting eyes.

<a id="l2840"></a>
**L2840** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2841"></a>
**L2841** — <code>    /// Draw a button or editable field with selection, hover, and keyboard focus.</code>

Documentation fragment: “Draw a button or editable field with selection, hover, and keyboard focus.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2842"></a>
**L2842** — <code>    void widget(const UiWidget&amp; w,const UiState&amp; ui) {</code>

Render a widget from the same model used by focus and pointer interaction.

<a id="l2843"></a>
**L2843** — <code>        bool primary=w.id==Control::Start||w.id==Control::Resume||w.id==Control::Again;</code>

Treat Start, Resume, and Again as primary actions with filled gold styling.

<a id="l2844"></a>
**L2844** — <code>        RGB fill=primary?gold:w.selected?RGB{0.17f,0.19f,0.17f}:panel;</code>

Choose primary gold, a subdued selected fill, or the ordinary panel fill.

<a id="l2845"></a>
**L2845** — <code>        RGB label=primary?ink:white;</code>

Use dark text on primary gold buttons and light text on ordinary controls.

<a id="l2846"></a>
**L2846** — <code>        if(!w.enabled) {fill={0.15f,0.16f,0.16f};label=muted;}</code>

Override disabled widgets with muted fill and label colors.

<a id="l2847"></a>
**L2847** — <code>        else if(ui.pressed==w.id) fill=shade(fill,0.72f);</code>

Darken an enabled pressed widget to give immediate mouse-down feedback.

<a id="l2848"></a>
**L2848** — <code>        else if(ui.hover==w.id) fill=shade(fill,1.25f);</code>

Otherwise brighten an enabled hovered widget; pressed feedback takes priority.

<a id="l2849"></a>
**L2849** — <code>        rect(w.box,fill);</code>

Emit the widget background rectangle.

<a id="l2850"></a>
**L2850** — <code>        border(w.box,w.selected||ui.focus==w.id?gold:RGB{0.22f,0.25f,0.27f},ui.focus==w.id?3:1);</code>

Use a gold outline for selection or keyboard focus, with a thicker three-pixel focused border and one-pixel ordinary border.

<a id="l2851"></a>
**L2851** — <code>        if(UiState::is_field(w.id)&amp;&amp;ui.focus==w.id&amp;&amp;ui.replace_field)</code>

Detect a focused numeric field whose next digit will replace the whole value.

<a id="l2852"></a>
**L2852** — <code>            rect({w.box.x+12,w.box.y+8,w.box.w-24,w.box.h-16},{0.23f,0.29f,0.35f});</code>

Draw an inset selection-colored rectangle behind its numeric label to communicate replacement mode.

<a id="l2853"></a>
**L2853** — <code>        if(w.id==Control::Low||w.id==Control::Medium||w.id==Control::High) {</code>

Give the difficulty cards their richer multi-line layout.

<a id="l2854"></a>
**L2854** — <code>            text(w.box.x+18,w.box.y+19,w.label,3,w.selected?gold:white);</code>

Draw the card heading at size three, gold for the selected difficulty.

<a id="l2855"></a>
**L2855** — <code>            const bool low=w.id==Control::Low,medium=w.id==Control::Medium;</code>

Cache which of the three difficulty IDs is being drawn for concise descriptive copy selection.

<a id="l2856"></a>
**L2856** — <code>            text(w.box.x+18,w.box.y+62,low?"MORE ROUTES":medium?"BALANCED CHASE":"FAST PURSUIT",1.7f,muted);</code>

Draw the first short difficulty description: route abundance, balanced chase, or fast pursuit.

<a id="l2857"></a>
**L2857** — <code>            text(w.box.x+18,w.box.y+84,low?"ROOM TO BREATHE":medium?"STAY ALERT":"LESS FORGIVING",1.6f,muted);</code>

Draw the second description emphasizing available breathing room or increased pressure.

<a id="l2858"></a>
**L2858** — <code>            if(w.selected)rect({w.box.x+18,w.box.y+w.box.h-7,w.box.w-36,3},gold);</code>

Underline the selected card, adding a non-color-only selection cue.

<a id="l2859"></a>
**L2859** — <code>        } else centered(w.box,w.label,w.id==Control::Start?3.0f:2.0f,label);</code>

Other controls use a centered single-line label, with larger text for Start.

<a id="l2860"></a>
**L2860** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2861"></a>
**L2861** — <code>    /// Render the setup's descriptive labels, settings summary, and character art.</code>

Documentation fragment: “Render the setup's descriptive labels, settings summary, and character art.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2862"></a>
**L2862** — <code>    void setup(const UiState&amp; ui) {</code>

Draw setup labels, form guidance, the live settings summary, and decorative player/monster art.

<a id="l2863"></a>
**L2863** — <code>        text(50,38,"MAZE RUNNER",5,white);text(52,91,"CHOOSE YOUR NEXT ESCAPE.",2,muted);</code>

Draw the title and subtitle at fixed logical positions.

<a id="l2864"></a>
**L2864** — <code>        text(850,51,"V" MAZERUNNER_VERSION,2,gold,true);rect({50,118,900,1},{0.22f,0.25f,0.27f});</code>

Draw the version with its lowercase suffix and then the separator beneath the setup heading.

<a id="l2865"></a>
**L2865** — <code>        text(50,134,"01  DIFFICULTY",2,gold);</code>

Label the first section as difficulty selection.

<a id="l2866"></a>
**L2866** — <code>        text(50,302,"02  MAZE SIZE",2,gold);rect({50,328,650,171},panel);</code>

Label and back the maze-size section with a dark panel.

<a id="l2867"></a>
**L2867** — <code>        text(542,356,"PRESETS",1.6f,muted);</code>

Place the Presets caption next to the size buttons.

<a id="l2868"></a>
**L2868** — <code>        text(70,402,"WIDTH",1.6f,muted);text(370,402,"HEIGHT",1.6f,muted);</code>

Label the independent width and height controls.

<a id="l2869"></a>
**L2869** — <code>        text(70,481,"5-501 CELLS. EVEN VALUES ROUND UP. CLICK VALUES TO TYPE.",1.6f,muted);</code>

Explain supported dimensions, upward odd normalization, and click-to-type editing directly in the UI.

<a id="l2870"></a>
**L2870** — <code>        text(50,522,"03  MONSTERS",2,gold);rect({50,546,650,76},panel);</code>

Label and back the monster-count section.

<a id="l2871"></a>
**L2871** — <code>        int monster_count=0;bool count_valid=SetupForm::number(ui.form.count,0,10,monster_count);</code>

Parse monster count for the illustration row without modifying the form text.

<a id="l2872"></a>
**L2872** — <code>        for(int i=0;i&lt;MAX_MONSTERS;++i) {</code>

Reserve one visual slot for each of the maximum ten monsters.

<a id="l2873"></a>
**L2873** — <code>            float x=300+37.0f*i;</code>

Space icon slots by 37 logical pixels from x = 300.

<a id="l2874"></a>
**L2874** — <code>            if(count_valid&amp;&amp;i&lt;monster_count)icon(x,565,*MONSTER_APPEARANCES[i].sprite,1.3f);</code>

Draw the actual registered sprite for each currently selected monster when the count is valid.

<a id="l2875"></a>
**L2875** — <code>            else border({x+4,569,18,20},{0.20f,0.23f,0.26f},1);</code>

Draw an empty outline for inactive slots or an invalid count.

<a id="l2876"></a>
**L2876** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2877"></a>
**L2877** — <code>        std::string error=ui.form.error();</code>

Obtain the form's first field-validation error.

<a id="l2878"></a>
**L2878** — <code>        if(!error.empty())text(50,634,error,1.8f,{0.99f,0.47f,0.39f});</code>

Show invalid-field feedback in a warm red tone near the Start area.

<a id="l2879"></a>
**L2879** — <code>        else if(!ui.message.empty())text(50,634,ui.message,1.6f,{0.99f,0.47f,0.39f});</code>

If fields are valid, show any application-level generation error instead.

<a id="l2880"></a>
**L2880** — <code>        else text(50,634,"0 MONSTERS FOR A QUIET EXPLORE. 10 FOR A FULL HUNT.",1.7f,muted);</code>

With no error, explain the exploratory zero-monster option and maximum ten-monster hunt.

<a id="l2881"></a>
**L2881** — <code>        rect({730,158,220,556},panel);border({730,158,220,556},{0.22f,0.25f,0.27f},1);</code>

Draw the bordered right-hand summary/help panel.

<a id="l2882"></a>
**L2882** — <code>        icon(805,187,KNIGHT_SPRITE,3.5f);text(760,288,"YOUR RUN",2,gold);</code>

Reuse the knight art at a larger UI scale and label the summary Your Run.

<a id="l2883"></a>
**L2883** — <code>        text(760,327,difficulty_settings(ui.form.difficulty).name,2.5f,white);</code>

Show the selected difficulty's canonical profile name.

<a id="l2884"></a>
**L2884** — <code>        int r=0,c=0,n=0;</code>

Allocate temporary parsed dimensions/count for the summary.

<a id="l2885"></a>
**L2885** — <code>        if(ui.form.values(r,c,n)) {</code>

Only present concrete dimensions/count when all fields validate.

<a id="l2886"></a>
**L2886** — <code>            text(760,366,std::to_string(c)+" X "+std::to_string(r)+" CELLS",1.7f,white);</code>

Display normalized width × height, so even input's eventual odd dimensions are visible before starting.

<a id="l2887"></a>
**L2887** — <code>            text(760,397,std::to_string(n)+" MONSTERS",1.7f,white);</code>

Display the parsed monster count.

<a id="l2888"></a>
**L2888** — <code>        } else text(760,366,"CHECK SETTINGS",1.6f,muted);</code>

Replace the numeric summary with Check Settings while any field is invalid.

<a id="l2889"></a>
**L2889** — <code>        rect({760,440,160,1},{0.22f,0.25f,0.27f});</code>

Separate the settings summary from the control legend.

<a id="l2890"></a>
**L2890** — <code>        text(760,467,"HOW TO PLAY",1.7f,gold);</code>

Draw the How to Play heading.

<a id="l2891"></a>
**L2891** — <code>        text(760,503,"WASD OR ARROWS",1.6f,white);text(760,530,"G   AUTO-SOLVE",1.6f,white);</code>

Explain manual directional movement and G for automatic solving.

<a id="l2892"></a>
**L2892** — <code>        text(760,557,"ESC PAUSE",1.6f,white);text(760,584,"WHEEL ZOOM",1.6f,white);</code>

Explain pause and mouse-wheel zoom.

<a id="l2893"></a>
**L2893** — <code>        text(760,641,"REACH THE GREEN",1.5f,muted);text(760,664,"EXIT TO ESCAPE.",1.5f,muted);</code>

Identify the green exit as the objective, using two short lines that fit the panel.

<a id="l2894"></a>
**L2894** — <code>        text(50,734,"MOUSE TO SELECT  /  TAB TO FOCUS  /  ENTER TO CONFIRM",1.7f,muted);</code>

Show mouse selection, Tab navigation, and Enter confirmation at the bottom of setup.

<a id="l2895"></a>
**L2895** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2896"></a>
**L2896** — <code>    /// Place persistent controls and status outside the maze's reserved viewport.</code>

Documentation fragment: “Place persistent controls and status outside the maze's reserved viewport.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2897"></a>
**L2897** — <code>    void hud(const Maze&amp; maze) {</code>

Draw status and help panels outside the world-rendering viewport.

<a id="l2898"></a>
**L2898** — <code>        rect({18,16,964,62},ink);border({18,16,964,62},{0.22f,0.25f,0.27f},1);</code>

Draw the top status panel and its thin border.

<a id="l2899"></a>
**L2899** — <code>        text(34,27,"MAZE RUNNER",2,gold);</code>

Label the HUD with the game name.

<a id="l2900"></a>
**L2900** — <code>        std::ostringstream time;time&lt;&lt;std::fixed&lt;&lt;std::setprecision(1)&lt;&lt;maze.elapsed_ms/1000.0;</code>

Format elapsed simulation time in seconds with one decimal, independent of menu/loading wall time.

<a id="l2901"></a>
**L2901** — <code>        if(!maze.timer_started&amp;&amp;maze.outcome==Outcome::Playing)</code>

Before the first input, while play is still pending...

<a id="l2902"></a>
**L2902** — <code>            text(34,53,std::string(maze.settings.name)+"  |  MOVE OR PRESS G TO START",1.6f,gold);</code>

...show the start prompt in gold instead of time and steps.

<a id="l2903"></a>
**L2903** — <code>        else {</code>

Otherwise show live statistics.

<a id="l2904"></a>
**L2904** — <code>            int hunting=0,searching=0;</code>

Count hunters and searchers.

<a id="l2905"></a>
**L2905** — <code>            for(const auto&amp; m:maze.monsters) {hunting+=m.state==MonsterState::Hunting;searching+=m.state==MonsterState::Searching;}</code>

Tally both states across all monsters.

<a id="l2906"></a>
**L2906** — <code>            text(186,28,std::string("DIRECTOR ")+maze.director_phase()+" | RADIO "+std::to_string(maze.director_reports())+</code>

Compose the HUD status line from the live director phase and cumulative radio report count.

<a id="l2907"></a>
**L2907** — <code>                 " | "+maze.radio_area_label(),1.5f,maze.director_hint_width()&gt;maze.settings.hint_area_size ? gold : muted);</code>

Add the actual clipped radio dimensions and choose the relief accent color while the local hint is wider than its normal width.

<a id="l2908"></a>
**L2908** — <code>            text(34,53,std::string(maze.settings.name)+"  |  "+time.str()+" S  |  "+</code>

Draw difficulty, elapsed seconds...

<a id="l2909"></a>
**L2909** — <code>                 std::to_string(maze.manual_steps+maze.automatic_steps)+" STEPS",1.6f,white);</code>

...and total steps.

<a id="l2910"></a>
**L2910** — <code>            if(hunting)text(400,53,std::to_string(hunting)+" HUNTING",1.6f,{1,0.43f,0.36f});</code>

Show the number of hunters in red when any are hunting...

<a id="l2911"></a>
**L2911** — <code>            else if(searching)text(400,53,std::to_string(searching)+" SEARCHING",1.6f,{1,0.62f,0.25f});</code>

...otherwise the number of searchers in orange.

<a id="l2912"></a>
**L2912** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2913"></a>
**L2913** — <code>        rect({18,711,964,35},ink);</code>

Draw a bottom panel for the persistent control legend.

<a id="l2914"></a>
**L2914** — <code>        text(34,722,"WASD / ARROWS MOVE (HOLD)   G AUTO-SOLVE   ESC PAUSE   WHEEL ZOOM   HOME FIT",1.65f,muted);</code>

Keyboard legend, noting that movement keys can be held.

<a id="l2915"></a>
**L2915** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2916"></a>
**L2916** — <code>    /// Release partially allocated GPU objects while their context is still current.</code>

Documentation fragment: “Release partially allocated GPU objects while their context is still current.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2917"></a>
**L2917** — <code>    void release() {</code>

Release UI GPU resources during normal destruction or initialization failure.

<a id="l2918"></a>
**L2918** — <code>        if(buffer)glDeleteBuffers(1,&amp;buffer);</code>

Delete the buffer only if it was allocated.

<a id="l2919"></a>
**L2919** — <code>        if(vao)glDeleteVertexArrays(1,&amp;vao);</code>

Delete the VAO only if it was allocated.

<a id="l2920"></a>
**L2920** — <code>        if(program)glDeleteProgram(program);</code>

Delete the linked UI shader program if present.

<a id="l2921"></a>
**L2921** — <code>        buffer=vao=program=0;</code>

Reset ownership handles to zero after deletion.

<a id="l2922"></a>
**L2922** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2923"></a>
**L2923** — <code>public:</code>

Set following class members to public access until another access label or class end; this changes compile-time accessibility, not runtime state.

<a id="l2924"></a>
**L2924** — <code>    /// Allocate a reusable overlay batch using the existing OpenGL 3.3 shader.</code>

Documentation fragment: “Allocate a reusable overlay batch using the existing OpenGL 3.3 shader.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2925"></a>
**L2925** — <code>    UiRenderer() {</code>

Construct the UI renderer with a current GL context, before any game maze is required.

<a id="l2926"></a>
**L2926** — <code>        try {</code>

Wrap initialization in exception cleanup.

<a id="l2927"></a>
**L2927** — <code>            program=create_shader_program();projection=glGetUniformLocation(program,"viewProjection");</code>

Create the shared-format shader and cache its projection uniform.

<a id="l2928"></a>
**L2928** — <code>            glGenVertexArrays(1,&amp;vao);glGenBuffers(1,&amp;buffer);</code>

Allocate one VAO/VBO pair for all UI geometry.

<a id="l2929"></a>
**L2929** — <code>            glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);</code>

Bind that pair to record vertex-attribute configuration.

<a id="l2930"></a>
**L2930** — <code>            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);</code>

Configure and enable the three-float position attribute at location zero.

<a id="l2931"></a>
**L2931** — <code>            glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast&lt;void*&gt;(offsetof(Vertex,r)));glEnableVertexAttribArray(1);</code>

Configure and enable RGB at location one using the actual Vertex member offset.

<a id="l2932"></a>
**L2932** — <code>            glBindVertexArray(0);</code>

Unbind the configured VAO.

<a id="l2933"></a>
**L2933** — <code>            if(projection&lt;0||glGetError()!=GL_NO_ERROR)throw std::runtime_error("Unable to initialize menu graphics.");</code>

Reject a missing projection uniform or GL setup error.

<a id="l2934"></a>
**L2934** — <code>        } catch(...) {release();throw;}</code>

Release partial resources and rethrow initialization failure.

<a id="l2935"></a>
**L2935** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2936"></a>
**L2936** — <code>    UiRenderer(const UiRenderer&amp;)=delete;</code>

Disable copying so UI GPU handles have one owner.

<a id="l2937"></a>
**L2937** — <code>    UiRenderer&amp; operator=(const UiRenderer&amp;)=delete;</code>

Disable copy assignment for the same reason.

<a id="l2938"></a>
**L2938** — <code>    /// Destroy the UI before destroying its window/context.</code>

Documentation fragment: “Destroy the UI before destroying its window/context.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2939"></a>
**L2939** — <code>    ~UiRenderer() {release();}</code>

Delete UI resources while the context is still alive.

<a id="l2940"></a>
**L2940** — <code>    /// Draw a complete setup/loading screen or a gameplay HUD and modal overlay.</code>

Documentation fragment: “Draw a complete setup/loading screen or a gameplay HUD and modal overlay.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2941"></a>
**L2941** — <code>    void draw(int width,int height,const UiState&amp; ui,const Maze* maze=nullptr) {</code>

Draw either a full menu screen or an overlay above a supplied maze frame.

<a id="l2942"></a>
**L2942** — <code>        if(width&lt;=0||height&lt;=0)return;</code>

Return immediately for minimized/invalid framebuffer sizes.

<a id="l2943"></a>
**L2943** — <code>        glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);</code>

Use the full framebuffer viewport and disable depth, blending, and culling so UI triangles draw in insertion order above the world.

<a id="l2944"></a>
**L2944** — <code>        vertices.clear();</code>

Clear the previous frame's UI geometry while preserving allocated capacity.

<a id="l2945"></a>
**L2945** — <code>        if(ui.screen==Screen::Setup||ui.screen==Screen::Loading) {</code>

Setup and Loading replace the entire scene instead of overlaying a maze.

<a id="l2946"></a>
**L2946** — <code>            glClearColor(0.02f,0.028f,0.035f,1);glClear(GL_COLOR_BUFFER_BIT);</code>

Clear to a nearly black background, including any letterboxed margins outside the logical canvas.

<a id="l2947"></a>
**L2947** — <code>            rect({0,0,UI_WIDTH,UI_HEIGHT},ink);</code>

Fill the logical UI canvas with the common ink color.

<a id="l2948"></a>
**L2948** — <code>            if(ui.screen==Screen::Setup)setup(ui);</code>

Draw the editable setup screen when applicable.

<a id="l2949"></a>
**L2949** — <code>            else {</code>

Otherwise draw the noninteractive Loading presentation.

<a id="l2950"></a>
**L2950** — <code>                icon(470,235,KNIGHT_SPRITE,3);</code>

Display a centered knight illustration during generation.

<a id="l2951"></a>
**L2951** — <code>                centered({100,345,800,50},"CARVING YOUR MAZE...",3,gold);</code>

Display the primary maze-generation message.

<a id="l2952"></a>
**L2952** — <code>                centered({100,408,800,30},"PREPARING PATHS AND MONSTERS",1.8f,muted);</code>

Display a secondary explanation that paths and monsters are being prepared.

<a id="l2953"></a>
**L2953** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2954"></a>
**L2954** — <code>        } else if(maze) {</code>

For gameplay-related screens, draw HUD/modal content only when a Maze pointer is available.

<a id="l2955"></a>
**L2955** — <code>            hud(*maze);</code>

Show the persistent gameplay status and control legend.

<a id="l2956"></a>
**L2956** — <code>            if(ui.screen==Screen::Paused||ui.screen==Screen::Result) {</code>

Paused and Result add modal panels over the existing world/HUD.

<a id="l2957"></a>
**L2957** — <code>                const bool paused=ui.screen==Screen::Paused;</code>

Distinguish pause from terminal outcome for panel content and height.

<a id="l2958"></a>
**L2958** — <code>                rect({284,224,440,paused?332.0f:396.0f},ink);border({284,224,440,paused?332.0f:396.0f},gold);</code>

Draw an opaque bordered modal, taller for results because it contains time and steps.

<a id="l2959"></a>
**L2959** — <code>                const bool won=maze-&gt;outcome==Outcome::Won;</code>

Check the actual simulation outcome for a successful escape.

<a id="l2960"></a>
**L2960** — <code>                centered({300,251,400,42},paused?"PAUSED":won?"YOU ESCAPED!":"CAUGHT!",3,won||paused?gold:RGB{1,0.43f,0.36f});</code>

Choose Paused, You Escaped, or Caught and appropriate gold/red emphasis.

<a id="l2961"></a>
**L2961** — <code>                if(paused)centered({300,305,400,26},"TAKE A BREATHER.",1.8f,muted);</code>

Show a short pause subtitle instead of final statistics.

<a id="l2962"></a>
**L2962** — <code>                else {</code>

Result panels show final run statistics.

<a id="l2963"></a>
**L2963** — <code>                    std::ostringstream result;result&lt;&lt;std::fixed&lt;&lt;std::setprecision(1)&lt;&lt;maze-&gt;elapsed_ms/1000.0&lt;&lt;" SECONDS";</code>

Format elapsed run time with one decimal followed by Seconds.

<a id="l2964"></a>
**L2964** — <code>                    centered({300,310,400,24},result.str(),2,white);</code>

Center the final duration in the modal.

<a id="l2965"></a>
**L2965** — <code>                    centered({300,346,400,24},std::to_string(maze-&gt;manual_steps+maze-&gt;automatic_steps)+" STEPS",2,muted);</code>

Center the combined successful movement count below the duration.

<a id="l2966"></a>
**L2966** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2967"></a>
**L2967** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2968"></a>
**L2968** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2969"></a>
**L2969** — <code>        for(const auto&amp; w:ui.widgets(maze))widget(w,ui);</code>

Draw every currently visible widget using the shared UiState list, after the screen background and labels.

<a id="l2970"></a>
**L2970** — <code>        UiTransform t(width,height);</code>

Compute the same uniform fit/letterboxing used by pointer-coordinate conversion.

<a id="l2971"></a>
**L2971** — <code>        std::array&lt;float,16&gt; matrix{{2*t.scale/width,0,0,0,0,-2*t.scale/height,0,0,0,0,1,0,</code>

Build an orthographic matrix mapping logical UI x rightward and logical y downward to OpenGL clip coordinates.

<a id="l2972"></a>
**L2972** — <code>                                   -1+2*t.x/width,1-2*t.y/height,0,1}};</code>

Apply letterbox translation and homogeneous coordinates; this matrix has no world-camera tilt.

<a id="l2973"></a>
**L2973** — <code>        glUseProgram(program);glUniformMatrix4fv(projection,1,GL_FALSE,matrix.data());</code>

Bind the UI shader and upload its matrix.

<a id="l2974"></a>
**L2974** — <code>        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);</code>

Bind the UI VAO/VBO for this frame.

<a id="l2975"></a>
**L2975** — <code>        glBufferData(GL_ARRAY_BUFFER,static_cast&lt;GLsizeiptr&gt;(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STREAM_DRAW);</code>

Upload all current overlay triangles with a streaming hint.

<a id="l2976"></a>
**L2976** — <code>        glDrawArrays(GL_TRIANGLES,0,static_cast&lt;GLsizei&gt;(vertices.size()));glBindVertexArray(0);</code>

Draw the complete UI in one triangle call and unbind the VAO.

<a id="l2977"></a>
**L2977** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l2978"></a>
**L2978** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l2979"></a>
**L2979** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Application lifecycle, hold-to-move, and asynchronous loading

<a id="l2980"></a>
**L2980** — <code>#ifndef MAZERUNNER_RENDER_TEST</code>

Exclude window/application orchestration from the dedicated renderer test build while retaining both renderers above.

<a id="l2981"></a>
**L2981** — <code>// The window owns a reusable menu renderer and at most one active maze renderer.</code>

Documentation fragment: “The window owns a reusable menu renderer and at most one active maze renderer.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2982"></a>
**L2982** — <code>// Generation runs on a worker; every OpenGL operation stays on the window thread.</code>

Documentation fragment: “Generation runs on a worker; every OpenGL operation stays on the window thread.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2983"></a>
**L2983** — <code>struct GameApplication {</code>

Coordinate the UI, active game, asynchronous generator, renderers, and terminal reporting.

<a id="l2984"></a>
**L2984** — <code>    UiState ui;</code>

Own the current screen, form, and interaction state.

<a id="l2985"></a>
**L2985** — <code>    UiRenderer menu;</code>

Keep a reusable menu renderer alive across maze generations.

<a id="l2986"></a>
**L2986** — <code>    std::unique_ptr&lt;Maze&gt; maze;</code>

Own at most one installed simulation via unique_ptr.

<a id="l2987"></a>
**L2987** — <code>    std::unique_ptr&lt;Renderer&gt; renderer;</code>

Own the world renderer corresponding to that simulation.

<a id="l2988"></a>
**L2988** — <code>    std::future&lt;std::unique_ptr&lt;Maze&gt;&gt; generation;</code>

Hold a future carrying an asynchronously generated Maze; the worker does not create GL objects.

<a id="l2989"></a>
**L2989** — <code>    std::uint32_t initial_seed;</code>

Remember the initial command-line or random seed.

<a id="l2990"></a>
**L2990** — <code>    std::mt19937 menu_rng;</code>

Use a separate PRNG to select replay seeds without changing a game's own random stream.

<a id="l2991"></a>
**L2991** — <code>    bool fixed_seed,quiet,verbose,first_game=true,reported=false;</code>

Store fixed-seed/logging options and flags for first-run seeding and one-time outcome reporting.

<a id="l2992"></a>
**L2992** — <code>    Millis last_console=-1000;</code>

Initialize periodic-log bookkeeping so the first active tick can report status immediately.

<a id="l2993"></a>
**L2993** — <code>    // GLFW identifies the physical key. The maze schedules its repeat deadlines.</code>

Describes the boundary between native key identity and the simulation's movement deadlines.

<a id="l2994"></a>
**L2994** — <code>    int held_key=-1;</code>

Remember which physical key currently owns the hold. The initial sentinel means none.

<a id="l2995"></a>
**L2995** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l2996"></a>
**L2996** — <code>    /// Forget any held direction (pause, focus loss, auto-solve, new game).</code>

Documentation fragment: “Forget any held direction (pause, focus loss, auto-solve, new game).” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l2997"></a>
**L2997** — <code>    void release_hold() {</code>

End the active hold from a key release, screen transition or focus change.

<a id="l2998"></a>
**L2998** — <code>        held_key = -1;</code>

Forget the GLFW key identity immediately.

<a id="l2999"></a>
**L2999** — <code>        if (maze) maze-&gt;cancel_held_move();</code>

Cancel the maze's scheduled repeat when a live maze is present.

<a id="l3000"></a>
**L3000** — <code>    }</code>

Close this C++ block and return control to its enclosing scope.

<a id="l3001"></a>
**L3001** — <code>    /// Move immediately and start the hold timer for this key.</code>

Documentation fragment: “Move immediately and start the hold timer for this key.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3002"></a>
**L3002** — <code>    void press_move(int key,int dr,int dc) {</code>

Submit a first move to the maze and retain the physical key while its timed repeats are armed.

<a id="l3003"></a>
**L3003** — <code>        held_key = maze &amp;&amp; maze-&gt;begin_held_move(dr, dc) ? key : -1;</code>

Remember the physical key only if the simulation accepts its first move and can continue the hold.

<a id="l3004"></a>
**L3004** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3005"></a>
**L3005** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3006"></a>
**L3006** — <code>    /// Initialize visible defaults; CLI difficulty only preselects its menu card.</code>

Documentation fragment: “Initialize visible defaults; CLI difficulty only preselects its menu card.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3007"></a>
**L3007** — <code>    GameApplication(std::uint32_t seed,bool fixed,bool silent,bool detailed,Difficulty difficulty)</code>

Construct the application from startup options, before a maze has been generated.

<a id="l3008"></a>
**L3008** — <code>        :initial_seed(seed),menu_rng(seed^0x9e3779b9u),fixed_seed(fixed),quiet(silent),verbose(detailed) {</code>

Seed the replay RNG using a fixed xor transformation and copy startup logging/seed policies.

<a id="l3009"></a>
**L3009** — <code>        ui.form.difficulty=difficulty;</code>

Apply CLI difficulty as the initial selected card; the user may still change it in setup.

<a id="l3010"></a>
**L3010** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3011"></a>
**L3011** — <code>    /// Release the old map once its player explicitly returns to setup.</code>

Documentation fragment: “Release the old map once its player explicitly returns to setup.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3012"></a>
**L3012** — <code>    void discard_game() { renderer.reset();maze.reset();reported=false;last_console=-1000;release_hold(); }</code>

Release the old map, reset reporting, and cancel any held direction.

<a id="l3013"></a>
**L3013** — <code>    /// Start a generation job from a validated immutable snapshot of the form.</code>

Documentation fragment: “Start a generation job from a validated immutable snapshot of the form.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3014"></a>
**L3014** — <code>    void begin_generation() {</code>

Start background generation from a validated snapshot of the form.

<a id="l3015"></a>
**L3015** — <code>        if(generation.valid()||ui.screen!=Screen::Loading)return;</code>

Do nothing if a job already exists or the screen has left Loading, preventing concurrent duplicate generation.

<a id="l3016"></a>
**L3016** — <code>        int rows=0,cols=0,count=0;</code>

Allocate parsed integer configuration variables.

<a id="l3017"></a>
**L3017** — <code>        if(!ui.form.values(rows,cols,count)) {ui.show(Screen::Setup);return;}</code>

Revalidate before launching; return to Setup on invalid fields without constructing a Maze.

<a id="l3018"></a>
**L3018** — <code>        discard_game();</code>

Release the previous run before creating the replacement.

<a id="l3019"></a>
**L3019** — <code>        std::uint32_t seed=first_game||fixed_seed?initial_seed:menu_rng();</code>

Use the original seed for the first game and every fixed-seed replay; otherwise obtain a new seed from menu_rng.

<a id="l3020"></a>
**L3020** — <code>        first_game=false;</code>

Mark the first-generation attempt as consumed for subsequent replay seed selection.

<a id="l3021"></a>
**L3021** — <code>        Difficulty difficulty=ui.form.difficulty;</code>

Copy difficulty so later form changes cannot alter a running generation job.

<a id="l3022"></a>
**L3022** — <code>        auto* logs=quiet?nullptr:&amp;std::cout;bool detailed=verbose;</code>

Choose either stdout or a null logger and snapshot verbosity.

<a id="l3023"></a>
**L3023** — <code>        generation=std::async(std::launch::async,[rows,cols,count,seed,difficulty,logs,detailed] {</code>

Force an actual async worker and capture all configuration values by value; no worker access to mutable UiState is required.

<a id="l3024"></a>
**L3024** — <code>            auto generated=std::make_unique&lt;Maze&gt;(rows,cols,seed,logs,difficulty);</code>

Allocate a Maze with the selected dimensions, seed, logger, and difficulty.

<a id="l3025"></a>
**L3025** — <code>            generated-&gt;verbose_logs=detailed;generated-&gt;generate(count);return generated;</code>

Set verbose logging, generate topology/spawns, and return unique ownership through the future.

<a id="l3026"></a>
**L3026** — <code>        });</code>

Finish the async callable and assignment to the stored future.

<a id="l3027"></a>
**L3027** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3028"></a>
**L3028** — <code>    /// Install a finished map on the GL thread and surface generation errors in UI.</code>

Documentation fragment: “Install a finished map on the GL thread and surface generation errors in UI.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3029"></a>
**L3029** — <code>    void finish_generation(bool window_focused) {</code>

Poll and install a completed Maze on the thread with the current GL context.

<a id="l3030"></a>
**L3030** — <code>        if(!generation.valid()||generation.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return;</code>

Use a zero-duration readiness check; do not block the event/render loop while generation is still running.

<a id="l3031"></a>
**L3031** — <code>        try {</code>

Catch both worker failures propagated by get() and graphics-allocation failures during installation.

<a id="l3032"></a>
**L3032** — <code>            auto created=generation.get();auto graphics=std::make_unique&lt;Renderer&gt;(*created);</code>

Retrieve the completed CPU-side Maze, then construct its renderer on the GL thread.

<a id="l3033"></a>
**L3033** — <code>            maze=std::move(created);renderer=std::move(graphics);</code>

Move the fully prepared objects into application ownership only after graphics construction succeeds.

<a id="l3034"></a>
**L3034** — <code>            if(!quiet)std::cout&lt;&lt;maze-&gt;configuration_summary()&lt;&lt;'\n';</code>

Print the effective configuration unless quiet mode is active.

<a id="l3035"></a>
**L3035** — <code>            ui.show(window_focused?Screen::Playing:Screen::Paused);</code>

Enter Playing if the window has focus; otherwise install directly into Paused so the new game cannot advance unnoticed.

<a id="l3036"></a>
**L3036** — <code>        } catch(const std::exception&amp; error) {</code>

Handle ordinary generation/rendering exceptions as recoverable setup errors.

<a id="l3037"></a>
**L3037** — <code>            discard_game();ui.show(Screen::Setup);</code>

Discard partial game state and return to the editable setup screen.

<a id="l3038"></a>
**L3038** — <code>            ui.message="COULD NOT CREATE MAZE. TRY A SMALLER SIZE.";</code>

Set a concise visible retry message, suggesting a smaller maze.

<a id="l3039"></a>
**L3039** — <code>            std::cerr&lt;&lt;"Maze setup failed: "&lt;&lt;error.what()&lt;&lt;'\n';</code>

Preserve the detailed exception diagnostic on stderr, even when normal state logging is quiet.

<a id="l3040"></a>
**L3040** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3041"></a>
**L3041** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3042"></a>
**L3042** — <code>    /// Freeze simulation while menus are open, and report outcomes through both UI and logs.</code>

Documentation fragment: “Freeze simulation while menus are open, and report outcomes through both UI and logs.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3043"></a>
**L3043** — <code>    void tick(Millis delta) {</code>

Advance the full nonnegative elapsed interval while Playing. The maze chronologically dispatches due manual repeats.

<a id="l3044"></a>
**L3044** — <code>        if(ui.screen!=Screen::Playing||!maze) {release_hold();return;}</code>

Outside active play, cancel any hold and leave the simulation frozen.

<a id="l3045"></a>
**L3045** — <code>        maze-&gt;advance_to(maze-&gt;now + std::max&lt;Millis&gt;(0, delta));</code>

Advance the complete nonnegative elapsed interval through the event scheduler on the Playing screen.

<a id="l3046"></a>
**L3046** — <code>        if(maze-&gt;outcome!=Outcome::Playing) {</code>

Detect a caught/won outcome immediately after advancing.

<a id="l3047"></a>
**L3047** — <code>            if(!reported&amp;&amp;!quiet)std::cout&lt;&lt;'\n'&lt;&lt;outcome_name(maze-&gt;outcome)&lt;&lt;" | Manual steps: "&lt;&lt;maze-&gt;manual_steps</code>

Begin one terminal outcome report unless it was already reported or quiet mode suppresses it.

<a id="l3048"></a>
**L3048** — <code>                &lt;&lt;" | Auto steps: "&lt;&lt;maze-&gt;automatic_steps&lt;&lt;" | Time: "&lt;&lt;std::fixed&lt;&lt;std::setprecision(2)&lt;&lt;maze-&gt;elapsed_ms/1000.0&lt;&lt;" s\n";</code>

Include separate manual/automatic movement totals and elapsed seconds with two decimals.

<a id="l3049"></a>
**L3049** — <code>            reported=true;ui.show(Screen::Result);</code>

Mark the outcome reported and transition to Result, stopping subsequent gameplay ticks.

<a id="l3050"></a>
**L3050** — <code>        } else if(maze-&gt;now-last_console&gt;=1000) {maze-&gt;log_status();last_console=maze-&gt;now;}</code>

While still playing, emit a state summary at most once per 1,000 simulation milliseconds and update the report timestamp.

<a id="l3051"></a>
**L3051** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3052"></a>
**L3052** — <code>    /// Keep the map clear of the top controls and bottom keyboard legend.</code>

Documentation fragment: “Keep the map clear of the top controls and bottom keyboard legend.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3053"></a>
**L3053** — <code>    void draw(int width,int height) {</code>

Draw the world in the central reserved area and then draw the full-frame UI above it.

<a id="l3054"></a>
**L3054** — <code>        if(width&lt;=0||height&lt;=0)return;</code>

Skip minimized framebuffers.

<a id="l3055"></a>
**L3055** — <code>        if(maze&amp;&amp;renderer&amp;&amp;ui.screen!=Screen::Setup&amp;&amp;ui.screen!=Screen::Loading) {</code>

Only draw a world when both game objects exist and the screen is not Setup or Loading.

<a id="l3056"></a>
**L3056** — <code>            UiTransform t(width,height);</code>

Use the UI's uniform transform to calculate physical HUD boundaries.

<a id="l3057"></a>
**L3057** — <code>            int bottom=int(t.y+702*t.scale),top=int(t.y+94*t.scale);</code>

Convert logical y = 94 and y = 702 into framebuffer offsets, reserving top controls and bottom instructions.

<a id="l3058"></a>
**L3058** — <code>            renderer-&gt;draw(width,std::max(1,bottom-top),*maze,height-bottom);</code>

Render the world between those boundaries, converting the top-origin UI position into OpenGL's bottom-origin viewport offset.

<a id="l3059"></a>
**L3059** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3060"></a>
**L3060** — <code>        menu.draw(width,height,ui,maze.get());</code>

Draw menus/HUD/modal overlays last across the full framebuffer; pass the Maze pointer when one exists.

<a id="l3061"></a>
**L3061** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3062"></a>
**L3062** — <code>};</code>

Close the aggregate/class definition or initializer and terminate its declaration.

<a id="l3063"></a>
**L3063** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Input callbacks

<a id="l3064"></a>
**L3064** — <code>/// Read current cursor coordinates using the same DPI-aware transform as rendering.</code>

Documentation fragment: “Read current cursor coordinates using the same DPI-aware transform as rendering.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3065"></a>
**L3065** — <code>std::pair&lt;float,float&gt; ui_cursor(GLFWwindow* window) {</code>

Convert the live GLFW cursor into the logical coordinate system used by widget rectangles.

<a id="l3066"></a>
**L3066** — <code>    double x=0,y=0;int ww=0,wh=0,fw=0,fh=0;</code>

Allocate window/framebuffer dimensions and cursor coordinates for the query.

<a id="l3067"></a>
**L3067** — <code>    glfwGetCursorPos(window,&amp;x,&amp;y);glfwGetWindowSize(window,&amp;ww,&amp;wh);glfwGetFramebufferSize(window,&amp;fw,&amp;fh);</code>

Read cursor position, logical window size, and physical framebuffer size separately for high-DPI correctness.

<a id="l3068"></a>
**L3068** — <code>    return UiTransform(fw,fh).pointer(x,y,ww,wh,fw,fh);</code>

Apply the common UiTransform inverse mapping, avoiding mismatched rendering and hit testing.

<a id="l3069"></a>
**L3069** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3070"></a>
**L3070** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3071"></a>
**L3071** — <code>/// Route menu navigation and game controls according to the visible screen.</code>

Documentation fragment: “Route menu navigation and game controls according to the visible screen.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3072"></a>
**L3072** — <code>void key_callback(GLFWwindow* window,int key,int,int action,int mods) {</code>

Dispatch GLFW key events according to application screen, editing focus, and press/repeat action.

<a id="l3073"></a>
**L3073** — <code>    auto* app=static_cast&lt;GameApplication*&gt;(glfwGetWindowUserPointer(window));if(!app)return;</code>

Recover the application object; ignore events when none is attached.

<a id="l3074"></a>
**L3074** — <code>    if(action==GLFW_RELEASE) {if(key==app-&gt;held_key)app-&gt;release_hold();return;}</code>

Releasing the held key stops walking; other releases are ignored.

<a id="l3075"></a>
**L3075** — <code>    if(action!=GLFW_PRESS&amp;&amp;action!=GLFW_REPEAT)return;</code>

Apart from the release handled above, only initial press and repeat events reach the controls.

<a id="l3076"></a>
**L3076** — <code>    auto&amp; ui=app-&gt;ui;</code>

Bind a shorter reference to the application's UI state.

<a id="l3077"></a>
**L3077** — <code>    // Esc never quits: it leaves a text field, pauses/resumes, or leaves results.</code>

Documentation fragment: “Esc never quits: it leaves a text field, pauses/resumes, or leaves results.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3078"></a>
**L3078** — <code>    // Quitting stays explicit (Q or the QUIT button).</code>

Documentation fragment: “Quitting stays explicit (Q or the QUIT button).” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3079"></a>
**L3079** — <code>    if(key==GLFW_KEY_ESCAPE&amp;&amp;action==GLFW_PRESS) {</code>

Process Escape only on an initial press so key repeat cannot rapidly toggle pause/resume.

<a id="l3080"></a>
**L3080** — <code>        if(ui.screen==Screen::Playing)ui.show(Screen::Paused);</code>

Escape pauses active gameplay.

<a id="l3081"></a>
**L3081** — <code>        else if(ui.screen==Screen::Paused)ui.show(Screen::Playing);</code>

Escape resumes a paused game.

<a id="l3082"></a>
**L3082** — <code>        else if(ui.screen==Screen::Result)ui.show(Screen::Setup);</code>

Escape returns a result screen to setup.

<a id="l3083"></a>
**L3083** — <code>        else if(ui.screen==Screen::Setup&amp;&amp;UiState::is_field(ui.focus))ui.select(Control::Start);</code>

On setup, Esc while editing a field leaves the field and focuses Start; Esc never quits.

<a id="l3084"></a>
**L3084** — <code>        app-&gt;release_hold();</code>

Any Esc press cancels a held direction.

<a id="l3085"></a>
**L3085** — <code>        return;</code>

Stop processing after the Escape action.

<a id="l3086"></a>
**L3086** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3087"></a>
**L3087** — <code>    if(ui.screen==Screen::Loading)return;</code>

Ignore all remaining keys while loading, preventing edits during generation.

<a id="l3088"></a>
**L3088** — <code>    if(key==GLFW_KEY_TAB) {ui.tab((mods&amp;GLFW_MOD_SHIFT)!=0,app-&gt;maze.get());return;}</code>

Tab navigates enabled widgets; Shift reverses direction. A maze pointer permits correct HUD toggle labels and state.

<a id="l3089"></a>
**L3089** — <code>    bool editing=ui.screen==Screen::Setup&amp;&amp;UiState::is_field(ui.focus);</code>

Determine whether the user is editing one of Setup's numeric fields.

<a id="l3090"></a>
**L3090** — <code>    if(editing) {</code>

Give numeric editing priority over general game hotkeys.

<a id="l3091"></a>
**L3091** — <code>        if(key==GLFW_KEY_BACKSPACE)ui.erase();</code>

Backspace removes selected contents or the final digit.

<a id="l3092"></a>
**L3092** — <code>        else if(key==GLFW_KEY_DELETE)ui.erase(true);</code>

Delete clears the whole field.

<a id="l3093"></a>
**L3093** — <code>        else if(key==GLFW_KEY_A&amp;&amp;(mods&amp;GLFW_MOD_CONTROL))ui.replace_field=true;</code>

Control+A arms whole-field replacement. The implementation checks Control, not macOS Command.

<a id="l3094"></a>
**L3094** — <code>        else if(key==GLFW_KEY_UP)ui.step(ui.focus,1);</code>

Up increments the focused numeric field using its bounds and odd-dimension rule.

<a id="l3095"></a>
**L3095** — <code>        else if(key==GLFW_KEY_DOWN)ui.step(ui.focus,-1);</code>

Down decrements the focused numeric field.

<a id="l3096"></a>
**L3096** — <code>        else if(key==GLFW_KEY_ENTER||key==GLFW_KEY_KP_ENTER)ui.select(Control::Start);</code>

Enter while editing moves focus to Start; a subsequent confirmation activates it if enabled.

<a id="l3097"></a>
**L3097** — <code>        return;</code>

Consume all keys while editing, so A/D/W/S/Q cannot accidentally trigger game actions there.

<a id="l3098"></a>
**L3098** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3099"></a>
**L3099** — <code>    if(action!=GLFW_PRESS)return;</code>

Outside editing, ignore OS key-repeat events; held movement is driven by tick() at a fixed cadence instead.

<a id="l3100"></a>
**L3100** — <code>    if(key==GLFW_KEY_ENTER||key==GLFW_KEY_KP_ENTER||key==GLFW_KEY_SPACE) {ui.activate(ui.focus,app-&gt;maze.get());return;}</code>

Enter, keypad Enter, or Space activates the focused visible control.

<a id="l3101"></a>
**L3101** — <code>    if(key==GLFW_KEY_Q) {ui.quit=true;return;}</code>

Q requests exit outside numeric editing.

<a id="l3102"></a>
**L3102** — <code>    if(ui.screen!=Screen::Playing||!app-&gt;maze||!app-&gt;renderer)return;</code>

All remaining gameplay controls require Playing and both installed game/renderer objects.

<a id="l3103"></a>
**L3103** — <code>    if(key==GLFW_KEY_HOME) {app-&gt;renderer-&gt;reset_view();return;}</code>

Home resets the camera to fit the entire maze.

<a id="l3104"></a>
**L3104** — <code>    if(key==GLFW_KEY_EQUAL||key==GLFW_KEY_KP_ADD) {app-&gt;renderer-&gt;change_zoom(1);return;}</code>

The equals/plus key or keypad add applies one positive zoom step.

<a id="l3105"></a>
**L3105** — <code>    if(key==GLFW_KEY_MINUS||key==GLFW_KEY_KP_SUBTRACT) {app-&gt;renderer-&gt;change_zoom(-1);return;}</code>

Minus or keypad subtract applies one negative zoom step.

<a id="l3106"></a>
**L3106** — <code>    if(key==GLFW_KEY_G) {app-&gt;release_hold();ui.activate(Control::AutoSolve,app-&gt;maze.get());return;}</code>

G cancels any hold before toggling auto-solve.

<a id="l3107"></a>
**L3107** — <code>    switch(key) {</code>

Dispatch the four movement directions in row/column coordinates.

<a id="l3108"></a>
**L3108** — <code>        case GLFW_KEY_W:case GLFW_KEY_UP:app-&gt;press_move(key,-1,0);break;</code>

W or Up moves one row up and starts a hold.

<a id="l3109"></a>
**L3109** — <code>        case GLFW_KEY_S:case GLFW_KEY_DOWN:app-&gt;press_move(key,1,0);break;</code>

S or Down moves one row down and starts a hold.

<a id="l3110"></a>
**L3110** — <code>        case GLFW_KEY_A:case GLFW_KEY_LEFT:app-&gt;press_move(key,0,-1);break;</code>

A or Left moves one column left and starts a hold.

<a id="l3111"></a>
**L3111** — <code>        case GLFW_KEY_D:case GLFW_KEY_RIGHT:app-&gt;press_move(key,0,1);break;</code>

D or Right moves one column right and starts a hold.

<a id="l3112"></a>
**L3112** — <code>        default:break;</code>

Other gameplay keys have no action.

<a id="l3113"></a>
**L3113** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3114"></a>
**L3114** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3115"></a>
**L3115** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3116"></a>
**L3116** — <code>/// Text input uses GLFW Unicode events; only decimal digits reach numeric fields.</code>

Documentation fragment: “Text input uses GLFW Unicode events; only decimal digits reach numeric fields.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3117"></a>
**L3117** — <code>void character_callback(GLFWwindow* window,unsigned int codepoint) {</code>

Receive GLFW text codepoints separately from physical-key events, enabling consistent numeric typing.

<a id="l3118"></a>
**L3118** — <code>    auto* app=static_cast&lt;GameApplication*&gt;(glfwGetWindowUserPointer(window));if(app)app-&gt;ui.digit(codepoint);</code>

Forward the codepoint to UiState::digit when the application is attached; that method filters to decimal ASCII digits.

<a id="l3119"></a>
**L3119** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3120"></a>
**L3120** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3121"></a>
**L3121** — <code>/// Activate menu buttons on a complete left click, with no click-through to gameplay.</code>

Documentation fragment: “Activate menu buttons on a complete left click, with no click-through to gameplay.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3122"></a>
**L3122** — <code>void mouse_callback(GLFWwindow* window,int button,int action,int) {</code>

Translate GLFW mouse button actions into UI press/release behavior.

<a id="l3123"></a>
**L3123** — <code>    if(button!=GLFW_MOUSE_BUTTON_LEFT)return;</code>

Ignore every mouse button except left click.

<a id="l3124"></a>
**L3124** — <code>    auto* app=static_cast&lt;GameApplication*&gt;(glfwGetWindowUserPointer(window));if(!app)return;</code>

Retrieve the application pointer and ignore detached-window events.

<a id="l3125"></a>
**L3125** — <code>    auto p=ui_cursor(window);app-&gt;ui.pointer_button(action==GLFW_PRESS,p.first,p.second,app-&gt;maze.get());</code>

Convert cursor coordinates and forward the press flag plus current maze; activation requires a matching press/release target.

<a id="l3126"></a>
**L3126** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3127"></a>
**L3127** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3128"></a>
**L3128** — <code>/// Keep camera zoom available during play without changing any game state.</code>

Documentation fragment: “Keep camera zoom available during play without changing any game state.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3129"></a>
**L3129** — <code>void scroll_callback(GLFWwindow* window,double,double dy) {</code>

Handle mouse-wheel input as a visual zoom operation.

<a id="l3130"></a>
**L3130** — <code>    auto* app=static_cast&lt;GameApplication*&gt;(glfwGetWindowUserPointer(window));</code>

Retrieve application ownership from the window.

<a id="l3131"></a>
**L3131** — <code>    if(app&amp;&amp;app-&gt;renderer&amp;&amp;app-&gt;ui.screen==Screen::Playing)app-&gt;renderer-&gt;change_zoom(dy);</code>

Zoom only during Playing with a valid world renderer; menus and paused screens ignore wheel motion.

<a id="l3132"></a>
**L3132** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3133"></a>
**L3133** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3134"></a>
**L3134** — <code>/// Pause automatically when the player switches away from the game window.</code>

Documentation fragment: “Pause automatically when the player switches away from the game window.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3135"></a>
**L3135** — <code>void focus_callback(GLFWwindow* window,int focused) {</code>

Handle focus transitions from GLFW.

<a id="l3136"></a>
**L3136** — <code>    auto* app=static_cast&lt;GameApplication*&gt;(glfwGetWindowUserPointer(window));</code>

Retrieve the attached application pointer.

<a id="l3137"></a>
**L3137** — <code>    if(app&amp;&amp;!focused)app-&gt;release_hold();</code>

Losing window focus cancels a held direction.

<a id="l3138"></a>
**L3138** — <code>    if(app&amp;&amp;!focused&amp;&amp;app-&gt;ui.screen==Screen::Playing)app-&gt;ui.show(Screen::Paused);</code>

Losing focus during Playing pauses immediately. Regaining focus does not automatically resume.

<a id="l3139"></a>
**L3139** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3140"></a>
**L3140** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## GL loader, frame limiter, and native window loop

<a id="l3141"></a>
**L3141** — <code>#ifndef MAZERUNNER_APP_TEST</code>

Exclude native window/loader startup from application tests, which inject callbacks and a test GL context.

<a id="l3142"></a>
**L3142** — <code>/// Initialize GLEW after context creation, retaining the narrow Wayland exception</code>

Documentation fragment: “Initialize GLEW after context creation, retaining the narrow Wayland exception” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3143"></a>
**L3143** — <code>/// used by V4.4 and verifying the actual GL capabilities before rendering.</code>

Documentation fragment: “used by V4.4 and verifying the actual GL capabilities before rendering.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3144"></a>
**L3144** — <code>void initialize_glew() {</code>

Initialize GLEW after making an OpenGL context current, then verify the capabilities this renderer actually requires.

<a id="l3145"></a>
**L3145** — <code>    glewExperimental = GL_TRUE;</code>

Enable GLEW's experimental entry-point discovery, important for core-profile function loading.

<a id="l3146"></a>
**L3146** — <code>    GLenum status = glewInit();</code>

Run the loader and retain its explicit status code.

<a id="l3147"></a>
**L3147** — <code>    bool no_glx_on_wayland = false;</code>

Default to rejecting any GLEW failure until the narrow Wayland exception is established.

<a id="l3148"></a>
**L3148** — <code>#if defined(GLFW_PLATFORM_WAYLAND) &amp;&amp; defined(GLEW_ERROR_NO_GLX_DISPLAY)</code>

Compile the exception only when both the GLFW platform API constant and GLEW's no-GLX diagnostic constant are available.

<a id="l3149"></a>
**L3149** — <code>    no_glx_on_wayland = status == GLEW_ERROR_NO_GLX_DISPLAY &amp;&amp; glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;</code>

Tolerate the specific no-GLX-display status only when GLFW confirms the active platform is Wayland; unrelated loader errors remain fatal.

<a id="l3150"></a>
**L3150** — <code>#endif</code>

Close the currently active conditional-compilation region; this directive itself executes no runtime code.

<a id="l3151"></a>
**L3151** — <code>    if (status != GLEW_OK &amp;&amp; !no_glx_on_wayland)</code>

Reject every other non-success GLEW status.

<a id="l3152"></a>
**L3152** — <code>        throw std::runtime_error(std::string("Failed to initialize GLEW: ") + reinterpret_cast&lt;const char*&gt;(glewGetErrorString(status)));</code>

Convert the loader's diagnostic to a runtime_error for top-level reporting.

<a id="l3153"></a>
**L3153** — <code>    if (!GLEW_VERSION_3_3 || !GLEW_VERSION_3_0 || !GLEW_VERSION_2_0 || !GLEW_VERSION_1_5 ||</code>

Require advertised support for the relevant GL feature versions, including OpenGL 3.3.

<a id="l3154"></a>
**L3154** — <code>        !glCreateShader || !glCreateProgram || !glGenVertexArrays || !glGenBuffers)</code>

Also require non-null shader/program/VAO/buffer entry points; version flags alone are insufficient.

<a id="l3155"></a>
**L3155** — <code>        throw std::runtime_error("OpenGL 3.3 shader, vertex-array, and buffer support is required.");</code>

Explain the minimum GL requirements when capability validation fails.

<a id="l3156"></a>
**L3156** — <code>    // Drain loader diagnostics; core-profile GL_INVALID_ENUM is tolerated here only.</code>

Documentation fragment: “Drain loader diagnostics; core-profile GL_INVALID_ENUM is tolerated here only.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3157"></a>
**L3157** — <code>    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())</code>

Drain loader-generated GL diagnostics before rendering begins.

<a id="l3158"></a>
**L3158** — <code>        if (error != GL_INVALID_ENUM) throw std::runtime_error("OpenGL initialization error: " + std::to_string(error));</code>

Tolerate GL_INVALID_ENUM only in this initialization drain; propagate any other GL error as a startup failure.

<a id="l3159"></a>
**L3159** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3160"></a>
**L3160** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

<a id="l3161"></a>
**L3161** — <code>/// Open the graphical setup immediately. Loading, play, pause, and results share</code>

Documentation fragment: “Open the graphical setup immediately. Loading, play, pause, and results share” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3162"></a>
**L3162** — <code>/// one window; elapsed menu/loading time never advances the maze simulation.</code>

Documentation fragment: “one window; elapsed menu/loading time never advances the maze simulation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3163"></a>
**L3163** — <code>int run_graphical(std::uint32_t seed,bool quiet=false,bool verbose=false,</code>

Open the native graphical application with seed and logging controls.

<a id="l3164"></a>
**L3164** — <code>                  Difficulty difficulty=Difficulty::Medium,bool fixed_seed=false) {</code>

Default to Medium and non-fixed replay seeds unless startup options specify otherwise.

<a id="l3165"></a>
**L3165** — <code>    if(!glfwInit())throw std::runtime_error("Failed to initialize GLFW.");</code>

Initialize GLFW and throw a clear error if the windowing backend cannot start.

<a id="l3166"></a>
**L3166** — <code>    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);</code>

Request an OpenGL 3.3 context.

<a id="l3167"></a>
**L3167** — <code>    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);</code>

Request the core profile used by the GLSL and VAO-based renderer.

<a id="l3168"></a>
**L3168** — <code>    glfwWindowHint(GLFW_DEPTH_BITS,24);glfwWindowHint(GLFW_SAMPLES,0);</code>

Request a 24-bit depth buffer and disable multisampling for crisp opaque pixel geometry.

<a id="l3169"></a>
**L3169** — <code>#ifdef __APPLE__</code>

Apply the following context hint only on Apple builds.

<a id="l3170"></a>
**L3170** — <code>    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);</code>

Request a forward-compatible context, as required for modern core-profile OpenGL contexts on macOS.

<a id="l3171"></a>
**L3171** — <code>#endif</code>

Close the currently active conditional-compilation region; this directive itself executes no runtime code.

<a id="l3172"></a>
**L3172** — <code>    GLFWwindow* window=glfwCreateWindow(1280,960,MAZERUNNER_TITLE,nullptr,nullptr);</code>

Create the window with the shared version title.

<a id="l3173"></a>
**L3173** — <code>    if(!window) {glfwTerminate();throw std::runtime_error("Failed to create GLFW window.");}</code>

Terminate GLFW and throw if window creation failed.

<a id="l3174"></a>
**L3174** — <code>    try {</code>

Protect the context-owning application lifetime with cleanup on exceptions.

<a id="l3175"></a>
**L3175** — <code>        glfwMakeContextCurrent(window);glfwSwapInterval(1);initialize_glew();</code>

Make this context current, request synchronized buffer swaps, and initialize/validate GLEW before creating renderers.

<a id="l3176"></a>
**L3176** — <code>        glfwSetWindowSizeLimits(window,800,600,GLFW_DONT_CARE,GLFW_DONT_CARE);</code>

Set an 800×600 minimum window size with no upper bound.

<a id="l3177"></a>
**L3177** — <code>        GameApplication app(seed,fixed_seed,quiet,verbose,difficulty);</code>

Construct the application and reusable UI renderer while the context is current.

<a id="l3178"></a>
**L3178** — <code>        glfwSetWindowUserPointer(window,&amp;app);glfwSetKeyCallback(window,key_callback);</code>

Attach the application's address to the window and register keyboard handling.

<a id="l3179"></a>
**L3179** — <code>        glfwSetCharCallback(window,character_callback);glfwSetMouseButtonCallback(window,mouse_callback);</code>

Register text-input and mouse-click callbacks.

<a id="l3180"></a>
**L3180** — <code>        glfwSetScrollCallback(window,scroll_callback);glfwSetWindowFocusCallback(window,focus_callback);</code>

Register scrolling and focus-loss callbacks.

<a id="l3181"></a>
**L3181** — <code>        if(!quiet)std::cout&lt;&lt;MAZERUNNER_TITLE " | Graphical setup ready.\n"</code>

Print the startup banner with the shared title.

<a id="l3182"></a>
**L3182** — <code>            &lt;&lt;"WASD/arrows: move | G: auto-solve | Esc: pause | Q: quit\n"</code>

Print movement, auto-solve, pause, and quit controls.

<a id="l3183"></a>
**L3183** — <code>            &lt;&lt;"Wheel or +/-: zoom | Home: fit maze | State logs enabled.\n";</code>

Print camera controls and confirm that state logs remain enabled.

<a id="l3184"></a>
**L3184** — <code>        auto previous=std::chrono::steady_clock::now();</code>

Initialize a monotonic host timestamp, avoiding wall-clock adjustments during gameplay.

<a id="l3185"></a>
**L3185** — <code>        while(!glfwWindowShouldClose(window)&amp;&amp;!app.ui.quit) {</code>

Run until the OS requests window closure or UI input sets quit.

<a id="l3186"></a>
**L3186** — <code>            auto current=std::chrono::steady_clock::now();</code>

Read the current monotonic host time.

<a id="l3187"></a>
**L3187** — <code>            // Frame limiter: with vsync disabled by the driver, cap the loop near</code>

Documentation fragment: “Frame limiter: with vsync disabled by the driver, cap the loop near” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3188"></a>
**L3188** — <code>            // 240 fps instead of spinning a CPU core at 100%.</code>

Documentation fragment: “240 fps instead of spinning a CPU core at 100%.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3189"></a>
**L3189** — <code>            const auto frame_deadline=current+std::chrono::microseconds(4166);</code>

Earliest time the next frame may start: about 4.2 ms, capping the loop near 240 fps when vsync is unavailable.

<a id="l3190"></a>
**L3190** — <code>            Millis delta=std::chrono::duration_cast&lt;std::chrono::milliseconds&gt;(current-previous).count();</code>

Compute elapsed whole milliseconds since the retained timestamp.

<a id="l3191"></a>
**L3191** — <code>            previous+=std::chrono::milliseconds(delta);</code>

Advance that timestamp by the consumed integer duration, retaining sub-millisecond remainder for later iterations.

<a id="l3192"></a>
**L3192** — <code>            app.tick(delta);glfwPollEvents();</code>

Advance the simulation before polling fresh native input. The application consumes elapsed time only while Playing.

<a id="l3193"></a>
**L3193** — <code>            if(app.ui.quit||glfwWindowShouldClose(window))break;</code>

Exit promptly after input processing if either close condition is now set.

<a id="l3194"></a>
**L3194** — <code>            if(app.ui.screen==Screen::Setup&amp;&amp;app.maze)app.discard_game();</code>

Dispose of a completed/abandoned Maze after the user returns to setup.

<a id="l3195"></a>
**L3195** — <code>            app.finish_generation(glfwGetWindowAttrib(window,GLFW_FOCUSED)==GLFW_TRUE);</code>

Install a ready generation result and respect current focus when choosing Playing or Paused.

<a id="l3196"></a>
**L3196** — <code>            auto mouse=ui_cursor(window);app.ui.hover=app.ui.hit(mouse.first,mouse.second,app.maze.get());</code>

Update hover by converting the cursor and testing the current visible widgets.

<a id="l3197"></a>
**L3197** — <code>            int width=0,height=0;glfwGetFramebufferSize(window,&amp;width,&amp;height);</code>

Read actual framebuffer pixels for both renderers, supporting resize and high-DPI displays.

<a id="l3198"></a>
**L3198** — <code>            app.draw(width,height);glfwSwapBuffers(window);</code>

Render the current screen and present it by swapping buffers.

<a id="l3199"></a>
**L3199** — <code>            // Publish the loading screen before starting CPU-heavy generation.</code>

Documentation fragment: “Publish the loading screen before starting CPU-heavy generation.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3200"></a>
**L3200** — <code>            if(app.ui.screen==Screen::Loading&amp;&amp;!app.generation.valid()) {</code>

Launch generation only after a Loading frame has been presented, keeping slow maze work from hiding the loading feedback.

<a id="l3201"></a>
**L3201** — <code>                try {app.begin_generation();}</code>

Start the worker inside a recoverable exception boundary.

<a id="l3202"></a>
**L3202** — <code>                catch(const std::exception&amp; error) {</code>

Catch errors launching the job itself; worker exceptions are handled separately by finish_generation.

<a id="l3203"></a>
**L3203** — <code>                    app.ui.show(Screen::Setup);app.ui.message="COULD NOT START MAZE GENERATION. TRY AGAIN.";</code>

Return to Setup with an actionable message if job launch fails.

<a id="l3204"></a>
**L3204** — <code>                    std::cerr&lt;&lt;"Maze setup failed: "&lt;&lt;error.what()&lt;&lt;'\n';</code>

Log the specific launch error to stderr.

<a id="l3205"></a>
**L3205** — <code>                }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3206"></a>
**L3206** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3207"></a>
**L3207** — <code>            if(app.ui.screen!=Screen::Playing)glfwWaitEventsTimeout(0.016);</code>

When gameplay is inactive, wait up to 16 ms for events to reduce menu-loop CPU usage without a long blocking sleep.

<a id="l3208"></a>
**L3208** — <code>            else std::this_thread::sleep_until(frame_deadline);</code>

While playing, sleep until the frame deadline; with vsync on, buffer swaps already take longer, so this rarely waits.

<a id="l3209"></a>
**L3209** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3210"></a>
**L3210** — <code>        glfwSetWindowUserPointer(window,nullptr);</code>

Detach the application pointer before its scoped destruction so later callbacks cannot refer to destroyed state.

<a id="l3211"></a>
**L3211** — <code>    } catch(...) {glfwSetWindowUserPointer(window,nullptr);glfwDestroyWindow(window);glfwTerminate();throw;}</code>

On exceptions, detach callbacks' user data, destroy the window, terminate GLFW, and rethrow after application resources unwind.

<a id="l3212"></a>
**L3212** — <code>    glfwDestroyWindow(window);glfwTerminate();return 0;</code>

After normal scoped application destruction, destroy the window/context, terminate GLFW, and return success.

<a id="l3213"></a>
**L3213** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3214"></a>
**L3214** — <code>#endif // MAZERUNNER_APP_TEST</code>

Close the native-startup exclusion used by MAZERUNNER_APP_TEST.

<a id="l3215"></a>
**L3215** — <code>#endif // MAZERUNNER_RENDER_TEST</code>

Close the application/window exclusion used by MAZERUNNER_RENDER_TEST.

<a id="l3216"></a>
**L3216** — <code>#endif // MAZERUNNER_HEADLESS</code>

Close the OpenGL-dependent region excluded by MAZERUNNER_HEADLESS.

<a id="l3217"></a>
**L3217** — <code>[blank line]</code>

Blank separator for readability; this physical line contributes no tokens or runtime behavior.

## Program entry point

<a id="l3218"></a>
**L3218** — <code>#ifndef MAZERUNNER_NO_MAIN</code>

Allow test translation units to include the production implementation while supplying their own main function.

<a id="l3219"></a>
**L3219** — <code>/// Dispatch normal play or deterministic simulation; report failures as exit code 1.</code>

Documentation fragment: “Dispatch normal play or deterministic simulation; report failures as exit code 1.” This comment belongs to the adjacent source explanation; the implementation, rather than the comment, determines executable behavior.

<a id="l3220"></a>
**L3220** — <code>int main(int argc, char** argv) {</code>

Program entry point for both graphical and headless executables.

<a id="l3221"></a>
**L3221** — <code>    try {</code>

Convert standard-library/application exceptions into a terminal diagnostic and nonzero process status.

<a id="l3222"></a>
**L3222** — <code>        if (argc &gt;= 2 &amp;&amp; std::string(argv[1]) == "--simulate") return run_simulation(argc, argv);</code>

Recognize --simulate only as the first argument and delegate all simulation parsing/execution to run_simulation.

<a id="l3223"></a>
**L3223** — <code>        if (argc == 2 &amp;&amp; std::string(argv[1]) == "--help") {</code>

Print help only for the standalone --help invocation.

<a id="l3224"></a>
**L3224** — <code>            std::cout &lt;&lt; MAZERUNNER_TITLE "\n  [--seed N] [--difficulty low|medium|high] [--verbose] [--quiet]\n"</code>

Help output begins with the shared title.

<a id="l3225"></a>
**L3225** — <code>                      &lt;&lt; "  --simulate ROWS COLS MONSTERS SEED [LIMIT_MS] [--difficulty low|medium|high] [--verbose] [--quiet]\n"</code>

Document simulation's positional rows, columns, monster count, seed, optional duration, and flags.

<a id="l3226"></a>
**L3226** — <code>                      &lt;&lt; "Interactive play opens graphical setup; --difficulty preselects a card. No terminal input is required.\n"</code>

Explain that normal play opens graphical setup and needs no terminal prompt interaction.

<a id="l3227"></a>
**L3227** — <code>                      &lt;&lt; "Simulation defaults to Medium. Low offers more routes and earlier relief; High increases pursuit pressure.\n"</code>

Describe the default Medium profile and the qualitative Low/High differences.

<a id="l3228"></a>
**L3228** — <code>                      &lt;&lt; "State logs are enabled by default; --verbose also logs every monster step.\n"</code>

Explain that ordinary state logging is on by default and --verbose adds per-monster-step detail.

<a id="l3229"></a>
**L3229** — <code>                      &lt;&lt; "Director radio hotfix: movement sends map-wide area reports. Local footsteps travel along open corridors.\n"</code>

Describe the independent map-wide movement report and corridor-local acoustic signal in CLI help.

<a id="l3230"></a>
**L3230** — <code>                      &lt;&lt; "RELIEF widens searches, spaces out reports, and releases interceptors while the solver is confined.\n"</code>

Explain the observable effects of relief on report frequency, search precision and interceptor roles.

<a id="l3231"></a>
**L3231** — <code>                      &lt;&lt; "Mouse/Tab/Enter: setup. WASD/arrows (hold to keep walking): move. G: auto-solve. Esc: pause. Q: quit. Wheel/+/-: zoom. Home: fit.\n";</code>

Control summary, including hold-to-move.

<a id="l3232"></a>
**L3232** — <code>            return 0;</code>

Return successful completion after displaying help.

<a id="l3233"></a>
**L3233** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3234"></a>
**L3234** — <code>#ifndef MAZERUNNER_HEADLESS</code>

Compile graphical-option parsing only in non-headless builds.

<a id="l3235"></a>
**L3235** — <code>        std::uint32_t seed = std::random_device{}();</code>

Obtain a fresh initial seed from std::random_device when the user has not supplied one.

<a id="l3236"></a>
**L3236** — <code>        bool quiet = false, verbose = false, fixed_seed = false;</code>

Initialize normal logging, normal verbosity, and non-fixed replay seeding.

<a id="l3237"></a>
**L3237** — <code>        Difficulty difficulty = Difficulty::Medium;</code>

Use Medium as the initial UI difficulty selection.

<a id="l3238"></a>
**L3238** — <code>        for (int i = 1; i &lt; argc; ++i) {</code>

Process remaining graphical command-line arguments in order.

<a id="l3239"></a>
**L3239** — <code>            std::string arg = argv[i];</code>

Copy the next option into a string for exact comparison.

<a id="l3240"></a>
**L3240** — <code>            if (arg == "--seed" &amp;&amp; i + 1 &lt; argc) {</code>

Recognize --seed only when a following numeric token exists.

<a id="l3241"></a>
**L3241** — <code>                seed = static_cast&lt;std::uint32_t&gt;(parse_number(argv[++i], std::numeric_limits&lt;std::uint32_t&gt;::max()));</code>

Parse a bounded unsigned 32-bit value and consume the seed token.

<a id="l3242"></a>
**L3242** — <code>                fixed_seed = true;</code>

Enable fixed-seed replays, making subsequent runs reproducible with the same settings/build.

<a id="l3243"></a>
**L3243** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3244"></a>
**L3244** — <code>            else if (arg == "--difficulty") {</code>

Recognize the difficulty preselection option.

<a id="l3245"></a>
**L3245** — <code>                if (i + 1 &gt;= argc) throw std::runtime_error("--difficulty requires low, medium, or high.");</code>

Reject a missing difficulty value with a targeted error.

<a id="l3246"></a>
**L3246** — <code>                difficulty = parse_difficulty(argv[++i]);</code>

Parse and consume low/medium/high using the central profile-name parser.

<a id="l3247"></a>
**L3247** — <code>            }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3248"></a>
**L3248** — <code>            else if (arg == "--quiet") quiet = true;</code>

Suppress normal stdout state logging when --quiet is present.

<a id="l3249"></a>
**L3249** — <code>            else if (arg == "--verbose") verbose = true;</code>

Enable per-monster-step verbosity when --verbose is present; quiet still disables the logger destination.

<a id="l3250"></a>
**L3250** — <code>            else throw std::runtime_error("Usage: mazerunnerV" MAZERUNNER_VERSION " [--seed N] [--difficulty low|medium|high] [--verbose] [--quiet]");</code>

Usage error naming the versioned executable.

<a id="l3251"></a>
**L3251** — <code>        }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3252"></a>
**L3252** — <code>        return run_graphical(seed, quiet, verbose, difficulty, fixed_seed);</code>

Launch the graphical setup with the parsed startup policies.

<a id="l3253"></a>
**L3253** — <code>#else</code>

Select the alternate branch compiled into headless builds.

<a id="l3254"></a>
**L3254** — <code>        throw std::runtime_error("Headless build: use --simulate ROWS COLS MONSTERS SEED [LIMIT_MS]");</code>

Reject a normal graphical launch in a headless binary and explain the required --simulate form.

<a id="l3255"></a>
**L3255** — <code>#endif</code>

Close the currently active conditional-compilation region; this directive itself executes no runtime code.

<a id="l3256"></a>
**L3256** — <code>    } catch (const std::exception&amp; error) {</code>

Catch standard exceptions from parsing, simulation, window startup, or rendering.

<a id="l3257"></a>
**L3257** — <code>        std::cerr &lt;&lt; "Error: " &lt;&lt; error.what() &lt;&lt; '\n';</code>

Print the diagnostic with an Error prefix to stderr.

<a id="l3258"></a>
**L3258** — <code>        return 1;</code>

Return exit code one for an operational/input failure; ordinary caught/time-limit simulation outcomes return zero elsewhere.

<a id="l3259"></a>
**L3259** — <code>    }</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3260"></a>
**L3260** — <code>}</code>

Close the innermost statement block or function body. Local variables in that scope cease to exist here; control continues according to the surrounding loop, branch, or caller.

<a id="l3261"></a>
**L3261** — <code>#endif</code>

Close the conditional main-function definition so included test builds can provide their own entry point.

