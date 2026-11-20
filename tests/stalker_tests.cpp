#include "v543a_tests.cpp"

void test_withdrawal_and_clue_privacy() {
    auto start = [](int row) {
        auto game = open_maze(31,31);
        game.player = game.id({row,10});
        game.monsters = {monster(0,game.id({12,13}),NEVER), monster(1,game.id({22,22}),NEVER)};
        game.move_player(0,1);
        int leads = 0;
        for (const auto& m : game.monsters) leads += m.duty == PackDuty::Investigate;
        require(leads == 1, "one lead owns the shared clue");
        for (int step=1; step<=30 && !game.withdrawing(); ++step) {
            game.advance_to(step*100);
            game.move_player(0,step%2 ? 1 : -1);
        }
        require(game.withdrawing() && game.menace() >= MENACE_HIGH, "sustained nearby evidence starts backstage");
        require(game.director_hint_width() == game.settings.hint_area_size, "menace can trigger before confinement relief");
        return game;
    };
    auto a = start(10), b = start(11);
    auto& retreat = a.monsters[0];
    require(a.now == b.now && a.menace() == b.menace() && retreat.duty == PackDuty::Withdraw &&
            b.monsters[0].duty == retreat.duty && b.monsters[0].withdrawal_goal == retreat.withdrawal_goal,
            "identical evidence in different hidden cells gives identical timing, assignment and destination");
    int before = retreat.cell;
    auto reports = a.director_reports();
    retreat.next_move = a.now + 1;
    a.advance_to(a.now + 1);
    require(a.manhattan(before, retreat.cell) == 1 && a.withdrawal_steps() == 1, "withdrawal takes a physical cardinal step");
    a.move_player(0,-1);
    require(a.director_reports() == reports && retreat.target == retreat.withdrawal_goal,
            "a footstep during backstage preserves the withdrawal and withholds radio");
    a.advance_to(a.now + 6000);
    require(a.outcome == Outcome::Playing && !a.withdrawing() && a.menace() <= MENACE_LOW &&
            a.director_reports() == reports, "silence lets menace decay without a new broadcast");
    a.reset_game();
    require(!a.withdrawing() && !a.menace() && !a.withdrawal_episodes(), "replay starts with clear pacing state");
}

void test_suspicion_scan() {
    auto game = open_maze(31,31);
    game.player = game.id({25,25});
    auto m = monster(0,game.id({12,12}),1);
    m.state = MonsterState::Searching; m.duty = PackDuty::Investigate;
    m.search_until = 5000; m.hint = {10,14,10,14,1};
    m.target = game.id({12,14});
    game.monsters = {m};
    game.set_auto_solve(true); game.set_auto_solve(false);
    game.advance_to(1);
    auto& lead = game.monsters[0];
    require(lead.cell == m.cell && lead.scan_until > game.now && lead.gaze == 3 && game.junction_scans()==1,
            "the searcher turns toward an unchecked arm and pauses once on arrival");
    require(lead.inspected.size()==1 && lead.inspected[0].cell==m.cell, "Medium only clears physically inspected cells");
    game.advance_to(1 + game.settings.monster_move_ms * SEARCH_PACE_PERCENT / 100);
    require(game.manhattan(m.cell,lead.cell)==1 && lead.inspected.size()>1 && game.junction_scans()==1,
            "the searcher commits to a corridor after its scan");
}

// Both hunters arrive on their first action. The prediction must continue with
// the copied pack's reserved corridors, preserving the live route until its turn.
void test_retreat_continuation_forecast() {
    auto game = open_maze(31,31);
    game.set_auto_solve(true);
    game.set_auto_solve(false);
    for (int i = 0; i < 2; ++i) {
        auto m = monster(i,game.id({15+i,16}),1);
        m.state = MonsterState::Hunting;
        m.duty = PackDuty::Withdraw;
        m.hint = {12,16,7,11,1};
        m.scent_until = 10000;
        m.target = m.withdrawal_goal = game.id({15+i,17});
        m.retreat_route = {m.target};
        game.monsters.push_back(m);
    }
    const auto forecast = game.forecast_monsters(AUTO_HORIZON_MAX);
    for (const auto& m : game.monsters)
        require(m.retreat_route.size() == 1 && m.retreat_route.back() == m.target,
                "forecasting leaves both live retreat orders intact");
    for (int t = 1; t <= forecast.horizon; ++t) {
        game.advance_to(t * AUTO_SOLVE_SPEED_MS);
        for (std::size_t i = 0; i < game.monsters.size(); ++i)
            require(game.monsters[i].cell == forecast.positions[t][i], "live and forecast retreat agree past arrival");
        require(game.monsters[0].cell != game.monsters[1].cell, "retreaters occupy distinct cells");
    }
    for (int i = 0; i < 2; ++i)
        require(game.monsters[i].cell != game.id({15+i,17}), "a free outward corridor permits another leg");
}

