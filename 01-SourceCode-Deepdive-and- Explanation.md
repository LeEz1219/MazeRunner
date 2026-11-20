# MazeRunner v5.4.4 source code deep dive

This is a reading guide for C++ developers studying the design of a deterministic game simulation. The production program fits in one translation unit. Its important boundaries are ownership, the source of an observation, the clock on which it becomes valid, and the point at which a prediction can diverge from live state. Each chapter follows one of those boundaries. Its function index links to the exact definition and gives the design reason for keeping it.

For a simulation walk-through, start with **Generation**, follow **Simulation events** and **Evidence**, then read **Automatic navigation**. For the desktop application, start at **Setup and input**, follow **Application and native callbacks**, then trace one frame through **Artwork** and **Font, shaders and rendering**. The CLI, desktop and tests all exercise the same Maze implementation. [Production translation unit](src/mazerunnerV5.4.4.cpp) · [Algorithm proofs and costs](02-AI-Design-Deepdive.md) · [Renderer and design assets](03-Graphics-Design.md)

## Difficulty and map coordinates

A grid cell is an integer key shared by terrain, adjacency, distance fields and rendering. For width $C$, flattening $(r,c)$ to $rC+c$ is a bijection on the checked rectangle. Division and remainder recover the coordinates because $0\le c<C$. It is unsafe to increment an arbitrary ID to step east. On a 7-column board, 13 and 14 differ by one but lie on opposite sides of a row boundary. The checked coordinate API and precomputed adjacency enforce the distinction. With each dimension capped at 501, valid products fit comfortably in a signed 32-bit integer. The negative sentinel must be rejected before converting to an unsigned vector index.

