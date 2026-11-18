// Build: g++ -std=c++17 -O2 -DNDEBUG mazerunnerV5.4.3a.cpp -o mazerunnerV5.4.3a -lglfw -lGLEW -lGL -pthread
// Headless: g++ -std=c++17 -O2 -DNDEBUG -DMAZERUNNER_HEADLESS mazerunnerV5.4.3a.cpp -o mazerunnerV5.4.3a-headless -pthread

#ifndef MAZERUNNER_HEADLESS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif
#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstddef>
#include <utility>
#include <vector>
#include <future>
#include <thread>

// One version string for logs, the window title, the setup screen, and --help.
// CMake supplies the release label. Standalone builds use the same default.
#ifndef MAZERUNNER_VERSION
#define MAZERUNNER_VERSION "5.4.3a"
#endif
#define MAZERUNNER_TITLE "Maze Runner V" MAZERUNNER_VERSION

// Times are integer simulation milliseconds. The renderer uses a monotonic clock.
constexpr int AUTO_SOLVE_SPEED_MS = 40;
constexpr int MANUAL_HOLD_DELAY_MS = 220;
constexpr int MANUAL_HOLD_REPEAT_MS = 120;
constexpr int MAX_MONSTERS = 10;
constexpr int MAX_MAZE_DIMENSION = 501;
constexpr int MONSTER_REPLAN_MS = 700;
constexpr int AUTO_HORIZON = 12;          // Default lookahead, in player actions.
constexpr int AUTO_HORIZON_MAX = 18;      // Extended lookahead while threats are at mid range.
constexpr int AUTO_BEAM_WIDTH = 64;
// Navigation retains the same intelligence and action rate on every difficulty.
constexpr int AUTO_STALL_MS = 2400;
constexpr int AUTO_RECOVERY_COMMIT_MS = 4000;
constexpr int AUTO_VISIT_MEMORY_MS = 12000;
// The solver hard-blocks only threats it could meet before they move away.
// Farther monsters add a soft detour cost instead of closing the route outright.
constexpr int AUTO_NEAR_THREAT_CELLS = 14;
constexpr int AUTO_FAR_THREAT_COST = 24;
constexpr int AUTO_STATIONARY_MS = 1500;
// Repeated detours inside this window raise boldness and shorten waiting.
constexpr int AUTO_LOOP_WINDOW_MS = 20000;
constexpr int AUTO_MAX_BOLDNESS = 4;
// Bounded per-monster distance fields; beyond this radius a lower bound is used.
constexpr int THREAT_RADIUS = 64;
// Interceptors walk at most this far along the exit tree looking for chokepoints.
constexpr int CUTTER_SEARCH_STEPS = 80;
constexpr int CUTTER_PACE_MS = 80;
constexpr int CUTTER_CAMP_MS = 4000;
// Patrols spread across coarse regions by preferring ones not recently visited.
constexpr int PATROL_REGION = 8;
// Corridor sight (High) only triggers at this distance or more.
constexpr int SIGHT_MIN_CELLS = 3;
// Generation retries before deterministically adding connections instead.
constexpr int MAX_GENERATION_ATTEMPTS = 64;
// The director changes information precision, never movement speed or collisions.
constexpr int DIRECTOR_TICK_MS = 200;
constexpr int DIRECTOR_RECOVERY_MS = 2000;
constexpr int INF_DISTANCE = 100000000;
using Millis = std::int64_t;
constexpr Millis NEVER = std::numeric_limits<Millis>::max() / 4;
constexpr char WALL = '#', PATH = ' ', EXIT = 'E';

// Integer row/column coordinates; grid cells and moving entities are independent.
struct Point {
    int r = -1, c = -1;
    /// Compare both coordinates.
    bool operator==(const Point& p) const { return r == p.r && c == p.c; }
    /// Compare coordinates for inequality.
    bool operator!=(const Point& p) const { return !(*this == p); }
};

enum class MonsterState { Patrolling, Hunting, Searching };
enum class MonsterRole { Chaser, Cutter };
enum class Outcome { Playing, Won, Caught };
enum class Difficulty { Low, Medium, High };

// Difficulty changes opposition and terrain, not the player's planning ability.
// All profiles share the same planner, director rules, and collision handling.
// hearing_cells is a budget in walkable corridor steps, widened on larger maps.
// search_ms is how long a monster sweeps the last clue area after losing the scent.
// cutters is how many hunters may be sent to a chokepoint between the clue and the exit.
// sight_cells lets a monster see the player down a straight open corridor (0 = never).
struct DifficultySettings {
    const char* name;
    int minimum_solutions, loop_density_percent;
    int monster_move_ms, hint_area_size, hint_interval_ms, scent_ms;
    int confinement_ms, relief_ms, padding_step, max_padding;
    int hearing_cells, hearing_per_100, search_ms, cutters, sight_cells;
    int report_area_size, report_interval_ms;  // Strategic radio, independent of local hearing.
};

/// Keep all balancing values together; scent memory is independent of move speed.
/// Escape-rate calibration uses 21x21/31x31 maps, five monsters, and a 60 s limit;
/// those aggregate targets are not guarantees for other sizes or monster counts.
constexpr DifficultySettings difficulty_settings(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Low:    return {"Low",    4, 225, 220, 9, 200, 420, 2500, 8000, 4, 16, 18, 10, 1500, 0, 0, 19, 3000};
        case Difficulty::Medium: return {"Medium", 2, 135, 148, 5,   0, 560, 4000, 6000, 4, 16, 18, 14, 2500, 1, 0, 17, 1800};
        case Difficulty::High:   return {"High",   2, 100, 118, 5,   0, 840, 6000, 4000, 2,  8, 28, 18, 3500, 2, 6, 11, 1000};
    }
    throw std::invalid_argument("Unknown difficulty.");
}

/// Accept menu numbers or case-insensitive names, trimming surrounding whitespace.
Difficulty parse_difficulty(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) throw std::invalid_argument("Difficulty cannot be empty.");
    text = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (text == "1" || text == "low") return Difficulty::Low;
    if (text == "2" || text == "medium") return Difficulty::Medium;
    if (text == "3" || text == "high") return Difficulty::High;
    throw std::invalid_argument("Choose 1/Low, 2/Medium, or 3/High.");
}

/// Prompt before maze generation. Enter selects Medium; invalid choices retry.
Difficulty prompt_difficulty(std::istream& input, std::ostream& output) {
    output << "Difficulty: 1 = Low | 2 = Medium | 3 = High\n";
    for (;;) {
        output << "Select difficulty [Enter = Medium]: " << std::flush;
        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("Difficulty input closed.");
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) return Difficulty::Medium;
        try { return parse_difficulty(line); }
        catch (const std::invalid_argument& e) { output << e.what() << '\n'; }
    }
}

// Fixed map regions conceal the exact position: the rectangle is not centered on
// the player. Hunting decisions receive only these bounds and a movement revision.
struct LocationHint {
    int min_row = -1, max_row = -1, min_col = -1, max_col = -1;
    std::uint64_t revision = 0;
    /// Test whether a grid coordinate lies inside this approximate location area.
    bool contains(Point p) const {
        return revision != 0 && p.r >= min_row && p.r <= max_row &&
               p.c >= min_col && p.c <= max_col;
    }
};

// A radio report starts an area search. Local hearing and High corridor sight
// start a hunt. Each monster's scent clock is independent of the radio.
struct Monster {
    int id = 0;
    int cell = -1;
    MonsterState state = MonsterState::Patrolling;
    MonsterRole role = MonsterRole::Chaser;
    LocationHint hint;
    Millis next_move = 0;
    Millis next_replan = 0;
    Millis scent_until = NEVER, search_until = NEVER, role_until = NEVER;
    Millis last_moved_at = 0;
    std::uint64_t planned_revision = 0;
    int target = -1;
    int blocked_turns = 0;
    bool sighted = false;
    std::deque<int> recent_cells;
};

// Time-based memory distinguishes actual progress from repeated moves and waits.
struct VisitMemory {
    Millis at = -NEVER;
    double heat = 0;
};

// Only the director sees player motion samples. Monsters receive LocationHint.
struct DirectorState {
    struct Sample { Millis at; int cell; };
    std::deque<Sample> samples;
    Millis next_update = NEVER, relief_until = NEVER, recovered_since = NEVER;
    Millis last_report_at = -NEVER;
    LocationHint last_report;  // Shared radio memory, including for hunters using fresher local evidence.
    std::uint64_t reports = 0;
    int hint_padding = 0, relief_anchor = -1, relief_exit_distance = INF_DISTANCE;
};

// All game rules live here, independent of GLFW. Tests use this same simulation.
class Maze {
public:
    int rows, cols;
    std::vector<char> grid;
    int player = -1, entrance = -1, exit_cell = -1;
    std::vector<Monster> monsters;
    std::vector<Point> solution_overlay;
    bool overlay_avoids_monsters = true;
    std::ostream* log_output;
    bool verbose_logs = false;
    Outcome outcome = Outcome::Playing;
    bool auto_solve = false;
    bool timer_started = false;
    int manual_steps = 0, automatic_steps = 0;
    Millis now = 0, timer_start = 0, elapsed_ms = 0, next_auto = NEVER;
    std::uint32_t seed;
    const Difficulty difficulty;
    const DifficultySettings settings;

    /// Validate dimensions, round upward to odd values, and allocate a solid grid.
    Maze(int r, int c, std::uint32_t random_seed, std::ostream* logs = &std::cout,
         Difficulty level = Difficulty::Medium)
        : rows(r), cols(c), log_output(logs), seed(random_seed),
          difficulty(level), settings(difficulty_settings(level)), rng(random_seed) {
        if (r < 5 || c < 5 || r > MAX_MAZE_DIMENSION || c > MAX_MAZE_DIMENSION)
            throw std::runtime_error("Maze dimensions must be between 5 and 501.");
        if (rows % 2 == 0) ++rows;
        if (cols % 2 == 0) ++cols;
        grid.assign(rows * cols, WALL);
        entrance = id({1, 0});
        exit_cell = id({rows - 2, cols - 1});
        player = entrance;
        graph_signature = terrain_signature();
    }

    /// Convert valid coordinates to a flat index; return -1 outside the grid.
    int id(Point p) const {
        return p.r >= 0 && p.r < rows && p.c >= 0 && p.c < cols ? p.r * cols + p.c : -1;
    }
    /// Convert a valid flat cell index to row and column.
    Point point(int cell) const { return {cell / cols, cell % cols}; }
    /// Test bounds and terrain, without making assumptions about entity occupancy.
    bool walkable(int cell) const {
        return cell >= 0 && cell < static_cast<int>(grid.size()) && grid[cell] != WALL;
    }
    /// Return four-neighbor Manhattan distance for valid cell indices.
    int manhattan(int a, int b) const {
        Point p = point(a), q = point(b);
        return std::abs(p.r - q.r) + std::abs(p.c - q.c);
    }
    /// Expose cached cardinal adjacency, with -1 marking absent neighbors.
    const std::array<int, 4>& neighbors(int cell) const { return adjacency[cell]; }
    /// Return the number of open cardinal neighbors of a cell.
    int degree(int cell) const { return degrees[cell]; }

    /// Generate until the selected profile's route-diversity requirement is met.
    /// The RNG is seeded once, so a failed attempt produces a new maze on the retry.
    /// Spawn monsters only after the topology passes validation.
    void generate(int count) {
        if (count < 0 || count > MAX_MONSTERS)
            throw std::runtime_error("Monster count must be between 0 and 10.");
        rng.seed(seed);
        reset_game();
        int attempts = 0;
        for (;;) {
            ++attempts;
            std::fill(grid.begin(), grid.end(), WALL);
            std::vector<int> stack{id({1, 1})};
            grid[stack.back()] = PATH;
            while (!stack.empty()) {
                Point p = point(stack.back());
                std::array<int, 4> order{0, 1, 2, 3};
                std::shuffle(order.begin(), order.end(), rng);
                bool carved = false;
                for (int d : order) {
                    Point q{p.r + 2 * directions[d].r, p.c + 2 * directions[d].c};
                    if (q.r <= 0 || q.r >= rows - 1 || q.c <= 0 || q.c >= cols - 1) continue;
                    int v = id(q);
                    if (grid[v] != WALL) continue;
                    grid[v] = PATH;
                    grid[id({p.r + directions[d].r, p.c + directions[d].c})] = PATH;
                    stack.push_back(v);
                    carved = true;
                    break;
                }
                if (!carved) stack.pop_back();
            }
            grid[entrance] = PATH;
            grid[exit_cell] = EXIT;
            std::vector<int> walls;
            for (int r = 1; r < rows - 1; ++r) {
                for (int c = 1; c < cols - 1; ++c) {
                    int u = id({r, c});
                    if (grid[u] != WALL) continue;
                    if ((walkable(u - 1) && walkable(u + 1)) ||
                        (walkable(u - cols) && walkable(u + cols))) walls.push_back(u);
                }
            }
            std::shuffle(walls.begin(), walls.end(), rng);
            // Integer percentages allow gradual route-density tuning, with no
            // extra RNG draws and the same count at legacy 100%/200% settings.
            int loop_count = settings.loop_density_percent *
                             std::max(1, rows * cols / (std::max(rows, cols) > 40 ? 150 : 100)) / 100;
            if (difficulty == Difficulty::Low) {
                // Refresh candidates after each carve: even a 5x5 maze can gain
                // a second connection that was not available in the DFS tree.
                std::vector<unsigned char> queued(grid.size(), 0);
                for (int u : walls) queued[u] = 1;
                for (int i = 0; i < loop_count; ++i) {
                    if (i >= static_cast<int>(walls.size())) break;
                    int opened = walls[i]; grid[opened] = PATH;
                    Point p = point(opened);
                    for (Point d : directions) {
                        Point q{p.r + d.r, p.c + d.c}; int u = id(q);
                        if (q.r <= 0 || q.r >= rows - 1 || q.c <= 0 || q.c >= cols - 1 || grid[u] != WALL) continue;
                        if (((walkable(u - 1) && walkable(u + 1)) ||
                             (walkable(u - cols) && walkable(u + cols))) &&
                            !queued[u]) { walls.push_back(u); queued[u] = 1; }
                    }
                }
            } else {
                for (int i = 0; i < std::min(loop_count, static_cast<int>(walls.size())); ++i)
                    grid[walls[i]] = PATH;
            }
            rebuild_graph();
            if (route_diversity_met()) break;
            if (attempts >= MAX_GENERATION_ATTEMPTS) {
                add_connections_until_diverse();
                break;
            }
            log_event("Maze generation attempt " + std::to_string(attempts) +
                      ": Insufficient entrance-to-exit route diversity; regenerating.");
        }
        log_event("Maze generated after " + std::to_string(attempts) +
                  " attempt(s) | At least " + (settings.minimum_solutions == 4 ? "FOUR" : "TWO") +
                  " distinct entrance-to-exit paths verified.");
        spawn_monsters(count);
    }

    /// Apply the selected profile's route-diversity requirement to the current terrain.
    bool route_diversity_met() const {
        return difficulty == Difficulty::Low ? solution_diversity(true) >= settings.minimum_solutions
                                             : has_at_least_two_solutions();
    }

    /// Guarantee termination after repeated unlucky layouts: keep this maze and
    /// open extra connections in batches. Every pass opens at least one wall, and a
    /// fully opened interior always satisfies every profile, so the loop is finite.
    void add_connections_until_diverse() {
        int passes = 0;
        while (!route_diversity_met()) {
            std::vector<int> candidates;
            for (int r = 1; r < rows - 1; ++r)
                for (int c = 1; c < cols - 1; ++c) {
                    int u = id({r, c});
                    if (grid[u] == WALL && ((walkable(u - 1) && walkable(u + 1)) ||
                                            (walkable(u - cols) && walkable(u + cols)))) candidates.push_back(u);
                }
            if (candidates.empty()) {
                for (int r = 1; r < rows - 1; ++r) for (int c = 1; c < cols - 1; ++c) grid[id({r, c})] = PATH;
                grid[exit_cell] = EXIT;
                rebuild_graph();
                break;
            }
            std::shuffle(candidates.begin(), candidates.end(), rng);
            std::size_t batch = std::max<std::size_t>(1, candidates.size() / 8);
            for (std::size_t i = 0; i < batch; ++i) grid[candidates[i]] = PATH;
            rebuild_graph();
            ++passes;
        }
        log_event("Maze generation fallback: added connections in " + std::to_string(passes) +
                  " pass(es) after " + std::to_string(MAX_GENERATION_ATTEMPTS) + " attempts.");
    }

    /// Verify two distinct simple solutions in O(V+E), ignoring moving entities.
    /// An iterative DFS marks bridges. If its entrance-to-exit tree path contains
    /// a non-bridge edge, an alternate route exists. A cycle on a side branch alone
    /// does not qualify. Iteration avoids recursion overflow on large mazes.
    bool has_at_least_two_solutions() const {
        if (!walkable(entrance) || !walkable(exit_cell)) return false;
        const int n = static_cast<int>(grid.size());
        std::vector<int> discovered(n, -1), low(n), parent(n, -1), next_neighbor(n, 0);
        std::vector<unsigned char> bridge_to_parent(n, 0);
        std::vector<int> stack{entrance};
        int sequence = 0;
        discovered[entrance] = low[entrance] = sequence++;
        while (!stack.empty()) {
            int u = stack.back();
            if (next_neighbor[u] < 4) {
                int v = adjacency[u][next_neighbor[u]++];
                if (v < 0 || v == parent[u]) continue;
                if (discovered[v] < 0) {
                    parent[v] = u;
                    discovered[v] = low[v] = sequence++;
                    stack.push_back(v);
                } else {
                    low[u] = std::min(low[u], discovered[v]);
                }
            } else {
                stack.pop_back();
                int p = parent[u];
                if (p >= 0) {
                    low[p] = std::min(low[p], low[u]);
                    bridge_to_parent[u] = low[u] > discovered[p];
                }
            }
        }
        if (discovered[exit_cell] < 0) return false;
        for (int u = exit_cell; u != entrance; u = parent[u])
            if (!bridge_to_parent[u]) return true;
        return false;
    }

    /// Return a conservative lower bound on simple solutions, capped at four.
    /// In a biconnected block, cycle rank r guarantees at least r+1 paths between
    /// its entry/exit vertices (start with a cycle and add ears). Independent
    /// blocks along the entrance/exit route multiply those choices. Off-route
    /// cycles contribute nothing. This iterative Tarjan traversal is O(V+E).
    /// Low additionally ignores tiny blocks: useful blocks need at least twelve
    /// vertices, scaled down for small maps where such separation cannot fit.
    int solution_diversity(bool substantial_only = false) const {
        if (!walkable(entrance) || !walkable(exit_cell)) return 0;
        const int n = static_cast<int>(grid.size());
        std::vector<int> discovered(n, -1), low(n), parent(n, -1), cursor(n, 0);
        std::vector<int> tree_block(n, -1), marked(n, -1), choices;
        std::vector<int> stack{entrance};
        std::vector<std::pair<int, int>> edges;
        int sequence = 0;
        const int minimum_block = substantial_only ? std::min(12, ((rows - 1) / 2) * ((cols - 1) / 2)) : 2;
        discovered[entrance] = low[entrance] = sequence++;
        while (!stack.empty()) {
            int u = stack.back();
            if (cursor[u] < 4) {
                int v = adjacency[u][cursor[u]++];
                if (v < 0 || v == parent[u]) continue;
                if (discovered[v] < 0) {
                    parent[v] = u; edges.emplace_back(u, v);
                    discovered[v] = low[v] = sequence++; stack.push_back(v);
                } else if (discovered[v] < discovered[u]) {
                    low[u] = std::min(low[u], discovered[v]); edges.emplace_back(u, v);
                }
            } else {
                stack.pop_back(); int p = parent[u];
                if (p < 0) continue;
                low[p] = std::min(low[p], low[u]);
                if (low[u] < discovered[p]) continue;
                const int block = static_cast<int>(choices.size());
                int edge_count = 0, vertex_count = 0;
                for (;;) {
                    auto edge = edges.back(); edges.pop_back(); ++edge_count;
                    for (int v : {edge.first, edge.second}) if (marked[v] != block) {
                        marked[v] = block; ++vertex_count;
                    }
                    if (parent[edge.second] == edge.first) tree_block[edge.second] = block;
                    if (edge.first == p && edge.second == u) break;
                }
                choices.push_back(vertex_count < minimum_block ? 1 : std::min(4, edge_count - vertex_count + 2));
            }
        }
        if (discovered[exit_cell] < 0) return 0;
        std::vector<unsigned char> used(choices.size(), 0);
        int total = 1;
        for (int u = exit_cell; u != entrance; u = parent[u]) {
            int block = tree_block[u];
            if (!used[block]) {
                total = std::min(4, total * choices[block]); used[block] = 1;
            }
        }
        return total;
    }