void test_observed_habits() {
    auto walk = [](Maze& g, int dc, int steps) {
        for (int i=0;i<steps;++i) { g.advance_to(g.now + 40); require(g.move_player(0,dc), "the habit fixture makes a successful footstep"); }
    };
    // Back-and-forth across one sector edge is neither a U-turn nor a loop.
    auto jitter = open_maze(61,61);
    jitter.player = jitter.id({10,9});
    jitter.monsters = {monster(0,jitter.id({12,16}),NEVER)};
    for (int i=0;i<40;++i) walk(jitter, i%2 ? -1 : 1, 1);
    require(!jitter.lesson_evidence(DoubleBack) && !jitter.lesson_evidence(JunctionAmbush),
            "sector-edge jitter teaches nothing");
    // A U-turn after a sustained heading is evidence, counted once per footstep however many listen.
    auto game = open_maze(61,61);
    game.player = game.id({10,9});
    game.monsters = {monster(0,game.id({12,16}),NEVER),monster(1,game.id({14,12}),NEVER)};
    walk(game,1,15); walk(game,-1,15);
    require(game.lesson_evidence(DoubleBack)==1, "one real U-turn heard by two monsters counts once");
    walk(game,1,15);
    require(game.lesson_evidence(DoubleBack)==2 && !game.pack_learned(DoubleBack),
            "enough evidence still waits for the lesson spacing");
    game.advance_to(game.timer_start + LESSON_SPACING_MS);
    int learned = 0;
    for (int k=0;k<LESSON_COUNT;++k) learned += game.pack_learned(Lesson(k));
    require(game.pack_learned(DoubleBack) && learned==1, "the pack learns one lesson at a time, after the spacing");
    // The learned double-back: a patrol leaves its last junction and returns to check it.
    auto m=monster(0,game.id({30,30}),game.now+1);
    m.state=MonsterState::Searching;m.duty=PackDuty::Investigate;m.search_until=game.now+1;m.last_junction=m.cell;
    game.player=game.id({55,55});game.monsters={m};
    const Millis patrol=game.settings.monster_move_ms*PATROL_PACE_PERCENT/100;
    game.advance_to(game.now+1+2*patrol);
    require(game.manhattan(m.cell,game.monsters[0].cell)==3,"the double-back first physically leaves the junction");
    game.advance_to(game.now+3*patrol);
    require(game.monsters[0].cell==m.cell && game.monsters[0].return_junction<0,
            "the patrol revisits its remembered junction once without a new player clue");
    game.reset_game();
    require(!game.pack_learned(DoubleBack) && !game.lesson_evidence(DoubleBack), "a new run forgets every lesson");
}

void test_learned_search_choices() {
    auto fixture = []() {
        Maze game(11,17,42,nullptr,Difficulty::Medium);
        for (int c=1;c<=15;++c) game.grid[game.id({5,c})]=PATH;
        for (int r=1;r<=5;++r) game.grid[game.id({r,1})]=PATH;
        for (int r=5;r<=9;++r) game.grid[game.id({r,15})]=PATH;
        game.grid[game.id({4,5})]=game.grid[game.id({6,7})]=PATH;
        game.grid[game.entrance]=PATH;game.grid[game.exit_cell]=EXIT;
        game.rebuild_graph();game.reset_game();
        return game;
    };
    auto search = [](Maze& game) {
        auto m=monster(0,game.id({5,5}),game.now+1);
        m.state=MonsterState::Searching;m.duty=PackDuty::Investigate;m.hint={4,6,4,8,1};m.search_until=game.now+5000;
        game.player=game.id({9,15});game.monsters={m};
        game.advance_to(game.now+1+game.settings.monster_move_ms*SEARCH_PACE_PERCENT/100);
        return game.monsters[0].target;
    };
    auto fresh=fixture();
    fresh.set_auto_solve(true);fresh.set_auto_solve(false);
    require(search(fresh)==fresh.id({5,7}),"an ordinary searcher heads for the junction");
    // The same received sector gives the same evidence at two different hidden positions.
    for (Point end : {Point{6,7}, Point{5,6}}) {
        auto hidden=fixture();
        hidden.player=hidden.id({5,7});
        hidden.monsters={monster(0,hidden.id({5,12}),NEVER)};
        hidden.move_player(end.r-5,end.c-7);
        hidden.advance_to(hidden.settings.scent_ms+hidden.settings.search_ms);
        require(!hidden.lesson_evidence(AlcoveSweep),"an unsearched hidden branch supplies no evidence");
    }
    auto taught=fixture();
    taught.player=taught.id({5,8});
    taught.monsters={monster(0,taught.id({5,7}),NEVER)};
    for (int pass=0;pass<2;++pass) {
        auto& lead=taught.monsters[0];
        lead.cell=taught.id({5,7});lead.scanned_junction=-1;
        taught.move_player(0,pass ? -1 : 1);
        taught.advance_to(lead.scent_until);
        require(lead.state==MonsterState::Searching && taught.lesson_evidence(AlcoveSweep)==pass,
                "losing scent opens a local search without awarding a vote");
        lead.target=taught.id({6,7});lead.planned_revision=lead.hint.revision;lead.next_replan=NEVER;
        lead.next_move=taught.now+1;
        taught.advance_to(lead.next_move);
        if(lead.cell!=lead.target) taught.advance_to(lead.next_move);
        require(lead.cell==taught.id({6,7}) && lead.inspected_alcove,"the lead physically checks a tip inside its local clue");
        lead.next_move=NEVER;
        taught.advance_to(lead.search_until);
        require(taught.lesson_evidence(AlcoveSweep)==pass+1,"a completed unsuccessful sweep credits its local clue once");
    }
    taught.advance_to(taught.timer_start + LESSON_SPACING_MS);
    require(taught.pack_learned(AlcoveSweep),"two observed empty alcove sweeps teach the pack");
    require(search(taught)==taught.id({4,5}),"the same clue now sends the searcher into the alcove");
}

