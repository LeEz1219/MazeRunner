# Monster senses, patrols and interception

[MazeRunner V5.4.3a wiki](Home.md)

## Monster AI: hearing, hunting, searching, and interception

Every monster uses the same behavioral rules. The ten creature appearances are visual variants, not classes with different speed, vision, damage, or intelligence. Spawn IDs give them distinct stable artwork. Spawn selection excludes the entrance/player and exit and keeps monsters on unique open cells. It favors positions sufficiently far from the player using graph distance, but can fall back to nearer candidates when necessary; there is no universal minimum separation guarantee on all allowed sizes. On a very small maze, the requested monster count is also capped by the number of distinct available spawn cells.

A monster is in one of three states: `Patrolling`, `Hunting`, or `Searching`. A hunter has one of two roles: a **chaser** converges on the clue area; a **cutter** waits at a chokepoint between the clue area and the exit.

### Patrol behavior

A patrolling monster has no player information. It samples 32 random open cells and scores each one. A cell scores higher the longer it has been since any monster entered its 8×8 region, capped at 20 seconds. Travel distance (capped at 20 steps) adds a small amount. The score is penalized for cells this monster visited recently, for cells another monster already claimed, and for each other monster positioned or heading within six cells. Together the monsters therefore cover the map instead of clumping. Patrol is not a stationary guard waiting for an alarm.

Following a target uses a reverse BFS distance gradient. Recent-cell penalties discourage immediate repetition. When blocked, a monster can accept limited non-improving movement to sidestep congestion, and repeated blockage causes a new target choice. Targets are reviewed on a 700-ms schedule, but review can preserve a still-valid waypoint. Each monster keeps a bounded recent-cell history rather than a permanent visited map.

### Local footsteps and map-wide director reports

An actual displacement can create two clues. A blocked keypress, a solver wait and a plain clock tick produce no new location report. Each clue is an inclusive, grid-quantized rectangle tagged with a revision. Fixed sector boundaries give many possible player cells the same reported bounds.

| Channel | Reach | Effect on a monster | Timing |
|---|---|---|---|
| Footstep | Shortest walkable corridor distance within the difficulty budget | The listener hunts its most recent local clue. Each genuine contact refreshes its own scent. | Test hearing on every successful step. Publishing a new local rectangle has its own throttle. |
| Director radio | All monsters on the map | A patrol or searcher investigates the broadcast sector. A hunter continues following fresher evidence and can later use an eligible radio fallback. | First eligible step, then movement after the profile cooldown. Cooldown doubles in relief. |
| Corridor sight | High only, 3–6 cells on a clear row or column | An eligible monster may pursue a genuine last-seen cell. | Checked by the monster's own behavior. |

Hearing uses `base + floor(per_100 × max(rows, cols) / 100)`. Manhattan distance is a cheap first rejection. A bounded BFS decides which remaining monsters really hear around walls. One acoustic field serves all possible listeners for that displacement. A Medium 63×63 map has a budget of 26 steps.

The local sector side is 9/5/5 cells in Low/Medium/High. The nominal radio side is 19/17/11. Its normal side is limited to `min(profile_radio, max(local_side, floor((2 × max_dimension + 1) / 3)))`. Width and height are separately clipped at map edges. Relief adds padding on both sides. A radio recipient gets these bounds and a revision. It samples walkable targets inside the rectangle.

Each audible step renews the listener's own scent even between clue publications. Normal continuing-motion publication intervals are 200/0/0 ms. During relief they are at least 400 ms. First motion after an idle spell immediately qualifies for a fresh local clue. The radio still observes its own cooldown after an idle spell. [First-step terminal trace](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/director-radio.log)
A hunting target is sampled inside the clue rectangle with weights favoring nearby cells and penalizing recent visits. A candidate already claimed by another hunter is strongly downweighted, encouraging a distributed search. A valid existing target may be retained after an updated clue.

### Searching after losing the scent

At a hunter's scent deadline, it can adopt a broadcast issued strictly after its last physical contact if that report is still within the profile's search lifetime. That sweep ends at the broadcast time plus search duration. Otherwise the monster sweeps its last local area from scent loss. A new broadcast can renew a remote search. Once its search time expires without fresh evidence the monster returns to roaming. Searching uses the orange-eye state.

Expiry is an exact simulation event (`next_monster_state_change`), not a frame-time check, so results do not depend on rendering cadence. A monster that hears a new step while searching becomes a hunter again immediately.

### Interceptors (Medium and High)

`rebuild_graph` builds an exit-rooted depth-first tree with an iterative Tarjan traversal. For each cell it records whether the cell's parent separates that cell's subtree from the exit. When a parent does, every route from anywhere in the subtree to the exit must pass through it. When a new clue is published, `assign_interceptors` starts from the walkable cell of the clue rectangle nearest the exit; it never uses the player's cell. It then walks up that tree for at most 80 links, collecting chokepoints outside the clue area and at least four steps ahead. For each chokepoint, the nearest eligible hunter is sent if it can arrive before a player moving one cell per 80 ms would. Medium sends at most one interceptor and High at most two. Low sends none.

A cutter can hold a reachable chokepoint, then release it at its role deadline. The assignment code accounts for guards outside the current group of listeners before choosing additional hunters. It reserves distinct target cells and enforces the profile-wide 0/1/2 cap. A monster mid-charge after a sighting is ineligible. Relief releases existing guards and suppresses new assignments until normal precision resumes.

### Corridor sight (High only)

High gives monsters short sight. A monster that is **not already hunting** (roaming or searching), or one already mid-charge, can see the player along a shared row or column. The distance must be 3–6 cells, and every cell between them must be open. A sighting turns the monster into a hunter targeting the exact cell where it saw the player. It keeps re-sighting while the line holds, and when the line breaks it continues to the last seen cell. Reaching that cell ends the charge.

Sight is the only sensory path that can supply an exact last-seen player cell. Losing the line of sight leaves the last observed destination available. A broadcast carries rectangular bounds even if a hunter somewhere else can see the player.

### Avoiding monster overlap

When several monsters act in a batch, movement priority rotates. Reservations include all starting cells and claimed destinations for that batch, kept in a short list (at most about twenty cells) instead of a grid-sized array. Monsters cannot stack, swap through each other, or immediately follow another monster into a cell it vacated in that same batch. This prevents order-dependent overlap but can create temporary corridor congestion. Target reassessment and sidesteps help resolve congestion over later batches.