A difficulty profile collects policy in a value. It controls topology requirements and evidence lifetimes in addition to movement rates. Changing a number here can alter the generated graph *before* it changes an encounter. When comparing policies, distinguish a same-seed end-to-end comparison from a fixed-terrain fixture. The latter isolates sensing or movement and is the better place to debug a single decision.

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Point::operator==`](src/mazerunnerV5.4.4.cpp#L101) | Coordinate equality compares both axes. Vector<Point> route comparisons rely on this operation. | Coordinates serve as values in route and replay comparisons. |
| [`Point::operator!=`](src/mazerunnerV5.4.4.cpp#L103) | Inequality delegates to equality so the two operators remain consistent. | Keeping inequality tied to equality prevents inconsistent coordinate tests. |
| [`difficulty_settings`](src/mazerunnerV5.4.4.cpp#L131) | Return a complete immutable balance profile. constexpr permits compile-time evaluation for known difficulty values. | A single profile feeds generation, clocks, clues and the calibration harness. |
| [`parse_difficulty`](src/mazerunnerV5.4.4.cpp#L141) | Normalize a supplied difficulty token and return the corresponding enum. Used by CLI parsing and the retained stream-prompt helper. | The CLI and interactive prompt accept the same names and numbers. |
| [`prompt_difficulty`](src/mazerunnerV5.4.4.cpp#L153) | Retained stream-based prompt, useful to tests/legacy callers. The graphical startup path no longer invokes it. | An invalid answer can be retried without losing the next setup input. |
| [`LocationHint::contains`](src/mazerunnerV5.4.4.cpp#L171) | Membership query for candidate hunting destinations inside a clue rectangle. | Listeners can plan from rectangular evidence without a hidden player coordinate. |
| [`Maze::Maze`](src/mazerunnerV5.4.4.cpp#L262) | Constructor accepts dimensions, an unsigned seed, and an optional logging stream. Its parameter list continues below. | Normalizing the grid at construction establishes valid dimensions for every index. |
| [`Maze::id`](src/mazerunnerV5.4.4.cpp#L278) | Checked coordinate-to-index conversion. Returning -1 lets callers reject out-of-bounds neighbors uniformly. | A compact integer key indexes the terrain and every navigation field. |
| [`Maze::point`](src/mazerunnerV5.4.4.cpp#L282) | Inverse flattening for valid, nonnegative indices: integer division yields row and remainder yields column. | Coordinates are needed for corridor geometry, input and drawing. |
| [`Maze::walkable`](src/mazerunnerV5.4.4.cpp#L284) | Terrain-only traversability query, deliberately independent of monsters and the player. | One terrain predicate keeps movement and search consistent at the border. |
| [`Maze::manhattan`](src/mazerunnerV5.4.4.cpp#L288) | Compute the Manhattan heuristic between two valid cells. It ignores walls and therefore underestimates corridor distance. | The inexpensive lower bound rejects obviously distant acoustic candidates. |
| [`Maze::neighbors`](src/mazerunnerV5.4.4.cpp#L293) | Return the precomputed four-neighbor array by const reference. Consumers must supply a valid cell index. | A common cardinal adjacency order makes traversals reproducible. |
| [`Maze::degree`](src/mazerunnerV5.4.4.cpp#L295) | Expose cached graph degree for junction and dead-end heuristics. | Searchers need to distinguish junctions, corridor interiors and tips. |

</details>

## Generation, graph and patrol coverage

Generation constructs a connected carved maze, proposes loops and certifies route diversity before spawning actors. The test concerns *simple paths between two specified endpoints*. Adding a loop behind a single articulation can raise the cycle rank while leaving exactly one escape route. Read the bridge test and the vertex-biconnected block test together. They use explicit DFS stacks, which move the worst-case traversal depth off the process call stack. Rebuilding topology also invalidates caches, clears cell histories and rebuilds the exit-rooted tree. A terrain edit followed by an old distance lookup is a semantic error even if every index stays in bounds.

Full reverse BFS answers repeated queries to the same destination with one array. For each reachable non-goal cell at distance $k>0$, a shortest path supplies a neighbor at $k-1$. Descending this potential reaches the goal in exactly $k$ steps on static unoccupied terrain. The bounded field serves a different consumer. Its radius-plus-one value lets a caller make a conservative bound outside the explored ball. A stamp and generation counter allow reuse without clearing the entire grid on every search. The returned shared pointer keeps a field alive through cache eviction. The local pool must likewise avoid rewriting arrays still pinned by a forecast. Epoch wrap requires a real clear before a numeric stamp can recur.

The return type of `distance_field` has two independent properties:

```cpp
std::shared_ptr<const std::vector<int>> distance_field(int destination);
```

Here `shared_ptr` expresses lifetime and the inner `const` restricts mutation through that handle. A caller may retain the exit field while 24 other goals displace it from the cache. A reference directly into a recycled buffer would still compile, and might return plausible *wrong distances*. This is precisely where a memory-safety test can pass while a replay diverges. To diagnose it, retain a small field, request enough distinct source/radius pairs to trigger recycling, and re-read the original distances. To diagnose a stale adjacency bug instead, hash or regenerate the terrain and compare fields only after rebuilding the graph. [Full and bounded fields](src/mazerunnerV5.4.4.cpp#L639)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::generate`](src/mazerunnerV5.4.4.cpp#L300) | Generate terrain until the profile topology certificate passes, falling back to adding connections after 64 attempts so generation always terminates. Then spawn monsters. | Seeded carving supplies a new playable maze with reproducible terrain and spawns. |
| [`Maze::route_diversity_met`](src/mazerunnerV5.4.4.cpp#L382) | Apply the selected profile’s route-diversity requirement to the current terrain. | A shared acceptance rule enforces the profile’s required route diversity. |
| [`Maze::add_connections_until_diverse`](src/mazerunnerV5.4.4.cpp#L390) | Generation fallback: open connecting walls in seeded batches until the requirement holds. Always terminates. | Bounded retries keep awkward thin grids from spending unbounded time regenerating. |
| [`Maze::has_at_least_two_solutions`](src/mazerunnerV5.4.4.cpp#L420) | Determine whether at least two distinct simple entrance-to-exit paths exist in O(V+E), using iterative bridge analysis. | An alternate route must help get from the entrance to the exit. |
| [`Maze::solution_diversity`](src/mazerunnerV5.4.4.cpp#L462) | Compute a capped lower-bound certificate for up to four simple solutions, with optional minimum-size filtering of useful cyclic blocks. | A conservative lower bound checks Low's extra choices without enumerating paths. |
| [`Maze::rebuild_graph`](src/mazerunnerV5.4.4.cpp#L514) | Rebuild adjacency, dead-end structure, cached fields, the exit chokepoint tree, patrol regions, and the terrain signature after terrain changes. | Reusing the graph makes frequent sensing and movement queries inexpensive. |
| [`Maze::terrain_signature`](src/mazerunnerV5.4.4.cpp#L569) | FNV-1a hash of the terrain, recorded when caches are rebuilt. | Changing terrain invalidates every derived path field. |
| [`Maze::verify_graph`](src/mazerunnerV5.4.4.cpp#L577) | Debug-only check that terrain was not edited without rebuild_graph(). | Debug builds catch map edits that would otherwise produce stale routes. |
| [`Maze::reset_game`](src/mazerunnerV5.4.4.cpp#L585) | Reset a run’s dynamic state, including clues, detour history, and patrol regions, without carving a map or reseeding the RNG. | An independent replay starts with fresh clocks, clues, lessons and solver memory. |
| [`Maze::spawn_monsters`](src/mazerunnerV5.4.4.cpp#L611) | Populate monsters in unique cells far from the player. Their spawn-relative deadlines stay frozen until the first input. | Distinct initial positions avoid immediate stacking and favor room to start. |
| [`Maze::distance_field`](src/mazerunnerV5.4.4.cpp#L639) | Return a shared reverse BFS field from a least-recently-used cache. A miss traverses the whole terrain graph. | Repeated queries to the same destination can reuse exact corridor distances. |
| [`Maze::LocalField::at`](src/mazerunnerV5.4.4.cpp#L672) | Read the distance under this field’s generation stamp, or return its radius plus one. | A truncated search still needs a safe distance for cells beyond its radius. |
| [`Maze::local_field`](src/mazerunnerV5.4.4.cpp#L677) | Build or reuse a bounded BFS by source and radius. Threat forecasts and step listeners can request different bounds. | Short-range threat and hearing queries avoid exploring a whole large maze. |
| [`Maze::build_exit_tree`](src/mazerunnerV5.4.4.cpp#L763) | Iterative Tarjan traversal rooted at the exit that marks route chokepoints. | Precomputed articulation relationships make frequent cutter assignment practical. |
| [`Maze::exit_chokepoints`](src/mazerunnerV5.4.4.cpp#L790) | List cells every route from a clue cell to the exit must pass, with exact distances. | An assigned intercept should sit on a route the player actually has to traverse. |
| [`Maze::mark_region`](src/mazerunnerV5.4.4.cpp#L804) | Record that a monster covered an 8x8 patrol region. | A visit by one agent can spread later patrols across the pack. |
| [`Maze::region_age`](src/mazerunnerV5.4.4.cpp#L809) | Time since any monster entered the region containing a cell. | Patrol decisions need a comparable age for each candidate region. |

</details>

## Diagnostics and encounter measurements

Telemetry exposes different units. An episode counts entry into Backstage, an order assigns a job and a withdrawal step records an actual change of cell. In a blocked junction the first two can occur without the third. Similarly, the current menace level is a sample, while the radio total is cumulative. Subtract counters across known simulation timestamps and preserve the seed, profile, board dimensions and monster count alongside them. A count by itself cannot establish that the player experienced a breather.

The seed-26 Medium trace provides a concrete audit. Menace crosses the entry threshold at 21.800 s, a retreat is ordered, and Backstage ends at 23.200 s. Nine physical retreat steps are attributed to the run. The outcome at 30.800 s is an escape, but that single observation cannot identify which decision caused it. To evaluate alternate policies, measure corridor clearance under matched seeds and report how many runs actually entered the measurement window. [Encounter trace](artifacts/results/director-relief.log)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::configuration_summary`](src/mazerunnerV5.4.4.cpp#L817) | Compose the startup summary, including hearing radius, interceptors, sight, and search time, without modifying state. | A player can reproduce a run using the settings printed at startup. |
| [`Maze::director_hint_width`](src/mazerunnerV5.4.4.cpp#L842) | Nominal uncertain rectangle width equals base side length plus twice the padding. Actual bounds may be clipped by map size. | Tests can observe the local clue footprint during relief. |
| [`Maze::director_phase`](src/mazerunnerV5.4.4.cpp#L843) | Expose the director's current intensity phase. Positive clue padding corresponds to RELIEF. | The HUD and traces need a consistent name for confinement precision. |
| [`Maze::menace`](src/mazerunnerV5.4.4.cpp#L844) | Read the Director’s current pressure score from 0 to 100. | Pressure can be compared between otherwise identical encounter replays. |
| [`Maze::withdrawing`](src/mazerunnerV5.4.4.cpp#L845) | Read the Backstage pacing flag. | The HUD can show Backstage while confinement precision has a separate state. |
| [`Maze::withdrawal_episodes`](src/mazerunnerV5.4.4.cpp#L846) | Read the count of Backstage episodes begun in this run. | Telemetry distinguishes a new withdrawal episode from another retreat step. |
| [`Maze::withdrawal_steps`](src/mazerunnerV5.4.4.cpp#L847) | Read the number of cardinal steps physically taken under a withdrawal order. | The release fixtures can distinguish a physical retreat from a duty assignment. |
| [`Maze::junction_scans`](src/mazerunnerV5.4.4.cpp#L848) | Read the number of junction scans performed in live play. | A real pause at a junction is observable in headless validation. |
| [`Maze::director_reports`](src/mazerunnerV5.4.4.cpp#L849) | Return the cumulative broadcast count for the HUD, terminal and regressions. | Radio activity can be measured independently of local footsteps. |
| [`Maze::radio_area_label`](src/mazerunnerV5.4.4.cpp#L851) | Format the current strategic footprint for diagnostics without changing simulation state. | Edge clipping changes what the player actually sees reported on a rectangular map. |
| [`Maze::route_progress_percent`](src/mazerunnerV5.4.4.cpp#L857) | Calculate the greatest fraction of the entrance-to-exit corridor distance covered so far. | Backtracking should preserve progress already earned in the radio ramp. |
| [`Maze::radio_interval_ms`](src/mazerunnerV5.4.4.cpp#L867) | Interpolate the normal radio cooldown from the opening interval to its full rate at 50% best progress. | An opening search can expire before the next broadcast while late pursuit tightens. |
| [`Maze::auto_recovery_target`](src/mazerunnerV5.4.4.cpp#L875) | Expose only the active recovery waypoint ID for testing/diagnostics, or -1 if none. | Recovery detours can be inspected independently of the displayed full route. |
| [`Maze::hearing_radius`](src/mazerunnerV5.4.4.cpp#L878) | Compute acoustic reach in walkable steps using the profile and longer map axis. | Corridor sound should scale with map size and remain consistent at walls. |
| [`Maze::auto_boldness`](src/mazerunnerV5.4.4.cpp#L882) | Solver loop-detection level from detours in the last 20 seconds (0–4). | Repeated detours should influence the solver’s appetite for another retreat. |
| [`Maze::cell_label`](src/mazerunnerV5.4.4.cpp#L888) | Format a terrain-valid cell for logs. This utility does not grant monsters player-position access. | Uniform point formatting makes movement traces comparable. |
| [`Maze::log_event`](src/mazerunnerV5.4.4.cpp#L895) | Append one timestamped diagnostic event when a logging destination exists. | Headless and graphical play can expose the same timestamped events. |
| [`Maze::log_status`](src/mazerunnerV5.4.4.cpp#L903) | Emit a snapshot of player state, director status, and hunting/searching counts. | A compact status snapshot supports comparison across replay seeds. |
| [`Maze::monster_name`](src/mazerunnerV5.4.4.cpp#L923) | One-based monster label for logs. | Stable actor names let a reader follow an individual through job changes. |

</details>

## Evidence, pack duties and the Director

Read the private information flow in this order: a successful player step emits a footstep, the bounded hearing query selects listeners, a listener records a sector and revision, and the pack assigns duties from eligible evidence. A sector rectangle can contain walls. Algorithms that need a graph origin must first choose a walkable representative. Firsthand sight can justify an exact cell, but a remote broadcast contains only a coarse region. Sharing a learned tactic across the pack gives no member a new coordinate.

A monster has three independent commitments. Sensory state describes what it knows, duty assigns responsibility for the encounter, and the Cutter role can supply an interception target. Treating them as one enum would require a product of states and obscure priority. For instance an investigator can still be Hunting when ordered to Withdraw. Its route is retained in reverse order so the next outward step is a constant-time pop from the back. For a learned ambush, nomination scans eligible listeners and minimizes a (distance, cell ID) pair. Assignment separately tests which hunter can arrive at that site in time. Conflating these two decisions used to make a later listener’s viable proposal disappear. The retreat planner accepts the actual or hypothetical pack as a parameter. Reading live teammates from inside it would make forecast decisions depend on actors at a different simulated time.

The retained withdrawal path intentionally runs from goal back toward the next move:

```cpp
for (int v = best; v != m.cell; v = retreat_parent[v])
    m.retreat_route.push_back(v);
```

For a planned leg A→B→C→D the vector holds [D,C,B]. Taking B needs `back()` and `pop_back()`, and leaves [D,C] for the next action. A supporter reserving this vector cares about membership, not iteration order. With blocked destinations, ordinary hunting may use a side step to get unstuck. Doing that on an ordered retreat could send the actor straight back toward the clue. That is why the withdrawal branch precedes the generic target-gradient branch and holds when replanning finds no outward edge. [Leg construction](src/mazerunnerV5.4.4.cpp#L1345) · [Consumption and repair](src/mazerunnerV5.4.4.cpp#L2088)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::radio_sector_size`](src/mazerunnerV5.4.4.cpp#L926) | Calculate the normal map-wide radio sector before relief padding or clipping by axis. | The Director needs a broader clue while respecting small maps. |
| [`Maze::widen_hint`](src/mazerunnerV5.4.4.cpp#L931) | Work on a copy of a remembered rectangle so callers choose which stored memory to replace. | Relief can reduce precision without disclosing any new location. |
| [`Maze::make_location_hint`](src/mazerunnerV5.4.4.cpp#L943) | Quantize an actual displacement by local or radio sector size, clip it to the board and apply current relief. | Quantization gives different hidden positions an identical local report. |
| [`Maze::sector_center`](src/mazerunnerV5.4.4.cpp#L952) | Compute the midpoint from the clue boundaries shared with the listener. | Hearing can compare sector transitions without storing a player cell. |
| [`Maze::clue_anchor`](src/mazerunnerV5.4.4.cpp#L957) | Reuse a walkable center when available and return a sentinel for an absent clue. | A walkable representative lets a rectangular clue seed real corridor searches. |
| [`Maze::note_evidence`](src/mazerunnerV5.4.4.cpp#L973) | Insert a source-clue revision into the pack’s evidence set for a pending tactic. | Several listeners and late-finishing searches can share one causal observation. |
| [`Maze::unlock_lessons`](src/mazerunnerV5.4.4.cpp#L978) | At a Director tick, unlock the best supported pending tactic with at least two distinct observations. Space unlocks by 12 seconds. | Spacing makes separate learned behaviors visible over a sufficiently long run. |
| [`Maze::junction_ahead`](src/mazerunnerV5.4.4.cpp#L995) | Search outward from a clue anchor for a usable junction beyond the leading edge of the heard heading. | A lead can intercept using heard direction and known terrain. |
| [`Maze::pack_learned`](src/mazerunnerV5.4.4.cpp#L1011) | Read the current run’s shared unlock flag for one tactic. | Diagnostics can read a shared lesson without changing its state. |
| [`Maze::lesson_evidence`](src/mazerunnerV5.4.4.cpp#L1012) | Count the distinct source-clue IDs credited to a tactic. | Tests distinguish observing evidence from reaching the scheduled unlock time. |
| [`Maze::clear_heard_route`](src/mazerunnerV5.4.4.cpp#L1016) | Clear the heard sector, recent departures, directional streak and continuous-contact flag. | A disconnected chase or widened clue cannot complete an earlier precise route. |
| [`Maze::sector_circuit`](src/mazerunnerV5.4.4.cpp#L1025) | Follow the observed sectors in order through cardinal passages and require a real closing edge back to the starting cell. | Returning after a delay also happens on straight paths and in cut-open rings. |
| [`Maze::observe_footstep`](src/mazerunnerV5.4.4.cpp#L1073) | Track up to six audible departures. Credit a sustained U-turn or a continuous return compatible with a terrain cycle. | Only a continuous sequence of audible transitions supports a directional habit. |
| [`Maze::set_duty`](src/mazerunnerV5.4.4.cpp#L1096) | Transfer pack duty, release an incompatible waypoint and cancel unfinished local-search evidence on withdrawal. | A job change must release incompatible plans and reservations. |
| [`Maze::coordinate_pack`](src/mazerunnerV5.4.4.cpp#L1111) | Find a single eligible investigator in stable monster order. | One investigator and several perimeter agents reduce crowding on a single clue. |
| [`Maze::deliver_hint`](src/mazerunnerV5.4.4.cpp#L1130) | Give a hearing monster the latest clue, switching it to hunting and keeping a valid target. | Physical hearing can start a hunt without teaching other agents an exact cell. |
| [`Maze::investigate_report`](src/mazerunnerV5.4.4.cpp#L1154) | Deliver a coarse report to a monster eligible for remote investigation. | A remote report motivates a search without renewing a listener’s scent. |
| [`Maze::publish_movement_hint`](src/mazerunnerV5.4.4.cpp#L1171) | Determine acoustic listeners, renew physical contact, publish eligible local clues and independently broadcast a strategic area. | The Director and listeners share a causal step while retaining different precision. |
| [`Maze::assign_interceptors`](src/mazerunnerV5.4.4.cpp#L1235) | Reserve existing guards and assign eligible listeners to distinct chokepoints within the global profile cap. | Reservations spread eligible guards over reachable chokepoints or learned ambushes. |
| [`Maze::set_director_precision`](src/mazerunnerV5.4.4.cpp#L1311) | Widen stored clues, clear precise hearing history and release stale guard targets when relief increases. | Widened clues must invalidate stale headings and targets before the next move. |
| [`Maze::retreat_anchor`](src/mazerunnerV5.4.4.cpp#L1338) | Choose a fresh firsthand sighting or a walkable representative of this monster’s clue. | Monotone withdrawal requires an origin the actor could legitimately know. |
| [`Maze::plan_retreat`](src/mazerunnerV5.4.4.cpp#L1345) | Explore outward edges, reject occupied or claimed cells, rank endpoints, and retain the path in reverse order. | The first step and every later step must separate from the clue. A destination alone cannot enforce this. |
| [`Maze::pace_encounter`](src/mazerunnerV5.4.4.cpp#L1388) | Advance menace, order a primary retreat, optionally select one sustained-pressure supporter and end the quiet interval. | Hysteresis separates entry from exit. Per-monster evidence and a two-order cap constrain how pacing can intervene. |
| [`Maze::update_director`](src/mazerunnerV5.4.4.cpp#L1455) | Run one fixed-time director sample: confinement, relief escalation, and recovery hysteresis. A manual player standing still is not counted as confined. | Confinement relief depends on sustained movement history and recovery. |

</details>

## Simulation events, input and sensing

The scheduler advances an integer clock to the next due event and drains same-time work in source order. This makes input ordering observable: a monster can move or complete a sensory transition at the same instant as an automatic action. Deadline equality must count as due, and each handler must move its own next deadline forward or disable it. Otherwise an event can be lost at the boundary, or a loop can process the same timestamp forever. Frame time determines how much simulated time to request. The state transition itself belongs to the simulation.

Consider the first successful manual move. It releases the staggered monster clocks, updates movement history, and can trigger immediate local and radio evidence. Walking into a wall does not stand in for a footstep. High visual acquisition is also a scheduled action: first notice spends a beat, confirmation can start a charge, and a confirmed hunter can track at closer range. Altering the relative order of sensory checks and movement can change who reaches a shared square. Compare timestamped transitions, not just the final board.

The decisive sequence inside `advance_to` is short. Read these four calls together:

```cpp
now = event;
expire_monster_states();
update_director();
update_monsters();
```

Only after these calls, provided the player is still alive, can the due automatic step or held manual repeat run. For example, if scent expiry, a 200 ms Director tick and a monster move share a deadline, the Director observes the *expired* sensory state. The monster then acts under the duty selected for that state. A capture here wins the race against a pending player move. Advancing to the same destination in two calls processes the same due timestamps in the same order under the same inputs; only the intermediate overlay refresh differs. Moving a gait update into the draw loop would invalidate that argument. [Scheduler](src/mazerunnerV5.4.4.cpp#L1609) · [Partitioned replay tests](tests/stalker_tests.cpp)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::next_monster_state_change`](src/mazerunnerV5.4.4.cpp#L1516) | Find the next scent, search or interceptor deadline. | State expiry must take effect even between two movement deadlines. |
| [`Maze::expire_monster_states`](src/mazerunnerV5.4.4.cpp#L1528) | Expire roles, change an abandoned hunt into a search, and credit an in-sector tip inspection only when the local search expires. | An unsuccessful local sweep can teach the pack only what the searcher checked. |
| [`Maze::sees_player`](src/mazerunnerV5.4.4.cpp#L1589) | Select the profile range for the monster’s current visual state and test a clear corridor. | Initial notice and an established charge have distinct legal distance windows. |
| [`Maze::line_of_sight`](src/mazerunnerV5.4.4.cpp#L1592) | Check axial alignment, the caller’s minimum and profile maximum distance, and intervening walls. | An intervening wall must break even perfect axial alignment. |
| [`Maze::advance_to`](src/mazerunnerV5.4.4.cpp#L1609) | Advance the discrete-event simulation. Monster moves and state expiries are scheduled only after the player’s first input. | Frame size should never reorder simulation events or collision outcomes. |
| [`Maze::update_monsters`](src/mazerunnerV5.4.4.cpp#L1641) | Move due monsters with a short reservation list, apply High corridor sightings and interceptor hold expiry, and track patrol coverage. | Monsters sharing a deadline need fair ordering and consistent occupancy. |
| [`Maze::set_auto_solve`](src/mazerunnerV5.4.4.cpp#L1731) | Toggle automatic assistance, reset its tactical memory, and update the overlay while preserving the existing map and actor positions. | Changing control mode releases conflicting movement timers. |
| [`Maze::move_player`](src/mazerunnerV5.4.4.cpp#L1748) | Manual movement wrapper. Both manual and automatic displacements eventually use commit_player_move. | Manual and automatic steps must use the same movement and collision rules. |
| [`Maze::begin_held_move`](src/mazerunnerV5.4.4.cpp#L1757) | Start held movement with a direction supplied by the graphical input layer. | A held key should give an immediate first step and deterministic repeats. |
| [`Maze::cancel_held_move`](src/mazerunnerV5.4.4.cpp#L1766) | Set the next repeat to the inactive sentinel without advancing time. | Focus loss, pause and release must stop future synthetic steps. |
| [`Maze::check_outcome`](src/mazerunnerV5.4.4.cpp#L1769) | Resolve outcomes with capture precedence, then cancel all pending scent and search deadlines. | Capture and escape need one precedence rule in every input mode. |
| [`Maze::remember`](src/mazerunnerV5.4.4.cpp#L1791) | Append an action cell to a bounded history deque. Shared by player history and monster recent-cell memory. | Behavioral memory stays bounded during long runs. |
| [`Maze::start_timer`](src/mazerunnerV5.4.4.cpp#L1796) | Start the clock once and release frozen monsters, preserving their spawn staggering. | The first action releases actors together while preserving their spawn staggering. |
| [`Maze::update_elapsed`](src/mazerunnerV5.4.4.cpp#L1814) | Elapsed gameplay time is now minus timer_start when active. Pre-movement patrol time is therefore excluded. | The displayed result needs the last elapsed game time even after play freezes. |
| [`Maze::visit_heat`](src/mazerunnerV5.4.4.cpp#L1818) | Return a linearly time-decayed revisit penalty without mutating stored memory. | Lingering briefly should count differently from repeatedly circling a route. |
| [`Maze::commit_player_move`](src/mazerunnerV5.4.4.cpp#L1825) | Common transaction for one cardinal move or wait: validate, update clocks/memory, publish movement information, and resolve capture/escape. | Only a successful displacement can publish movement evidence. |

</details>

## Target selection, searching and threat prediction

Suspicion is a bounded reachable set with discrete scores, not a normalized probability distribution. A clue seeds multiple walkable cells, time since the clue expands the set, and recent inspections suppress already checked candidates. At a junction a look has its own (junction, arm, time) memory. An orange marker can consequently identify a useful arm even on profiles where vision cannot certify the distant cells as inspected. The visual and acoustic knowledge rules survive the display effect.

A forecast starts with copies of all monsters and advances their due clocks and reservations. Holding copies of the route vectors matters: simulating one reverse-pop on a live retreat route would consume a future game action. Geometry caches may be reused as scratch because they do not encode learned behavior. A projected action can still diverge after actual player movement emits a clue. The solver evaluates both projected occupancy and an earliest physical arrival bound. Four edges with 126 ms left to the first move and 107 ms minimum subsequent steps yield 447 ms. Multiplying four by the roaming gait would claim safety for too long.

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::target_claimed`](src/mazerunnerV5.4.4.cpp#L1858) | Determine whether another monster of the same behavior state has already selected a candidate waypoint. | Nearby hunters should spread their plans over different plausible cells. |
| [`Maze::move_period`](src/mazerunnerV5.4.4.cpp#L1865) | Scale the base hunt interval to 130% for patrol, 115% for search or 85% during a visual charge. | Different gaits make patrol, sweep and pursuit distinguishable in motion. |
| [`Maze::sight_check`](src/mazerunnerV5.4.4.cpp#L1877) | Sample corridor sight at a due monster action. Return no contact, a notice pause or a charge. | A visual chase requires an observable beat before it can accelerate. |
| [`Maze::inspect_cell`](src/mazerunnerV5.4.4.cpp#L1889) | Refresh the bounded inspection history and remember a lead’s dead-end tip check in its unresolved local clue. | The lead can credit a local trail only after visiting or seeing an in-sector tip. |
| [`Maze::recently_inspected`](src/mazerunnerV5.4.4.cpp#L1900) | A cell is discounted until its observation ages past the inspection-memory window. | Recent inspections prevent the searcher repeatedly checking the same alcove. |
| [`Maze::scan_junction`](src/mazerunnerV5.4.4.cpp#L1908) | Choose an eligible arm by clue gradient, straight reach and the learned alcove bonus. Record the glance and pause for one gait period. | Remembering looks separately from confirmed inspections gives the marker direction without granting extra vision. |
| [`Maze::sweep_target`](src/mazerunnerV5.4.4.cpp#L1954) | Start a sweep from the stored sector and its genuine observation time. | Directing the lead through unexplored branches creates a methodical local search. |
| [`Maze::choose_target`](src/mazerunnerV5.4.4.cpp#L2000) | Prioritize firsthand sight, retreat, a Cutter assignment, a learned return, a local sweep, heading interception or ordinary patrol as appropriate. | Each sensory state and pack job needs an appropriate destination. |
| [`Maze::is_reserved`](src/mazerunnerV5.4.4.cpp#L2082) | Membership test for the short reservation list. | Two monsters cannot claim the same place in a simultaneous movement batch. |
| [`Maze::choose_monster_step`](src/mazerunnerV5.4.4.cpp#L2088) | Consume or repair a withdrawal path. Otherwise descend the target field with batch reservations and bounded backtracking. | Ordered retreat enforces outward progress even when an ordinary shortest path would turn back toward the clue. |
| [`Maze::forecast_monsters`](src/mazerunnerV5.4.4.cpp#L2135) | Advance copied agents, including their individual withdrawal paths, using hypothetical occupancy and batch reservations. | A solver query must preserve live route vectors, learned evidence, scan counts and movement decisions. |
| [`Maze::earliest_arrival`](src/mazerunnerV5.4.4.cpp#L2189) | Combine the next-move delay with remaining edges at the fastest possible gait for the difficulty. | A player planning a retreat needs a bound even if a hunter accelerates next beat. |
| [`Maze::evaluate_action`](src/mazerunnerV5.4.4.cpp#L2198) | Evaluate one time-indexed player action under forecast occupancy and conservative/soft uncertainty rules. | A route into a branch needs enough time to leave before the mouth is sealed. |

</details>

## Automatic navigation and recovery

Global guidance, local action selection and recovery solve different optimization problems. The global field prices occupied cells and gives an exit gradient. Beam search then admits five actions, including waiting, and enforces near-term threat checks. Two arrivals at the same cell and depth can have different parent cells and different turn-back costs. Keeping the lower-ranked candidate saves work but loses part of the information necessary for a globally optimal solution. A width increase can improve some runs while also changing which later states appear.

Repeated stalls trigger bounded detour search. The score combines clearance, visitation heat, branch depth and progress. Its temporary target may increase terrain distance to the exit. That is consistent with the tactical objective of surviving the next encounter. The displayed route can extend beyond the beam horizon using guidance. Distinguish the tested first action, the hypothetical tactical prefix and the illustrative tail when diagnosing a seemingly hazardous overlay. With zero monsters the field can instead give an exact shortest path.

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`Maze::route_guidance`](src/mazerunnerV5.4.4.cpp#L2232) | Weighted exit guidance: near or parked monsters are walls, distant moving ones add a soft detour cost (Dial’s algorithm). | A clear detour can beat the terrain-shortest path through a distant occupied cell. |
| [`Maze::terrain_exit_step`](src/mazerunnerV5.4.4.cpp#L2282) | Select a shortest-path successor using the cached terrain field. | An empty maze only needs the shortest-path gradient. |
| [`Maze::recovery_distance_field`](src/mazerunnerV5.4.4.cpp#L2290) | Compute an independent occupancy-aware BFS toward a recovery waypoint. This avoids redirecting the visual exit overlay toward a temporary detour. | A committed side route needs guidance toward its own temporary goal. |
| [`Maze::finish_recovery`](src/mazerunnerV5.4.4.cpp#L2305) | Retire a committed detour and remember its neighborhood so the next attempt avoids oscillating between the same choices. | Completion or cancellation must release the old commitment and its cached route. |
| [`Maze::begin_recovery`](src/mazerunnerV5.4.4.cpp#L2319) | Choose a detour waypoint within 4,096 expansions and depth 32. Recent detours raise boldness, which weights exit progress more. | A blocked player needs a physical alternative after bounded waiting. |
| [`Maze::update_recovery`](src/mazerunnerV5.4.4.cpp#L2376) | Maintain an existing recovery commitment, reacting to progress, changed enemy occupancy, arrival, cutoff, or timeout. | Repeated failure and renewed progress both need to alter a detour’s lifetime. |
| [`Maze::recovery_step`](src/mazerunnerV5.4.4.cpp#L2403) | Select a safe adjacent step that makes progress toward the committed recovery goal, even when that step moves away from the maze exit. | A chosen detour still has to respect the current threats on the next step. |
| [`Maze::beam_plan`](src/mazerunnerV5.4.4.cpp#L2419) | Time-expanded beam search to the forecast’s horizon with a conservative or relaxed first-step policy. | Bounded time-expanded planning keeps interactive replanning affordable. |
| [`Maze::choose_horizon`](src/mazerunnerV5.4.4.cpp#L2484) | Use 18 actions of lookahead while the nearest threat is 10–36 steps away, otherwise 12. | More lookahead is worth its cost when threats are approaching. |
| [`Maze::refresh_solution_overlay`](src/mazerunnerV5.4.4.cpp#L2495) | Trace weighted successors to the exit or follow terrain when blocked. Mark occupied routes amber. | The overlay starts at the current player and reaches the exit as plans change. |
| [`Maze::plan_auto_route`](src/mazerunnerV5.4.4.cpp#L2531) | Choose between terrain guidance, a bounded wait, a committed detour, time-expanded planning and an emergency action. | A single decision point keeps waiting, emergency moves and normal routing compatible. |
| [`Maze::auto_step`](src/mazerunnerV5.4.4.cpp#L2627) | Execute one tactical action and let the common movement transaction update state, scent, outcomes, and the complete overlay. | The next action must go through the same publication and outcome logic as manual play. |

</details>

## Simulation CLI

The CLI is a second front end to the same simulation. Parsing bounds counts and timestamps before narrowing integer values; the common simulation functions then enforce the same generation and scheduling contracts as desktop play. A result of RUNNING records censoring at the selected time limit. Counting it as a capture would change the meaning of a balance measurement. Record both counts and the deadline.

For a reproducibility check, hold the build, standard-library implementation, seed, dimensions, profile, action schedule and limit fixed. Compare simulated milliseconds, moves, outcome and encounter counters. Host compute time is a benchmark for that execution environment. The standard library may implement a distribution or shuffle differently across toolchains even with the same generator seed. [Build commands and seed protocol](04-Build-Test-and-Reproduce.md)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`outcome_name`](src/mazerunnerV5.4.4.cpp#L2643) | Map the outcome enum to consistent labels shared by HUD-adjacent diagnostics and simulation output. | Logs and calibration scripts require stable terminal outcome names. |
| [`parse_number`](src/mazerunnerV5.4.4.cpp#L2652) | Parse a whole nonnegative decimal argument with a caller-selected maximum, reporting overflow as a readable range error. | External numeric arguments need range and syntax checks. |
| [`run_simulation`](src/mazerunnerV5.4.4.cpp#L2664) | Headless CLI entry point using exactly the same Maze implementation. No OpenGL context or desktop is needed. | Automated tests can run the complete simulation with a specified seed and cutoff. |

</details>

## Artwork, wall geometry and projection

The simulation stores occupancy in cells. The renderer converts it into static wall vertices and camera-facing sprite quads. Hidden wall faces never enter the vertex buffer; transparent sprite texels emit no geometry. A twenty-by-twenty fully opaque character needs at most 400 quads or 2,400 triangle vertices. World depth testing can therefore occlude just the lower portion behind a wall. Wall variation comes from a hash of the cell index, keeping rendering independent of the simulation random stream.

The sprite array stores string views of static literals. Their storage lasts for the program lifetime, and compile-time validation catches bad dimensions and symbols. Replacing them with views of temporary importer strings would introduce dangling references. A Figma editor can hand over twenty indexed rows using the graphics guide. State-sensitive eyes remain a material symbol recolored at draw time, so a red pixel baked into every export would destroy a gameplay cue. [Sprite, font and wall handoff](03-Graphics-Design.md)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`monster_appearance`](src/mazerunnerV5.4.4.cpp#L2881) | Resolve an integer spawn ID to an immutable appearance record. | Different silhouettes remain stable across a monster’s sensory state changes. |
| [`valid_sprite`](src/mazerunnerV5.4.4.cpp#L2886) | Compile-time-capable validation of one sprite's dimensions and palette membership. | Malformed sprite rows or palette symbols would produce missing or misaligned pixels. |
| [`valid_monster_sprites`](src/mazerunnerV5.4.4.cpp#L2896) | Validate the entire monster registry, including entries added in future revisions. | Every supported spawn index needs a valid drawing. |
| [`shade`](src/mazerunnerV5.4.4.cpp#L2905) | Scale RGB vertex brightness for a wall face. | Face brightness conveys wall depth under a fixed camera. |
| [`append_quad`](src/mazerunnerV5.4.4.cpp#L2908) | Accept four quad corners and a constant color, appending to a caller-owned vertex batch. | The shared triangle representation allows a small number of draw calls. |
| [`append_floor_rect`](src/mazerunnerV5.4.4.cpp#L2913) | Start a horizontal rectangle helper using opposite x/y corners. | Floor-level overlays need the same world coordinates as the maze. |
| [`build_wall_mesh`](src/mazerunnerV5.4.4.cpp#L2921) | Build the static wall mesh, first counting exposed faces so the vertex reservation matches the mesh exactly. | Immutable wall geometry can be uploaded once for the lifetime of a map. |
| [`sprite_color`](src/mazerunnerV5.4.4.cpp#L2965) | Map an embedded pixel symbol to a color. Eyes are red while hunting and orange while searching. | Semantic palette entries let eye color communicate sensory state. |
| [`append_sprite`](src/mazerunnerV5.4.4.cpp#L2990) | Emit an actor or exit as colored pixel quads in one grid cell, with optional searching-eye coloring. | Embedded pixel art can render in the 3D scene with no texture asset dependency. |
| [`append_actor_ring`](src/mazerunnerV5.4.4.cpp#L3008) | Create a ground marker around the knight so its position remains recognizable when the sprite becomes small. | A ground ring helps locate a small or hovering actor on the grid. |
| [`scene_projection`](src/mazerunnerV5.4.4.cpp#L3022) | Calculate a column-major 4×4 orthographic projection with fixed tilt, aspect preservation, and optional player-following zoom. | Viewport fit must stay stable across aspect ratios and zoom levels. |
| [`hue_color`](src/mazerunnerV5.4.4.cpp#L3042) | Convert a hue parameter into full-saturation, full-value RGB for the route highlight. | A hue gradient shows forward progress along a clear route. |
| [`append_route`](src/mazerunnerV5.4.4.cpp#L3053) | Render the already-computed complete player-to-exit overlay. This function performs no path search. | Distinct clear and occupied-route colors communicate the overlay’s current cost. |

</details>

## Setup and input model

The interface uses a fixed logical canvas while GLFW reports cursor positions in window coordinates and OpenGL draws in framebuffer pixels. The conversion must apply the window-to-framebuffer ratio, then undo the centered viewport offset and scale. For a 2000×1520 framebuffer displayed in a 1000×760 window, a cursor at the window center lands at logical (500,380). On an ultrawide display the horizontal bars must also be subtracted. Testing only a 1:1 aspect ratio hides either mistake.

The edit fields keep strings while a user types. An empty string can be a valid *editing state* even though it cannot start a game. Widget validity, keyboard focus, mouse capture and activation therefore belong to separate decisions. A press on one control and release over another must not transfer activation. Reuse the same logical rectangles for display and hit testing when altering the layout.

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`UiRect::contains`](src/mazerunnerV5.4.4.cpp#L3080) | Use half-open bounds: left/top edges are included, right/bottom excluded. Adjacent buttons therefore do not both claim a shared boundary. | Buttons and hit testing should agree at rectangle boundaries. |
| [`UiTransform::UiTransform`](src/mazerunnerV5.4.4.cpp#L3086) | Construct the transform from framebuffer width and height, not GLFW window-coordinate dimensions. | Logical UI positions should survive resizing and letterboxing. |
| [`UiTransform::pointer`](src/mazerunnerV5.4.4.cpp#L3091) | Convert a GLFW cursor position to UI coordinates, taking both window size and framebuffer size to account for high-DPI displays. | A pointer in window coordinates must hit its visible framebuffer control. |
| [`SetupForm::number`](src/mazerunnerV5.4.4.cpp#L3101) | Parse a bounded decimal field into an output integer, reporting validity without throwing. | Editable setup values require strict numeric input. |
| [`SetupForm::values`](src/mazerunnerV5.4.4.cpp#L3109) | Validate the three setup fields and obtain normalized integer game settings. | Starting a game must use the exact normalized values shown by validation. |
| [`SetupForm::error`](src/mazerunnerV5.4.4.cpp#L3114) | Return the most relevant validation message for the current edit state. | Players need a specific reason when the Start action is unavailable. |
| [`UiState::is_field`](src/mazerunnerV5.4.4.cpp#L3131) | Identify the three numeric fields, used consistently by typing, selection, and keyboard handling. | Typing and navigation keys should affect only a focused editable control. |
| [`UiState::field`](src/mazerunnerV5.4.4.cpp#L3133) | Return a mutable reference to the string associated with a numeric control. | Both typing and stepping must modify the selected setup value. |
| [`UiState::show`](src/mazerunnerV5.4.4.cpp#L3139) | Transition to a new screen while resetting transient pointer/focus state. | Screen changes must release stale focus, presses, and hover state. |
| [`UiState::widgets`](src/mazerunnerV5.4.4.cpp#L3145) | Construct the current screen's widget list from live UI and optional maze state. The same list governs drawing and interaction. | Keyboard and pointer navigation should operate on the same widget definitions. |
| [`UiState::select`](src/mazerunnerV5.4.4.cpp#L3185) | Move keyboard focus and arm whole-field replacement for the next digit typed. | A focused control must match the next keyboard action. |
| [`UiState::tab`](src/mazerunnerV5.4.4.cpp#L3187) | Advance or reverse keyboard focus through enabled widgets only. | Keyboard-only players can traverse the interactive controls. |
| [`UiState::digit`](src/mazerunnerV5.4.4.cpp#L3196) | Handle one text-input codepoint for numeric editing. | Digit input needs consistent replacement or append behavior in numeric fields. |
| [`UiState::erase`](src/mazerunnerV5.4.4.cpp#L3204) | Implement Backspace or whole-field deletion without leaving the setup model. | Correction of numeric input should respect field selection. |
| [`UiState::step`](src/mazerunnerV5.4.4.cpp#L3211) | Adjust a numeric field using its +/- button or Up/Down while editing. | Numeric adjustments must stay within the game’s supported bounds. |
| [`UiState::request_start`](src/mazerunnerV5.4.4.cpp#L3220) | Validate a request to generate a game before entering Loading. | The UI must validate input before launching asynchronous generation. |
| [`UiState::activate`](src/mazerunnerV5.4.4.cpp#L3227) | Dispatch one enabled visible control, optionally operating on the current maze. | All input devices should trigger identical underlying actions. |
| [`UiState::hit`](src/mazerunnerV5.4.4.cpp#L3257) | Find the control under a logical UI point. | Overlapping or disabled widgets require a single hit-resolution rule. |
| [`UiState::pointer_button`](src/mazerunnerV5.4.4.cpp#L3262) | Track a press/release pair for one logical pointer location. | Dragging off a control should cancel an unintended activation. |

</details>

## Font, shaders and rendering

Wall geometry is built once for a terrain layout. Dynamic geometry includes actors and overlays whose position, tint or direction can change each frame. The shader accepts vertex position and color, and the projection controls how the raised geometry appears. Adding an image export alone cannot make the shader sample it. That requires UVs, a sampler, a texture upload path and a draw policy for transparency.

The UI renders filled bitmap glyph cells as geometry. For an A, the first mask is decimal 14, binary 01110. Read its five bits left to right to illuminate the middle three columns. The six-unit advance leaves one column between nominal five-column glyphs. Changing just the masks preserves text metrics; changing the advance requires rechecking centered labels, button widths and HUD fit. This is a layout contract shared by measuring and drawing.

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`ui_glyph`](src/mazerunnerV5.4.4.cpp#L3270) | Resolve a character to seven five-bit font rows. This embedded bitmap font needs no font files or text library. | An embedded font makes labels portable and preserves lowercase release suffixes. |
| [`compile_shader`](src/mazerunnerV5.4.4.cpp#L3303) | Compile one GLSL shader stage in the current OpenGL context and return its owned object handle. | Compiler diagnostics locate real shader failures on the user’s graphics driver. |
| [`create_shader_program`](src/mazerunnerV5.4.4.cpp#L3317) | Create the shared position/color shader program used by the world and UI renderers. | One linked shader interface supports world and menu geometry. |
| [`GLSL vertex::main`](src/mazerunnerV5.4.4.cpp#L3325) | Transform each position to homogeneous clip space and forward its color. There are no normals, UVs, skeletal transforms, or lighting calculations. | The camera transformation must apply equally to walls, sprites and route pixels. |
| [`GLSL fragment::main`](src/mazerunnerV5.4.4.cpp#L3332) | Write opaque RGB with alpha one. Pixel-art holes are omitted geometry rather than sampled alpha. | Opaque pixels give the palette stable colors across the supported renderers. |
| [`Renderer::release`](src/mazerunnerV5.4.4.cpp#L3371) | Centralize cleanup for normal destruction and partially completed construction. | Construction failures and normal shutdown both need to release owned GPU objects. |
| [`Renderer::Renderer`](src/mazerunnerV5.4.4.cpp#L3380) | Construct the world renderer from immutable maze terrain while an OpenGL context is current. | Static terrain and dynamic actors have different upload lifetimes. |
| [`Renderer::Renderer`](src/mazerunnerV5.4.4.cpp#L3402) | Delete the copy constructor to prevent two Renderers from owning and deleting the same GL handles. | Copying a renderer would duplicate ownership of the same OpenGL handles. |
| [`Renderer::operator=`](src/mazerunnerV5.4.4.cpp#L3404) | Delete copy assignment for the same exclusive-ownership reason. | Assignment would let two owners delete identical graphics objects. |
| [`Renderer::~Renderer`](src/mazerunnerV5.4.4.cpp#L3406) | Release GL objects in the destructor. The application must destroy the renderer before destroying the context. | The graphics context must still be current while freeing its resources. |
| [`Renderer::change_zoom`](src/mazerunnerV5.4.4.cpp#L3409) | Apply a scroll/key zoom request without changing any Maze field or simulation clock. | Zoom is a presentation choice independent of simulated movement and time. |
| [`Renderer::reset_view`](src/mazerunnerV5.4.4.cpp#L3413) | Reset the visual camera to a full-maze fit. | A player can restore the whole-maze view after zooming in. |
| [`Renderer::zoom_factor`](src/mazerunnerV5.4.4.cpp#L3415) | Expose the current zoom as a const accessor for verification and diagnostics. | Tests can confirm zoom limits without inspecting private GPU state. |
| [`Renderer::draw`](src/mazerunnerV5.4.4.cpp#L3419) | Render the world into a caller-selected vertical viewport within the current framebuffer. | World drawing combines cached terrain with movement, clue cues and route changes. |
| [`UiRenderer::rect`](src/mazerunnerV5.4.4.cpp#L3484) | Convert a logical UI rectangle into a flat colored quad at z = 0, appending to the overlay batch. | A small set of primitive geometry supports every menu and HUD panel. |
| [`UiRenderer::border`](src/mazerunnerV5.4.4.cpp#L3486) | Draw an outline from four filled rectangles, with configurable thickness defaulting to two logical pixels. | Edges distinguish focused or selected controls against their background. |
| [`UiRenderer::text`](src/mazerunnerV5.4.4.cpp#L3491) | Render a string as enlarged 5×7 bitmap glyphs at a requested logical pixel size. | Bitmap text shares the UI batch and needs no host font installation. |
| [`UiRenderer::centered`](src/mazerunnerV5.4.4.cpp#L3500) | Center one line of bitmap text within a rectangle. | Constant glyph metrics keep button and panel labels aligned. |
| [`UiRenderer::icon`](src/mazerunnerV5.4.4.cpp#L3505) | Draw one embedded sprite in flat UI coordinates. No world tilt or camera transform is used for this illustration. | The menu preview should match the appearance assigned in play. |
| [`UiRenderer::widget`](src/mazerunnerV5.4.4.cpp#L3510) | Render a widget from the same model used by focus and pointer interaction. | One widget model keeps visual state consistent with interaction state. |
| [`UiRenderer::setup`](src/mazerunnerV5.4.4.cpp#L3530) | Draw setup labels, form guidance, the live settings summary, and decorative player/monster art. | Players can inspect difficulty, dimensions and count before generating a maze. |
| [`UiRenderer::hud`](src/mazerunnerV5.4.4.cpp#L3565) | Draw status and help panels: a start prompt before the first input, then time, steps, and hunting/searching counts. | Players can read active pressure and direct the game while watching the maze. |
| [`UiRenderer::release`](src/mazerunnerV5.4.4.cpp#L3585) | Release UI GPU resources during normal destruction or initialization failure. | The menu batch must be freed on error as well as normal shutdown. |
| [`UiRenderer::UiRenderer`](src/mazerunnerV5.4.4.cpp#L3593) | Construct the UI renderer with a current GL context, before any game maze is required. | A reusable UI batch avoids rebuilding GPU objects on every screen. |
| [`UiRenderer::UiRenderer`](src/mazerunnerV5.4.4.cpp#L3604) | Disable copying so UI GPU handles have one owner. | Copy construction would alias the overlay’s program, vertex array and buffer. |
| [`UiRenderer::operator=`](src/mazerunnerV5.4.4.cpp#L3605) | Disable copy assignment for the same reason. | Copy assignment would violate the overlay’s exclusive ownership. |
| [`UiRenderer::~UiRenderer`](src/mazerunnerV5.4.4.cpp#L3607) | Delete UI resources while the context is still alive. | Menu graphics resources must be released before destroying the window. |
| [`UiRenderer::draw`](src/mazerunnerV5.4.4.cpp#L3609) | Draw either a full menu screen or an overlay above a supplied maze frame. | Only the active screen should contribute UI geometry for a frame. |

</details>

## Application and native callbacks

GameApplication owns the menu state, asynchronous maze construction and the active renderer. The worker captures configuration by value and returns a CPU-side maze. The main thread takes the result and builds GPU resources with the context current. This boundary makes failure recoverable at the setup screen. A future delivering an exception needs the same cleanup path as an ordinary allocation or initialization failure. GLFW callbacks reach the active application through the window's stored pointer, so its lifetime must extend beyond every possible callback.

Pause, focus loss and replay are transitions with temporal consequences. Resetting the wall-clock baseline on resume prevents the pause from being simulated as a burst of catch-up actions. Releasing held input removes stale keystrokes on focus loss. Fixed-seed replay and new-seed replay also have different ownership of randomness. Trace a full Setup → Loading → Playing → Paused → Playing cycle when debugging clocks, and exercise the injected worker-failure fixture when altering asynchronous construction. [Application integration tests](tests/app_tests.cpp)

<details>
<summary>Functions in this section · behavior and design notes</summary>

| Function | What it does | Why it exists |
|---|---|---|
| [`GameApplication::release_hold`](src/mazerunnerV5.4.4.cpp#L3665) | Forget the physical key and disarm the simulation repeat. | Input from a previous screen must not continue moving the player. |
| [`GameApplication::press_move`](src/mazerunnerV5.4.4.cpp#L3670) | Submit a first move to the maze and retain the physical key while its timed repeats are armed. | A physical key and its held-repeat direction need a common lifetime. |
| [`GameApplication::GameApplication`](src/mazerunnerV5.4.4.cpp#L3675) | Construct the application from startup options, before a maze has been generated. | A fixed seed supports repeatable play while the setup remains editable. |
| [`GameApplication::discard_game`](src/mazerunnerV5.4.4.cpp#L3680) | Destroy world graphics before the Maze, reset reporting, and cancel any held movement key. | Re-entering setup must release the old renderer before its scene. |
| [`GameApplication::begin_generation`](src/mazerunnerV5.4.4.cpp#L3682) | Start background generation from a validated snapshot of the form. | Large maze generation can run while the UI continues drawing a loading screen. |
| [`GameApplication::finish_generation`](src/mazerunnerV5.4.4.cpp#L3697) | Poll and install a completed Maze on the thread with the current GL context. | Completed results and worker exceptions must reach the main thread safely. |
| [`GameApplication::tick`](src/mazerunnerV5.4.4.cpp#L3711) | Advance the full nonnegative elapsed interval while Playing. The maze chronologically dispatches due manual repeats. | Only active play should advance the simulation clock. |
| [`GameApplication::draw`](src/mazerunnerV5.4.4.cpp#L3721) | Draw the world in the central reserved area and then draw the full-frame UI above it. | The game world and controls need distinct viewport regions. |
| [`ui_cursor`](src/mazerunnerV5.4.4.cpp#L3733) | Convert the live GLFW cursor into the logical coordinate system used by widget rectangles. | High-DPI displays require conversion between window and framebuffer coordinates. |
| [`key_callback`](src/mazerunnerV5.4.4.cpp#L3740) | Dispatch key events: Esc pauses, resumes, or leaves a field but never quits. Movement keys start a hold. Releases stop it. | Each key needs one interpretation in setup, active play, pause, or results. |
| [`character_callback`](src/mazerunnerV5.4.4.cpp#L3785) | Receive GLFW text codepoints separately from physical-key events, enabling consistent numeric typing. | Text entry must use translated digit characters independently of key positions. |
| [`mouse_callback`](src/mazerunnerV5.4.4.cpp#L3790) | Translate GLFW mouse button actions into UI press/release behavior. | Mouse clicks need the same activation behavior as keyboard commands. |
| [`scroll_callback`](src/mazerunnerV5.4.4.cpp#L3797) | Handle mouse-wheel input as a visual zoom operation. | Scroll-wheel zoom must apply to the active world view. |
| [`focus_callback`](src/mazerunnerV5.4.4.cpp#L3803) | Pause on focus loss and cancel any held movement key. | Losing focus must prevent involuntary movement in the background. |
| [`initialize_glew`](src/mazerunnerV5.4.4.cpp#L3812) | Initialize GLEW after making an OpenGL context current, then verify the capabilities this renderer actually requires. | The renderer needs confirmed driver capabilities before creating resources. |
| [`run_graphical`](src/mazerunnerV5.4.4.cpp#L3831) | Open the native window with the shared title and run the loop, sleeping to a frame deadline while playing. | Window, context, workers and GPU owners must shut down in a safe order. |
| [`main`](src/mazerunnerV5.4.4.cpp#L3888) | Program entry point for both graphical and headless executables. | Both desktop users and automation need one versioned executable entry point. |

</details>