    /// Rebuild adjacency, dead-end depths, and search buffers after terrain edits.
    void rebuild_graph() {
        const int n = static_cast<int>(grid.size());
        adjacency.assign(n, {-1, -1, -1, -1});
        degrees.assign(n, 0);
        open_cells.clear();
        for (int u = 0; u < n; ++u) {
            if (!walkable(u)) continue;
            open_cells.push_back(u);
            Point p = point(u);
            for (int d = 0; d < 4; ++d) {
                int v = id({p.r + directions[d].r, p.c + directions[d].c});
                if (walkable(v)) { adjacency[u][d] = v; ++degrees[u]; }
            }
        }
        // Prune leaves toward junctions; protect the entrance-to-exit backbone.
        std::vector<int> remaining = degrees, parent(n, -1), removed;
        std::queue<int> leaves;
        for (int u : open_cells)
            if (remaining[u] == 1 && u != entrance && u != exit_cell) leaves.push(u);
        while (!leaves.empty()) {
            int u = leaves.front(); leaves.pop();
            if (remaining[u] != 1) continue;
            remaining[u] = 0;
            removed.push_back(u);
            for (int v : adjacency[u]) if (v >= 0 && remaining[v] > 0) {
                parent[u] = v;
                if (--remaining[v] == 1 && v != entrance && v != exit_cell) leaves.push(v);
                break;
            }
        }
        dead_end_depth.assign(n, 0);
        branch_gate.assign(n, -1);
        for (auto it = removed.rbegin(); it != removed.rend(); ++it) {
            int p = parent[*it];
            if (p >= 0) {
                dead_end_depth[*it] = dead_end_depth[p] + 1;
                branch_gate[*it] = branch_gate[p] >= 0 ? branch_gate[p] : p;
            }
        }
        fields.clear();
        guide_distances.clear(); guide_occupancy.clear();
        overlay_dirty = true;
        exit_distances = distance_field(exit_cell);
        beam_seen.assign(n, 0); beam_index.resize(n); beam_epoch = 0;
        visits.assign(n, {});
        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();
        local_pool.clear();
        build_exit_tree();
        region_cols = (cols + PATROL_REGION - 1) / PATROL_REGION;
        region_seen.assign(static_cast<std::size_t>(region_cols) * ((rows + PATROL_REGION - 1) / PATROL_REGION), 0);
        graph_signature = terrain_signature();
    }

    /// Hash the terrain so debug builds can detect edits made without rebuild_graph().
    std::uint64_t terrain_signature() const {
        std::uint64_t hash = 1469598103934665603ull;
        for (char c : grid) { hash ^= static_cast<unsigned char>(c); hash *= 1099511628211ull; }
        return hash ^ grid.size();
    }

    /// Debug-only guard: cached adjacency and distance fields describe the old map
    /// if grid[] changes without rebuild_graph(). Release builds (NDEBUG) skip it.
    void verify_graph() const {
#ifndef NDEBUG
        if (graph_signature != terrain_signature())
            throw std::logic_error("Maze terrain changed without rebuild_graph().");
#endif
    }

    /// Reset runtime state, movement hints, idle deadlines, and route visualization.
    void reset_game() {
        player = entrance;
        monsters.clear(); solution_overlay.clear(); player_history.clear();
        outcome = Outcome::Playing; auto_solve = false; timer_started = false;
        manual_steps = automatic_steps = 0;
        now = timer_start = elapsed_ms = 0; next_auto = NEVER;
        cancel_held_move();
        movement_revision = 0; idle_deadline = NEVER; last_movement_hint_at = -NEVER;
        overlay_dirty = true; overlay_reported = false; overlay_avoids_monsters = true;
        last_auto_wait = false; planned_wait_for_idle = false;
        priority_rotation = 0;
        guide_distances.clear(); guide_occupancy.clear();
        director = {};
        visits.assign(grid.size(), {}); tried_recovery_goals.clear();
        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();
        blocked_since = clear_route_since = NEVER; next_recovery_attempt = 0;
        navigation_progress_at = recovery_progress_at = 0;
        best_exit_distance = recovery_best_distance = INF_DISTANCE;
        recovery_deadline = NEVER;
        recovery_pause_until = 0;
        latest_hint = {};
        detour_history.clear(); last_reported_boldness = 0;
        std::fill(region_seen.begin(), region_seen.end(), 0);
    }

    /// Place unique monsters, preferring cells at a substantial corridor distance.
    void spawn_monsters(int count) {
        monsters.clear();
        auto from_player = distance_field(player);
        std::vector<int> candidates;
        for (int u : open_cells) if (u != player && u != exit_cell) candidates.push_back(u);
        std::shuffle(candidates.begin(), candidates.end(), rng);
        const int preferred_distance = std::min(rows, cols) / 2;
        std::stable_partition(candidates.begin(), candidates.end(), [&](int u) {
            return (*from_player)[u] > preferred_distance;
        });
        count = std::min(std::max(0, count), std::min(MAX_MONSTERS, static_cast<int>(candidates.size())));
        for (int i = 0; i < count; ++i) {
            Monster m;
            m.id = i; m.cell = candidates[i];
            // Offsets are relative to spawn. Monsters stay frozen until the player's
            // first input; start_timer() shifts these deadlines to that moment.
            m.next_move = now + 1 + static_cast<int>(rng() % settings.monster_move_ms);
            m.last_moved_at = now;
            monsters.push_back(m);
            mark_region(m.cell);
            log_event("Monster " + std::to_string(m.id + 1) + " spawned at " + cell_label(m.cell) + " | ROAMING");
        }
        overlay_dirty = true;
        log_event("Player spawned at " + cell_label(player) + " | Exit " + cell_label(exit_cell));
    }

    /// Cache reverse BFS fields by destination; shared ownership survives eviction.
    /// Hits move to the back, so long-lived target fields are not evicted by churn.
    std::shared_ptr<const std::vector<int>> distance_field(int destination) {
        for (auto it = fields.begin(); it != fields.end(); ++it) if (it->first == destination) {
            auto hit = *it;
            if (std::next(it) != fields.end()) { fields.erase(it); fields.push_back(hit); }
            return hit.second;
        }
        auto distances = std::make_shared<std::vector<int>>(grid.size(), INF_DISTANCE);
        std::vector<int> queue;
        if (walkable(destination)) {
            (*distances)[destination] = 0;
            queue.push_back(destination);
        }
        for (std::size_t head = 0; head < queue.size(); ++head) {
            int u = queue[head];
            for (int v : adjacency[u]) if (v >= 0 && (*distances)[v] == INF_DISTANCE) {
                (*distances)[v] = (*distances)[u] + 1;
                queue.push_back(v);
            }
        }
        if (fields.size() >= 24) fields.pop_front();
        fields.emplace_back(destination, distances);
        return distances;
    }

    /// Bounded corridor distances. Unreached cells report radius + 1, a lower
    /// bound for threat estimates and an out-of-range value for hearing.
    struct LocalField {
        int source = -1;
        int radius = THREAT_RADIUS;
        std::uint32_t epoch = 0;
        std::vector<std::uint32_t> stamp;
        std::vector<std::uint8_t> dist;
        std::vector<int> queue;
        int at(int cell) const { return stamp[cell] == epoch ? dist[cell] : radius + 1; }
    };

