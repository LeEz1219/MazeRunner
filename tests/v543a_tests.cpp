#include "v542_tests.cpp"

void test_weighted_overlay_fork() {
    Maze maze(9,41,17,nullptr,Difficulty::Medium);
    for(int c=0;c<=39;++c) maze.grid[maze.id({1,c})]=PATH;
    for(int r=1;r<=7;++r) maze.grid[maze.id({r,39})]=PATH;
    for(int c=19;c<=21;++c) maze.grid[maze.id({2,c})]=PATH;
    maze.grid[maze.exit_cell]=EXIT;
    maze.rebuild_graph();
    maze.reset_game();
    maze.monsters={monster(0,maze.id({1,20}),148)};
    maze.set_auto_solve(true);
    check_overlay(maze);
    require(maze.overlay_avoids_monsters && maze.solution_overlay.size()==49,
            "weighted overlay picks the 48-step clear detour over the 46-step occupied route");
    maze.grid[maze.id({2,20})]=WALL;
    maze.rebuild_graph();
    maze.set_auto_solve(true);
    check_overlay(maze);
    require(!maze.overlay_avoids_monsters && maze.solution_overlay.size()==47,
            "the only remaining route crosses the soft blocker and is amber");
}

void test_sound_around_walls_and_at_range() {
    Maze maze(15,31,13,nullptr,Difficulty::Medium);
    for(int c=1;c<=29;++c) {
        maze.grid[maze.id({1,c})]=PATH;
        maze.grid[maze.id({3,c})]=PATH;
    }
    for(int r=1;r<=13;++r) maze.grid[maze.id({r,29})]=PATH;
    maze.grid[maze.entrance]=PATH;
    maze.grid[maze.exit_cell]=EXIT;
    maze.rebuild_graph();
    maze.reset_game();
    maze.player=maze.id({1,2});
    maze.monsters={monster(0,maze.id({1,7}),NEVER),monster(1,maze.id({3,3}),NEVER)};
    maze.move_player(0,1);
    require(maze.monsters[0].state==MonsterState::Hunting &&
            maze.monsters[0].hint.contains(maze.point(maze.player)), "a nearby footstep delivers its area clue");
    require(maze.monsters[1].state==MonsterState::Searching && maze.monsters[1].scent_until==NEVER &&
            maze.monsters[1].hint.contains(maze.point(maze.player)),
            "through a wall with a 54-step detour, only the map-wide radio supplies an area");
    maze.advance_to(maze.settings.scent_ms);
    require(maze.monsters[0].state==MonsterState::Searching, "a stationary player stops refreshing scent");

    Maze wide(5,501,4,nullptr,Difficulty::High);
    for(int c=0;c<=499;++c) wide.grid[wide.id({1,c})]=PATH;
    for(int r=1;r<=3;++r) wide.grid[wide.id({r,499})]=PATH;
    wide.grid[wide.exit_cell]=EXIT;
    wide.rebuild_graph();
    wide.reset_game();
    wide.player=wide.id({1,4});
    int radius=wide.hearing_radius(), edge=wide.id({1,5+radius});
    require(radius>THREAT_RADIUS,"max-width fixture tests beyond the threat field");
    wide.monsters={monster(0,edge,NEVER),monster(1,edge+1,NEVER)};
    auto threat=wide.local_field(wide.id({1,5}));
    wide.move_player(0,1);
    auto sound=wide.local_field(wide.player,radius);
    require(threat->at(edge)==THREAT_RADIUS+1 && sound->at(edge)==radius &&
            sound->at(edge+1)==radius+1,"each cached field keeps its own search radius");
    require(wide.monsters[0].state==MonsterState::Hunting &&
            wide.monsters[1].state==MonsterState::Searching&&wide.monsters[1].scent_until==NEVER,
            "hearing includes exactly the last in-range step; radio also reaches the next tile");
}