void test_charge_reach_bound() {
    auto game=fixture(Difficulty::High,31,41);
    game.player=game.id({10,14});
    auto hunter=monster(0,game.id({10,10}),1);
    hunter.state=MonsterState::Hunting;hunter.scent_until=10000;hunter.target=game.player;
    game.monsters={hunter};game.set_auto_solve(true);game.set_auto_solve(false);
    game.advance_to(1);
    require(game.monsters[0].glimpses==1,"a sound hunter has already spent its notice beat");
    const auto forecast=game.forecast_monsters();
    const Millis predicted=game.earliest_arrival(forecast,0,game.player);
    const Millis noticed=game.now;
    game.advance_to(900);
    require(game.outcome==Outcome::Caught && game.now-noticed==447 && predicted==447,
            "the four-edge charge meets the physical lower bound after a pending notice");
}

void test_sector_circuit_evidence() {
    // Both maps cross the same four sectors. Removing one tile in the starting sector opens the ring.
    auto tour=[](bool closed, bool interrupt) {
        Maze game(31,41,123,nullptr,Difficulty::High);
        for(int n=12;n<=20;++n) for(Point cell : {Point{12,n},Point{20,n},Point{n,12},Point{n,20}})
            game.grid[game.id(cell)]=PATH;
        if(!closed) game.grid[game.id({12,13})]=WALL;
        game.grid[game.id({21,17})]=PATH;
        game.grid[game.entrance]=PATH;game.grid[game.exit_cell]=EXIT;game.rebuild_graph();game.reset_game();
        game.player=game.id({12,14});game.monsters={monster(0,game.id({21,17}),NEVER)};
        auto walk=[&](int dr,int dc,int steps) {
            for(int i=0;i<steps;++i) {game.advance_to(game.now+180);require(game.move_player(dr,dc),"circuit fixture step");}
        };
        walk(0,1,6);walk(1,0,8);
        if(interrupt) game.advance_to(game.now+game.settings.scent_ms+1);
        walk(0,-1,8);walk(-1,0,8);
        return game.lesson_evidence(JunctionAmbush);
    };
    require(tour(true,false)==1,"a continuous return around a real terrain circuit supplies a vote");
    require(tour(false,false)==0,"identical sector crossings along a cut-open ring supply no loop vote");
    require(tour(true,true)==0,"returning across separate hunts cannot complete an earlier circuit");
    auto game=fixture(Difficulty::High,31,41);
    game.player=game.id({10,14});game.monsters={monster(0,game.id({11,18}),NEVER)};
    game.move_player(0,1);game.move_player(0,1);
    for(int i=0;i<40;++i) {game.advance_to(game.now+100);game.move_player(0,i%2 ? -1 : 1);}
    game.move_player(0,-1);
    require(!game.lesson_evidence(JunctionAmbush),"time alone cannot turn a two-sector reversal into a loop");
}

void test_stalker_replay() {
    for (auto diff : {Difficulty::Medium,Difficulty::High}) {
        Maze a(21,31,2,nullptr,diff), b(21,31,2,nullptr,diff);
        a.generate(5); b.generate(5); a.set_auto_solve(true); b.set_auto_solve(true);
        for (Millis t=13;t<28000 && a.outcome==Outcome::Playing;t+=13) a.advance_to(t);
        a.advance_to(28000); b.advance_to(28000);
        require(snapshot(a)==snapshot(b) && a.menace()==b.menace() &&
                a.withdrawal_steps()==b.withdrawal_steps() && a.junction_scans()==b.junction_scans(),
                "encounter timings and search behavior replay across frame partitions");
        for (std::size_t i=0;i<a.monsters.size();++i) {
            const auto& x=a.monsters[i];const auto& y=b.monsters[i];
            require(x.duty==y.duty && x.withdrawal_goal==y.withdrawal_goal && x.inspected.size()==y.inspected.size() &&
                    x.heard_sectors==y.heard_sectors,"local memories replay identically");
        }
        for (int k=0;k<LESSON_COUNT;++k)
            require(a.pack_learned(Lesson(k))==b.pack_learned(Lesson(k)) && a.lesson_evidence(Lesson(k))==b.lesson_evidence(Lesson(k)),
                    "pack lessons replay identically");
    }
}