    /// Return a bounded field, reusing pooled buffers. Entries still referenced by
    /// a live forecast are never overwritten, because only unshared ones are recycled.
    std::shared_ptr<const LocalField> local_field(int source, int radius = THREAT_RADIUS) {
        if (radius < 0 || radius > std::numeric_limits<std::uint8_t>::max())
            throw std::invalid_argument("Local distance radius exceeds the byte field.");
        for (std::size_t i = local_pool.size(); i-- > 0;) {
            if (local_pool[i]->source != source || local_pool[i]->radius != radius) continue;
            auto hit = local_pool[i];
            local_pool.erase(local_pool.begin() + static_cast<std::ptrdiff_t>(i));
            local_pool.push_back(hit);
            return hit;
        }
        std::shared_ptr<LocalField> field;
        if (local_pool.size() >= MAX_MONSTERS + 4) {
            for (std::size_t i = 0; i < local_pool.size(); ++i) if (local_pool[i].use_count() == 1) {
                field = local_pool[i];
                local_pool.erase(local_pool.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
        if (!field) field = std::make_shared<LocalField>();
        if (field->stamp.size() != grid.size()) {
            field->stamp.assign(grid.size(), 0); field->dist.assign(grid.size(), 0); field->epoch = 0;
        }
        if (++field->epoch == 0) { std::fill(field->stamp.begin(), field->stamp.end(), 0); field->epoch = 1; }
        field->source = source;
        field->radius = radius;
        field->queue.clear();
        if (walkable(source)) {
            field->stamp[source] = field->epoch; field->dist[source] = 0; field->queue.push_back(source);
        }
        for (std::size_t head = 0; head < field->queue.size(); ++head) {
            int u = field->queue[head], d = field->dist[u];
            if (d >= radius) continue;
            for (int v : adjacency[u]) if (v >= 0 && field->stamp[v] != field->epoch) {
                field->stamp[v] = field->epoch; field->dist[v] = static_cast<std::uint8_t>(d + 1); field->queue.push_back(v);
            }
        }
        local_pool.push_back(field);
        return field;
    }

private:
    static constexpr std::array<Point, 4> directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    std::mt19937 rng;
    std::vector<std::array<int, 4>> adjacency;
    std::vector<int> degrees, open_cells, dead_end_depth, branch_gate;
    std::deque<std::pair<int, std::shared_ptr<const std::vector<int>>>> fields;
    std::shared_ptr<const std::vector<int>> exit_distances;
    std::vector<int> beam_index;
    std::vector<int> guide_distances, guide_occupancy, guide_next;
    std::vector<std::uint32_t> beam_seen;
    std::uint32_t beam_epoch = 0;
    std::uint64_t movement_revision = 0;
    Millis idle_deadline = NEVER;
    Millis next_manual_repeat = NEVER;
    Point held_direction{0, 0};
    Millis last_movement_hint_at = -NEVER;
    bool overlay_dirty = true, overlay_reported = false, last_auto_wait = false;
    bool planned_wait_for_idle = false;
    std::size_t priority_rotation = 0;
    std::deque<int> player_history;
    DirectorState director;
    std::vector<VisitMemory> visits;
    Millis blocked_since = NEVER, clear_route_since = NEVER, next_recovery_attempt = 0;
    Millis navigation_progress_at = 0, recovery_deadline = NEVER, recovery_progress_at = 0;
    Millis recovery_pause_until = 0;
    int best_exit_distance = INF_DISTANCE, recovery_target = -1, recovery_best_distance = INF_DISTANCE;
    std::vector<int> recovery_guidance, recovery_occupancy;
    std::deque<std::pair<int, Millis>> tried_recovery_goals;
    std::vector<std::shared_ptr<LocalField>> local_pool;
    // Exit-rooted DFS tree: exit_cut_above[u] means exit_parent[u] separates u's
    // subtree from the exit, i.e. every route from there must pass that parent.
    std::vector<int> exit_parent;
    std::vector<unsigned char> exit_cut_above;
    std::vector<Millis> region_seen;
    int region_cols = 1;
    LocationHint latest_hint;
    std::deque<Millis> detour_history;
    int last_reported_boldness = 0;
    std::uint64_t graph_signature = 0;

    /// Iterative Tarjan rooted at the exit. Used to find route chokepoints for
    /// interceptors from an approximate area only; it never reads the player cell.
    void build_exit_tree() {
        const int n = static_cast<int>(grid.size());
        exit_parent.assign(n, -1); exit_cut_above.assign(n, 0);
        if (!walkable(exit_cell)) return;
        std::vector<int> disc(n, -1), low(n, 0), cursor(n, 0), stack{exit_cell};
        int sequence = 0;
        disc[exit_cell] = low[exit_cell] = sequence++;
        while (!stack.empty()) {
            int u = stack.back();
            if (cursor[u] < 4) {
                int v = adjacency[u][cursor[u]++];
                if (v < 0 || v == exit_parent[u]) continue;
                if (disc[v] < 0) {
                    exit_parent[v] = u; disc[v] = low[v] = sequence++; stack.push_back(v);
                } else low[u] = std::min(low[u], disc[v]);
            } else {
                stack.pop_back();
                int p = exit_parent[u];
                if (p < 0) continue;
                low[p] = std::min(low[p], low[u]);
                exit_cut_above[u] = p != exit_cell && low[u] >= disc[p];
            }
        }
    }

    /// Every route from `from` to the exit passes these cells. Each entry is the
    /// chokepoint and its exact corridor distance from `from`.
    std::vector<std::pair<int, int>> exit_chokepoints(int from, const LocationHint& area) const {
        std::vector<std::pair<int, int>> result;
        int previous = from, c = exit_parent[from];
        for (int steps = 0; c >= 0 && c != exit_cell && steps < CUTTER_SEARCH_STEPS; ++steps) {
            if (exit_cut_above[previous] && !area.contains(point(c))) {
                int ahead = (*exit_distances)[from] - (*exit_distances)[c];
                if (ahead >= 4) result.emplace_back(c, ahead);
            }
            previous = c; c = exit_parent[c];
        }
        return result;
    }

    /// Record that a monster has covered a coarse patrol region.
    void mark_region(int cell) {
        if (region_seen.empty() || !walkable(cell)) return;
        Point p = point(cell);
        region_seen[static_cast<std::size_t>((p.r / PATROL_REGION) * region_cols + p.c / PATROL_REGION)] = now;
    }
    Millis region_age(int cell) const {
        if (region_seen.empty()) return 0;
        Point p = point(cell);
        return now - region_seen[static_cast<std::size_t>((p.r / PATROL_REGION) * region_cols + p.c / PATROL_REGION)];
    }

public:
    /// Describe the chosen balance and reproduction inputs in the startup terminal.
    std::string configuration_summary() const {
        std::ostringstream text;
        text << MAZERUNNER_TITLE << " | Difficulty: " << settings.name << " | seed=" << seed
             << " | " << rows << 'x' << cols << " | Monsters: " << monsters.size()
             << "\nMonster interval: " << settings.monster_move_ms << " ms | Player interval: "
             << AUTO_SOLVE_SPEED_MS << " ms | Scent timeout: " << settings.scent_ms << " ms"
             << " | Search sweep: " << settings.search_ms << " ms"
             << "\nNormal hint: up to " << settings.hint_area_size << 'x' << settings.hint_area_size
             << " | Moving updates: ";
        if (settings.hint_interval_ms) text << "at most every " << settings.hint_interval_ms << " ms";
        else text << "every successful move";
        text << " | Hearing radius: " << hearing_radius() << " corridor steps | Interceptors: " << settings.cutters
             << " | Corridor sight: ";
        if (settings.sight_cells) text << settings.sight_cells << " cells"; else text << "off";
        text << "\nDirector radio: first successful step, then every " << settings.report_interval_ms
             << " ms on a successful step | Normal radio area: " << std::min(rows, radio_sector_size())
             << 'x' << std::min(cols, radio_sector_size()) << " | Remote monsters investigate without gaining scent.";
        text << "\nMonsters wait for the first move or auto-solve | Director confinement: "
             << settings.confinement_ms << " ms | Relief hold: " << settings.relief_ms
             << " ms | Minimum maze solutions: " << settings.minimum_solutions
             << " | Extra-connection budget: " << settings.loop_density_percent << "% of base";
        return text.str();
    }
    /// Expose director precision for diagnostics; edge clipping can reduce the area.
    int director_hint_width() const { return settings.hint_area_size + 2 * director.hint_padding; }
    const char* director_phase() const { return director.hint_padding ? "RELIEF" : "NORMAL"; }
    std::uint64_t director_reports() const { return director.reports; }
    /// Actual radio footprint on this map, including any current relief.
    std::string radio_area_label() const {
        int size = radio_sector_size() + 2 * director.hint_padding;
        return std::to_string(std::min(rows, size)) + "x" + std::to_string(std::min(cols, size));
    }
    /// Expose the committed detour endpoint, or -1 during ordinary exit planning.
    int auto_recovery_target() const { return recovery_target; }
    /// Walkable distance within which a monster hears a player step. Larger maps
    /// widen it so difficulty does not collapse as monster density falls.
    int hearing_radius() const {
        return settings.hearing_cells + settings.hearing_per_100 * std::max(rows, cols) / 100;
    }
    /// Expose solver escalation for diagnostics: recent detours raise it.
    int auto_boldness() const {
        int recent = 0;
        for (Millis at : detour_history) recent += now - at < AUTO_LOOP_WINDOW_MS;
        return std::min(AUTO_MAX_BOLDNESS, std::max(0, recent - 1));
    }
    /// Format one cell for terminal diagnostics without exposing it to monster AI.
    std::string cell_label(int cell) const {
        if (!walkable(cell)) return "none";
        Point p = point(cell);
        return "(" + std::to_string(p.r) + "," + std::to_string(p.c) + ")";
    }

    /// Append a complete timestamped line; avoid overwriting earlier state messages.
    void log_event(const std::string& message) const {
        if (!log_output) return;
        std::ostringstream line;
        line << '[' << std::fixed << std::setprecision(3) << now / 1000.0 << "s] " << message << '\n';
        *log_output << line.str();
    }

    /// Provide an explicit status snapshot for the terminal while preserving history.
    void log_status() const {
        std::ostringstream text;
        int hunting = 0, searching = 0;
        for (const auto& m : monsters) {
            hunting += m.state == MonsterState::Hunting;
            searching += m.state == MonsterState::Searching;
        }
        text << "Player " << cell_label(player) << " | Manual steps: " << manual_steps
             << " | Auto steps: " << automatic_steps << " | Time: " << std::fixed
             << std::setprecision(2) << elapsed_ms / 1000.0 << " s | Auto-solve: " << (auto_solve ? "ON" : "OFF")
             << " | Difficulty: " << settings.name
             << " | Director: " << director_phase() << " | Radio reports: " << director.reports
             << " | Radio area: " << radio_area_label()
             << " | Hint width: " << director_hint_width()
             << " | Hunting: " << hunting << " | Searching: " << searching;
        log_event(text.str());
    }

private:
    static std::string monster_name(const Monster& m) { return "Monster " + std::to_string(m.id + 1); }

    /// On a small board a normal radio sector still identifies part of the maze.
    int radio_sector_size() const {
        return std::min(settings.report_area_size, std::max(settings.hint_area_size, (2 * std::max(rows, cols) + 1) / 3));
    }

    /// Grow a remembered rectangle on the map without looking up the player.
    LocationHint widen_hint(LocationHint hint, int padding) const {
        if (!hint.revision || !padding) return hint;
        int height = std::min(rows, hint.max_row - hint.min_row + 1 + 2 * padding);
        int width = std::min(cols, hint.max_col - hint.min_col + 1 + 2 * padding);
        hint.min_row = std::clamp(hint.min_row - padding, 0, rows - height);
        hint.min_col = std::clamp(hint.min_col - padding, 0, cols - width);
        hint.max_row = hint.min_row + height - 1;
        hint.max_col = hint.min_col + width - 1;
        return hint;
    }

    /// Quantize a successful step into a fixed sector before applying relief.
    LocationHint make_location_hint(int cell, int sector, std::uint64_t revision) const {
        Point p = point(cell);
        int height = std::min(rows, sector), width = std::min(cols, sector);
        int row = std::min(p.r / sector * sector, rows - height);
        int col = std::min(p.c / sector * sector, cols - width);
        return widen_hint({row, row + height - 1, col, col + width - 1, revision}, director.hint_padding);
    }

    /// Give a monster the latest area clue and keep a still-valid target.
    void deliver_hint(Monster& m, const LocationHint& hint) {
        bool was_hunting = m.state == MonsterState::Hunting;
        if (!was_hunting) {
            log_event(monster_name(m) + ": Heard the player, switching " +
                      (m.state == MonsterState::Searching ? "SEARCHING" : "ROAMING") + " -> HUNTING.");
            m.state = MonsterState::Hunting;
            m.search_until = NEVER;
        }
        if (!was_hunting || m.hint.revision != hint.revision) {
            m.hint = hint;
            if (!was_hunting || (!m.sighted && m.role == MonsterRole::Chaser &&
                                 (!walkable(m.target) || !hint.contains(point(m.target))))) {
                m.target = -1; m.blocked_turns = 0;
            }
        }
    }

    /// Direct a remote patrol to investigate a sector. A report never grants
    /// hearing, renews scent, or displaces a live hunt or first-hand sighting.
    void investigate_report(Monster& m, const LocationHint& report) {
        const bool was_patrolling = m.state == MonsterState::Patrolling;
        if (was_patrolling)
            log_event(monster_name(m) + ": Director radio, switching ROAMING -> SEARCHING.");
        m.state = MonsterState::Searching;
        m.hint = report;
        m.search_until = now + settings.search_ms;
        if (was_patrolling || !walkable(m.target) || !report.contains(point(m.target))) {
            m.target = -1;
            m.blocked_turns = 0;
        }
    }

    /// A real displacement produces local sound and can also trigger an independent
    /// map-wide director report. Waiting, blocked input and clock ticks send neither.
    void publish_movement_hint() {
        bool first_movement = idle_deadline == NEVER;
        idle_deadline = now + settings.scent_ms;
        const int radius = hearing_radius();
        std::vector<std::size_t> listeners;
        std::shared_ptr<const LocalField> sound;
        for (std::size_t i = 0; i < monsters.size(); ++i) {
            const int cell = monsters[i].cell;
            if (manhattan(cell, player) > radius) continue;  // Corridor distance is at least this large.
            if (!sound) sound = local_field(player, radius);  // One bounded BFS for all potential listeners.
            if (sound->at(cell) <= radius) listeners.push_back(i);
        }
        // Scent tracks every audible displacement even when the director withholds
        // a location update. No queued report can fire after the player stops.
        const int local_interval = director.hint_padding
            ? std::max(settings.hint_interval_ms, 2 * DIRECTOR_TICK_MS) : settings.hint_interval_ms;
        bool publish = first_movement || !latest_hint.revision || now - last_movement_hint_at >= local_interval;
        const int radio_interval = settings.report_interval_ms * (director.hint_padding ? 2 : 1);
        bool radio_due = !monsters.empty() && now - director.last_report_at >= radio_interval;
        if (publish || radio_due) ++movement_revision;
        if (publish) {
            latest_hint = make_location_hint(player, settings.hint_area_size, movement_revision);
            last_movement_hint_at = now;
            if (!monsters.empty())
                log_event("Movement hint #" + std::to_string(latest_hint.revision) + " | Local footstep area: rows " +
                          std::to_string(latest_hint.min_row) + "-" + std::to_string(latest_hint.max_row) + ", cols " +
                          std::to_string(latest_hint.min_col) + "-" + std::to_string(latest_hint.max_col) +
                          " | Heard by " + std::to_string(listeners.size()) + " of " + std::to_string(monsters.size()));
        }
        for (std::size_t i : listeners) {
            Monster& m = monsters[i];
            m.scent_until = now + settings.scent_ms;
            deliver_hint(m, latest_hint);
        }
        if (radio_due) {
            director.last_report = make_location_hint(player, radio_sector_size(), movement_revision);
            const auto& report = director.last_report;
            int remote = 0;
            for (auto& m : monsters) {
                if (m.state == MonsterState::Hunting) continue;  // Fresh local evidence takes priority.
                investigate_report(m, report);
                ++remote;
            }
            director.last_report_at = now;
            ++director.reports;
            log_event("Director: REPORT " + std::to_string(director.reports) + " | " + director_phase() +
                      " | Estimated player area: rows " + std::to_string(report.min_row) + "-" +
                      std::to_string(report.max_row) + ", cols " + std::to_string(report.min_col) + "-" +
                      std::to_string(report.max_col) + " | Investigating by radio: " + std::to_string(remote) +
                      " | Using local evidence: " + std::to_string(monsters.size() - remote));
        }
        if (publish && !director.hint_padding) assign_interceptors(listeners);
    }

    /// Send up to settings.cutters hunters to chokepoints that every route from the
    /// clue area to the exit must pass. Uses the area and terrain only: the walk
    /// starts from the area cell nearest the exit, never from the player cell.
    void assign_interceptors(const std::vector<std::size_t>& listeners) {
        if (settings.cutters <= 0 || listeners.empty()) return;
        int from = -1;
        for (int r = latest_hint.min_row; r <= latest_hint.max_row; ++r)
            for (int c = latest_hint.min_col; c <= latest_hint.max_col; ++c) {
                int v = id({r, c});
                if (walkable(v) && (from < 0 || (*exit_distances)[v] < (*exit_distances)[from])) from = v;
            }
        if (from < 0) return;
        auto chokepoints = exit_chokepoints(from, latest_hint);
        std::vector<std::size_t> pool;
        std::vector<int> reserved_chokes;
        for (std::size_t i = 0; i < monsters.size(); ++i) {
            const Monster& m = monsters[i];
            if (m.sighted || m.state != MonsterState::Hunting) continue;
            if (std::find(listeners.begin(), listeners.end(), i) != listeners.end()) {
                pool.push_back(i);
            } else if (m.role == MonsterRole::Cutter) {
                // This hunter heard no new clue. Its assignment still consumes a slot.
                reserved_chokes.push_back(m.target);
            }
        }
        std::vector<std::size_t> cutters;
        for (const auto& choke : chokepoints) {
            if (reserved_chokes.size() >= static_cast<std::size_t>(settings.cutters)) break;
            if (is_reserved(reserved_chokes, choke.first)) continue;
            std::size_t best = monsters.size();
            int best_distance = THREAT_RADIUS + 1;
            for (std::size_t i : pool) {
                if (std::find(cutters.begin(), cutters.end(), i) != cutters.end()) continue;
                int d = local_field(monsters[i].cell)->at(choke.first);
                // Only a monster that can plausibly arrive first is worth sending.
                if (d > THREAT_RADIUS || Millis(d) * settings.monster_move_ms > Millis(choke.second) * CUTTER_PACE_MS) continue;
                if (d < best_distance) { best_distance = d; best = i; }
            }
            if (best == monsters.size()) continue;
            Monster& m = monsters[best];
            if (m.role != MonsterRole::Cutter || m.target != choke.first) {
                m.role = MonsterRole::Cutter; m.target = choke.first; m.blocked_turns = 0;
                m.role_until = now + CUTTER_CAMP_MS + Millis(best_distance) * settings.monster_move_ms;
                m.planned_revision = latest_hint.revision; m.next_replan = now + MONSTER_REPLAN_MS;
                log_event(monster_name(m) + " | INTERCEPTING at chokepoint " + cell_label(choke.first) +
                          " between the clue area and the exit");
            }
            cutters.push_back(best);
            reserved_chokes.push_back(choke.first);
        }
        // Hunters not chosen this time return to closing in on the area.
        for (std::size_t i : pool) if (monsters[i].role == MonsterRole::Cutter &&
                                       std::find(cutters.begin(), cutters.end(), i) == cutters.end()) {
            monsters[i].role = MonsterRole::Chaser; monsters[i].target = -1; monsters[i].role_until = NEVER;
        }
    }

    /// Widen each monster's own evidence, release guards and discard tighter
    /// waypoints. Precision recovery affects future reports only. Neither action
    /// supplies a fresh position or extends a scent or search deadline.
    void set_director_precision(int padding, const std::string& reason) {
        const int increase = padding - director.hint_padding;
        director.hint_padding = padding;
        int redirected = 0, released = 0;
        if (increase > 0) {
            latest_hint = widen_hint(latest_hint, increase);
            director.last_report = widen_hint(director.last_report, increase);
            for (auto& m : monsters) {
                if (m.role == MonsterRole::Cutter) {
                    m.role = MonsterRole::Chaser; m.role_until = NEVER;
                    m.target = -1; ++released;
                }
                if (!m.hint.revision || m.state == MonsterState::Patrolling) continue;
                m.hint = widen_hint(m.hint, increase);
                if (m.sighted && sees_player(m)) continue;  // Direct High-difficulty sight takes priority.
                m.sighted = false;
                m.target = -1; m.planned_revision = 0; m.blocked_turns = 0; m.next_replan = now;
                ++redirected;
            }
        }
        log_event("Director: " + reason + " | Radio area: " + radio_area_label() + " | Retasked: " + std::to_string(redirected) +
                  " | Interceptors released: " + std::to_string(released));
    }

    /// Sample confinement on a fixed simulation clock, independent of frame rate.
    /// The profile's confinement window starts relief; persistent confinement widens
    /// it again. A minimum hold and sustained escape prevent precision oscillation.
    /// Choosing to stand still is not confinement: it already sheds scent, so idle
    /// time is excluded from the window instead of being rewarded with wider clues.
    /// The solver is trying to escape even when a tactical wait has open terrain.
    void update_director() {
        if (director.next_update > now) return;
        director.next_update = now + DIRECTOR_TICK_MS;
        if (idle_deadline == NEVER && !auto_solve) {
            director.samples.clear();
            director.samples.push_back({now, player});
            director.recovered_since = NEVER;
            return;
        }
        director.samples.push_back({now, player});
        while (director.samples.size() > 1 && director.samples[1].at <= now - settings.confinement_ms)
            director.samples.pop_front();
        if (monsters.empty() || director.samples.empty()) return;
        const int padding_limit = settings.max_padding;
        Point low = point(player), high = low;
        std::vector<int> unique;
        for (const auto& sample : director.samples) {
            Point p = point(sample.cell);
            low.r = std::min(low.r, p.r); low.c = std::min(low.c, p.c);
            high.r = std::max(high.r, p.r); high.c = std::max(high.c, p.c);
            if (std::find(unique.begin(), unique.end(), sample.cell) == unique.end()) unique.push_back(sample.cell);
        }
        bool full_window = now - director.samples.front().at >= settings.confinement_ms;
        bool local_loop = unique.size() <= 12 && high.r - low.r <= 8 && high.c - low.c <= 8;
        // Longer repeated detours can span several corridors. Lack of novel cells
        // or exit progress catches those loops even when the bounding box is larger.
        bool repeated_detours = auto_solve && now - navigation_progress_at >= settings.confinement_ms;
        bool confined = full_window && (local_loop || repeated_detours);
        if (!director.hint_padding) {
            if (!confined || padding_limit == 0) return;
            director.relief_anchor = player;
            director.relief_exit_distance = (*exit_distances)[player];
            director.relief_until = now + settings.relief_ms;
            director.recovered_since = NEVER;
            set_director_precision(std::min(settings.padding_step, padding_limit),
                                   "RELIEF started; player confined, widening monster search areas");
            return;
        }
        bool escaped_area = (*exit_distances)[player] <= director.relief_exit_distance - 6 ||
                            manhattan(player, director.relief_anchor) >= 6;
        bool exit_route_open = !auto_solve || (!guide_distances.empty() && guide_distances[player] != INF_DISTANCE);
        if (!confined && escaped_area && exit_route_open) {
            if (director.recovered_since == NEVER) director.recovered_since = now;
        } else director.recovered_since = NEVER;
        if (now >= director.relief_until && director.recovered_since != NEVER &&
            now - director.recovered_since >= DIRECTOR_RECOVERY_MS) {
            set_director_precision(0, "NORMAL precision restored after sustained escape from confinement");
            director.relief_until = director.recovered_since = NEVER;
        } else if (now >= director.relief_until && confined) {
            director.relief_until = now + settings.relief_ms;
            if (director.hint_padding < padding_limit)
                set_director_precision(std::min(director.hint_padding + settings.padding_step, padding_limit),
                                       "RELIEF increased; confinement persists");
            // At maximum uncertainty, retain the broad clue without repeatedly
            // discarding targets that are already taking monsters away.
        }
    }

    /// Earliest deadline for a scent, a search sweep, or an interceptor assignment.
    Millis next_monster_state_change() const {
        Millis event = NEVER;
        for (const auto& m : monsters) {
            if (m.state == MonsterState::Hunting) event = std::min(event, m.scent_until);
            else if (m.state == MonsterState::Searching) event = std::min(event, m.search_until);
            if (m.role == MonsterRole::Cutter) event = std::min(event, m.role_until);
        }
        return event;
    }

    /// A monster that stops hearing the player sweeps the last clue area for the
    /// profile's search time before roaming again. Standing still is not immunity.
    void expire_monster_states() {
        if (idle_deadline != NEVER && now >= idle_deadline) {
            idle_deadline = NEVER;
            log_event("Player stopped moving for " + std::to_string(settings.scent_ms) + " ms | Scent fading.");
        }
        for (auto& m : monsters) {
            if (m.role == MonsterRole::Cutter && now >= m.role_until) {
                m.role = MonsterRole::Chaser;
                m.role_until = NEVER;
                m.target = -1;
            }
            if (m.state == MonsterState::Hunting && now >= m.scent_until) {
                const Millis last_contact = m.scent_until - settings.scent_ms;
                m.scent_until = NEVER; m.sighted = false;
                m.role = MonsterRole::Chaser; m.role_until = NEVER;
                m.target = -1; m.blocked_turns = 0; m.planned_revision = 0; m.next_replan = now;
                // A hunter also received the broadcast. Once its scent expires it
                // can use a newer, still-live report; this never restarts a hunt.
                const bool use_radio = director.last_report.revision && director.last_report_at > last_contact &&
                                       now - director.last_report_at < settings.search_ms;
                if (use_radio) m.hint = director.last_report;
                if (settings.search_ms > 0 && m.hint.revision != 0) {
                    m.state = MonsterState::Searching;
                    m.search_until = (use_radio ? director.last_report_at : now) + settings.search_ms;
                    log_event(monster_name(m) + ": Lost the scent, switching HUNTING -> SEARCHING " +
                              (use_radio ? "the last radio sector." : "the last clue area."));
                } else {
                    m.state = MonsterState::Patrolling; m.hint = {};
                    log_event(monster_name(m) + ": Player idle, switching HUNTING -> ROAMING.");
                }
            } else if (m.state == MonsterState::Searching && now >= m.search_until) {
                m.state = MonsterState::Patrolling; m.search_until = NEVER;
                m.hint = {}; m.target = -1; m.blocked_turns = 0; m.planned_revision = 0; m.next_replan = now;
                log_event(monster_name(m) + ": Search found nothing, switching SEARCHING -> ROAMING.");
            }
        }
    }

    /// High difficulty only: a monster sees the player down a straight open corridor.
    bool sees_player(const Monster& m) const { return line_of_sight(m.cell, player); }
    bool line_of_sight(int from, int to) const {
        if (settings.sight_cells <= 0 || !walkable(from) || !walkable(to)) return false;
        Point a = point(from), b = point(to);
        if (a.r != b.r && a.c != b.c) return false;
        int distance = std::abs(a.r - b.r) + std::abs(a.c - b.c);
        // Spotting works at a distance; at close quarters monsters behave normally,
        // otherwise every adjacent monster would become an instant capture.
        if (distance < SIGHT_MIN_CELLS || distance > settings.sight_cells) return false;
        int dr = (b.r > a.r) - (b.r < a.r), dc = (b.c > a.c) - (b.c < a.c);
        for (int k = 1; k < distance; ++k) if (!walkable(id({a.r + dr * k, a.c + dc * k}))) return false;
        return true;
    }

public:
    /// Advance all events chronologically. On ties monsters move before the player.
    /// Occupancy is checked after every move, so exchanges cannot bypass capture.
    /// Monsters stay frozen until the player's first move or auto-solve toggle.
    void advance_to(Millis destination) {
        if (destination < now) throw std::runtime_error("Simulation time cannot go backward.");
        verify_graph();
        while (outcome == Outcome::Playing) {
            Millis event = std::min(auto_solve ? next_auto : NEVER, idle_deadline);
            event = std::min(event, next_manual_repeat);
            event = std::min(event, director.next_update);
            if (timer_started) {
                event = std::min(event, next_monster_state_change());
                for (const auto& m : monsters) event = std::min(event, m.next_move);
            }
            if (event > destination || event == NEVER) break;
            now = event;
            expire_monster_states();
            update_director();
            update_monsters();
            if (outcome != Outcome::Playing) break;
            if (auto_solve && next_auto <= now) {
                auto_step();
                next_auto = auto_solve ? now + AUTO_SOLVE_SPEED_MS : NEVER;
            } else if (next_manual_repeat <= now) {
                const bool moved = move_player(held_direction.r, held_direction.c);
                next_manual_repeat = moved && outcome == Outcome::Playing ? now + MANUAL_HOLD_REPEAT_MS : NEVER;
            }
        }
        if (outcome == Outcome::Playing) now = destination;
        update_elapsed();
        refresh_solution_overlay();
    }

    /// Process only due monsters. Snapshot occupancy and rotating priority reserve
    /// destinations, prevent stacking/swaps, and avoid a fixed winner at bottlenecks.
    void update_monsters() {
        if (outcome != Outcome::Playing || monsters.empty() || !timer_started) return;
        bool any_due = false;
        for (const auto& m : monsters) if (m.next_move <= now) any_due = true;
        if (!any_due) return;
        std::vector<int> reserved;
        reserved.reserve(monsters.size() * 2);
        for (const auto& m : monsters) reserved.push_back(m.cell);
        const std::size_t n = monsters.size();
        for (std::size_t k = 0; k < n; ++k) {
            Monster& m = monsters[(priority_rotation + k) % n];
            if (m.next_move > now) continue;
            m.next_move = now + settings.monster_move_ms;
            // Sight is a discovery mechanic: a monster that is not already hunting
            // (or one mid-charge) can spot the player; hunters rely on area clues.
            if ((m.state != MonsterState::Hunting || m.sighted) && sees_player(m)) {
                if (!m.sighted)
                    log_event(monster_name(m) + ": SPOTTED the player down a corridor; charging.");
                if (m.state != MonsterState::Hunting) { m.state = MonsterState::Hunting; m.search_until = NEVER; }
                if (m.hint.revision == 0) m.hint = latest_hint;
                m.sighted = true; m.role = MonsterRole::Chaser; m.role_until = NEVER;
                m.scent_until = std::max(m.scent_until == NEVER ? Millis(0) : m.scent_until, now + settings.scent_ms);
                m.target = player; m.planned_revision = m.hint.revision; m.next_replan = now + MONSTER_REPLAN_MS;
            }
            // Movement hints survive across frames until this particular monster acts.
            if (m.target < 0 || m.planned_revision != m.hint.revision ||
                now >= m.next_replan || m.blocked_turns >= 2 ||
                m.cell == m.target) {
                int previous_target = m.target;
                m.target = choose_target(m);
                m.planned_revision = m.hint.revision;
                m.next_replan = now + MONSTER_REPLAN_MS;
                if (m.target != previous_target)
                    log_event(monster_name(m) + " | " +
                              (m.sighted ? "CHASING last seen " :
                               m.role == MonsterRole::Cutter ? "INTERCEPT point " :
                               m.state == MonsterState::Hunting ? "HUNTING area target " :
                               m.state == MonsterState::Searching ? "SEARCHING area target " : "ROAMING waypoint ") +
                              cell_label(m.target) + " | From " + cell_label(m.cell));
            }
            int next = choose_monster_step(m, reserved);
            if (next != m.cell) {
                if (verbose_logs)
                    log_event(monster_name(m) + ": " + cell_label(m.cell) + " -> " + cell_label(next));
                remember(m.recent_cells, m.cell, 12);
                m.cell = next; reserved.push_back(next); m.blocked_turns = 0;
                m.last_moved_at = now;
                mark_region(next);
                overlay_dirty = true;
                if (m.sighted && m.cell == m.target) m.sighted = false;
            } else if (m.cell != m.target) {
                ++m.blocked_turns;
                if (m.blocked_turns == 1 || verbose_logs)
                    log_event(monster_name(m) + ": Movement blocked at " + cell_label(m.cell));
            }
            check_outcome();
            if (outcome != Outcome::Playing) break;
        }
        priority_rotation = (priority_rotation + 1) % n;
        // advance_to() refreshes the overlay once per frame; doing it per monster
        // event rebuilt whole-grid guidance several times per player action.
    }

    /// Toggle assistance; include waiting time even when the initial route is blocked.
    void set_auto_solve(bool enabled) {
        if (outcome != Outcome::Playing) return;
        cancel_held_move();
        auto_solve = enabled;
        solution_overlay.clear();
        overlay_dirty = true; overlay_reported = false; last_auto_wait = false; planned_wait_for_idle = false;
        next_auto = enabled ? now + AUTO_SOLVE_SPEED_MS : NEVER;
        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();
        blocked_since = clear_route_since = NEVER;
        navigation_progress_at = now; next_recovery_attempt = now;
        recovery_pause_until = 0;
        if (enabled) start_timer();
        log_event(std::string("Auto-solve ") + (enabled ? "activated." : "deactivated."));
        refresh_solution_overlay();
    }

    /// Apply one cardinal manual move through the same rules as automatic movement.
    bool move_player(int dr, int dc) {
        if (auto_solve || std::abs(dr) + std::abs(dc) != 1) return false;
        verify_graph();
        Point p = point(player);
        solution_overlay.clear();
        return commit_player_move(id({p.r + dr, p.c + dc}), false);
    }

    /// Apply the first keypress immediately. Later repeats share the monster and auto-solver clock.
    bool begin_held_move(int dr, int dc) {
        cancel_held_move();
        if (!move_player(dr, dc) || outcome != Outcome::Playing) return false;
        held_direction = {dr, dc};
        next_manual_repeat = now + MANUAL_HOLD_DELAY_MS;
        return true;
    }

    /// Disarm the repeat when a key is released, a menu opens, or the input mode changes.
    void cancel_held_move() { next_manual_repeat = NEVER; }

    /// Resolve capture before victory, including occupied exits and manual movement.
    void check_outcome() {
        if (outcome != Outcome::Playing) return;
        for (const auto& m : monsters) if (m.cell == player) {
            outcome = Outcome::Caught;
            break;
        }
        if (outcome == Outcome::Playing && player == exit_cell) outcome = Outcome::Won;
        if (outcome != Outcome::Playing) {
            update_elapsed();
            cancel_held_move();
            auto_solve = false; next_auto = NEVER; solution_overlay.clear();
            idle_deadline = NEVER;
            director.next_update = NEVER;
            for (auto& m : monsters) { m.scent_until = m.search_until = NEVER; }
            log_event(std::string("Player ") + (outcome == Outcome::Caught ? "CAUGHT BY A MONSTER" : "REACHED THE EXIT") +
                      " at " + cell_label(player));
            log_status();
        }
    }

private:
    /// Maintain a bounded history without unbounded per-entity memory growth.
    static void remember(std::deque<int>& history, int cell, std::size_t limit) {
        history.push_back(cell);
        if (history.size() > limit) history.pop_front();
    }
    /// Start the gameplay clock once, regardless of which input mode starts play.
    void start_timer() {
        if (!timer_started) {
            timer_started = true; timer_start = now;
            // Release the frozen monsters with their spawn-relative staggering intact.
            for (auto& m : monsters) {
                if (m.next_move < NEVER) m.next_move += now;
                m.last_moved_at = now;
            }
            if (!monsters.empty()) log_event("Player started | Monsters released.");
            director.samples.push_back({now, player});
            director.next_update = now + DIRECTOR_TICK_MS;
            best_exit_distance = (*exit_distances)[player];
            navigation_progress_at = now;
            visits[player] = {now, 1.0};
        }
    }
    /// Sample elapsed simulation time before final state is frozen by advance_to.
    void update_elapsed() { if (timer_started) elapsed_ms = now - timer_start; }

    /// Decay revisit cost by elapsed time. Waiting does not add visits or erase
    /// the evidence of a loop, unlike a history truncated after a few actions.
    double visit_heat(int cell) const {
        const auto& visit = visits[cell];
        return visit.heat * std::max(0.0, 1.0 - double(now - visit.at) / AUTO_VISIT_MEMORY_MS);
    }

    /// Commit one move or wait and resolve collision/victory atomically. Only a
    /// successful displacement publishes a movement hint; waits do not prolong hunting.
    bool commit_player_move(int destination, bool automatic) {
        if (outcome != Outcome::Playing || !walkable(destination) ||
            manhattan(player, destination) > 1) {
            if (!automatic && outcome == Outcome::Playing)
                log_event("Player move blocked at " + cell_label(player));
            return false;
        }
        start_timer();
        if (destination != player) {
            int previous = player;
            player = destination;
            if (automatic) ++automatic_steps; else ++manual_steps;
            overlay_dirty = true;
            last_auto_wait = false;
            double heat = visit_heat(player);
            if (visits[player].heat == 0 || (*exit_distances)[player] < best_exit_distance) navigation_progress_at = now;
            best_exit_distance = std::min(best_exit_distance, (*exit_distances)[player]);
            visits[player] = {now, std::min(6.0, heat + 1.0)};
            log_event(std::string("Player ") + (automatic ? "AUTO" : "MANUAL") + ": " +
                      cell_label(previous) + " -> " + cell_label(player));
            publish_movement_hint();
        } else if (automatic && !last_auto_wait) {
            log_event("Auto-solve: WAITING at " + cell_label(player) + " for a safer move.");
            last_auto_wait = true;
        }
        remember(player_history, player, 32);
        update_elapsed();
        check_outcome();
        refresh_solution_overlay();
        return true;
    }

    /// Avoid concentrating every monster on one candidate inside the hinted area.
    bool target_claimed(const Monster& m, int cell) const {
        for (const auto& other : monsters)
            if (other.id != m.id && other.state == m.state && other.target == cell) return true;
        return false;
    }

    /// Hunt or search a reachable cell inside the approximate region, never an exact
    /// player coordinate (except a High-difficulty corridor sighting). Keep a valid
    /// target until reached to avoid zigzagging. Interceptors hold their chokepoint.
    /// Roaming prefers regions no monster has covered recently and avoids crowding.
    int choose_target(const Monster& m) {
        if (m.sighted && walkable(m.target) && m.target != m.cell) return m.target;
        if (m.role == MonsterRole::Cutter && walkable(m.target)) return m.target;
        bool area_mode = (m.state == MonsterState::Hunting || m.state == MonsterState::Searching) && m.hint.revision != 0;
        if (walkable(m.target) && m.target != m.cell && m.blocked_turns < 2 &&
            (!area_mode || m.hint.contains(point(m.target)))) return m.target;
        auto from_self = local_field(m.cell);
        if (area_mode) {
            const bool searching = m.state == MonsterState::Searching;
            std::vector<int> candidates;
            std::vector<double> weights;
            for (int r = m.hint.min_row; r <= m.hint.max_row; ++r) {
                for (int c = m.hint.min_col; c <= m.hint.max_col; ++c) {
                    int v = id({r, c});
                    if (!walkable(v) || v == m.cell) continue;
                    int distance = from_self->at(v);
                    int recent = static_cast<int>(std::count(m.recent_cells.begin(), m.recent_cells.end(), v));
                    // Hunters converge on nearby cells; searchers sweep, so distance matters less.
                    double weight = 1.0 / (1.0 + (searching ? 0.01 : 0.03) * distance + (searching ? 4.0 : 2.0) * recent);
                    if (target_claimed(m, v)) weight *= 0.15;
                    candidates.push_back(v); weights.push_back(weight);
                }
            }
            if (!candidates.empty()) {
                std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());
                return candidates[pick(rng)];
            }
            return m.cell;
        }
        int best = m.cell;
        double best_score = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < 32 && !open_cells.empty(); ++i) {
            int v = open_cells[rng() % open_cells.size()];
            if (v == m.cell) continue;
            int distance = from_self->at(v);
            int recent = static_cast<int>(std::count(m.recent_cells.begin(), m.recent_cells.end(), v));
            double staleness = double(std::min<Millis>(region_age(v), 20000)) / 1000.0;
            int crowd = 0;
            for (const auto& other : monsters)
                if (other.id != m.id && (manhattan(other.cell, v) <= 6 || (walkable(other.target) && manhattan(other.target, v) <= 6))) ++crowd;
            double score = std::min(distance, 20) * 0.3 + staleness * 0.8 - recent * 8
                         - (target_claimed(m, v) ? 8 : 0) - crowd * 4.0;
            if (score > best_score) { best_score = score; best = v; }
        }
        return best;
    }

    static bool is_reserved(const std::vector<int>& reserved, int cell) {
        return std::find(reserved.begin(), reserved.end(), cell) != reserved.end();
    }

    /// Follow a shared reverse-distance field while respecting reserved cells.
    /// If blocked, allow a temporary side/back step to make room at bottlenecks.
    int choose_monster_step(const Monster& m, const std::vector<int>& reserved) {
        if (!walkable(m.target) || m.cell == m.target) return m.cell;
        auto distance = distance_field(m.target);
        int best = m.cell;
        double best_score = (*distance)[m.cell];
        if (m.blocked_turns >= 1) best_score += 3.0;
        for (int v : adjacency[m.cell]) {
            if (v < 0 || is_reserved(reserved, v) || (*distance)[v] == INF_DISTANCE) continue;
            double score = (*distance)[v] + 0.15 * std::count(m.recent_cells.begin(), m.recent_cells.end(), v);
            if (score < best_score) { best_score = score; best = v; }
        }
        return best;
    }

    // Intent predictions are estimates: new movement hints/replanning can change them.
    struct Forecast {
        int horizon = AUTO_HORIZON;
        std::vector<std::vector<int>> positions;
        std::vector<std::shared_ptr<const LocalField>> distances;
        std::vector<Millis> first_move_delay;
    };

    /// Roll out cached intentions at actual deadlines using the same reservation
    /// rules. Also retain conservative reachability fields for uncertain behavior.
    /// Distances are bounded around each monster; far cells get a lower bound.
    Forecast forecast_monsters(int horizon = AUTO_HORIZON) {
        Forecast forecast;
        forecast.horizon = std::clamp(horizon, 1, AUTO_HORIZON_MAX);
        forecast.positions.resize(static_cast<std::size_t>(forecast.horizon) + 1);
        std::vector<Monster> predicted = monsters;
        for (const auto& m : monsters) {
            forecast.distances.push_back(local_field(m.cell));
            forecast.first_move_delay.push_back(std::max<Millis>(0, m.next_move - now));
            forecast.positions[0].push_back(m.cell);
        }
        std::size_t rotation = priority_rotation;
        std::vector<int> reserved;
        for (int depth = 1; depth <= forecast.horizon; ++depth) {
            Millis boundary = now + depth * AUTO_SOLVE_SPEED_MS;
            while (!predicted.empty()) {
                Millis event = NEVER;
                for (const auto& m : predicted) event = std::min(event, m.next_move);
                if (event > boundary) break;
                reserved.clear();
                for (const auto& m : predicted) reserved.push_back(m.cell);
                for (std::size_t k = 0; k < predicted.size(); ++k) {
                    auto& m = predicted[(rotation + k) % predicted.size()];
                    if (m.next_move != event) continue;
                    // Apply the same corridor-sight rule as the game: a monster that
                    // can see the player's current cell is predicted to charge it.
                    if ((m.state != MonsterState::Hunting || m.sighted) && line_of_sight(m.cell, player)) m.target = player;
                    int next = choose_monster_step(m, reserved);
                    if (next != m.cell) {
                        remember(m.recent_cells, m.cell, 12);
                        m.cell = next; reserved.push_back(next); m.blocked_turns = 0;
                    } else if (m.cell != m.target) ++m.blocked_turns;
                    m.next_move += settings.monster_move_ms;
                }
                rotation = (rotation + 1) % predicted.size();
            }
            for (const auto& m : predicted) forecast.positions[depth].push_back(m.cell);
        }
        return forecast;
    }

    /// Earliest possible arrival using corridor distance and the next due move.
    /// A monster can wait, so this bounds possibility, not capture probability.
    Millis earliest_arrival(const Forecast& forecast, std::size_t monster, int cell) const {
        int distance = forecast.distances[monster]->at(cell);
        if (distance == 0) return 0;
        return forecast.first_move_delay[monster] + (distance - 1) * settings.monster_move_ms;
    }

    /// Score time-indexed actions. For the first interval, prefer excluding all
    /// reachable threats; later intervals use soft risk to avoid total paralysis.
    bool evaluate_action(const Forecast& forecast, int cell, int depth, bool conservative,
                         double& risk) const {
        risk = 0;
        const Millis end = (depth + 1) * AUTO_SOLVE_SPEED_MS;
        for (std::size_t i = 0; i < monsters.size(); ++i) {
            // Arrival at the exit terminates play immediately after occupancy checks.
            if (forecast.positions[depth][i] == cell) return false;
            if (cell != exit_cell && forecast.positions[depth + 1][i] == cell) return false;
            Millis arrival = earliest_arrival(forecast, i, cell);
            if (depth == 0 && conservative && cell != exit_cell && arrival <= end) return false;
            Millis margin = arrival - (cell == exit_cell ? depth * AUTO_SOLVE_SPEED_MS : end);
            if (margin <= 0) risk += 5.0;
            else if (margin < settings.monster_move_ms) risk += 2.0 * (1.0 - double(margin) / settings.monster_move_ms);
            // A deep branch can look safe locally while its only mouth is cut off.
            // Compare the fastest return to that mouth with a monster's arrival.
            if (branch_gate[cell] >= 0 && cell != exit_cell) {
                Millis return_time = (depth + dead_end_depth[cell]) * AUTO_SOLVE_SPEED_MS;
                if (earliest_arrival(forecast, i, branch_gate[cell]) <= return_time + settings.monster_move_ms)
                    risk += 3.0 + std::min(dead_end_depth[cell], 12) * 0.2;
            }
        }
        return true;
    }

    // Each surviving node stores a parent in the bounded search tree for recovery.
    struct PlanNode {
        int cell, parent, depth;
        double cost, rank;
    };

    /// Guide the short temporal search around occupied bottlenecks. A monster is a
    /// wall only if the player could reach it before it is likely to move (near, or
    /// parked); a distant moving monster adds a soft detour cost instead, so it
    /// cannot freeze the whole plan from across the map.
    const std::vector<int>& route_guidance() {
        auto from_player = local_field(player);
        std::vector<int> hard, soft;
        for (const auto& m : monsters) {
            bool parked = now - m.last_moved_at >= AUTO_STATIONARY_MS;
            if (from_player->at(m.cell) <= AUTO_NEAR_THREAT_CELLS || parked) hard.push_back(m.cell);
            else soft.push_back(m.cell);
        }
        std::sort(hard.begin(), hard.end()); std::sort(soft.begin(), soft.end());
        std::vector<int> key = hard;
        key.push_back(-1);
        key.insert(key.end(), soft.begin(), soft.end());
        if (guide_distances.empty() || key != guide_occupancy) {
            overlay_dirty = true;
            guide_occupancy = std::move(key);
            const int n = static_cast<int>(grid.size());
            guide_distances.assign(n, INF_DISTANCE);
            guide_next.assign(n, -1);
            const auto is_hard = [&](int u) { return std::binary_search(hard.begin(), hard.end(), u); };
            const auto entry_cost = [&](int u) {
                return 1 + (std::binary_search(soft.begin(), soft.end(), u) ? AUTO_FAR_THREAT_COST : 0);
            };
            // Dial's algorithm: integer edge costs in [1, AUTO_FAR_THREAT_COST + 1].
            std::vector<std::vector<int>> buckets(AUTO_FAR_THREAT_COST + 2);
            std::size_t pending = 0;
            if (!is_hard(exit_cell)) {
                guide_distances[exit_cell] = 0; buckets[0].push_back(exit_cell); pending = 1;
            }
            for (int current = 0; pending > 0; ++current) {
                auto& bucket = buckets[static_cast<std::size_t>(current) % buckets.size()];
                while (!bucket.empty()) {
                    int u = bucket.back(); bucket.pop_back(); --pending;
                    if (guide_distances[u] != current) continue;
                    // Entering u costs entry_cost(u); its neighbors pay that to step onto u.
                    for (int v : adjacency[u]) if (v >= 0 && !is_hard(v)) {
                        int candidate = current + entry_cost(u);
                        if (candidate < guide_distances[v]) {
                            guide_distances[v] = candidate;
                            guide_next[v] = u;  // The cost of entering u is already in candidate.
                            buckets[static_cast<std::size_t>(candidate) % buckets.size()].push_back(v);
                            ++pending;
                        }
                    }
                }
            }
        }
        return guide_distances[player] == INF_DISTANCE ? *exit_distances : guide_distances;
    }

    /// On unit-cost terrain the first lower neighbor is a shortest step to the exit.
    int terrain_exit_step(int cell) const {
        for (int v : adjacency[cell])
            if (v >= 0 && (*exit_distances)[v] < (*exit_distances)[cell]) return v;
        return -1;
    }

    /// Construct a complete distance field around the current occupied cells.
    /// Recovery uses its own field so the highlighted route always targets the exit.
    std::vector<int> recovery_distance_field(int goal) const {
        std::vector<int> distance(grid.size(), INF_DISTANCE), queue, blocked;
        for (const auto& m : monsters) blocked.push_back(m.cell);
        if (walkable(goal) && !is_reserved(blocked, goal)) { distance[goal] = 0; queue.push_back(goal); }
        for (std::size_t head = 0; head < queue.size(); ++head) {
            int u = queue[head];
            for (int v : adjacency[u]) if (v >= 0 && distance[v] == INF_DISTANCE && !is_reserved(blocked, v)) {
                distance[v] = distance[u] + 1; queue.push_back(v);
            }
        }
        return distance;
    }

    /// Retire a waypoint and remember its neighborhood long enough to avoid
    /// alternating between the same two retreat destinations on successive plans.
    void finish_recovery(const std::string& reason) {
        if (recovery_target < 0) return;
        log_event("Auto-solve: Detour " + reason + " at " + cell_label(player) +
                  " | Previous waypoint " + cell_label(recovery_target));
        tried_recovery_goals.emplace_back(recovery_target, now + AUTO_VISIT_MEMORY_MS);
        if (tried_recovery_goals.size() > 16) tried_recovery_goals.pop_front();
        recovery_target = -1; recovery_guidance.clear(); recovery_occupancy.clear();
        next_recovery_attempt = now + 400;
    }

    /// Search reachable alternatives, including cells farther from the exit.
    /// Favor unvisited junctions and room to maneuver; penalize recently tried
    /// neighborhoods and blind branches. Work is bounded on maximum-sized mazes.
    /// Each recent detour raises boldness, which weights exit progress more.
    bool begin_recovery(const Forecast& forecast) {
        next_recovery_attempt = now + 400;
        while (!tried_recovery_goals.empty() && tried_recovery_goals.front().second <= now)
            tried_recovery_goals.pop_front();
        const double progress_weight = 1.5 + 1.0 * auto_boldness();
        std::vector<int> blocked;
        for (const auto& m : monsters) blocked.push_back(m.cell);
        std::vector<int> distance(grid.size(), INF_DISTANCE), queue{player};
        distance[player] = 0;
        int best = -1;
        double best_score = -std::numeric_limits<double>::infinity();
        for (std::size_t head = 0; head < queue.size() && head < 4096; ++head) {
            int u = queue[head], d = distance[u];
            if (d > 0) {
                Millis clearance = NEVER;
                for (std::size_t i = 0; i < monsters.size(); ++i)
                    clearance = std::min(clearance, earliest_arrival(forecast, i, u));
                // A static occupancy gap alone is insufficient if a monster can
                // close it before arrival. The next action is checked separately.
                if (d >= 4 && clearance > d * AUTO_SOLVE_SPEED_MS + settings.monster_move_ms) {
                    double score = 2.0 * std::min(d, 16) + double(std::min<Millis>(clearance, 20 * settings.monster_move_ms)) / settings.monster_move_ms
                                   - 4.0 * visit_heat(u) - 0.6 * dead_end_depth[u]
                                   - progress_weight * ((*exit_distances)[u] - (*exit_distances)[player]);
                    if (degrees[u] >= 3) score += 6.0;
                    if (u == entrance) score -= 12.0;
                    for (const auto& tried : tried_recovery_goals)
                        if (manhattan(u, tried.first) <= 3) score -= 24.0;
                    if (score > best_score) { best_score = score; best = u; }
                }
            }
            if (d >= 32) continue;
            for (int v : adjacency[u]) if (v >= 0 && distance[v] == INF_DISTANCE && !is_reserved(blocked, v)) {
                if (d == 0) {
                    double risk;
                    if (!evaluate_action(forecast, v, 0, true, risk)) continue;
                }
                distance[v] = d + 1; queue.push_back(v);
            }
        }
        if (best < 0) return false;
        recovery_target = best;
        recovery_guidance = recovery_distance_field(best);
        recovery_occupancy.clear();
        for (const auto& m : monsters) recovery_occupancy.push_back(m.cell);
        recovery_best_distance = recovery_guidance[player];
        recovery_progress_at = now; recovery_deadline = now + AUTO_RECOVERY_COMMIT_MS;
        detour_history.push_back(now);
        while (!detour_history.empty() && now - detour_history.front() >= AUTO_LOOP_WINDOW_MS) detour_history.pop_front();
        log_event(std::string("Auto-solve: ") +
                  ((*exit_distances)[best] > (*exit_distances)[player] ? "BACKTRACK" : "DETOUR") +
                  " committed toward " + cell_label(best) + " | " +
                  std::to_string(recovery_best_distance) + " steps | Rechecking safety every action.");
        return true;
    }

    /// Keep a chosen escape goal across actions. Rebuild only when occupancy
    /// changes; retire goals that are reached, cut off, or no longer making progress.
    void update_recovery() {
        if (recovery_target < 0) return;
        if (player == recovery_target) {
            finish_recovery("reached");
            // A retreat should create a chance to lose the scent, rather than
            // immediately reversing and continuously re-alerting every monster.
            recovery_pause_until = now + settings.scent_ms + settings.monster_move_ms;
            next_recovery_attempt = recovery_pause_until;
            return;
        }
        std::vector<int> occupancy;
        for (const auto& m : monsters) occupancy.push_back(m.cell);
        if (occupancy != recovery_occupancy) {
            recovery_occupancy = occupancy;
            recovery_guidance = recovery_distance_field(recovery_target);
        }
        if (recovery_guidance[player] == INF_DISTANCE) { finish_recovery("cut off"); return; }
        if (recovery_guidance[player] < recovery_best_distance) {
            recovery_best_distance = recovery_guidance[player]; recovery_progress_at = now;
        }
        if (now >= recovery_deadline || now - recovery_progress_at >= (2 * settings.scent_ms))
            finish_recovery("timed out; selecting another branch");
    }

    /// Follow the committed global detour one verified action at a time. A safe
    /// retreat may temporarily increase exit distance. If no step toward the goal
    /// is safe, the temporal planner remains free to wait or evade nearby threats.
    int recovery_step(const Forecast& forecast) const {
        int best = -1;
        double best_score = std::numeric_limits<double>::infinity();
        for (int v : adjacency[player]) {
            if (v < 0 || recovery_guidance[v] >= recovery_guidance[player]) continue;
            double risk;
            if (!evaluate_action(forecast, v, 0, true, risk)) continue;
            double score = risk + 0.2 * visit_heat(v);
            if (score < best_score) { best_score = score; best = v; }
        }
        return best;
    }

    /// Search (cell, action time) with a fixed beam and depth budget. Keep the
    /// deepest survivable frontier, then prefer lower risk, fewer repeats, and exit
    /// progress. This bounded search is deliberately not an optimality guarantee.
    std::vector<int> beam_plan(const Forecast& forecast, bool conservative, const std::vector<int>& guidance) {
        std::vector<PlanNode> tree{{player, -1, 0, 0, double(guidance[player])}};
        std::vector<int> frontier{0};
        int chosen = -1;
        const auto better = [](const PlanNode& a, const PlanNode& b) {
            if (a.rank != b.rank) return a.rank < b.rank;
            if (a.cell != b.cell) return a.cell < b.cell;
            return a.parent < b.parent;
        };
        for (int depth = 0; depth < forecast.horizon; ++depth) {
            if (++beam_epoch == 0) {
                std::fill(beam_seen.begin(), beam_seen.end(), 0); ++beam_epoch;
            }
            std::vector<PlanNode> candidates;
            candidates.reserve(frontier.size() * 5);
            for (int parent : frontier) {
                const PlanNode& previous = tree[parent];
                std::array<int, 5> moves{previous.cell, -1, -1, -1, -1};
                std::copy(adjacency[previous.cell].begin(), adjacency[previous.cell].end(), moves.begin() + 1);
                for (int v : moves) {
                    if (v < 0 || guidance[v] == INF_DISTANCE) continue;
                    double danger;
                    if (!evaluate_action(forecast, v, depth, conservative, danger)) continue;
                    int repeats = static_cast<int>(std::count(player_history.begin(), player_history.end(), v));
                    double cost = previous.cost + 1.0 + 2.0 * danger + 0.1 * repeats + 0.5 * visit_heat(v);
                    if (v == previous.cell) cost += 0.35;
                    if (previous.parent >= 0 && tree[previous.parent].cell == v) cost += 0.8;
                    // Penalize entering deep branches only while a threat is nearby.
                    if (danger > 0) cost += std::min(dead_end_depth[v], 12) * 0.2;
                    // Prefer a junction over a blind branch when danger is comparable.
                    if (danger > 0 && degrees[v] < 2 && v != exit_cell) cost += 2.0;
                    PlanNode candidate{v, parent, depth + 1, cost, cost + guidance[v]};
                    if (beam_seen[v] == beam_epoch) {
                        auto& old = candidates[beam_index[v]];
                        if (better(candidate, old)) old = candidate;
                    } else {
                        beam_seen[v] = beam_epoch; beam_index[v] = static_cast<int>(candidates.size());
                        candidates.push_back(candidate);
                    }
                }
            }
            if (candidates.empty()) break;
            // Preserve a feasible goal even if many low-cost partial paths fill the beam.
            auto goal = std::find_if(candidates.begin(), candidates.end(), [&](const PlanNode& n) {
                return n.cell == exit_cell;
            });
            if (goal != candidates.end()) {
                chosen = static_cast<int>(tree.size()); tree.push_back(*goal); break;
            }
            int keep = std::min(AUTO_BEAM_WIDTH, static_cast<int>(candidates.size()));
            std::partial_sort(candidates.begin(), candidates.begin() + keep, candidates.end(), better);
            frontier.clear();
            for (int i = 0; i < keep; ++i) {
                frontier.push_back(static_cast<int>(tree.size())); tree.push_back(candidates[i]);
            }
            chosen = frontier.front();
        }
        std::vector<int> result;
        for (int node = chosen; node >= 0; node = tree[node].parent) result.push_back(tree[node].cell);
        std::reverse(result.begin(), result.end());
        return result;
    }

    /// Look further ahead while a threat is at mid range: close enough to shape the
    /// route, far enough that the default window cannot see how it will arrive.
    int choose_horizon() {
        auto from_player = local_field(player);
        int nearest = THREAT_RADIUS + 1;
        for (const auto& m : monsters) nearest = std::min(nearest, from_player->at(m.cell));
        return nearest >= 10 && nearest <= 36 ? AUTO_HORIZON_MAX : AUTO_HORIZON;
    }

public:
    /// Show the complete current-to-exit route independently of tactical lookahead.
    /// Trace the chosen weighted successors. Amber indicates an occupied cell on
    /// the displayed route. A fully blocked field falls back to the terrain route.
    void refresh_solution_overlay() {
        if (!auto_solve || outcome != Outcome::Playing || !overlay_dirty) return;
        const auto& guidance = route_guidance();
        overlay_dirty = false;
        std::vector<Point> route;
        const bool guided = guide_distances[player] != INF_DISTANCE;
        bool avoids_monsters = guided;
        int current = player;
        if (guidance[current] != INF_DISTANCE) {
            route.push_back(point(current));
            while (current != exit_cell) {
                int next = guided && !monsters.empty() ? guide_next[current] : terrain_exit_step(current);
                if (next < 0) { route.clear(); break; }
                if (avoids_monsters)
                    for (const auto& m : monsters) if (m.cell == next) avoids_monsters = false;
                route.push_back(point(next)); current = next;
            }
        }
        if (route.empty()) avoids_monsters = false;
        bool changed = route != solution_overlay || avoids_monsters != overlay_avoids_monsters;
        if (!overlay_reported || avoids_monsters != overlay_avoids_monsters || (verbose_logs && changed)) {
            if (route.empty()) log_event("Auto-solve: No terrain route to the exit.");
            else if (avoids_monsters)
                log_event("Auto-solve: Full path to exit updated | " + std::to_string(route.size() - 1) +
                          " steps | Current monster cells avoided.");
            else
                log_event("Auto-solve: Path crosses monster(s) | Full route shown in AMBER | Evasion active.");
        }
        solution_overlay = std::move(route);
        overlay_avoids_monsters = avoids_monsters;
        overlay_reported = true;
    }

    /// Plan a short safe route, including waits. If conservative avoidance has no
    /// action, relax uncertain threats; if every prediction fails, maximize time
    /// before contact among currently unoccupied neighbors. No survival guarantee.
    std::vector<int> plan_auto_route() {
        planned_wait_for_idle = false;
        if (monsters.empty()) {
            // Without threats, a reverse BFS gradient gives exact shortest movement.
            std::vector<int> route{player};
            int u = player;
            for (int step = 0; step < AUTO_HORIZON && u != exit_cell; ++step) {
                int best = terrain_exit_step(u);
                if (best < 0) break;
                route.push_back(best); u = best;
            }
            return route;
        }
        // Loop detection: several detours in a short window mean waiting is not
        // working, so wait less (and begin_recovery favors exit progress more).
        // An all-out "press on" mode was measured and rejected: it turned
        // timeouts into captures rather than escapes.
        const int boldness = auto_boldness();
        if (boldness > 0 && boldness != last_reported_boldness)
            log_event("Auto-solve: " + std::to_string(boldness + 1) + " detours in " +
                      std::to_string(AUTO_LOOP_WINDOW_MS / 1000) + " s | Boldness " + std::to_string(boldness) +
                      " | Shorter waits, detours favor exit progress.");
        last_reported_boldness = boldness;
        const Millis wait_budget = std::max<Millis>(settings.scent_ms,
                                                    2 * settings.scent_ms - boldness * settings.scent_ms / 2);
        Forecast forecast = forecast_monsters(choose_horizon());
        const auto& guidance = route_guidance();
        bool blocked = guide_distances[player] == INF_DISTANCE;
        if (blocked) {
            clear_route_since = NEVER;
            if (blocked_since == NEVER) blocked_since = now;
        } else {
            if (clear_route_since == NEVER) clear_route_since = now;
            // Brief gaps at a bottleneck do not repeatedly grant a fresh wait budget.
            if (now - clear_route_since >= settings.scent_ms) blocked_since = NEVER;
        }
        update_recovery();
        if (recovery_target >= 0 && !blocked && now - clear_route_since >= DIRECTOR_TICK_MS) {
            for (int v : adjacency[player]) {
                if (v < 0 || guidance[v] >= guidance[player]) continue;
                double risk;
                if (evaluate_action(forecast, v, 0, true, risk) && risk < 1.0) {
                    finish_recovery("cancelled; exit route reopened");
                    break;
                }
            }
        }
        // Stand still long enough to shed the movement scent, but never restart
        // this allowance on each auto tick. Imminent danger still overrides waiting.
        // Never idle for scent loss while a monster is within one step of arrival.
        Millis nearest_arrival = NEVER;
        for (std::size_t i = 0; i < monsters.size(); ++i)
            nearest_arrival = std::min(nearest_arrival, earliest_arrival(forecast, i, player));
        if (blocked && recovery_target < 0 &&
            nearest_arrival > AUTO_SOLVE_SPEED_MS + settings.monster_move_ms &&
            (now - blocked_since < wait_budget || now < recovery_pause_until)) {
            double risk = 0;
            if (evaluate_action(forecast, player, 0, true, risk)) {
                planned_wait_for_idle = true;
                return {player, player};
            }
        }
        bool stalled = now - navigation_progress_at >= AUTO_STALL_MS;
        if (recovery_target < 0 && now >= next_recovery_attempt &&
            ((blocked && now - blocked_since >= wait_budget) || stalled))
            begin_recovery(forecast);
        if (recovery_target >= 0) {
            int next = recovery_step(forecast);
            if (next >= 0) return {player, next};
        }
        const auto& tactical_guidance = recovery_target >= 0 ? recovery_guidance : guidance;
        auto route = beam_plan(forecast, true, tactical_guidance);
        if (route.size() > 1) return route;
        route = beam_plan(forecast, false, tactical_guidance);
        if (route.size() > 1) return route;
        int best = player;
        double best_score = -std::numeric_limits<double>::infinity();
        std::array<int, 5> candidates{player, -1, -1, -1, -1};
        std::copy(adjacency[player].begin(), adjacency[player].end(), candidates.begin() + 1);
        for (int v : candidates) {
            if (v < 0) continue;
            bool occupied = false;
            Millis margin = NEVER;
            for (std::size_t i = 0; i < monsters.size(); ++i) {
                if (monsters[i].cell == v) occupied = true;
                margin = std::min(margin, earliest_arrival(forecast, i, v));
            }
            if (occupied) continue;
            double score = std::min<Millis>(margin, 2000) - (*exit_distances)[v] * 0.3 + degrees[v] * 2;
            if (v == exit_cell) score += 10000;
            if (score > best_score) { best_score = score; best = v; }
        }
        return {player, best};
    }

    /// Execute one tactical action, then independently refresh the full exit route.
    void auto_step() {
        if (!auto_solve || outcome != Outcome::Playing) return;
        auto route = plan_auto_route();
        if (planned_wait_for_idle && !last_auto_wait)
            log_event("Auto-solve: All exit routes blocked; standing still so monsters lose the movement hint. Bounded wait; then try a detour or backtrack.");
        if (route.size() < 2) { commit_player_move(player, true); return; }
        int next = route[1];
        if (next != player && !solution_overlay.empty() && solution_overlay.size() > 1 &&
            point(next) != solution_overlay[1]) {
            log_event("Auto-solve: EVADING toward " + cell_label(next) + "; replanning full exit path.");
        }
        commit_player_move(next, true);
    }
};

/// Describe the current result consistently for graphical and headless sessions.
const char* outcome_name(Outcome outcome) {
    switch (outcome) {
        case Outcome::Won: return "ESCAPED";
        case Outcome::Caught: return "CAUGHT";
        default: return "RUNNING";
    }
}

/// Parse a nonnegative integer completely, rejecting signs and trailing characters.
std::uint64_t parse_number(const std::string& text, std::uint64_t maximum) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected an unsigned integer: " + text);
    std::size_t consumed = 0;
    unsigned long long value = 0;
    try { value = std::stoull(text, &consumed); }
    catch (const std::out_of_range&) { throw std::runtime_error("Number out of range: " + text); }
    if (consumed != text.size() || value > maximum) throw std::runtime_error("Number out of range: " + text);
    return value;
}