void test_global_interceptor_reservations() {
    Maze maze(15,51,3,nullptr,Difficulty::Medium);
    for(int r=1;r<=13;++r) for(int c=1;c<=9;++c) maze.grid[maze.id({r,c})]=PATH;
    for(int c=10;c<=49;++c) maze.grid[maze.id({7,c})]=PATH;
    for(int r=7;r<=13;++r) maze.grid[maze.id({r,49})]=PATH;
    maze.grid[maze.entrance]=PATH;
    maze.grid[maze.exit_cell]=EXIT;
    maze.rebuild_graph();
    maze.reset_game();
    maze.player=maze.id({7,2});
    auto outside=monster(0,maze.id({7,40}),NEVER);
    outside.state=MonsterState::Hunting;
    outside.role=MonsterRole::Cutter;
    outside.role_until=200;
    outside.target=maze.id({7,17});
    outside.hint={5,9,35,39,7};
    maze.monsters={outside,monster(1,maze.id({7,16}),NEVER)};
    maze.move_player(0,1);
    require(maze.monsters[0].target==outside.target && maze.monsters[0].hint.revision==7 &&
            maze.monsters[0].role==MonsterRole::Cutter,"out-of-earshot interceptor keeps its target and previous clue");
    require(maze.monsters[1].state==MonsterState::Hunting && maze.monsters[1].role==MonsterRole::Chaser,
            "a distant active interceptor still occupies Medium's only slot");
    maze.advance_to(199);
    require(maze.monsters[0].role==MonsterRole::Cutter,"reservation lasts until its deadline");
    maze.advance_to(200);
    require(maze.monsters[0].role==MonsterRole::Chaser,"reservation expires even between monster moves");
    maze.move_player(0,1);
    require(maze.monsters[1].role==MonsterRole::Cutter,"the next audible clue can fill a released slot");

    for(auto difficulty:{Difficulty::Low,Difficulty::Medium,Difficulty::High}) {
        for(unsigned seed:{1u,7u,13u,42u}) {
            Maze game(33,33,seed,nullptr,difficulty);
            game.generate(10);
            game.set_auto_solve(true);
            for(Millis t=40;t<=10000 && game.outcome==Outcome::Playing;t+=40) {
                game.advance_to(t);
                std::set<int> targets;
                for(const auto& m:game.monsters) if(m.role==MonsterRole::Cutter) {
                    require(m.state==MonsterState::Hunting && m.role_until>game.now,"only live hunts hold reservations");
                    require(targets.insert(m.target).second,"active interceptors have distinct targets");
                }
                require(static_cast<int>(targets.size())<=game.settings.cutters,"global interceptor limit holds");
            }
        }
    }
}

void test_director_radio_without_local_hearing() {
    for(auto difficulty:{Difficulty::Low,Difficulty::Medium,Difficulty::High}) {
        auto run=[difficulty](int offset) {
            auto game=fixture(difficulty,81,81);
            int sector=game.settings.report_area_size;
            game.player=game.id({sector+2+offset,sector+2});
            game.monsters={monster(0,game.id({75,75}),1),monster(1,game.id({72,75}),1)};
            game.set_auto_solve(true);game.set_auto_solve(false);
            require(!game.director_reports(),"starting the simulation does not give away a stationary player");
            game.move_player(0,1);
            require(game.director_reports()==1,"first step sends one independent director report");
            game.advance_to(1);
            for(auto& m:game.monsters) {
                require(m.state==MonsterState::Searching && m.scent_until==NEVER &&
                        m.hint.contains(game.point(game.player)),"every remote monster investigates without local scent");
                require(m.hint.max_row-m.hint.min_row+1==sector && m.hint.contains(game.point(m.target)),
                        "the remote monster actually picks a waypoint inside the broad radio sector");
                m.next_move=NEVER;
            }
            return game;
        };
        auto a=run(0),b=run(1);
        for(std::size_t i=0;i<a.monsters.size();++i)
            require(a.monsters[i].target==b.monsters[i].target && a.monsters[i].cell==b.monsters[i].cell,
                    "two different player positions in the same radio sector give identical monster decisions");
        const int interval=a.radio_interval_ms();
        a.advance_to(interval-1);a.move_player(0,-1);
        require(a.director_reports()==1,"an early movement cannot send another radio report");
        a.advance_to(interval);
        require(a.director_reports()==1 && !a.move_player(0,0) && a.director_reports()==1,
                "an expired radio cooldown or invalid input alone does not broadcast");
        a.move_player(0,1);
        require(a.director_reports()==2,"a real step at the difficulty deadline renews the remote search");
        a.advance_to(a.now+a.settings.search_ms);
        for(const auto& m:a.monsters)
            require(m.state==MonsterState::Patrolling&&!m.hint.revision,"a remote search expires when the player stays still");
        require(a.director_reports()==2,"silence produces no further director reports");
    }
}

