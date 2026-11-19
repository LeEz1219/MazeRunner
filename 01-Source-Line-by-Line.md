# MazeRunner V5.4.3c function guide

Each entry links to its definition and gives its behavior and design rationale. The guide follows declaration order and includes named C++ functions, constructors, deleted copy operations and the two embedded GLSL entry points. Local lambdas belong to the enclosing function. [Production source](src/mazerunnerV5.4.3c.cpp)

For an encounter, begin with `Maze::advance_to`, `Maze::publish_movement_hint` and `Maze::update_monsters`. For the graphics path, begin with `GameApplication::draw`, `Renderer::draw` and `UiRenderer::draw`.

## Difficulty and map coordinates

Use these entry points to interpret a profile and establish the grid an encounter will use.

| Function | What it does | Why it exists |
|---|---|---|
| [`Point::operator==`](src/mazerunnerV5.4.3c.cpp#L101) | Coordinate equality compares both axes. Vector<Point> route comparisons rely on this operation. | Coordinates serve as values in route and replay comparisons. |
| [`Point::operator!=`](src/mazerunnerV5.4.3c.cpp#L103) | Inequality delegates to equality so the two operators remain consistent. | Keeping inequality tied to equality prevents inconsistent coordinate tests. |
| [`difficulty_settings`](src/mazerunnerV5.4.3c.cpp#L131) | Return a complete immutable balance profile. constexpr permits compile-time evaluation for known difficulty values. | A single profile feeds generation, clocks, clues and the calibration harness. |
| [`parse_difficulty`](src/mazerunnerV5.4.3c.cpp#L141) | Normalize a supplied difficulty token and return the corresponding enum. Used by CLI parsing and the retained stream-prompt helper. | The CLI and interactive prompt accept the same names and numbers. |
| [`prompt_difficulty`](src/mazerunnerV5.4.3c.cpp#L153) | Retained stream-based prompt, useful to tests/legacy callers. The graphical startup path no longer invokes it. | An invalid answer can be retried without losing the next setup input. |
| [`LocationHint::contains`](src/mazerunnerV5.4.3c.cpp#L171) | Membership query for candidate hunting destinations inside a clue rectangle. | Listeners can plan from rectangular evidence without a hidden player coordinate. |
| [`Maze::Maze`](src/mazerunnerV5.4.3c.cpp#L257) | Constructor accepts dimensions, an unsigned seed, and an optional logging stream. Its parameter list continues below. | Normalizing the grid at construction establishes valid dimensions for every index. |
| [`Maze::id`](src/mazerunnerV5.4.3c.cpp#L273) | Checked coordinate-to-index conversion. Returning -1 lets callers reject out-of-bounds neighbors uniformly. | A compact integer key indexes the terrain and every navigation field. |
| [`Maze::point`](src/mazerunnerV5.4.3c.cpp#L277) | Inverse flattening for valid, nonnegative indices: integer division yields row and remainder yields column. | Coordinates are needed for corridor geometry, input and drawing. |
| [`Maze::walkable`](src/mazerunnerV5.4.3c.cpp#L279) | Terrain-only traversability query, deliberately independent of monsters and the player. | One terrain predicate keeps movement and search consistent at the border. |
| [`Maze::manhattan`](src/mazerunnerV5.4.3c.cpp#L283) | Compute the Manhattan heuristic between two valid cells. It ignores walls and therefore underestimates corridor distance. | The inexpensive lower bound rejects obviously distant acoustic candidates. |
| [`Maze::neighbors`](src/mazerunnerV5.4.3c.cpp#L288) | Return the precomputed four-neighbor array by const reference. Consumers must supply a valid cell index. | A common cardinal adjacency order makes traversals reproducible. |
| [`Maze::degree`](src/mazerunnerV5.4.3c.cpp#L290) | Expose cached graph degree for junction and dead-end heuristics. | Searchers need to distinguish junctions, corridor interiors and tips. |

## Generation, graph and patrol coverage

Generation runs once per map. The resulting graph supplies route diversity, corridor distances and intercept points throughout the run.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::generate`](src/mazerunnerV5.4.3c.cpp#L295) | Generate terrain until the profile topology certificate passes, falling back to adding connections after 64 attempts so generation always terminates. Then spawn monsters. | Seeded carving supplies a new playable maze with reproducible terrain and spawns. |
| [`Maze::route_diversity_met`](src/mazerunnerV5.4.3c.cpp#L377) | Apply the selected profile’s route-diversity requirement to the current terrain. | A shared acceptance rule enforces the profile’s required route diversity. |
| [`Maze::add_connections_until_diverse`](src/mazerunnerV5.4.3c.cpp#L385) | Generation fallback: open connecting walls in seeded batches until the requirement holds. Always terminates. | Bounded retries keep awkward thin grids from spending unbounded time regenerating. |
| [`Maze::has_at_least_two_solutions`](src/mazerunnerV5.4.3c.cpp#L415) | Determine whether at least two distinct simple entrance-to-exit paths exist in O(V+E), using iterative bridge analysis. | An alternate route must help get from the entrance to the exit. |
| [`Maze::solution_diversity`](src/mazerunnerV5.4.3c.cpp#L457) | Compute a capped lower-bound certificate for up to four simple solutions, with optional minimum-size filtering of useful cyclic blocks. | A conservative lower bound checks Low's extra choices without enumerating paths. |
| [`Maze::rebuild_graph`](src/mazerunnerV5.4.3c.cpp#L509) | Rebuild adjacency, dead-end structure, cached fields, the exit chokepoint tree, patrol regions, and the terrain signature after terrain changes. | Reusing the graph makes frequent sensing and movement queries inexpensive. |
| [`Maze::terrain_signature`](src/mazerunnerV5.4.3c.cpp#L564) | FNV-1a hash of the terrain, recorded when caches are rebuilt. | Changing terrain invalidates every derived path field. |
| [`Maze::verify_graph`](src/mazerunnerV5.4.3c.cpp#L572) | Debug-only check that terrain was not edited without rebuild_graph(). | Debug builds catch map edits that would otherwise produce stale routes. |
| [`Maze::reset_game`](src/mazerunnerV5.4.3c.cpp#L580) | Reset a run’s dynamic state, including clues, detour history, and patrol regions, without carving a map or reseeding the RNG. | An independent replay starts with fresh clocks, clues, lessons and solver memory. |
| [`Maze::spawn_monsters`](src/mazerunnerV5.4.3c.cpp#L606) | Populate monsters in unique cells far from the player. Their spawn-relative deadlines stay frozen until the first input. | Distinct initial positions avoid immediate stacking and favor room to start. |
| [`Maze::distance_field`](src/mazerunnerV5.4.3c.cpp#L634) | Return a shared reverse BFS field from a least-recently-used cache. A miss traverses the whole terrain graph. | Repeated queries to the same destination can reuse exact corridor distances. |
| [`Maze::LocalField::at`](src/mazerunnerV5.4.3c.cpp#L667) | Read the distance under this field’s generation stamp, or return its radius plus one. | A truncated search still needs a safe distance for cells beyond its radius. |
| [`Maze::local_field`](src/mazerunnerV5.4.3c.cpp#L672) | Build or reuse a bounded BFS by source and radius. Threat forecasts and step listeners can request different bounds. | Short-range threat and hearing queries avoid exploring a whole large maze. |
| [`Maze::build_exit_tree`](src/mazerunnerV5.4.3c.cpp#L757) | Iterative Tarjan traversal rooted at the exit that marks route chokepoints. | Precomputed articulation relationships make frequent cutter assignment practical. |
| [`Maze::exit_chokepoints`](src/mazerunnerV5.4.3c.cpp#L784) | List cells every route from a clue cell to the exit must pass, with exact distances. | An assigned intercept should sit on a route the player actually has to traverse. |
| [`Maze::mark_region`](src/mazerunnerV5.4.3c.cpp#L798) | Record that a monster covered an 8x8 patrol region. | A visit by one agent can spread later patrols across the pack. |
| [`Maze::region_age`](src/mazerunnerV5.4.3c.cpp#L803) | Time since any monster entered the region containing a cell. | Patrol decisions need a comparable age for each candidate region. |

## Diagnostics and encounter measurements

These functions expose profile settings, clocks and counters for the HUD, replays and headless measurements.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::configuration_summary`](src/mazerunnerV5.4.3c.cpp#L811) | Compose the startup summary, including hearing radius, interceptors, sight, and search time, without modifying state. | A player can reproduce a run using the settings printed at startup. |
| [`Maze::director_hint_width`](src/mazerunnerV5.4.3c.cpp#L836) | Nominal uncertain rectangle width equals base side length plus twice the padding. Actual bounds may be clipped by map size. | Tests can observe the local clue footprint during relief. |
| [`Maze::director_phase`](src/mazerunnerV5.4.3c.cpp#L837) | Expose the director's current intensity phase. Positive clue padding corresponds to RELIEF. | The HUD and traces need a consistent name for confinement precision. |
| [`Maze::menace`](src/mazerunnerV5.4.3c.cpp#L838) | Read the Director’s current pressure score from 0 to 100. | Pressure can be compared between otherwise identical encounter replays. |
| [`Maze::withdrawing`](src/mazerunnerV5.4.3c.cpp#L839) | Read the Backstage pacing flag. | The HUD can show Backstage while confinement precision has a separate state. |
| [`Maze::withdrawal_episodes`](src/mazerunnerV5.4.3c.cpp#L840) | Read the count of Backstage episodes begun in this run. | Telemetry distinguishes a new withdrawal episode from another retreat step. |
| [`Maze::withdrawal_steps`](src/mazerunnerV5.4.3c.cpp#L841) | Read the number of cardinal steps physically taken under a withdrawal order. | The release fixtures can distinguish a physical retreat from a duty assignment. |
| [`Maze::junction_scans`](src/mazerunnerV5.4.3c.cpp#L842) | Read the number of junction scans performed in live play. | A real pause at a junction is observable in headless validation. |
| [`Maze::director_reports`](src/mazerunnerV5.4.3c.cpp#L843) | Return the cumulative broadcast count for the HUD, terminal and regressions. | Radio activity can be measured independently of local footsteps. |
| [`Maze::radio_area_label`](src/mazerunnerV5.4.3c.cpp#L845) | Format the current strategic footprint for diagnostics without changing simulation state. | Edge clipping changes what the player actually sees reported on a rectangular map. |
| [`Maze::route_progress_percent`](src/mazerunnerV5.4.3c.cpp#L851) | Calculate the greatest fraction of the entrance-to-exit corridor distance covered so far. | Backtracking should preserve progress already earned in the radio ramp. |
| [`Maze::radio_interval_ms`](src/mazerunnerV5.4.3c.cpp#L861) | Interpolate the normal radio cooldown from the opening interval to its full rate at 50% best progress. | An opening search can expire before the next broadcast while late pursuit tightens. |
| [`Maze::auto_recovery_target`](src/mazerunnerV5.4.3c.cpp#L869) | Expose only the active recovery waypoint ID for testing/diagnostics, or -1 if none. | Recovery detours can be inspected independently of the displayed full route. |
| [`Maze::hearing_radius`](src/mazerunnerV5.4.3c.cpp#L872) | Compute acoustic reach in walkable steps using the profile and longer map axis. | Corridor sound should scale with map size and remain consistent at walls. |
| [`Maze::auto_boldness`](src/mazerunnerV5.4.3c.cpp#L876) | Solver loop-detection level from detours in the last 20 seconds (0–4). | Repeated detours should influence the solver’s appetite for another retreat. |
| [`Maze::cell_label`](src/mazerunnerV5.4.3c.cpp#L882) | Format a terrain-valid cell for logs. This utility does not grant monsters player-position access. | Uniform point formatting makes movement traces comparable. |
| [`Maze::log_event`](src/mazerunnerV5.4.3c.cpp#L889) | Append one timestamped diagnostic event when a logging destination exists. | Headless and graphical play can expose the same timestamped events. |
| [`Maze::log_status`](src/mazerunnerV5.4.3c.cpp#L897) | Emit a snapshot of player state, director status, and hunting/searching counts. | A compact status snapshot supports comparison across replay seeds. |
| [`Maze::monster_name`](src/mazerunnerV5.4.3c.cpp#L917) | One-based monster label for logs. | Stable actor names let a reader follow an individual through job changes. |

## Evidence, pack duties and the Director

Start with `publish_movement_hint` for a player step and `update_director` for a 200 ms pacing tick. A search can provide evidence long after the footstep that caused it.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::radio_sector_size`](src/mazerunnerV5.4.3c.cpp#L920) | Calculate the normal map-wide radio sector before relief padding or clipping by axis. | The Director needs a broader clue while respecting small maps. |
| [`Maze::widen_hint`](src/mazerunnerV5.4.3c.cpp#L925) | Work on a copy of a remembered rectangle so callers choose which stored memory to replace. | Relief can reduce precision without disclosing any new location. |
| [`Maze::make_location_hint`](src/mazerunnerV5.4.3c.cpp#L937) | Quantize an actual displacement by local or radio sector size, clip it to the board and apply current relief. | Quantization gives different hidden positions an identical local report. |
| [`Maze::sector_center`](src/mazerunnerV5.4.3c.cpp#L946) | Compute the midpoint from the clue boundaries shared with the listener. | Hearing can compare sector transitions without storing a player cell. |
| [`Maze::clue_anchor`](src/mazerunnerV5.4.3c.cpp#L951) | Reuse a walkable center when available and return a sentinel for an absent clue. | A walkable representative lets a rectangular clue seed real corridor searches. |
| [`Maze::note_evidence`](src/mazerunnerV5.4.3c.cpp#L967) | Insert a source-clue revision into the pack’s evidence set for a pending tactic. | Several listeners and late-finishing searches can share one causal observation. |
| [`Maze::unlock_lessons`](src/mazerunnerV5.4.3c.cpp#L972) | At a Director tick, unlock the best supported pending tactic with at least two distinct observations. Space unlocks by 12 seconds. | Spacing makes separate learned behaviors visible over a sufficiently long run. |
| [`Maze::junction_ahead`](src/mazerunnerV5.4.3c.cpp#L989) | Search outward from a clue anchor for a usable junction beyond the leading edge of the heard heading. | A lead can intercept using heard direction and known terrain. |
| [`Maze::pack_learned`](src/mazerunnerV5.4.3c.cpp#L1005) | Read the current run’s shared unlock flag for one tactic. | Diagnostics can read a shared lesson without changing its state. |
| [`Maze::lesson_evidence`](src/mazerunnerV5.4.3c.cpp#L1006) | Count the distinct source-clue IDs credited to a tactic. | Tests distinguish observing evidence from reaching the scheduled unlock time. |
| [`Maze::clear_heard_route`](src/mazerunnerV5.4.3c.cpp#L1010) | Clear the heard sector, recent departures, directional streak and continuous-contact flag. | A disconnected chase or widened clue cannot complete an earlier precise route. |
| [`Maze::sector_circuit`](src/mazerunnerV5.4.3c.cpp#L1019) | Follow the observed sectors in order through cardinal passages and require a real closing edge back to the starting cell. | Returning after a delay also happens on straight paths and in cut-open rings. |
| [`Maze::observe_footstep`](src/mazerunnerV5.4.3c.cpp#L1067) | Track up to six audible departures. Credit a sustained U-turn or a continuous return compatible with a terrain cycle. | Only a continuous sequence of audible transitions supports a directional habit. |
| [`Maze::set_duty`](src/mazerunnerV5.4.3c.cpp#L1090) | Transfer pack duty, release an incompatible waypoint and cancel unfinished local-search evidence on withdrawal. | A job change must release incompatible plans and reservations. |
| [`Maze::coordinate_pack`](src/mazerunnerV5.4.3c.cpp#L1101) | Find a single eligible investigator in stable monster order. | One investigator and several perimeter agents reduce crowding on a single clue. |
| [`Maze::deliver_hint`](src/mazerunnerV5.4.3c.cpp#L1120) | Give a hearing monster the latest clue, switching it to hunting and keeping a valid target. | Physical hearing can start a hunt without teaching other agents an exact cell. |
| [`Maze::investigate_report`](src/mazerunnerV5.4.3c.cpp#L1144) | Deliver a coarse report to a monster eligible for remote investigation. | A remote report motivates a search without renewing a listener’s scent. |
| [`Maze::publish_movement_hint`](src/mazerunnerV5.4.3c.cpp#L1161) | Determine acoustic listeners, renew physical contact, publish eligible local clues and independently broadcast a strategic area. | The Director and listeners share a causal step while retaining different precision. |
| [`Maze::assign_interceptors`](src/mazerunnerV5.4.3c.cpp#L1225) | Reserve existing guards and assign eligible listeners to distinct chokepoints within the global profile cap. | Reservations spread eligible guards over reachable chokepoints or learned ambushes. |
| [`Maze::set_director_precision`](src/mazerunnerV5.4.3c.cpp#L1300) | Widen stored clues, clear precise hearing history and release stale guard targets when relief increases. | Widened clues must invalidate stale headings and targets before the next move. |
| [`Maze::withdrawal_target`](src/mazerunnerV5.4.3c.cpp#L1327) | Use a representative of the monster's evidence as the origin for measuring separation. | Retreat needs a reachable waypoint away from known evidence. |
| [`Maze::pace_encounter`](src/mazerunnerV5.4.3c.cpp#L1342) | Initialize the pressure aggregate, sight indicator and withdrawal candidate for one fixed director tick. | Hysteresis and a minimum quiet time prevent rapid flips into and out of withdrawal. |
| [`Maze::update_director`](src/mazerunnerV5.4.3c.cpp#L1403) | Run one fixed-time director sample: confinement, relief escalation, and recovery hysteresis. A manual player standing still is not counted as confined. | Confinement relief depends on sustained movement history and recovery. |

## Simulation events, input and sensing

Follow `advance_to` into expiry and monster decisions to see which event wins when deadlines coincide.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::next_monster_state_change`](src/mazerunnerV5.4.3c.cpp#L1464) | Find the next scent, search or interceptor deadline. | State expiry must take effect even between two movement deadlines. |
| [`Maze::expire_monster_states`](src/mazerunnerV5.4.3c.cpp#L1476) | Expire roles, change an abandoned hunt into a search, and credit an in-sector tip inspection only when the local search expires. | An unsuccessful local sweep can teach the pack only what the searcher checked. |
| [`Maze::sees_player`](src/mazerunnerV5.4.3c.cpp#L1536) | Select the profile range for the monster’s current visual state and test a clear corridor. | Initial notice and an established charge have distinct legal distance windows. |
| [`Maze::line_of_sight`](src/mazerunnerV5.4.3c.cpp#L1539) | Check axial alignment, the caller’s minimum and profile maximum distance, and intervening walls. | An intervening wall must break even perfect axial alignment. |
| [`Maze::advance_to`](src/mazerunnerV5.4.3c.cpp#L1556) | Advance the discrete-event simulation. Monster moves and state expiries are scheduled only after the player’s first input. | Frame size should never reorder simulation events or collision outcomes. |
| [`Maze::update_monsters`](src/mazerunnerV5.4.3c.cpp#L1588) | Move due monsters with a short reservation list, apply High corridor sightings and interceptor hold expiry, and track patrol coverage. | Monsters sharing a deadline need fair ordering and consistent occupancy. |
| [`Maze::set_auto_solve`](src/mazerunnerV5.4.3c.cpp#L1677) | Toggle automatic assistance, reset its tactical memory, and update the overlay while preserving the existing map and actor positions. | Changing control mode releases conflicting movement timers. |
| [`Maze::move_player`](src/mazerunnerV5.4.3c.cpp#L1694) | Manual movement wrapper. Both manual and automatic displacements eventually use commit_player_move. | Manual and automatic steps must use the same movement and collision rules. |
| [`Maze::begin_held_move`](src/mazerunnerV5.4.3c.cpp#L1703) | Start held movement with a direction supplied by the graphical input layer. | A held key should give an immediate first step and deterministic repeats. |
| [`Maze::cancel_held_move`](src/mazerunnerV5.4.3c.cpp#L1712) | Set the next repeat to the inactive sentinel without advancing time. | Focus loss, pause and release must stop future synthetic steps. |
| [`Maze::check_outcome`](src/mazerunnerV5.4.3c.cpp#L1715) | Resolve outcomes with capture precedence, then cancel all pending scent and search deadlines. | Capture and escape need one precedence rule in every input mode. |
| [`Maze::remember`](src/mazerunnerV5.4.3c.cpp#L1737) | Append an action cell to a bounded history deque. Shared by player history and monster recent-cell memory. | Behavioral memory stays bounded during long runs. |
| [`Maze::start_timer`](src/mazerunnerV5.4.3c.cpp#L1742) | Start the clock once and release frozen monsters, preserving their spawn staggering. | The first action releases actors together while preserving their spawn staggering. |
| [`Maze::update_elapsed`](src/mazerunnerV5.4.3c.cpp#L1760) | Elapsed gameplay time is now minus timer_start when active. Pre-movement patrol time is therefore excluded. | The displayed result needs the last elapsed game time even after play freezes. |
| [`Maze::visit_heat`](src/mazerunnerV5.4.3c.cpp#L1764) | Return a linearly time-decayed revisit penalty without mutating stored memory. | Lingering briefly should count differently from repeatedly circling a route. |
| [`Maze::commit_player_move`](src/mazerunnerV5.4.3c.cpp#L1771) | Common transaction for one cardinal move or wait: validate, update clocks/memory, publish movement information, and resolve capture/escape. | Only a successful displacement can publish movement evidence. |

## Target selection, searching and threat prediction

A monster chooses its target from its current job and evidence. The solver estimates what that actor can reach on future actions.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::target_claimed`](src/mazerunnerV5.4.3c.cpp#L1804) | Determine whether another monster of the same behavior state has already selected a candidate waypoint. | Nearby hunters should spread their plans over different plausible cells. |
| [`Maze::move_period`](src/mazerunnerV5.4.3c.cpp#L1811) | Scale the base hunt interval to 130% for patrol, 115% for search or 85% during a visual charge. | Different gaits make patrol, sweep and pursuit distinguishable in motion. |
| [`Maze::sight_check`](src/mazerunnerV5.4.3c.cpp#L1823) | Sample corridor sight at a due monster action. Return no contact, a notice pause or a charge. | A visual chase requires an observable beat before it can accelerate. |
| [`Maze::inspect_cell`](src/mazerunnerV5.4.3c.cpp#L1835) | Refresh the bounded inspection history and remember a lead’s dead-end tip check in its unresolved local clue. | The lead can credit a local trail only after visiting or seeing an in-sector tip. |
| [`Maze::recently_inspected`](src/mazerunnerV5.4.3c.cpp#L1846) | A cell is discounted until its observation ages past the inspection-memory window. | Recent inspections prevent the searcher repeatedly checking the same alcove. |
| [`Maze::scan_junction`](src/mazerunnerV5.4.3c.cpp#L1854) | A scan requires a lead searcher at a junction in the clue area or close to its last sighting. | A pause makes the selected corridor arm legible and records the check. |
| [`Maze::sweep_target`](src/mazerunnerV5.4.3c.cpp#L1882) | Start a sweep from the stored sector and its genuine observation time. | Directing the lead through unexplored branches creates a methodical local search. |
| [`Maze::choose_target`](src/mazerunnerV5.4.3c.cpp#L1928) | Prioritize firsthand sight, retreat, a Cutter assignment, a learned return, a local sweep, heading interception or ordinary patrol as appropriate. | Each sensory state and pack job needs an appropriate destination. |
| [`Maze::is_reserved`](src/mazerunnerV5.4.3c.cpp#L2011) | Membership test for the short reservation list. | Two monsters cannot claim the same place in a simultaneous movement batch. |
| [`Maze::choose_monster_step`](src/mazerunnerV5.4.3c.cpp#L2017) | Choose one step toward a waypoint from a reverse BFS field while respecting a short list of reserved cells. | Following the graph enforces physical travel around walls and other actors. |
| [`Maze::forecast_monsters`](src/mazerunnerV5.4.3c.cpp#L2044) | Roll copies forward at their state-specific deadlines and cache corridor fields and the profile’s fastest legal gait. | Expected positions and physical reach answer different questions for the solver. |
| [`Maze::earliest_arrival`](src/mazerunnerV5.4.3c.cpp#L2094) | Combine the next-move delay with remaining edges at the fastest possible gait for the difficulty. | A player planning a retreat needs a bound even if a hunter accelerates next beat. |
| [`Maze::evaluate_action`](src/mazerunnerV5.4.3c.cpp#L2103) | Evaluate one time-indexed player action under forecast occupancy and conservative/soft uncertainty rules. | A route into a branch needs enough time to leave before the mouth is sealed. |

## Automatic navigation and recovery

Exit guidance, a committed recovery destination and the route drawn on screen serve different lifetimes.

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::route_guidance`](src/mazerunnerV5.4.3c.cpp#L2137) | Weighted exit guidance: near or parked monsters are walls, distant moving ones add a soft detour cost (Dial’s algorithm). | A clear detour can beat the terrain-shortest path through a distant occupied cell. |
| [`Maze::terrain_exit_step`](src/mazerunnerV5.4.3c.cpp#L2187) | Select a shortest-path successor using the cached terrain field. | An empty maze only needs the shortest-path gradient. |
| [`Maze::recovery_distance_field`](src/mazerunnerV5.4.3c.cpp#L2195) | Compute an independent occupancy-aware BFS toward a recovery waypoint. This avoids redirecting the visual exit overlay toward a temporary detour. | A committed side route needs guidance toward its own temporary goal. |
| [`Maze::finish_recovery`](src/mazerunnerV5.4.3c.cpp#L2210) | Retire a committed detour and remember its neighborhood so the next attempt avoids oscillating between the same choices. | Completion or cancellation must release the old commitment and its cached route. |
| [`Maze::begin_recovery`](src/mazerunnerV5.4.3c.cpp#L2224) | Choose a detour waypoint within 4,096 expansions and depth 32. Recent detours raise boldness, which weights exit progress more. | A blocked player needs a physical alternative after bounded waiting. |
| [`Maze::update_recovery`](src/mazerunnerV5.4.3c.cpp#L2281) | Maintain an existing recovery commitment, reacting to progress, changed enemy occupancy, arrival, cutoff, or timeout. | Repeated failure and renewed progress both need to alter a detour’s lifetime. |
| [`Maze::recovery_step`](src/mazerunnerV5.4.3c.cpp#L2308) | Select a safe adjacent step that makes progress toward the committed recovery goal, even when that step moves away from the maze exit. | A chosen detour still has to respect the current threats on the next step. |
| [`Maze::beam_plan`](src/mazerunnerV5.4.3c.cpp#L2324) | Time-expanded beam search to the forecast’s horizon with a conservative or relaxed first-step policy. | Bounded time-expanded planning keeps interactive replanning affordable. |
| [`Maze::choose_horizon`](src/mazerunnerV5.4.3c.cpp#L2389) | Use 18 actions of lookahead while the nearest threat is 10–36 steps away, otherwise 12. | More lookahead is worth its cost when threats are approaching. |
| [`Maze::refresh_solution_overlay`](src/mazerunnerV5.4.3c.cpp#L2400) | Trace weighted successors to the exit or follow terrain when blocked. Mark occupied routes amber. | The overlay starts at the current player and reaches the exit as plans change. |
| [`Maze::plan_auto_route`](src/mazerunnerV5.4.3c.cpp#L2436) | Choose between terrain guidance, a bounded wait, a committed detour, time-expanded planning and an emergency action. | A single decision point keeps waiting, emergency moves and normal routing compatible. |
| [`Maze::auto_step`](src/mazerunnerV5.4.3c.cpp#L2532) | Execute one tactical action and let the common movement transaction update state, scent, outcomes, and the complete overlay. | The next action must go through the same publication and outcome logic as manual play. |

## Simulation CLI

The simulator shares the production encounter rules and accepts a seed, profile and simulated deadline.

| Function | What it does | Why it exists |
|---|---|---|
| [`outcome_name`](src/mazerunnerV5.4.3c.cpp#L2548) | Map the outcome enum to consistent labels shared by HUD-adjacent diagnostics and simulation output. | Logs and calibration scripts require stable terminal outcome names. |
| [`parse_number`](src/mazerunnerV5.4.3c.cpp#L2557) | Parse a whole nonnegative decimal argument with a caller-selected maximum, reporting overflow as a readable range error. | External numeric arguments need range and syntax checks. |
| [`run_simulation`](src/mazerunnerV5.4.3c.cpp#L2569) | Headless CLI entry point using exactly the same Maze implementation. No OpenGL context or desktop is needed. | Automated tests can run the complete simulation with a specified seed and cutoff. |

## Artwork, wall geometry and projection

These functions turn embedded artwork, terrain and route data into GPU-ready geometry.

| Function | What it does | Why it exists |
|---|---|---|
| [`monster_appearance`](src/mazerunnerV5.4.3c.cpp#L2786) | Resolve an integer spawn ID to an immutable appearance record. | Different silhouettes remain stable across a monster’s sensory state changes. |
| [`valid_sprite`](src/mazerunnerV5.4.3c.cpp#L2791) | Compile-time-capable validation of one sprite's dimensions and palette membership. | Malformed sprite rows or palette symbols would produce missing or misaligned pixels. |
| [`valid_monster_sprites`](src/mazerunnerV5.4.3c.cpp#L2801) | Validate the entire monster registry, including entries added in future revisions. | Every supported spawn index needs a valid drawing. |
| [`shade`](src/mazerunnerV5.4.3c.cpp#L2810) | Scale RGB vertex brightness for a wall face. | Face brightness conveys wall depth under a fixed camera. |
| [`append_quad`](src/mazerunnerV5.4.3c.cpp#L2813) | Accept four quad corners and a constant color, appending to a caller-owned vertex batch. | The shared triangle representation allows a small number of draw calls. |
| [`append_floor_rect`](src/mazerunnerV5.4.3c.cpp#L2818) | Start a horizontal rectangle helper using opposite x/y corners. | Floor-level overlays need the same world coordinates as the maze. |
| [`build_wall_mesh`](src/mazerunnerV5.4.3c.cpp#L2826) | Build the static wall mesh, first counting exposed faces so the vertex reservation matches the mesh exactly. | Immutable wall geometry can be uploaded once for the lifetime of a map. |
| [`sprite_color`](src/mazerunnerV5.4.3c.cpp#L2870) | Map an embedded pixel symbol to a color. Eyes are red while hunting and orange while searching. | Semantic palette entries let eye color communicate sensory state. |
| [`append_sprite`](src/mazerunnerV5.4.3c.cpp#L2895) | Emit an actor or exit as colored pixel quads in one grid cell, with optional searching-eye coloring. | Embedded pixel art can render in the 3D scene with no texture asset dependency. |
| [`append_actor_ring`](src/mazerunnerV5.4.3c.cpp#L2913) | Create a ground marker around the knight so its position remains recognizable when the sprite becomes small. | A ground ring helps locate a small or hovering actor on the grid. |
| [`scene_projection`](src/mazerunnerV5.4.3c.cpp#L2927) | Calculate a column-major 4×4 orthographic projection with fixed tilt, aspect preservation, and optional player-following zoom. | Viewport fit must stay stable across aspect ratios and zoom levels. |
| [`hue_color`](src/mazerunnerV5.4.3c.cpp#L2947) | Convert a hue parameter into full-saturation, full-value RGB for the route highlight. | A hue gradient shows forward progress along a clear route. |
| [`append_route`](src/mazerunnerV5.4.3c.cpp#L2958) | Render the already-computed complete player-to-exit overlay. This function performs no path search. | Distinct clear and occupied-route colors communicate the overlay’s current cost. |

## Setup and input model

The setup form and widgets use one logical coordinate system for validation, painting and hit testing.

| Function | What it does | Why it exists |
|---|---|---|
| [`UiRect::contains`](src/mazerunnerV5.4.3c.cpp#L2985) | Use half-open bounds: left/top edges are included, right/bottom excluded. Adjacent buttons therefore do not both claim a shared boundary. | Buttons and hit testing should agree at rectangle boundaries. |
| [`UiTransform::UiTransform`](src/mazerunnerV5.4.3c.cpp#L2991) | Construct the transform from framebuffer width and height, not GLFW window-coordinate dimensions. | Logical UI positions should survive resizing and letterboxing. |
| [`UiTransform::pointer`](src/mazerunnerV5.4.3c.cpp#L2996) | Convert a GLFW cursor position to UI coordinates, taking both window size and framebuffer size to account for high-DPI displays. | A pointer in window coordinates must hit its visible framebuffer control. |
| [`SetupForm::number`](src/mazerunnerV5.4.3c.cpp#L3006) | Parse a bounded decimal field into an output integer, reporting validity without throwing. | Editable setup values require strict numeric input. |
| [`SetupForm::values`](src/mazerunnerV5.4.3c.cpp#L3014) | Validate the three setup fields and obtain normalized integer game settings. | Starting a game must use the exact normalized values shown by validation. |
| [`SetupForm::error`](src/mazerunnerV5.4.3c.cpp#L3019) | Return the most relevant validation message for the current edit state. | Players need a specific reason when the Start action is unavailable. |
| [`UiState::is_field`](src/mazerunnerV5.4.3c.cpp#L3036) | Identify the three numeric fields, used consistently by typing, selection, and keyboard handling. | Typing and navigation keys should affect only a focused editable control. |
| [`UiState::field`](src/mazerunnerV5.4.3c.cpp#L3038) | Return a mutable reference to the string associated with a numeric control. | Both typing and stepping must modify the selected setup value. |
| [`UiState::show`](src/mazerunnerV5.4.3c.cpp#L3044) | Transition to a new screen while resetting transient pointer/focus state. | Screen changes must release stale focus, presses, and hover state. |
| [`UiState::widgets`](src/mazerunnerV5.4.3c.cpp#L3050) | Construct the current screen's widget list from live UI and optional maze state. The same list governs drawing and interaction. | Keyboard and pointer navigation should operate on the same widget definitions. |
| [`UiState::select`](src/mazerunnerV5.4.3c.cpp#L3090) | Move keyboard focus and arm whole-field replacement for the next digit typed. | A focused control must match the next keyboard action. |
| [`UiState::tab`](src/mazerunnerV5.4.3c.cpp#L3092) | Advance or reverse keyboard focus through enabled widgets only. | Keyboard-only players can traverse the interactive controls. |
| [`UiState::digit`](src/mazerunnerV5.4.3c.cpp#L3101) | Handle one text-input codepoint for numeric editing. | Digit input needs consistent replacement or append behavior in numeric fields. |
| [`UiState::erase`](src/mazerunnerV5.4.3c.cpp#L3109) | Implement Backspace or whole-field deletion without leaving the setup model. | Correction of numeric input should respect field selection. |
| [`UiState::step`](src/mazerunnerV5.4.3c.cpp#L3116) | Adjust a numeric field using its +/- button or Up/Down while editing. | Numeric adjustments must stay within the game’s supported bounds. |
| [`UiState::request_start`](src/mazerunnerV5.4.3c.cpp#L3125) | Validate a request to generate a game before entering Loading. | The UI must validate input before launching asynchronous generation. |
| [`UiState::activate`](src/mazerunnerV5.4.3c.cpp#L3132) | Dispatch one enabled visible control, optionally operating on the current maze. | All input devices should trigger identical underlying actions. |
| [`UiState::hit`](src/mazerunnerV5.4.3c.cpp#L3162) | Find the control under a logical UI point. | Overlapping or disabled widgets require a single hit-resolution rule. |
| [`UiState::pointer_button`](src/mazerunnerV5.4.3c.cpp#L3167) | Track a press/release pair for one logical pointer location. | Dragging off a control should cancel an unintended activation. |

## Font, shaders and rendering

The two renderer classes own separate graphics resources. The embedded shader entry points are listed alongside their C++ creation functions.

| Function | What it does | Why it exists |
|---|---|---|
| [`ui_glyph`](src/mazerunnerV5.4.3c.cpp#L3175) | Resolve a character to seven five-bit font rows. This embedded bitmap font needs no font files or text library. | An embedded font makes labels portable and preserves lowercase release suffixes. |
| [`compile_shader`](src/mazerunnerV5.4.3c.cpp#L3208) | Compile one GLSL shader stage in the current OpenGL context and return its owned object handle. | Compiler diagnostics locate real shader failures on the user’s graphics driver. |
| [`create_shader_program`](src/mazerunnerV5.4.3c.cpp#L3222) | Create the shared position/color shader program used by the world and UI renderers. | One linked shader interface supports world and menu geometry. |
| [`GLSL vertex::main`](src/mazerunnerV5.4.3c.cpp#L3230) | Transform each position to homogeneous clip space and forward its color. There are no normals, UVs, skeletal transforms, or lighting calculations. | The camera transformation must apply equally to walls, sprites and route pixels. |
| [`GLSL fragment::main`](src/mazerunnerV5.4.3c.cpp#L3237) | Write opaque RGB with alpha one. Pixel-art holes are omitted geometry rather than sampled alpha. | Opaque pixels give the palette stable colors across the supported renderers. |
| [`Renderer::release`](src/mazerunnerV5.4.3c.cpp#L3276) | Centralize cleanup for normal destruction and partially completed construction. | Construction failures and normal shutdown both need to release owned GPU objects. |
| [`Renderer::Renderer`](src/mazerunnerV5.4.3c.cpp#L3285) | Construct the world renderer from immutable maze terrain while an OpenGL context is current. | Static terrain and dynamic actors have different upload lifetimes. |
| [`Renderer::Renderer`](src/mazerunnerV5.4.3c.cpp#L3307) | Delete the copy constructor to prevent two Renderers from owning and deleting the same GL handles. | Copying a renderer would duplicate ownership of the same OpenGL handles. |
| [`Renderer::operator=`](src/mazerunnerV5.4.3c.cpp#L3309) | Delete copy assignment for the same exclusive-ownership reason. | Assignment would let two owners delete identical graphics objects. |
| [`Renderer::~Renderer`](src/mazerunnerV5.4.3c.cpp#L3311) | Release GL objects in the destructor. The application must destroy the renderer before destroying the context. | The graphics context must still be current while freeing its resources. |
| [`Renderer::change_zoom`](src/mazerunnerV5.4.3c.cpp#L3314) | Apply a scroll/key zoom request without changing any Maze field or simulation clock. | Zoom is a presentation choice independent of simulated movement and time. |
| [`Renderer::reset_view`](src/mazerunnerV5.4.3c.cpp#L3318) | Reset the visual camera to a full-maze fit. | A player can restore the whole-maze view after zooming in. |
| [`Renderer::zoom_factor`](src/mazerunnerV5.4.3c.cpp#L3320) | Expose the current zoom as a const accessor for verification and diagnostics. | Tests can confirm zoom limits without inspecting private GPU state. |
| [`Renderer::draw`](src/mazerunnerV5.4.3c.cpp#L3324) | Render the world into a caller-selected vertical viewport within the current framebuffer. | World drawing combines cached terrain with movement, clue cues and route changes. |
| [`UiRenderer::rect`](src/mazerunnerV5.4.3c.cpp#L3389) | Convert a logical UI rectangle into a flat colored quad at z = 0, appending to the overlay batch. | A small set of primitive geometry supports every menu and HUD panel. |
| [`UiRenderer::border`](src/mazerunnerV5.4.3c.cpp#L3391) | Draw an outline from four filled rectangles, with configurable thickness defaulting to two logical pixels. | Edges distinguish focused or selected controls against their background. |
| [`UiRenderer::text`](src/mazerunnerV5.4.3c.cpp#L3396) | Render a string as enlarged 5×7 bitmap glyphs at a requested logical pixel size. | Bitmap text shares the UI batch and needs no host font installation. |
| [`UiRenderer::centered`](src/mazerunnerV5.4.3c.cpp#L3405) | Center one line of bitmap text within a rectangle. | Constant glyph metrics keep button and panel labels aligned. |
| [`UiRenderer::icon`](src/mazerunnerV5.4.3c.cpp#L3410) | Draw one embedded sprite in flat UI coordinates. No world tilt or camera transform is used for this illustration. | The menu preview should match the appearance assigned in play. |
| [`UiRenderer::widget`](src/mazerunnerV5.4.3c.cpp#L3415) | Render a widget from the same model used by focus and pointer interaction. | One widget model keeps visual state consistent with interaction state. |
| [`UiRenderer::setup`](src/mazerunnerV5.4.3c.cpp#L3435) | Draw setup labels, form guidance, the live settings summary, and decorative player/monster art. | Players can inspect difficulty, dimensions and count before generating a maze. |
| [`UiRenderer::hud`](src/mazerunnerV5.4.3c.cpp#L3470) | Draw status and help panels: a start prompt before the first input, then time, steps, and hunting/searching counts. | Players can read active pressure and direct the game while watching the maze. |
| [`UiRenderer::release`](src/mazerunnerV5.4.3c.cpp#L3490) | Release UI GPU resources during normal destruction or initialization failure. | The menu batch must be freed on error as well as normal shutdown. |
| [`UiRenderer::UiRenderer`](src/mazerunnerV5.4.3c.cpp#L3498) | Construct the UI renderer with a current GL context, before any game maze is required. | A reusable UI batch avoids rebuilding GPU objects on every screen. |
| [`UiRenderer::UiRenderer`](src/mazerunnerV5.4.3c.cpp#L3509) | Disable copying so UI GPU handles have one owner. | Copy construction would alias the overlay’s program, vertex array and buffer. |
| [`UiRenderer::operator=`](src/mazerunnerV5.4.3c.cpp#L3510) | Disable copy assignment for the same reason. | Copy assignment would violate the overlay’s exclusive ownership. |
| [`UiRenderer::~UiRenderer`](src/mazerunnerV5.4.3c.cpp#L3512) | Delete UI resources while the context is still alive. | Menu graphics resources must be released before destroying the window. |
| [`UiRenderer::draw`](src/mazerunnerV5.4.3c.cpp#L3514) | Draw either a full menu screen or an overlay above a supplied maze frame. | Only the active screen should contribute UI geometry for a frame. |

## Application and native callbacks

The application coordinates input, background generation, window lifetime and the simulation clock.

| Function | What it does | Why it exists |
|---|---|---|
| [`GameApplication::release_hold`](src/mazerunnerV5.4.3c.cpp#L3570) | Forget the physical key and disarm the simulation repeat. | Input from a previous screen must not continue moving the player. |
| [`GameApplication::press_move`](src/mazerunnerV5.4.3c.cpp#L3575) | Submit a first move to the maze and retain the physical key while its timed repeats are armed. | A physical key and its held-repeat direction need a common lifetime. |
| [`GameApplication::GameApplication`](src/mazerunnerV5.4.3c.cpp#L3580) | Construct the application from startup options, before a maze has been generated. | A fixed seed supports repeatable play while the setup remains editable. |
| [`GameApplication::discard_game`](src/mazerunnerV5.4.3c.cpp#L3585) | Destroy world graphics before the Maze, reset reporting, and cancel any held movement key. | Re-entering setup must release the old renderer before its scene. |
| [`GameApplication::begin_generation`](src/mazerunnerV5.4.3c.cpp#L3587) | Start background generation from a validated snapshot of the form. | Large maze generation can run while the UI continues drawing a loading screen. |
| [`GameApplication::finish_generation`](src/mazerunnerV5.4.3c.cpp#L3602) | Poll and install a completed Maze on the thread with the current GL context. | Completed results and worker exceptions must reach the main thread safely. |
| [`GameApplication::tick`](src/mazerunnerV5.4.3c.cpp#L3616) | Advance the full nonnegative elapsed interval while Playing. The maze chronologically dispatches due manual repeats. | Only active play should advance the simulation clock. |
| [`GameApplication::draw`](src/mazerunnerV5.4.3c.cpp#L3626) | Draw the world in the central reserved area and then draw the full-frame UI above it. | The game world and controls need distinct viewport regions. |
| [`ui_cursor`](src/mazerunnerV5.4.3c.cpp#L3638) | Convert the live GLFW cursor into the logical coordinate system used by widget rectangles. | High-DPI displays require conversion between window and framebuffer coordinates. |
| [`key_callback`](src/mazerunnerV5.4.3c.cpp#L3645) | Dispatch key events: Esc pauses, resumes, or leaves a field but never quits. Movement keys start a hold. Releases stop it. | Each key needs one interpretation in setup, active play, pause, or results. |
| [`character_callback`](src/mazerunnerV5.4.3c.cpp#L3690) | Receive GLFW text codepoints separately from physical-key events, enabling consistent numeric typing. | Text entry must use translated digit characters independently of key positions. |
| [`mouse_callback`](src/mazerunnerV5.4.3c.cpp#L3695) | Translate GLFW mouse button actions into UI press/release behavior. | Mouse clicks need the same activation behavior as keyboard commands. |
| [`scroll_callback`](src/mazerunnerV5.4.3c.cpp#L3702) | Handle mouse-wheel input as a visual zoom operation. | Scroll-wheel zoom must apply to the active world view. |
| [`focus_callback`](src/mazerunnerV5.4.3c.cpp#L3708) | Pause on focus loss and cancel any held movement key. | Losing focus must prevent involuntary movement in the background. |
| [`initialize_glew`](src/mazerunnerV5.4.3c.cpp#L3717) | Initialize GLEW after making an OpenGL context current, then verify the capabilities this renderer actually requires. | The renderer needs confirmed driver capabilities before creating resources. |
| [`run_graphical`](src/mazerunnerV5.4.3c.cpp#L3736) | Open the native window with the shared title and run the loop, sleeping to a frame deadline while playing. | Window, context, workers and GPU owners must shut down in a safe order. |
| [`main`](src/mazerunnerV5.4.3c.cpp#L3793) | Program entry point for both graphical and headless executables. | Both desktop users and automation need one versioned executable entry point. |