/// Run the exact game simulation without graphics for reproducible AI experiments.
int run_simulation(int argc, char** argv) {
    if (argc < 6)
        throw std::runtime_error("Usage: --simulate ROWS COLS MONSTERS SEED [LIMIT_MS] [--difficulty low|medium|high] [--quiet] [--verbose]");
    int rows = static_cast<int>(parse_number(argv[2], MAX_MAZE_DIMENSION));
    int cols = static_cast<int>(parse_number(argv[3], MAX_MAZE_DIMENSION));
    int count = static_cast<int>(parse_number(argv[4], MAX_MONSTERS));
    auto seed = static_cast<std::uint32_t>(parse_number(argv[5], std::numeric_limits<std::uint32_t>::max()));
    Millis limit = 120000;
    Difficulty difficulty = Difficulty::Medium;
    bool quiet = false, verbose = false, limit_set = false;
    for (int i = 6; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") quiet = true;
        else if (arg == "--verbose") verbose = true;
        else if (arg == "--difficulty") {
            if (i + 1 >= argc) throw std::runtime_error("--difficulty requires low, medium, or high.");
            difficulty = parse_difficulty(argv[++i]);
        } else if (!limit_set && arg.find_first_not_of("0123456789") == std::string::npos) {
            limit = static_cast<Millis>(parse_number(arg, 3600000)); limit_set = true;
        }
        else throw std::runtime_error("Unrecognized simulation argument: " + arg);
    }
    Maze maze(rows, cols, seed, quiet ? nullptr : &std::cout, difficulty);
    maze.verbose_logs = verbose;
    maze.generate(count);
    if (!quiet) std::cout << maze.configuration_summary() << '\n';
    maze.set_auto_solve(true);
    auto start = std::chrono::steady_clock::now();
    maze.advance_to(limit);
    double compute_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << MAZERUNNER_TITLE " | difficulty=" << maze.settings.name << " | seed=" << seed << " | " << maze.rows << 'x' << maze.cols
              << " | monsters=" << maze.monsters.size() << " | " << outcome_name(maze.outcome)
              << " | simulation_ms=" << maze.elapsed_ms << " | auto_steps=" << maze.automatic_steps
              << " | compute_ms=" << std::fixed << std::setprecision(2) << compute_ms << '\n';
    // A caught player is a valid experiment result, not a process failure.
    return 0;
}