void test_director_relief_retasks_stale_clues_and_guards() {
    Maze game(15,51,3,nullptr,Difficulty::Medium);
    for(int r=1;r<=13;++r) for(int c=1;c<=9;++c) game.grid[game.id({r,c})]=PATH;
    for(int c=10;c<=49;++c) game.grid[game.id({7,c})]=PATH;
    for(int r=7;r<=13;++r) game.grid[game.id({r,49})]=PATH;
    game.grid[game.entrance]=PATH;game.grid[game.exit_cell]=EXIT;
    game.rebuild_graph();game.reset_game();
    game.player=game.id({7,2});
    game.monsters={monster(0,game.id({7,16}),NEVER),monster(1,game.id({7,44}),NEVER)};
    game.move_player(0,1);oscillate(game,3900);
    auto& guard=game.monsters[0];auto& searcher=game.monsters[1];
    require(guard.role==MonsterRole::Cutter && game.walkable(guard.target),"fixture has a genuine guard on duty");
    // A previous report from elsewhere, retained by an out-of-earshot searcher.
    searcher.state=MonsterState::Searching;searcher.hint={5,9,35,39,47};
    searcher.target=game.id({7,37});searcher.search_until=8000;
    guard.heading=3;guard.heading_streak=2;guard.heard_sector=game.id({7,4});
    guard.heard_sectors={{game.id({7,2}),game.now-1000}};
    Millis scent=guard.scent_until;
    auto reports=game.director_reports();
    game.advance_to(4000);
    require(guard.heading==-1 && !guard.heading_streak && guard.heard_sectors.empty(),
            "relief immediately invalidates directional evidence before another footstep");
    const Millis doubled=2*game.radio_interval_ms();  // Progress is settled; relief doubles the cooldown.
    require(game.director_hint_width()==13 && game.director_reports()==reports,
            "relief changes precision without inventing a new movement report");
    require(guard.role==MonsterRole::Chaser && guard.target<0 && guard.role_until==NEVER && guard.scent_until==scent,
            "relief releases the actual guard without refreshing its scent");
    require(searcher.target<0 && searcher.search_until==8000 && searcher.scent_until==NEVER &&
            searcher.hint.min_col==31 && searcher.hint.max_col==43 && !searcher.hint.contains(game.point(game.player)),
            "stale radio evidence widens around its own sector without disclosing the current player or extending its life");
    oscillate(game,doubled-100);
    require(game.director_reports()==reports && guard.role==MonsterRole::Chaser,"relief spaces reports and suppresses new guards");
    game.advance_to(doubled);game.move_player(0,1);
    require(game.director_reports()==reports+1 && guard.role==MonsterRole::Chaser,"doubled radio cadence resumes with broad search only");
}

void test_hunter_retains_radio_until_scent_fades() {
    auto game=fixture(Difficulty::Medium,81,81);
    game.player=game.id({20,20});
    game.monsters={monster(0,game.id({20,21+game.hearing_radius()}),NEVER)};
    game.move_player(0,1);
    const Millis report_time=game.radio_interval_ms();
    game.advance_to(report_time-200);
    game.move_player(0,-1);game.move_player(0,1);
    const auto local=game.monsters[0].hint;
    const Millis expiry=game.monsters[0].scent_until;
    game.advance_to(report_time);game.move_player(0,-1);
    const auto& hunter=game.monsters[0];
    require(game.director_reports()==2 && hunter.state==MonsterState::Hunting &&
            hunter.hint.revision==local.revision && hunter.scent_until==expiry,
            "an out-of-earshot hunter prioritizes its live local evidence on a new radio broadcast");
    game.advance_to(expiry);
    require(hunter.state==MonsterState::Searching && hunter.scent_until==NEVER &&
            hunter.hint.max_col-hunter.hint.min_col+1==game.settings.report_area_size &&
            hunter.search_until==report_time+game.settings.search_ms,
            "on scent loss the hunter uses its newer radio sector with the original radio expiry");
    game.advance_to(report_time+game.settings.search_ms);
    require(hunter.state==MonsterState::Patrolling && game.director_reports()==2,
            "expiring the fallback does not extend its lifetime or request a new player position");
}

void test_director_observes_solver_waits() {
    auto game=open_maze(21,31);
    game.monsters={monster(0,game.id({18,29}),NEVER)};
    game.set_auto_solve(true);
    // Isolate the director scheduler: an active solver has an open route but
    // submits no displacements for a full window. The same pause in manual play
    // is covered separately by test_idle_player_gets_no_relief().
    game.next_auto=NEVER;
    require(game.overlay_avoids_monsters && game.automatic_steps==0,"unoccupied exit route exists during the wait");
    game.advance_to(game.settings.confinement_ms);
    require(game.director_hint_width()>game.settings.hint_area_size && game.director_reports()==0 &&
            game.monsters[0].state==MonsterState::Patrolling,"solver inactivity counts even before a scent or blocked-route flag exists");

    std::ostringstream log;Maze real(21,31,8,&log);
    real.generate(5);real.set_auto_solve(true);real.advance_to(60000);
    const auto trace=log.str();auto relief=trace.find("Director: RELIEF started");
    require(relief!=std::string::npos && trace.find("Director: NORMAL precision restored",relief)!=std::string::npos,
            "a generated game enters relief and returns to normal through the normal simulation loop");
}
