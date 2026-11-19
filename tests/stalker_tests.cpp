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
    game.monsters = {m};
    game.set_auto_solve(true); game.set_auto_solve(false);
    game.advance_to(1);
    auto& lead = game.monsters[0];
    require(lead.cell == m.cell && lead.scan_until > game.now && lead.gaze >= 0 && game.junction_scans()==1,
            "the searcher turns toward an unchecked arm and pauses once on arrival");
    require(lead.inspected.size()==1 && lead.inspected[0].cell==m.cell, "Medium only clears physically inspected cells");
    game.advance_to(1 + game.settings.monster_move_ms);
    require(game.manhattan(m.cell,lead.cell)==1 && lead.inspected.size()>1 && game.junction_scans()==1,
            "the searcher commits to a corridor after its scan");
}

void test_observed_habits() {
    auto game = open_maze(61,61);
    game.player = game.id({10,10});
    game.monsters = {monster(0,game.id({12,16}),NEVER),monster(1,game.id({56,55}),NEVER)};
    auto walk = [&](int direction, int steps) {
        for (int i=0;i<steps;++i) {
            game.advance_to(game.now + 40);
            require(game.move_player(0,direction), "the habit fixture makes a successful footstep");
        }
    };
    walk(1,15); walk(-1,15); walk(1,10);
    auto& listener = game.monsters.front();
    require(listener.extrapolates && listener.loop_returns==2 && listener.reversals==2,
            "heard sector transitions unlock heading, junction and double-back tactics");
    game.advance_to(game.now + game.settings.scent_ms);
    require(listener.scent_losses==1,"one actual loss counts once");
    walk(1,1);
    game.advance_to(game.now + game.settings.scent_ms);
    require(listener.scent_losses==2,"a second audible hunt and loss unlocks alcove checks");
    const auto& remote = game.monsters.back();
    require(!remote.scent_losses && !remote.loop_returns && !remote.reversals && !remote.extrapolates,
            "radio and global idle expiries cannot teach a remote monster player habits");
    auto still = open_maze(31,31);
    still.player = still.id({10,10});
    still.monsters = {monster(0,still.id({12,13}),NEVER)};
    for (int i=0;i<4;++i) {
        still.move_player(0,i%2 ? -1 : 1);
        still.advance_to(still.now + still.settings.scent_ms);
    }
    require(still.monsters[0].scent_losses==2 && !still.monsters[0].loop_returns,
            "separate hunts in a single sector cannot unlock junction ambush");
}

void test_learned_search_choices() {
    auto sample = [](int losses) {
        Maze game(11,17,42,nullptr,Difficulty::Medium);
        for (int c=1;c<=15;++c) game.grid[game.id({5,c})]=PATH;
        for (int r=1;r<=5;++r) game.grid[game.id({r,1})]=PATH;
        for (int r=5;r<=9;++r) game.grid[game.id({r,15})]=PATH;
        game.grid[game.id({4,5})]=game.grid[game.id({6,7})]=PATH;
        game.grid[game.entrance]=PATH;game.grid[game.exit_cell]=EXIT;
        game.rebuild_graph();game.reset_game();
        auto m=monster(0,game.id({5,5}),1);
        m.state=MonsterState::Searching;m.duty=PackDuty::Investigate;m.hint={4,6,4,8,1};
        m.scent_losses=losses;m.search_until=5000;
        game.monsters={m};game.set_auto_solve(true);game.set_auto_solve(false);
        game.advance_to(1+game.settings.monster_move_ms);
        return game;
    };
    auto fresh=sample(0), experienced=sample(2);
    require(fresh.monsters[0].target==fresh.id({5,7}) && experienced.monsters[0].target==experienced.id({4,5}),
            "the same clue sends an ordinary searcher to a junction and a seasoned one into an alcove");
    auto game=open_maze(31,31);
    auto m=monster(0,game.id({15,15}),1);
    m.state=MonsterState::Searching;m.duty=PackDuty::Investigate;m.search_until=1;
    m.last_junction=m.cell;m.reversals=2;game.monsters={m};
    game.set_auto_solve(true);game.set_auto_solve(false);
    game.advance_to(1+2*game.settings.monster_move_ms);
    require(game.manhattan(m.cell,game.monsters[0].cell)==3,"the double-back first physically leaves the junction");
    game.advance_to(1+5*game.settings.monster_move_ms);
    require(game.monsters[0].cell==m.cell && game.monsters[0].return_junction<0,
            "the patrol revisits its remembered junction once without a new player clue");
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
                    x.scent_losses==y.scent_losses && x.reversals==y.reversals,"local memories replay identically");
        }
    }
}