// Presentation geometry uses world coordinates (column, row, height). It never
// consumes the simulation RNG or changes gameplay state.
struct Vertex { float x, y, z, r, g, b; };
struct RGB { float r, g, b; };
struct Vec3 { float x, y, z; };
constexpr float WALL_HEIGHT = 0.38f;
constexpr float VIEW_COS = 0.906307787f, VIEW_SIN = 0.422618262f;
using PixelSprite = std::array<std::string_view, 20>;

// Original twenty-pixel silhouettes are embedded so no texture files are needed.
constexpr PixelSprite KNIGHT_SPRITE{{
    "........RRRR........", ".......RRrrRR.......",
    "........SSSS........", ".......SLLLLS.......",
    "......SLLLLLLS......", "......SLKKKKLS......",
    "......SLKYYKLS......", ".......SLLLLS.......",
    "...A....SSSS....L...", "..AABBBSLLLS....L...",
    "..ABBAASLLLS...LLL..", "..ABBBASTTTS....L...",
    "..ABBAASBBBS...AAA..", "...AA..SBBBS....S...",
    ".......SBBBS........", ".......SSSSS........",
    ".......SL.SL........", ".......SL.SL........",
    "......SSS.SSS.......", "......KKK.KKK......."
}};
constexpr PixelSprite GOBLIN_SPRITE{{
    "....................", "...G............G...",
    "...GG..........GG...", "...gGG..GGGG..GGg...",
    "....gGGGGGGGGGGg....", ".....GGGGGGGGGG.....",
    ".....GEEGGGGEEG.....", ".....GKKGGGGKKG.....",
    "......GGWWWWGG......", "......gGKKKKGg......",
    "....GGggGGGGggGG....", "...GGGTTTTTTTTGGG...",
    "...gGGTTTTTTTTGGg...", "....ggTTAAAATTgg....",
    "......TTTTTTTT......", "......gggggggg......",
    "......GGG..GGG......", "......gGG..GGg......",
    ".....WWWW..WWWW.....", "...................."
}};
constexpr PixelSprite DEMON_SPRITE{{
    "....W..........W....", "...WW..........WW...",
    "...WPP........PPW...", "...pPPPPPPPPPPPPp...",
    "....PPPPPPPPPPPP....", ".....PEEPPPPEEP.....",
    ".....PKKPPPPKKP.....", "......PPWWWWPP......",
    "......PPKRRKPP......", "....PPppWWWWppPP....",
    "...PPPppPPPPppPPP...", "..pPPPppPPPPppPPPp..",
    "..pPP.ppPPPPpp.PPp..", "...WW.ppPPPPpp.WW...",
    "......pppppppp......", "......PPP..PPP......",
    "......pPP..PPp......", ".....WWW....WWW.....",
    "....................", "...................."
}};
constexpr PixelSprite WRAITH_SPRITE{{
    "....................", "........CCCC........",
    ".......CCCCCC.......", "......CCLLLLCC......",
    ".....CCLLLLLLCC.....", ".....CCEECCEECC.....",
    ".....CCKKCCKKCC.....", "......CCCCCCCC......",
    "......CCCKKCCC......", "....CCCCCKKCCCCC....",
    "...CCCDCCCCCCDCCC...", "...CCDCCCCCCCCDCC...",
    "....CDCCCCCCCCDC....", ".....DCCCCCCCCD.....",
    ".....DCCCCCCCCD.....", ".....DCCC..CCCD.....",
    "......CC....CC......", "......C......C......",
    "....................", "...................."
}};
constexpr PixelSprite SKELETON_SPRITE{{
    "....................", ".......WWWWWW.......",
    "......WLLLLLLW......", ".....WLLLLLLLLW.....",
    "....WLKEELLEEKLW....", "....WLKKKLLKKKLW....",
    ".....WLLLLLLLLW.....", ".....WWLKLLKLWW.....",
    "......WLWLWLW.......", ".........WW.........",
    "....WW..LWWL..WW....", "...WW..WWWWWW..WW...",
    "....W...LWWL...W....", "...WW..WWWWWW..WW...",
    "....W...LWWL...W....", ".......WWWWWW.......",
    ".......WL..LW.......", ".......WL..LW.......",
    ".......WL..LW.......", ".....WWWW..WWWW....."
}};
constexpr PixelSprite SLIME_SPRITE{{
    "....................", "....................",
    "....................", "....................",
    "....................", "....................",
    ".......GGGGGG.......", ".....GGLLLGGGGG.....",
    "....GLLLLGGGGGGG....", "...GGLLGGGGGGGGGG...",
    "...GGGGGGGGGGGGGG...", "..GGGEEGGGGEEGGGGG..",
    "..GGGKKGGGGKKGGGGG..", ".GGGGGGGGGGGGGGGGGG.",
    ".GGGGGGGKKGGGGGGGGG.", ".gGGGGGGGGGGGGGGGGg.",
    ".ggGGGGGGGGGGGGGGgg.", ".gggGGGGGGGGGGGGggg.",
    "..gggggggggggggggg..", ".gggggggggggggggggg."
}};
constexpr PixelSprite BAT_SPRITE{{
    "....................", "....................",
    "...P............P...", "...PP..........PP...",
    "P.PP..PP....PP..PP.P", "PP.PPPPP....PPPPP.PP",
    "PPP.PPPPPPPPPPPP.PPP", "PPPPPppPEEPPppPPPPP.",
    "PPPPpppPKKPPpppPPPP.", "PPPppppPPWWPppppPPP.",
    "PPpppppPPPPppppppPP.", "PppppppPPPPpppppppP.",
    "Ppp.PppPPPPppP.pppP.", ".P....PpPPPPpP....P.",
    ".....P..pppp..P.....", "........pppp........",
    "........pppp........", "........W..W........",
    "....................", "...................."
}};
constexpr PixelSprite SPIDER_SPRITE{{
    "....................", "...A............A...",
    "...AA..........AA...", "....AA..rrrr..AA....",
    ".....AArrrrrrAA.....", ".A....rrRRRRrr....A.",
    ".AA...rRRRRRRr...AA.", "..AAA.rRRRRRRr.AAA..",
    "....AArrRRRRrrAA....", "......rrrrrrrr......",
    "...AAAArREErRAAAA...", "..AA..rREEEEr..AA...",
    "..AA...rRKKRr...AA..", "..A....rrrrrr....A..",
    "....AAT..TT..TAA....", "....AA...TT...AA....",
    "....AA........AA....", "....A..........A....",
    "....................", "...................."
}};
constexpr PixelSprite GOLEM_SPRITE{{
    "....................", "......SSSSSSSS......",
    "......SLLLLLLS......", "......SSESSESS......",
    "......SSESSESS......", "......SSSSSSSS......",
    "......SSSLLSSS......", "..SSSSSSSSSSSSSSSS..",
    "..SLLLSSLLLLSSLLLS..", "..SLLLSSSCCSSSLLLS..",
    "..SLLLSSCCCCSSLLLS..", "..SSSSSSSCCSSSSSSS..",
    "..SSSSSSLLLLSSSSSS..", "..SSSSSSSSSSSSSSSS..",
    ".SSSS..SSSSSS..SSSS.", ".....SSSSSSSSSS.....",
    ".....SLLS..SLLS.....", ".....SLLS..SLLS.....",
    ".....SSSS..SSSS.....", "...SSSSSS..SSSSSS..."
}};
constexpr PixelSprite EVIL_EYE_SPRITE{{
    "....................", "......pppppppp......",
    ".....ppWWWWWWpp.....", "....pWWRWWWWRWp.....",
    "....pWWWWWWWWWWp....", "...pWRWWEEEEWWRWp...",
    "...pWWWEEEEEEWWWp...", "...pWWWEEKKKKWWWp...",
    "...pWWWEEKKKKWWWp...", "...pWRWWEEEEWWRWp...",
    "....pWWWWWWWWWWp....", "....pWWRWWWWRWp.....",
    ".....ppWWWWWWpp.....", "......pppppppp......",
    ".....P..P..P..P.....", ".....P..P..P..P.....",
    "....P...P..P...P....", "...P....P..P....P...",
    "...PP...P..P...PP...", ".......PP..PP......."
}};
constexpr PixelSprite MUSHROOM_SPRITE{{
    "....................", ".......RRRRRR.......",
    ".....RRRWWRRRRR.....", "....RRRWWRRRRRRR....",
    "...RRRRRRRRRWWRRR...", "..RRRWWRRRRWWRRRRR..",
    "..RRRWWRRRRRRRRRRR..", ".rRRRRRRRWWRRRRRRRr.",
    ".rrRRRRRRWWRRRRRRrr.", ".rrrrrrrrrrrrrrrrrr.",
    ".....TTWWWWWWTT.....", "......WWWWWWWW......",
    "......WEEWWEEW......", "......WKKWWKKW......",
    "......WWWWWWWW......", "......WWKWWKWW......",
    ".......WWWWWW.......", "......TWWWWWWT......",
    ".....TTWWWWWWTT.....", "....TTTTTTTTTTTT...."
}};
constexpr PixelSprite EXIT_SPRITE{{
    ".......AAAAAA.......", ".....AAGGGGGGAA.....",
    "....AGggggggggGA....", "...AGgg......ggGA...",
    "...AGg........gGA...", "..AGg..........gGA..",
    "..AGg..........gGA..", "..AGg..........gGA..",
    "..AGg....GG....gGA..", "..AGg...GggG...gGA..",
    "..AGg....GG....gGA..", "..AGg..........gGA..",
    "..AGg..........gGA..", "..AGg..........gGA..",
    "..AGg..........gGA..", "..AGg..........gGA..",
    "..AGg..........gGA..", "..AGGGGGGGGGGGGGGA..",
    "..AAAAAAAAAAAAAAAA..", "...................."
}};

// Stable spawn IDs select distinct silhouettes; appearance never changes AI rules.
struct MonsterAppearance {
    std::string_view name;
    const PixelSprite* sprite;
    bool hovering;
};
constexpr std::array<MonsterAppearance, 10> MONSTER_APPEARANCES{{
    {"Goblin", &GOBLIN_SPRITE, false},
    {"Demon", &DEMON_SPRITE, false},
    {"Wraith", &WRAITH_SPRITE, true},
    {"Skeleton", &SKELETON_SPRITE, false},
    {"Slime", &SLIME_SPRITE, false},
    {"Bat", &BAT_SPRITE, true},
    {"Spider", &SPIDER_SPRITE, false},
    {"Stone golem", &GOLEM_SPRITE, false},
    {"Evil eye", &EVIL_EYE_SPRITE, true},
    {"Mushroom", &MUSHROOM_SPRITE, false}
}};
static_assert(MONSTER_APPEARANCES.size() >= MAX_MONSTERS,
              "Every supported monster needs its own appearance.");

/// Resolve a spawn's appearance without consuming any simulation randomness.
const MonsterAppearance& monster_appearance(int id) {
    return MONSTER_APPEARANCES[static_cast<unsigned>(id) % MONSTER_APPEARANCES.size()];
}

/// Reject malformed sprite dimensions or unknown palette entries at compile time.
constexpr bool valid_sprite(const PixelSprite& sprite) {
    constexpr std::string_view palette = ".KSLWYABRrTGgPpCDE";
    for (auto row : sprite) {
        if (row.size() != sprite.size()) return false;
        for (char pixel : row) if (palette.find(pixel) == std::string_view::npos) return false;
    }
    return true;
}

/// Validate all registered creature art, including future additions.
constexpr bool valid_monster_sprites() {
    for (const auto& appearance : MONSTER_APPEARANCES)
        if (!appearance.sprite || !valid_sprite(*appearance.sprite)) return false;
    return true;
}
static_assert(valid_sprite(KNIGHT_SPRITE) && valid_sprite(EXIT_SPRITE) &&
              valid_monster_sprites(), "Sprites must use twenty-pixel rows and valid palette entries.");

/// Scale an opaque material color without introducing texture dependencies.
RGB shade(RGB color, float amount) { return {color.r * amount, color.g * amount, color.b * amount}; }

/// Expand one planar quadrilateral into the shared triangle vertex format.
void append_quad(std::vector<Vertex>& vertices, Vec3 a, Vec3 b, Vec3 c, Vec3 d, RGB color) {
    for (Vec3 p : {a, b, c, a, c, d}) vertices.push_back({p.x, p.y, p.z, color.r, color.g, color.b});
}

/// Append a horizontal rectangle in grid space, for caps and ground markings.
void append_floor_rect(std::vector<Vertex>& vertices, float x0, float y0, float x1, float y1,
                       float height, RGB color) {
    append_quad(vertices, {x0,y0,height}, {x1,y0,height}, {x1,y1,height}, {x0,y1,height}, color);
}

/// Build raised gray masonry once. Exposed vertical faces and beveled caps have
/// directional shading; hidden interior faces are omitted. Large maps omit small
/// cap bevels to bound mesh memory while retaining real three-dimensional walls.
std::vector<Vertex> build_wall_mesh(const Maze& maze) {
    std::vector<Vertex> vertices;
    const bool detailed = maze.grid.size() <= 15000;
    // Count exposed faces first so the reservation matches the mesh instead of
    // over-allocating by a third on maximum-size maps.
    std::size_t quads = 0;
    for (int u = 0; u < static_cast<int>(maze.grid.size()); ++u) {
        if (maze.grid[u] != WALL) continue;
        Point p = maze.point(u);
        quads += detailed ? 5 : 1;
        for (Point q : {Point{p.r-1,p.c}, Point{p.r+1,p.c}, Point{p.r,p.c-1}, Point{p.r,p.c+1}}) {
            int v = maze.id(q);
            quads += v < 0 || maze.walkable(v);
        }
    }
    vertices.reserve(quads * 6);
    for (int u = 0; u < static_cast<int>(maze.grid.size()); ++u) {
        if (maze.grid[u] != WALL) continue;
        Point p = maze.point(u);
        float x = static_cast<float>(p.c), y = static_cast<float>(p.r);
        // Coordinate hashing changes only stone coloration, never gameplay RNG.
        std::uint32_t grain = std::uint32_t(u) * 2654435761u;
        float value = 0.36f + float((grain >> 24) & 15u) * 0.004f;
        RGB stone{value, value, value};
        float bevel = detailed ? 0.035f : 0.0f;
        float shoulder = WALL_HEIGHT - (detailed ? 0.045f : 0.0f);
        append_floor_rect(vertices, x+bevel, y+bevel, x+1-bevel, y+1-bevel, WALL_HEIGHT, shade(stone,1.18f));
        if (detailed) {
            append_quad(vertices,{x,y,shoulder},{x+1,y,shoulder},{x+1-bevel,y+bevel,WALL_HEIGHT},{x+bevel,y+bevel,WALL_HEIGHT},shade(stone,1.42f));
            append_quad(vertices,{x+1,y,shoulder},{x+1,y+1,shoulder},{x+1-bevel,y+1-bevel,WALL_HEIGHT},{x+1-bevel,y+bevel,WALL_HEIGHT},shade(stone,0.85f));
            append_quad(vertices,{x+1,y+1,shoulder},{x,y+1,shoulder},{x+bevel,y+1-bevel,WALL_HEIGHT},{x+1-bevel,y+1-bevel,WALL_HEIGHT},shade(stone,0.78f));
            append_quad(vertices,{x,y+1,shoulder},{x,y,shoulder},{x+bevel,y+bevel,WALL_HEIGHT},{x+bevel,y+1-bevel,WALL_HEIGHT},shade(stone,1.25f));
        }
        const auto exposed = [&](Point q) { int v=maze.id(q); return v<0 || maze.walkable(v); };
        if (exposed({p.r-1,p.c})) append_quad(vertices,{x,y,0},{x+1,y,0},{x+1,y,shoulder},{x,y,shoulder},shade(stone,0.85f));
        if (exposed({p.r+1,p.c})) append_quad(vertices,{x+1,y+1,0},{x,y+1,0},{x,y+1,shoulder},{x+1,y+1,shoulder},shade(stone,0.57f));
        if (exposed({p.r,p.c-1})) append_quad(vertices,{x,y+1,0},{x,y,0},{x,y,shoulder},{x,y+1,shoulder},shade(stone,0.76f));
        if (exposed({p.r,p.c+1})) append_quad(vertices,{x+1,y,0},{x+1,y+1,0},{x+1,y+1,shoulder},{x+1,y,shoulder},shade(stone,0.52f));
    }
    return vertices;
}

/// Resolve a pixel's material. Hunting changes monster eyes from gold to red;
/// creature appearance does not assign new AI roles or alter their abilities.
RGB sprite_color(char pixel, bool hunting, bool searching = false) {
    switch (pixel) {
        case 'K': return {0.045f,0.05f,0.065f};
        case 'S': return {0.29f,0.35f,0.43f};
        case 'L': return {0.86f,0.92f,0.97f};
        case 'W': return {0.94f,0.87f,0.67f};
        case 'Y': return {1.0f,0.83f,0.43f};
        case 'A': return {0.82f,0.58f,0.19f};
        case 'B': return {0.19f,0.40f,0.87f};
        case 'R': return {0.96f,0.19f,0.21f};
        case 'r': return {0.55f,0.06f,0.12f};
        case 'T': return {0.45f,0.23f,0.12f};
        case 'G': return {0.40f,0.78f,0.27f};
        case 'g': return {0.17f,0.39f,0.13f};
        case 'P': return {0.71f,0.29f,0.77f};
        case 'p': return {0.36f,0.11f,0.43f};
        case 'C': return {0.32f,0.73f,0.84f};
        case 'D': return {0.14f,0.34f,0.51f};
        case 'E': return hunting ? RGB{1.0f,0.12f,0.08f} : searching ? RGB{1.0f,0.52f,0.10f} : RGB{1.0f,0.83f,0.23f};
        default: return {1,0,1};
    }
}

/// Place a crisp, camera-facing sprite within its grid cell. Transparent pixels
/// emit no geometry, so depth testing preserves proper gaps and wall occlusion.
void append_sprite(std::vector<Vertex>& vertices, Point cell, const PixelSprite& sprite,
                   bool mirror = false, bool hunting = false, float lift = 0.0f, bool searching = false) {
    constexpr float pixel_size = 0.044f;
    const float left = cell.c + 0.5f - 10 * pixel_size;
    const float foot = cell.r + 0.82f;
    for (int r=0;r<20;++r) for (int c=0;c<20;++c) {
        char pixel = sprite[r][mirror ? 19-c : c];
        if (pixel == '.') continue;
        float x = left + c*pixel_size, bottom = (19-r)*pixel_size;
        const auto at = [&](float px, float up) {
            return Vec3{px,foot-VIEW_COS*up,0.035f+lift+VIEW_SIN*up};
        };
        append_quad(vertices,at(x,bottom),at(x+pixel_size,bottom),
                    at(x+pixel_size,bottom+pixel_size),at(x,bottom+pixel_size),sprite_color(pixel,hunting,searching));
    }
}

/// Draw a small octagonal ground ring so the knight stays identifiable when tiny.
void append_actor_ring(std::vector<Vertex>& vertices, Point cell, RGB color) {
    constexpr float pi = 3.14159265359f;
    for (int i=0;i<8;++i) {
        float a=i*pi/4, b=(i+1)*pi/4;
        const auto at=[&](float angle,float radius) {
            return Vec3{cell.c+0.5f+radius*std::cos(angle),cell.r+0.55f+radius*std::sin(angle),0.022f};
        };
        append_quad(vertices,at(a,0.30f),at(b,0.30f),at(b,0.265f),at(a,0.265f),color);
    }
}

/// Keep cells proportionate on any framebuffer. A 25-degree orthographic tilt
/// exposes wall height without rotating WASD directions or hiding maze junctions.
/// Zoom follows the knight and clamps at map edges; zoom=1 shows the entire maze.
std::array<float,16> scene_projection(const Maze& maze, int width, int height, float zoom=1.0f) {
    if (width<=0 || height<=0) throw std::invalid_argument("Framebuffer dimensions must be positive.");
    zoom=std::clamp(zoom,1.0f,64.0f);
    float x_extent=float(maze.cols), y_min=-maze.rows*VIEW_COS, y_max=WALL_HEIGHT*VIEW_SIN;
    float fit=std::min(width/(x_extent+1.4f),height/(y_max-y_min+1.4f))*zoom;
    float sx=2*fit/width, sy=2*fit/height;
    float center_x=x_extent/2, center_y=(y_min+y_max)/2;
    if (zoom>1.0f) {
        Point p=maze.point(maze.player);
        float half_x=width/(2*fit), half_y=height/(2*fit);
        if (2*half_x<x_extent) center_x=std::clamp(p.c+0.5f,half_x,x_extent-half_x);
        if (2*half_y<y_max-y_min) center_y=std::clamp(-(p.r+0.5f)*VIEW_COS,y_min+half_y,y_max-half_y);
    }
    float depth=1.8f/(maze.rows*VIEW_SIN+2.0f);
    return {{sx,0,0,0, 0,-VIEW_COS*sy,-VIEW_SIN*depth,0,
             0,VIEW_SIN*sy,-VIEW_COS*depth,0,
             -center_x*sx,-center_y*sy,maze.rows*0.5f*VIEW_SIN*depth,1}};
}

/// Convert a wrapped hue to a fully saturated RGB color for the complete route.
RGB hue_color(float hue) {
    hue-=std::floor(hue);
    float sector=hue*6.0f, f=sector-std::floor(sector);
    switch (static_cast<int>(sector)) {
        case 0:return {1,f,0}; case 1:return {1-f,1,0}; case 2:return {0,1,f};
        case 3:return {0,1-f,1}; case 4:return {f,0,1}; default:return {1,0,1-f};
    }
}

/// Render every segment of the current-to-exit route as a narrow luminous trail.
/// An amber trail retains the existing warning that all snapshot routes are blocked.
void append_route(std::vector<Vertex>& vertices, const Maze& maze) {
    if (!maze.auto_solve) return;
    RGB color=maze.overlay_avoids_monsters ? hue_color(float(maze.now%100000)/1000.0f*0.3f) : RGB{1,0.65f,0};
    for (std::size_t i=1;i<maze.solution_overlay.size();++i) {
        Point a=maze.solution_overlay[i-1], b=maze.solution_overlay[i];
        for (int layer=0;layer<2;++layer) {
            float half=layer ? 0.055f : 0.135f;
            append_floor_rect(vertices,std::min(a.c,b.c)+0.5f-half,std::min(a.r,b.r)+0.5f-half,
                              std::max(a.c,b.c)+0.5f+half,std::max(a.r,b.r)+0.5f+half,
                              layer ? 0.014f : 0.008f,layer ? color : shade(color,0.22f));
        }
    }
}

// The front end is independent of simulation time and uses a fixed logical canvas.
// The same widget rectangles drive drawing, pointer hit testing, and keyboard focus.
constexpr float UI_WIDTH=1000, UI_HEIGHT=760;
enum class Screen { Setup, Loading, Playing, Paused, Result };
enum class Control {
    None, Low, Medium, High, Small, Standard, Large,
    WidthMinus, Width, WidthPlus, HeightMinus, Height, HeightPlus,
    CountMinus, Count, CountPlus, Start, Quit, AutoSolve, Pause,
    Resume, Setup, Again
};
struct UiRect {
    float x,y,w,h;
    /// Use half-open bounds so touching edges never activate two controls.
    bool contains(float px,float py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; }
};
struct UiWidget { Control id; UiRect box; std::string label; bool selected=false, enabled=true; };
struct UiTransform {
    float scale=1, x=0, y=0;
    /// Fit all controls inside the framebuffer, preserving proportions on resize.
    UiTransform(int width,int height) {
        scale=std::max(0.001f,std::min(width/UI_WIDTH,height/UI_HEIGHT));
        x=(width-UI_WIDTH*scale)/2; y=(height-UI_HEIGHT*scale)/2;
    }
    /// Translate GLFW's window coordinates through framebuffer scaling to the UI.
    std::pair<float,float> pointer(double px,double py,int ww,int wh,int fw,int fh) const {
        if(ww<=0||wh<=0||fw<=0||fh<=0) return {-1,-1};
        return {float((px*fw/ww-x)/scale),float((py*fh/wh-y)/scale)};
    }
};

struct SetupForm {
    Difficulty difficulty=Difficulty::Medium;
    std::string width="31",height="21",count="5";
    /// Accept a complete bounded decimal field; empty or partial edits stay invalid.
    static bool number(const std::string& text,int minimum,int maximum,int& result) {
        if(text.empty()||text.size()>3) return false;
        int value=0;
        for(char c:text) { if(c<'0'||c>'9') return false; value=value*10+c-'0'; }
        if(value<minimum||value>maximum) return false;
        result=value; return true;
    }
    /// Return normalized game dimensions only after every field validates.
    bool values(int& rows,int& cols,int& monsters) const {
        if(!number(height,5,501,rows)||!number(width,5,501,cols)||!number(count,0,10,monsters)) return false;
        rows+=rows%2==0; cols+=cols%2==0; return true;
    }
    /// Explain the first invalid field directly in the setup screen.
    std::string error() const {
        int ignored=0;
        if(!number(width,5,501,ignored)) return "WIDTH MUST BE BETWEEN 5 AND 501.";
        if(!number(height,5,501,ignored)) return "HEIGHT MUST BE BETWEEN 5 AND 501.";
        if(!number(count,0,10,ignored)) return "MONSTERS MUST BE BETWEEN 0 AND 10.";
        return {};
    }
};

// Input state is separate from Maze, so menu actions cannot move actors or emit scent.
struct UiState {
    Screen screen=Screen::Setup;
    SetupForm form;
    Control focus=Control::Start,hover=Control::None,pressed=Control::None;
    bool replace_field=true, quit=false;
    std::string message;
    /// Identify controls which accept decimal text input.
    static bool is_field(Control id) { return id==Control::Width||id==Control::Height||id==Control::Count; }
    /// Obtain the selected edit buffer without changing the other fields.
    std::string& field(Control id) {
        if(id==Control::Width) return form.width;
        if(id==Control::Height) return form.height;
        return form.count;
    }
    /// Change screens with no stale held mouse button or text edit state.
    void show(Screen next) {
        screen=next; hover=pressed=Control::None; replace_field=true; message.clear();
        focus=next==Screen::Setup?Control::Start:next==Screen::Paused?Control::Resume:
              next==Screen::Result?Control::Again:Control::None;
    }
    /// Describe every interactive control in visual and keyboard navigation order.
    std::vector<UiWidget> widgets(const Maze* maze=nullptr) const {
        if(screen==Screen::Setup) {
            int r=0,c=0,n=0; bool valid=form.values(r,c,n);
            return {
                {Control::Low,{50,158,206,116},"LOW",form.difficulty==Difficulty::Low},
                {Control::Medium,{272,158,206,116},"MEDIUM",form.difficulty==Difficulty::Medium},
                {Control::High,{494,158,206,116},"HIGH",form.difficulty==Difficulty::High},
                {Control::Small,{70,342,142,42},"SMALL",r==33&&c==33},
                {Control::Standard,{224,342,142,42},"STANDARD",r==63&&c==63},
                {Control::Large,{378,342,142,42},"LARGE",r==101&&c==101},
                {Control::WidthMinus,{70,422,42,44},"-"},
                {Control::Width,{118,422,116,44},form.width},
                {Control::WidthPlus,{240,422,42,44},"+"},
                {Control::HeightMinus,{370,422,42,44},"-"},
                {Control::Height,{418,422,116,44},form.height},
                {Control::HeightPlus,{540,422,42,44},"+"},
                {Control::CountMinus,{70,554,44,46},"-"},
                {Control::Count,{122,554,90,46},form.count},
                {Control::CountPlus,{220,554,44,46},"+"},
                {Control::Start,{50,658,500,56},"ENTER THE MAZE",false,valid},
                {Control::Quit,{568,658,132,56},"QUIT"}
            };
        }
        if(screen==Screen::Playing) return {
            {Control::AutoSolve,{560,22,260,48},maze&&maze->auto_solve?"AUTO-SOLVE: ON":"AUTO-SOLVE: OFF",maze&&maze->auto_solve},
            {Control::Pause,{836,22,130,48},"PAUSE"}
        };
        if(screen==Screen::Paused) return {
            {Control::Resume,{325,354,350,48},"RESUME"},
            {Control::Setup,{325,416,350,48},"NEW GAME"},
            {Control::Quit,{325,478,350,48},"QUIT"}
        };
        if(screen==Screen::Result) return {
            {Control::Again,{325,414,350,48},"PLAY AGAIN"},
            {Control::Setup,{325,476,350,48},"CHANGE SETUP"},
            {Control::Quit,{325,538,350,48},"QUIT"}
        };
        return {};
    }
    /// Focus a control and select its numeric text for convenient replacement.
    void select(Control id) { focus=id; replace_field=true; }
    /// Move keyboard focus through enabled controls, wrapping at both ends.
    void tab(bool backwards,const Maze* maze=nullptr) {
        auto all=widgets(maze);std::vector<Control> enabled;
        for(const auto& w:all) if(w.enabled) enabled.push_back(w.id);
        if(enabled.empty()) return;
        auto found=std::find(enabled.begin(),enabled.end(),focus);
        int at=found==enabled.end()?(backwards?0:-1):int(found-enabled.begin());
        select(enabled[(at+(backwards?-1:1)+int(enabled.size()))%enabled.size()]);
    }
    /// Accept only ASCII decimal digits, with a three-digit bound on each buffer.
    void digit(unsigned int codepoint) {
        if(screen!=Screen::Setup||!is_field(focus)||codepoint<'0'||codepoint>'9') return;
        auto& text=field(focus);
        if(replace_field) { text.clear();replace_field=false; }
        if(text.size()<3) text.push_back(char(codepoint));
        message.clear();
    }
    /// Clear the selected text or remove the last digit from the active field.
    void erase(bool all=false) {
        if(screen!=Screen::Setup||!is_field(focus)) return;
        auto& text=field(focus);
        if(all||replace_field) text.clear(); else if(!text.empty()) text.pop_back();
        replace_field=false; message.clear();
    }
    /// Step dimensions through odd sizes and keep the monster count inside 0..10.
    void step(Control id,int direction) {
        int value=0;const bool count=id==Control::Count;
        int minimum=count?0:5,maximum=count?10:501;
        if(!SetupForm::number(field(id),minimum,maximum,value)) value=count?5:31;
        if(!count&&value%2==0) value+=direction>0?1:-1; else value+=direction*(count?1:2);
        field(id)=std::to_string(std::clamp(value,minimum,maximum));
        replace_field=true;message.clear();
    }
    /// Commit settings and defer generation until the loading screen can be drawn.
    void request_start() {
        int r=0,c=0,n=0;
        if(!form.values(r,c,n)) { message=form.error();return; }
        form.width=std::to_string(c);form.height=std::to_string(r);form.count=std::to_string(n);
        show(Screen::Loading);
    }
    /// Apply one enabled button action; gameplay receives only explicit auto toggles.
    void activate(Control id,Maze* maze=nullptr) {
        auto all=widgets(maze);
        auto found=std::find_if(all.begin(),all.end(),[&](const UiWidget& w){return w.id==id&&w.enabled;});
        if(found==all.end()) return;
        select(id);
        switch(id) {
            case Control::Low:form.difficulty=Difficulty::Low;break;
            case Control::Medium:form.difficulty=Difficulty::Medium;break;
            case Control::High:form.difficulty=Difficulty::High;break;
            case Control::Small:form.width=form.height="33";break;
            case Control::Standard:form.width=form.height="63";break;
            case Control::Large:form.width=form.height="101";break;
            case Control::WidthMinus:step(Control::Width,-1);break;
            case Control::WidthPlus:step(Control::Width,1);break;
            case Control::HeightMinus:step(Control::Height,-1);break;
            case Control::HeightPlus:step(Control::Height,1);break;
            case Control::CountMinus:step(Control::Count,-1);break;
            case Control::CountPlus:step(Control::Count,1);break;
            case Control::Start:case Control::Again:request_start();break;
            case Control::Pause:
                if (maze) maze->cancel_held_move();
                show(Screen::Paused);break;
            case Control::Resume:show(Screen::Playing);break;
            case Control::Setup:show(Screen::Setup);break;
            case Control::AutoSolve:if(maze)maze->set_auto_solve(!maze->auto_solve);break;
            case Control::Quit:quit=true;break;
            default:break;
        }
    }
    /// Hit test against exactly the rectangles used by the renderer.
    Control hit(float x,float y,const Maze* maze=nullptr) const {
        for(const auto& w:widgets(maze)) if(w.enabled&&w.box.contains(x,y)) return w.id;
        return Control::None;
    }
    /// Trigger a click only when press and release both land on the same control.
    void pointer_button(bool down,float x,float y,Maze* maze=nullptr) {
        Control target=hit(x,y,maze);
        if(down) { pressed=target; if(target!=Control::None)select(target); }
        else { Control origin=pressed;pressed=Control::None;if(origin!=Control::None&&origin==target)activate(target,maze); }
    }
};

/// Resolve a small embedded 5x7 font. UI text uses uppercase, digits, and punctuation.
std::array<unsigned char,7> ui_glyph(char c) {
    switch(c) {
        case 'a':return {0,0,14,1,15,17,15};
        case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
        case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
        case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
        case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
        case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};
        case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
        case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
        case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
        case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
        case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
        case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
        case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
        case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
        case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
        case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
        case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
        case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
        case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
        case ':':return {0,4,4,0,4,4,0};case '.':return {0,0,0,0,0,12,12};
        case '-':return {0,0,0,31,0,0,0};case '+':return {0,4,4,31,4,4,0};
        case '/':return {1,2,2,4,8,8,16};case '|':return {4,4,4,4,4,4,4};
        case '>':return {16,8,4,2,4,8,16};case '<':return {1,2,4,8,4,2,1};
        case '!':return {4,4,4,4,4,0,4};
        case '(':return {2,4,8,8,8,4,2};case ')':return {8,4,2,2,2,4,8};
        case ' ':return {};default:return {14,17,1,2,4,0,4};
    }
}

#ifndef MAZERUNNER_HEADLESS
/// Compile a shader and throw its diagnostic after releasing a failed object.
GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr); glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[2048]{}; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compilation failed: ") + log);
    }
    return shader;
}

/// Link the renderer's GLSL stages; all intermediate shaders are released.
GLuint create_shader_program() {
    const char* vertex_source = R"glsl(
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 viewProjection;
layout(location = 1) in vec3 color;
out vec3 vertexColor;
// Project real wall height and billboard pixels through the same camera.
void main() { gl_Position = viewProjection * vec4(position, 1.0); vertexColor = color; }
)glsl";
    const char* fragment_source = R"glsl(
#version 330 core
in vec3 vertexColor;
out vec4 fragmentColor;
// Render opaque cell and entity colors without texture or lighting lookups.
void main() { fragmentColor = vec4(vertexColor, 1.0); }
)glsl";
    GLuint vs = 0, fs = 0, program = 0;
    try {
        vs = compile_shader(GL_VERTEX_SHADER, vertex_source);
        fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
        program = glCreateProgram();
        glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);
        GLint success = 0; glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[2048]{}; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("Shader link failed: ") + log);
        }
    } catch (...) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        if (program) glDeleteProgram(program);
        throw;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return program;
}

// Own GPU resources only while their OpenGL context remains current. All camera,
// facing and animation state belongs to the renderer, independent of Maze.
class Renderer {
    GLuint program = 0;
    GLint projection_location = -1;
    std::array<GLuint,2> vaos{}, buffers{};
    GLsizei terrain_vertices = 0;
    std::vector<Vertex> dynamic_vertices;
    float zoom = 1.0f;
    int previous_player = -1;
    bool player_mirrored = false;
    Millis player_moved_at = -NEVER;
    std::vector<int> previous_monsters;
    std::vector<unsigned char> monster_mirrored;

    /// Release partially initialized handles as well as a complete renderer.
    void release() {
        glDeleteVertexArrays(2,vaos.data()); glDeleteBuffers(2,buffers.data());
        if (program) glDeleteProgram(program);
        vaos={}; buffers={}; program=0;
    }

public:
    /// Upload immutable wall geometry once; reserve the second batch for sprites
    /// and the complete route. No image loading libraries or asset paths are used.
    explicit Renderer(const Maze& maze) {
        auto terrain=build_wall_mesh(maze);
        dynamic_vertices.reserve((MAX_MONSTERS+2)*20*20*6);
        try {
            program=create_shader_program();
            projection_location=glGetUniformLocation(program,"viewProjection");
            if (projection_location<0) throw std::runtime_error("Renderer projection uniform is unavailable.");
            glGenVertexArrays(2,vaos.data()); glGenBuffers(2,buffers.data());
            for (int i=0;i<2;++i) {
                glBindVertexArray(vaos[i]); glBindBuffer(GL_ARRAY_BUFFER,buffers[i]);
                glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
                glEnableVertexAttribArray(1);
            }
            glBindBuffer(GL_ARRAY_BUFFER,buffers[0]);
            glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(terrain.size()*sizeof(Vertex)),terrain.data(),GL_STATIC_DRAW);
            terrain_vertices=static_cast<GLsizei>(terrain.size()); glBindVertexArray(0);
            if (glGetError()!=GL_NO_ERROR) throw std::runtime_error("Unable to allocate maze graphics buffers.");
        } catch (...) { release(); throw; }
    }
    /// GPU object ownership cannot be copied.
    Renderer(const Renderer&)=delete;
    /// GPU object ownership cannot be assigned.
    Renderer& operator=(const Renderer&)=delete;
    /// Delete graphics resources before the window's context is destroyed.
    ~Renderer() { release(); }

    /// Adjust only the camera; zoom follows the knight and never pauses gameplay.
    void change_zoom(double wheel_steps) {
        zoom=std::clamp(zoom*std::pow(1.25f,float(std::clamp(wheel_steps,-4.0,4.0))),1.0f,64.0f);
    }
    /// Return to the complete-maze view, including entrance and exit.
    void reset_view() { zoom=1.0f; }
    /// Expose the current visual zoom for rendering regression tests.
    float zoom_factor() const { return zoom; }

    /// Draw a depth-tested scene into any current OpenGL framebuffer. Keeping
    /// window access outside this function also permits real offscreen rendering.
    void draw(int width, int height, const Maze& maze, int viewport_y = 0) {
        if (width<=0 || height<=0) return;
        auto projection=scene_projection(maze,width,height,zoom);
        glViewport(0,viewport_y,width,height);
        glClearColor(0,0,0,1); glClearDepth(1.0);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_TRUE);
        glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDisable(GL_MULTISAMPLE); glDisable(GL_DITHER);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(program); glUniformMatrix4fv(projection_location,1,GL_FALSE,projection.data());
        glBindVertexArray(vaos[0]); glDrawArrays(GL_TRIANGLES,0,terrain_vertices);

        dynamic_vertices.clear(); append_route(dynamic_vertices,maze);
        append_sprite(dynamic_vertices,maze.point(maze.exit_cell),EXIT_SPRITE);
        if (previous_monsters.size()!=maze.monsters.size()) {
            previous_monsters.assign(maze.monsters.size(),-1);
            monster_mirrored.assign(maze.monsters.size(),0);
        }
        for (std::size_t i=0;i<maze.monsters.size();++i) {
            const auto& monster=maze.monsters[i]; Point p=maze.point(monster.cell);
            if (previous_monsters[i]>=0 && previous_monsters[i]!=monster.cell) {
                int dc=p.c-maze.point(previous_monsters[i]).c;
                if (dc) monster_mirrored[i]=dc<0;
            }
            previous_monsters[i]=monster.cell;
            const auto& appearance=monster_appearance(monster.id);
            const PixelSprite& sprite=*appearance.sprite;
            float lift=appearance.hovering ? 0.014f*(1.0f+std::sin(float(maze.now%100000)/300.0f+float(monster.id))) : 0.0f;
            append_sprite(dynamic_vertices,p,sprite,monster_mirrored[i]!=0,monster.state==MonsterState::Hunting,lift,
                          monster.state==MonsterState::Searching);
        }
        Point player=maze.point(maze.player);
        if (previous_player>=0 && previous_player!=maze.player) {
            int dc=player.c-maze.point(previous_player).c;
            if (dc) player_mirrored=dc<0;
            player_moved_at=maze.now;
        }
        previous_player=maze.player;
        append_actor_ring(dynamic_vertices,player,{0.31f,0.53f,0.95f});
        float step_lift=maze.now-player_moved_at<160 && (maze.now/80)%2 ? 0.015f : 0.0f;
        append_sprite(dynamic_vertices,player,KNIGHT_SPRITE,player_mirrored,false,step_lift);

        glBindVertexArray(vaos[1]); glBindBuffer(GL_ARRAY_BUFFER,buffers[1]);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(dynamic_vertices.size()*sizeof(Vertex)),dynamic_vertices.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(dynamic_vertices.size()));
        glBindVertexArray(0);
    }
};

// Opaque UI quads share the game's vertex format and shader. The font and icons
// are embedded, so menus require neither a font package nor image asset files.
class UiRenderer {
    GLuint program=0,vao=0,buffer=0;
    GLint projection=-1;
    std::vector<Vertex> vertices;
    const RGB ink{0.055f,0.07f,0.085f}, panel{0.085f,0.105f,0.125f};
    const RGB white{0.92f,0.92f,0.86f},muted{0.56f,0.62f,0.66f},gold{0.87f,0.71f,0.39f};
    /// Add an opaque rectangle in logical UI pixels.
    void rect(UiRect box,RGB color) { append_floor_rect(vertices,box.x,box.y,box.x+box.w,box.y+box.h,0,color); }
    /// Add a one- or two-pixel outline without covering its interior.
    void border(UiRect b,RGB color,float thickness=2) {
        rect({b.x,b.y,b.w,thickness},color);rect({b.x,b.y+b.h-thickness,b.w,thickness},color);
        rect({b.x,b.y,thickness,b.h},color);rect({b.x+b.w-thickness,b.y,thickness,b.h},color);
    }
    /// Emit the embedded bitmap font, keeping pixel edges aligned within a glyph.
    void text(float x,float y,std::string value,float size,RGB color,bool preserve_case=false) {
        for(unsigned char c:value) {
            auto glyph=ui_glyph(preserve_case ? static_cast<char>(c) : static_cast<char>(std::toupper(c)));
            for(int r=0;r<7;++r)for(int col=0;col<5;++col)
                if(glyph[r]&(1u<<(4-col)))rect({x+col*size,y+r*size,size,size},color);
            x+=6*size;
        }
    }
    /// Center a single-line label in its rectangle.
    void centered(UiRect b,const std::string& label,float size,RGB color) {
        float length=label.empty()?0.0f:float(label.size()*6-1)*size;
        text(b.x+(b.w-length)/2,b.y+(b.h-7*size)/2,label,size,color);
    }
    /// Reuse the actual game sprites as flat menu illustrations.
    void icon(float x,float y,const PixelSprite& sprite,float size,bool hunting=false) {
        for(int r=0;r<20;++r)for(int c=0;c<20;++c)if(sprite[r][c]!='.')
            rect({x+c*size,y+r*size,size,size},sprite_color(sprite[r][c],hunting));
    }
    /// Draw a button or editable field with selection, hover, and keyboard focus.
    void widget(const UiWidget& w,const UiState& ui) {
        bool primary=w.id==Control::Start||w.id==Control::Resume||w.id==Control::Again;
        RGB fill=primary?gold:w.selected?RGB{0.17f,0.19f,0.17f}:panel;
        RGB label=primary?ink:white;
        if(!w.enabled) {fill={0.15f,0.16f,0.16f};label=muted;}
        else if(ui.pressed==w.id) fill=shade(fill,0.72f);
        else if(ui.hover==w.id) fill=shade(fill,1.25f);
        rect(w.box,fill);
        border(w.box,w.selected||ui.focus==w.id?gold:RGB{0.22f,0.25f,0.27f},ui.focus==w.id?3:1);
        if(UiState::is_field(w.id)&&ui.focus==w.id&&ui.replace_field)
            rect({w.box.x+12,w.box.y+8,w.box.w-24,w.box.h-16},{0.23f,0.29f,0.35f});
        if(w.id==Control::Low||w.id==Control::Medium||w.id==Control::High) {
            text(w.box.x+18,w.box.y+19,w.label,3,w.selected?gold:white);
            const bool low=w.id==Control::Low,medium=w.id==Control::Medium;
            text(w.box.x+18,w.box.y+62,low?"MORE ROUTES":medium?"BALANCED CHASE":"FAST PURSUIT",1.7f,muted);
            text(w.box.x+18,w.box.y+84,low?"ROOM TO BREATHE":medium?"STAY ALERT":"LESS FORGIVING",1.6f,muted);
            if(w.selected)rect({w.box.x+18,w.box.y+w.box.h-7,w.box.w-36,3},gold);
        } else centered(w.box,w.label,w.id==Control::Start?3.0f:2.0f,label);
    }
    /// Render the setup's descriptive labels, settings summary, and character art.
    void setup(const UiState& ui) {
        text(50,38,"MAZE RUNNER",5,white);text(52,91,"CHOOSE YOUR NEXT ESCAPE.",2,muted);
        text(850,51,"V" MAZERUNNER_VERSION,2,gold,true);rect({50,118,900,1},{0.22f,0.25f,0.27f});
        text(50,134,"01  DIFFICULTY",2,gold);
        text(50,302,"02  MAZE SIZE",2,gold);rect({50,328,650,171},panel);
        text(542,356,"PRESETS",1.6f,muted);
        text(70,402,"WIDTH",1.6f,muted);text(370,402,"HEIGHT",1.6f,muted);
        text(70,481,"5-501 CELLS. EVEN VALUES ROUND UP. CLICK VALUES TO TYPE.",1.6f,muted);
        text(50,522,"03  MONSTERS",2,gold);rect({50,546,650,76},panel);
        int monster_count=0;bool count_valid=SetupForm::number(ui.form.count,0,10,monster_count);
        for(int i=0;i<MAX_MONSTERS;++i) {
            float x=300+37.0f*i;
            if(count_valid&&i<monster_count)icon(x,565,*MONSTER_APPEARANCES[i].sprite,1.3f);
            else border({x+4,569,18,20},{0.20f,0.23f,0.26f},1);
        }
        std::string error=ui.form.error();
        if(!error.empty())text(50,634,error,1.8f,{0.99f,0.47f,0.39f});
        else if(!ui.message.empty())text(50,634,ui.message,1.6f,{0.99f,0.47f,0.39f});
        else text(50,634,"0 MONSTERS FOR A QUIET EXPLORE. 10 FOR A FULL HUNT.",1.7f,muted);
        rect({730,158,220,556},panel);border({730,158,220,556},{0.22f,0.25f,0.27f},1);
        icon(805,187,KNIGHT_SPRITE,3.5f);text(760,288,"YOUR RUN",2,gold);
        text(760,327,difficulty_settings(ui.form.difficulty).name,2.5f,white);
        int r=0,c=0,n=0;
        if(ui.form.values(r,c,n)) {
            text(760,366,std::to_string(c)+" X "+std::to_string(r)+" CELLS",1.7f,white);
            text(760,397,std::to_string(n)+" MONSTERS",1.7f,white);
        } else text(760,366,"CHECK SETTINGS",1.6f,muted);
        rect({760,440,160,1},{0.22f,0.25f,0.27f});
        text(760,467,"HOW TO PLAY",1.7f,gold);
        text(760,503,"WASD OR ARROWS",1.6f,white);text(760,530,"G   AUTO-SOLVE",1.6f,white);
        text(760,557,"ESC PAUSE",1.6f,white);text(760,584,"WHEEL ZOOM",1.6f,white);
        text(760,641,"REACH THE GREEN",1.5f,muted);text(760,664,"EXIT TO ESCAPE.",1.5f,muted);
        text(50,734,"MOUSE TO SELECT  /  TAB TO FOCUS  /  ENTER TO CONFIRM",1.7f,muted);
    }
    /// Place persistent controls and status outside the maze's reserved viewport.
    void hud(const Maze& maze) {
        rect({18,16,964,62},ink);border({18,16,964,62},{0.22f,0.25f,0.27f},1);
        text(34,27,"MAZE RUNNER",2,gold);
        std::ostringstream time;time<<std::fixed<<std::setprecision(1)<<maze.elapsed_ms/1000.0;
        if(!maze.timer_started&&maze.outcome==Outcome::Playing)
            text(34,53,std::string(maze.settings.name)+"  |  MOVE OR PRESS G TO START",1.6f,gold);
        else {
            int hunting=0,searching=0;
            for(const auto& m:maze.monsters) {hunting+=m.state==MonsterState::Hunting;searching+=m.state==MonsterState::Searching;}
            text(186,28,std::string("DIRECTOR ")+maze.director_phase()+" | RADIO "+std::to_string(maze.director_reports())+
                 " | "+maze.radio_area_label(),1.5f,maze.director_hint_width()>maze.settings.hint_area_size ? gold : muted);
            text(34,53,std::string(maze.settings.name)+"  |  "+time.str()+" S  |  "+
                 std::to_string(maze.manual_steps+maze.automatic_steps)+" STEPS",1.6f,white);
            if(hunting)text(400,53,std::to_string(hunting)+" HUNTING",1.6f,{1,0.43f,0.36f});
            else if(searching)text(400,53,std::to_string(searching)+" SEARCHING",1.6f,{1,0.62f,0.25f});
        }
        rect({18,711,964,35},ink);
        text(34,722,"WASD / ARROWS MOVE (HOLD)   G AUTO-SOLVE   ESC PAUSE   WHEEL ZOOM   HOME FIT",1.65f,muted);
    }
    /// Release partially allocated GPU objects while their context is still current.
    void release() {
        if(buffer)glDeleteBuffers(1,&buffer);
        if(vao)glDeleteVertexArrays(1,&vao);
        if(program)glDeleteProgram(program);
        buffer=vao=program=0;
    }
public:
    /// Allocate a reusable overlay batch using the existing OpenGL 3.3 shader.
    UiRenderer() {
        try {
            program=create_shader_program();projection=glGetUniformLocation(program,"viewProjection");
            glGenVertexArrays(1,&vao);glGenBuffers(1,&buffer);
            glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
            glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));glEnableVertexAttribArray(1);
            glBindVertexArray(0);
            if(projection<0||glGetError()!=GL_NO_ERROR)throw std::runtime_error("Unable to initialize menu graphics.");
        } catch(...) {release();throw;}
    }
    UiRenderer(const UiRenderer&)=delete;
    UiRenderer& operator=(const UiRenderer&)=delete;
    /// Destroy the UI before destroying its window/context.
    ~UiRenderer() {release();}
    /// Draw a complete setup/loading screen or a gameplay HUD and modal overlay.
    void draw(int width,int height,const UiState& ui,const Maze* maze=nullptr) {
        if(width<=0||height<=0)return;
        glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        vertices.clear();
        if(ui.screen==Screen::Setup||ui.screen==Screen::Loading) {
            glClearColor(0.02f,0.028f,0.035f,1);glClear(GL_COLOR_BUFFER_BIT);
            rect({0,0,UI_WIDTH,UI_HEIGHT},ink);
            if(ui.screen==Screen::Setup)setup(ui);
            else {
                icon(470,235,KNIGHT_SPRITE,3);
                centered({100,345,800,50},"CARVING YOUR MAZE...",3,gold);
                centered({100,408,800,30},"PREPARING PATHS AND MONSTERS",1.8f,muted);
            }
        } else if(maze) {
            hud(*maze);
            if(ui.screen==Screen::Paused||ui.screen==Screen::Result) {
                const bool paused=ui.screen==Screen::Paused;
                rect({284,224,440,paused?332.0f:396.0f},ink);border({284,224,440,paused?332.0f:396.0f},gold);
                const bool won=maze->outcome==Outcome::Won;
                centered({300,251,400,42},paused?"PAUSED":won?"YOU ESCAPED!":"CAUGHT!",3,won||paused?gold:RGB{1,0.43f,0.36f});
                if(paused)centered({300,305,400,26},"TAKE A BREATHER.",1.8f,muted);
                else {
                    std::ostringstream result;result<<std::fixed<<std::setprecision(1)<<maze->elapsed_ms/1000.0<<" SECONDS";
                    centered({300,310,400,24},result.str(),2,white);
                    centered({300,346,400,24},std::to_string(maze->manual_steps+maze->automatic_steps)+" STEPS",2,muted);
                }
            }
        }
        for(const auto& w:ui.widgets(maze))widget(w,ui);
        UiTransform t(width,height);
        std::array<float,16> matrix{{2*t.scale/width,0,0,0,0,-2*t.scale/height,0,0,0,0,1,0,
                                   -1+2*t.x/width,1-2*t.y/height,0,1}};
        glUseProgram(program);glUniformMatrix4fv(projection,1,GL_FALSE,matrix.data());
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));glBindVertexArray(0);
    }
};

#ifndef MAZERUNNER_RENDER_TEST
// The window owns a reusable menu renderer and at most one active maze renderer.
// Generation runs on a worker; every OpenGL operation stays on the window thread.
struct GameApplication {
    UiState ui;
    UiRenderer menu;
    std::unique_ptr<Maze> maze;
    std::unique_ptr<Renderer> renderer;
    std::future<std::unique_ptr<Maze>> generation;
    std::uint32_t initial_seed;
    std::mt19937 menu_rng;
    bool fixed_seed,quiet,verbose,first_game=true,reported=false;
    Millis last_console=-1000;
    // GLFW identifies the physical key. The maze schedules its repeat deadlines.
    int held_key=-1;

    /// Forget any held direction (pause, focus loss, auto-solve, new game).
    void release_hold() {
        held_key = -1;
        if (maze) maze->cancel_held_move();
    }
    /// Move immediately and start the hold timer for this key.
    void press_move(int key,int dr,int dc) {
        held_key = maze && maze->begin_held_move(dr, dc) ? key : -1;
    }

    /// Initialize visible defaults; CLI difficulty only preselects its menu card.
    GameApplication(std::uint32_t seed,bool fixed,bool silent,bool detailed,Difficulty difficulty)
        :initial_seed(seed),menu_rng(seed^0x9e3779b9u),fixed_seed(fixed),quiet(silent),verbose(detailed) {
        ui.form.difficulty=difficulty;
    }
    /// Release the old map once its player explicitly returns to setup.
    void discard_game() { renderer.reset();maze.reset();reported=false;last_console=-1000;release_hold(); }
    /// Start a generation job from a validated immutable snapshot of the form.
    void begin_generation() {
        if(generation.valid()||ui.screen!=Screen::Loading)return;
        int rows=0,cols=0,count=0;
        if(!ui.form.values(rows,cols,count)) {ui.show(Screen::Setup);return;}
        discard_game();
        std::uint32_t seed=first_game||fixed_seed?initial_seed:menu_rng();
        first_game=false;
        Difficulty difficulty=ui.form.difficulty;
        auto* logs=quiet?nullptr:&std::cout;bool detailed=verbose;
        generation=std::async(std::launch::async,[rows,cols,count,seed,difficulty,logs,detailed] {
            auto generated=std::make_unique<Maze>(rows,cols,seed,logs,difficulty);
            generated->verbose_logs=detailed;generated->generate(count);return generated;
        });
    }
    /// Install a finished map on the GL thread and surface generation errors in UI.
    void finish_generation(bool window_focused) {
        if(!generation.valid()||generation.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return;
        try {
            auto created=generation.get();auto graphics=std::make_unique<Renderer>(*created);
            maze=std::move(created);renderer=std::move(graphics);
            if(!quiet)std::cout<<maze->configuration_summary()<<'\n';
            ui.show(window_focused?Screen::Playing:Screen::Paused);
        } catch(const std::exception& error) {
            discard_game();ui.show(Screen::Setup);
            ui.message="COULD NOT CREATE MAZE. TRY A SMALLER SIZE.";
            std::cerr<<"Maze setup failed: "<<error.what()<<'\n';
        }
    }
    /// Freeze simulation while menus are open, and report outcomes through both UI and logs.
    void tick(Millis delta) {
        if(ui.screen!=Screen::Playing||!maze) {release_hold();return;}
        maze->advance_to(maze->now + std::max<Millis>(0, delta));
        if(maze->outcome!=Outcome::Playing) {
            if(!reported&&!quiet)std::cout<<'\n'<<outcome_name(maze->outcome)<<" | Manual steps: "<<maze->manual_steps
                <<" | Auto steps: "<<maze->automatic_steps<<" | Time: "<<std::fixed<<std::setprecision(2)<<maze->elapsed_ms/1000.0<<" s\n";
            reported=true;ui.show(Screen::Result);
        } else if(maze->now-last_console>=1000) {maze->log_status();last_console=maze->now;}
    }
    /// Keep the map clear of the top controls and bottom keyboard legend.
    void draw(int width,int height) {
        if(width<=0||height<=0)return;
        if(maze&&renderer&&ui.screen!=Screen::Setup&&ui.screen!=Screen::Loading) {
            UiTransform t(width,height);
            int bottom=int(t.y+702*t.scale),top=int(t.y+94*t.scale);
            renderer->draw(width,std::max(1,bottom-top),*maze,height-bottom);
        }
        menu.draw(width,height,ui,maze.get());
    }
};

/// Read current cursor coordinates using the same DPI-aware transform as rendering.
std::pair<float,float> ui_cursor(GLFWwindow* window) {
    double x=0,y=0;int ww=0,wh=0,fw=0,fh=0;
    glfwGetCursorPos(window,&x,&y);glfwGetWindowSize(window,&ww,&wh);glfwGetFramebufferSize(window,&fw,&fh);
    return UiTransform(fw,fh).pointer(x,y,ww,wh,fw,fh);
}

/// Route menu navigation and game controls according to the visible screen.
void key_callback(GLFWwindow* window,int key,int,int action,int mods) {
    auto* app=static_cast<GameApplication*>(glfwGetWindowUserPointer(window));if(!app)return;
    if(action==GLFW_RELEASE) {if(key==app->held_key)app->release_hold();return;}
    if(action!=GLFW_PRESS&&action!=GLFW_REPEAT)return;
    auto& ui=app->ui;
    // Esc never quits: it leaves a text field, pauses/resumes, or leaves results.
    // Quitting stays explicit (Q or the QUIT button).
    if(key==GLFW_KEY_ESCAPE&&action==GLFW_PRESS) {
        if(ui.screen==Screen::Playing)ui.show(Screen::Paused);
        else if(ui.screen==Screen::Paused)ui.show(Screen::Playing);
        else if(ui.screen==Screen::Result)ui.show(Screen::Setup);
        else if(ui.screen==Screen::Setup&&UiState::is_field(ui.focus))ui.select(Control::Start);
        app->release_hold();
        return;
    }
    if(ui.screen==Screen::Loading)return;
    if(key==GLFW_KEY_TAB) {ui.tab((mods&GLFW_MOD_SHIFT)!=0,app->maze.get());return;}
    bool editing=ui.screen==Screen::Setup&&UiState::is_field(ui.focus);
    if(editing) {
        if(key==GLFW_KEY_BACKSPACE)ui.erase();
        else if(key==GLFW_KEY_DELETE)ui.erase(true);
        else if(key==GLFW_KEY_A&&(mods&GLFW_MOD_CONTROL))ui.replace_field=true;
        else if(key==GLFW_KEY_UP)ui.step(ui.focus,1);
        else if(key==GLFW_KEY_DOWN)ui.step(ui.focus,-1);
        else if(key==GLFW_KEY_ENTER||key==GLFW_KEY_KP_ENTER)ui.select(Control::Start);
        return;
    }
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_ENTER||key==GLFW_KEY_KP_ENTER||key==GLFW_KEY_SPACE) {ui.activate(ui.focus,app->maze.get());return;}
    if(key==GLFW_KEY_Q) {ui.quit=true;return;}
    if(ui.screen!=Screen::Playing||!app->maze||!app->renderer)return;
    if(key==GLFW_KEY_HOME) {app->renderer->reset_view();return;}
    if(key==GLFW_KEY_EQUAL||key==GLFW_KEY_KP_ADD) {app->renderer->change_zoom(1);return;}
    if(key==GLFW_KEY_MINUS||key==GLFW_KEY_KP_SUBTRACT) {app->renderer->change_zoom(-1);return;}
    if(key==GLFW_KEY_G) {app->release_hold();ui.activate(Control::AutoSolve,app->maze.get());return;}
    switch(key) {
        case GLFW_KEY_W:case GLFW_KEY_UP:app->press_move(key,-1,0);break;
        case GLFW_KEY_S:case GLFW_KEY_DOWN:app->press_move(key,1,0);break;
        case GLFW_KEY_A:case GLFW_KEY_LEFT:app->press_move(key,0,-1);break;
        case GLFW_KEY_D:case GLFW_KEY_RIGHT:app->press_move(key,0,1);break;
        default:break;
    }
}

/// Text input uses GLFW Unicode events; only decimal digits reach numeric fields.
void character_callback(GLFWwindow* window,unsigned int codepoint) {
    auto* app=static_cast<GameApplication*>(glfwGetWindowUserPointer(window));if(app)app->ui.digit(codepoint);
}

/// Activate menu buttons on a complete left click, with no click-through to gameplay.
void mouse_callback(GLFWwindow* window,int button,int action,int) {
    if(button!=GLFW_MOUSE_BUTTON_LEFT)return;
    auto* app=static_cast<GameApplication*>(glfwGetWindowUserPointer(window));if(!app)return;
    auto p=ui_cursor(window);app->ui.pointer_button(action==GLFW_PRESS,p.first,p.second,app->maze.get());
}

/// Keep camera zoom available during play without changing any game state.
void scroll_callback(GLFWwindow* window,double,double dy) {
    auto* app=static_cast<GameApplication*>(glfwGetWindowUserPointer(window));
    if(app&&app->renderer&&app->ui.screen==Screen::Playing)app->renderer->change_zoom(dy);
}

/// Pause automatically when the player switches away from the game window.
void focus_callback(GLFWwindow* window,int focused) {
    auto* app=static_cast<GameApplication*>(glfwGetWindowUserPointer(window));
    if(app&&!focused)app->release_hold();
    if(app&&!focused&&app->ui.screen==Screen::Playing)app->ui.show(Screen::Paused);
}

#ifndef MAZERUNNER_APP_TEST
/// Initialize GLEW after context creation, retaining the narrow Wayland exception
/// used by V4.4 and verifying the actual GL capabilities before rendering.
void initialize_glew() {
    glewExperimental = GL_TRUE;
    GLenum status = glewInit();
    bool no_glx_on_wayland = false;
#if defined(GLFW_PLATFORM_WAYLAND) && defined(GLEW_ERROR_NO_GLX_DISPLAY)
    no_glx_on_wayland = status == GLEW_ERROR_NO_GLX_DISPLAY && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
#endif
    if (status != GLEW_OK && !no_glx_on_wayland)
        throw std::runtime_error(std::string("Failed to initialize GLEW: ") + reinterpret_cast<const char*>(glewGetErrorString(status)));
    if (!GLEW_VERSION_3_3 || !GLEW_VERSION_3_0 || !GLEW_VERSION_2_0 || !GLEW_VERSION_1_5 ||
        !glCreateShader || !glCreateProgram || !glGenVertexArrays || !glGenBuffers)
        throw std::runtime_error("OpenGL 3.3 shader, vertex-array, and buffer support is required.");
    // Drain loader diagnostics; core-profile GL_INVALID_ENUM is tolerated here only.
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        if (error != GL_INVALID_ENUM) throw std::runtime_error("OpenGL initialization error: " + std::to_string(error));
}

/// Open the graphical setup immediately. Loading, play, pause, and results share
/// one window; elapsed menu/loading time never advances the maze simulation.
int run_graphical(std::uint32_t seed,bool quiet=false,bool verbose=false,
                  Difficulty difficulty=Difficulty::Medium,bool fixed_seed=false) {
    if(!glfwInit())throw std::runtime_error("Failed to initialize GLFW.");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS,24);glfwWindowHint(GLFW_SAMPLES,0);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
    GLFWwindow* window=glfwCreateWindow(1280,960,MAZERUNNER_TITLE,nullptr,nullptr);
    if(!window) {glfwTerminate();throw std::runtime_error("Failed to create GLFW window.");}
    try {
        glfwMakeContextCurrent(window);glfwSwapInterval(1);initialize_glew();
        glfwSetWindowSizeLimits(window,800,600,GLFW_DONT_CARE,GLFW_DONT_CARE);
        GameApplication app(seed,fixed_seed,quiet,verbose,difficulty);
        glfwSetWindowUserPointer(window,&app);glfwSetKeyCallback(window,key_callback);
        glfwSetCharCallback(window,character_callback);glfwSetMouseButtonCallback(window,mouse_callback);
        glfwSetScrollCallback(window,scroll_callback);glfwSetWindowFocusCallback(window,focus_callback);
        if(!quiet)std::cout<<MAZERUNNER_TITLE " | Graphical setup ready.\n"
            <<"WASD/arrows: move | G: auto-solve | Esc: pause | Q: quit\n"
            <<"Wheel or +/-: zoom | Home: fit maze | State logs enabled.\n";
        auto previous=std::chrono::steady_clock::now();
        while(!glfwWindowShouldClose(window)&&!app.ui.quit) {
            auto current=std::chrono::steady_clock::now();
            // Frame limiter: with vsync disabled by the driver, cap the loop near
            // 240 fps instead of spinning a CPU core at 100%.
            const auto frame_deadline=current+std::chrono::microseconds(4166);
            Millis delta=std::chrono::duration_cast<std::chrono::milliseconds>(current-previous).count();
            previous+=std::chrono::milliseconds(delta);
            app.tick(delta);glfwPollEvents();
            if(app.ui.quit||glfwWindowShouldClose(window))break;
            if(app.ui.screen==Screen::Setup&&app.maze)app.discard_game();
            app.finish_generation(glfwGetWindowAttrib(window,GLFW_FOCUSED)==GLFW_TRUE);
            auto mouse=ui_cursor(window);app.ui.hover=app.ui.hit(mouse.first,mouse.second,app.maze.get());
            int width=0,height=0;glfwGetFramebufferSize(window,&width,&height);
            app.draw(width,height);glfwSwapBuffers(window);
            // Publish the loading screen before starting CPU-heavy generation.
            if(app.ui.screen==Screen::Loading&&!app.generation.valid()) {
                try {app.begin_generation();}
                catch(const std::exception& error) {
                    app.ui.show(Screen::Setup);app.ui.message="COULD NOT START MAZE GENERATION. TRY AGAIN.";
                    std::cerr<<"Maze setup failed: "<<error.what()<<'\n';
                }
            }
            if(app.ui.screen!=Screen::Playing)glfwWaitEventsTimeout(0.016);
            else std::this_thread::sleep_until(frame_deadline);
        }
        glfwSetWindowUserPointer(window,nullptr);
    } catch(...) {glfwSetWindowUserPointer(window,nullptr);glfwDestroyWindow(window);glfwTerminate();throw;}
    glfwDestroyWindow(window);glfwTerminate();return 0;
}
#endif // MAZERUNNER_APP_TEST
#endif // MAZERUNNER_RENDER_TEST
#endif // MAZERUNNER_HEADLESS

#ifndef MAZERUNNER_NO_MAIN
/// Dispatch normal play or deterministic simulation; report failures as exit code 1.
int main(int argc, char** argv) {
    try {
        if (argc >= 2 && std::string(argv[1]) == "--simulate") return run_simulation(argc, argv);
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << MAZERUNNER_TITLE "\n  [--seed N] [--difficulty low|medium|high] [--verbose] [--quiet]\n"
                      << "  --simulate ROWS COLS MONSTERS SEED [LIMIT_MS] [--difficulty low|medium|high] [--verbose] [--quiet]\n"
                      << "Interactive play opens graphical setup; --difficulty preselects a card. No terminal input is required.\n"
                      << "Simulation defaults to Medium. Low offers more routes and earlier relief; High increases pursuit pressure.\n"
                      << "State logs are enabled by default; --verbose also logs every monster step.\n"
                      << "Director radio hotfix: movement sends map-wide area reports. Local footsteps travel along open corridors.\n"
                      << "RELIEF widens searches, spaces out reports, and releases interceptors while the solver is confined.\n"
                      << "Mouse/Tab/Enter: setup. WASD/arrows (hold to keep walking): move. G: auto-solve. Esc: pause. Q: quit. Wheel/+/-: zoom. Home: fit.\n";
            return 0;
        }
#ifndef MAZERUNNER_HEADLESS
        std::uint32_t seed = std::random_device{}();
        bool quiet = false, verbose = false, fixed_seed = false;
        Difficulty difficulty = Difficulty::Medium;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) {
                seed = static_cast<std::uint32_t>(parse_number(argv[++i], std::numeric_limits<std::uint32_t>::max()));
                fixed_seed = true;
            }
            else if (arg == "--difficulty") {
                if (i + 1 >= argc) throw std::runtime_error("--difficulty requires low, medium, or high.");
                difficulty = parse_difficulty(argv[++i]);
            }
            else if (arg == "--quiet") quiet = true;
            else if (arg == "--verbose") verbose = true;
            else throw std::runtime_error("Usage: mazerunnerV" MAZERUNNER_VERSION " [--seed N] [--difficulty low|medium|high] [--verbose] [--quiet]");
        }
        return run_graphical(seed, quiet, verbose, difficulty, fixed_seed);
#else
        throw std::runtime_error("Headless build: use --simulate ROWS COLS MONSTERS SEED [LIMIT_MS]");
#endif
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
#endif
