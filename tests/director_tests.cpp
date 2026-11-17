#define main previous_regression_main
#include "feedback_tests.cpp"
#undef main

void test_bounded_wait_and_committed_retreat(){
    std::ostringstream log;Maze m(9,31,0,&log);
    for(int c=1;c<=29;++c)m.grid[m.id({1,c})]=PATH;
    for(int r=1;r<=7;++r){m.grid[m.id({r,5})]=PATH;m.grid[m.id({r,29})]=PATH;}
    for(int c=1;c<=9;++c)m.grid[m.id({7,c})]=PATH;
    m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();
    m.player=m.id({1,20});m.monsters={monster(0,m.id({1,23}),NEVER)};
    m.move_player(0,-1);int start=m.player;m.set_auto_solve(true);m.advance_to(560);
    require(m.player==start&&m.monsters[0].state==MonsterState::Patrolling,"initial wait really sheds scent");
    m.advance_to(1160);
    require(m.player!=start,"exhausted wait budget starts moving");
    require(m.point(m.player).c<m.point(start).c,"retreat increases distance from blocked exit");
    int goal=m.auto_recovery_target();require(goal>=0,"retreat has a persistent goal");
    for(int t=1200;t<=1320;t+=40){m.advance_to(t);require(m.auto_recovery_target()==goal,"goal persists across actions");check_overlay(m);}
    m.advance_to(1800);require(m.manhattan(m.player,start)>=8,"backtracking escapes original small region");
    require(log.str().find("BACKTRACK committed")!=std::string::npos,"backtracking is logged");
    int at=m.player;m.monsters.clear();m.advance_to(10000);
    require(m.outcome==Outcome::Won&&m.player!=at,"cleared obstruction resumes exit progress");
}

void test_alternate_branch_backtracking(){
    Maze m(9,31,0,nullptr);
    for(int c=1;c<=29;++c){m.grid[m.id({1,c})]=PATH;m.grid[m.id({7,c})]=PATH;}
    for(int r=1;r<=7;++r){m.grid[m.id({r,1})]=PATH;m.grid[m.id({r,29})]=PATH;}
    m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.player=m.id({1,15});
    m.monsters={monster(0,m.id({1,20}),NEVER)};m.set_auto_solve(true);
    m.advance_to(40);require(m.player==m.id({1,14}),"solver immediately backtracks to an available alternate branch");
    m.advance_to(10000);require(m.outcome==Outcome::Won,"alternate branch reaches exit without waiting for permanent blocker");
}

void oscillate(Maze& m,Millis until){
    for(Millis t=m.now+100;t<=until;t+=100){m.advance_to(t);m.move_player(0,(t/100)%2?1:-1);}
}

void test_director_precision_and_privacy(){
    std::ostringstream log;auto a=open_maze(41,41,&log),b=open_maze(41,41);
    a.player=a.id({11,11});b.player=b.id({13,11});
    for(auto* m:{&a,&b})m->monsters={monster(0,m->id({1,1}),NEVER),monster(1,m->id({1,3}),NEVER)};
    a.move_player(0,1);b.move_player(0,1);
    oscillate(a,3900);oscillate(b,3900);
    auto old=a.monsters[0].hint;
    for(auto* m:{&a,&b})for(auto& x:m->monsters){x.target=m->id({11,11});x.planned_revision=x.hint.revision;}
    require(a.director_hint_width()==5,"no early relief before confinement window");
    a.advance_to(4000);b.advance_to(4000);
    require(a.director_hint_width()==13,"four seconds of oscillation widens hint to 13");
    for(const auto& x:a.monsters){
        require(x.target==-1,"expansion discards old precise hunting target even inside larger region");
        require(x.hint.min_row<=old.min_row&&x.hint.max_row>=old.max_row&&x.hint.min_col<=old.min_col&&x.hint.max_col>=old.max_col,"expanded hint contains original region");
        require(x.hint.contains(a.point(a.player)),"expanded hint still includes player");
    }
    for(auto* m:{&a,&b})for(auto& x:m->monsters)x.next_move=4001;
    a.advance_to(4001);b.advance_to(4001);
    for(size_t i=0;i<a.monsters.size();++i){
        require(a.monsters[i].target==b.monsters[i].target&&a.monsters[i].cell==b.monsters[i].cell,"broad hint does not leak exact player location");
        require(a.monsters[i].hint.contains(a.point(a.monsters[i].target)),"broad hunt selects a target in approximate area");
        a.monsters[i].next_move=NEVER;
    }
    a.advance_to(4460);
    for(const auto& x:a.monsters)require(x.state==MonsterState::Patrolling&&x.hint.revision==0,"director does not extend original idle deadline");
    a.advance_to(10000);require(a.director_hint_width()==21,"continued confinement escalates precision relief");
    for(const auto&x:a.monsters)require(x.state==MonsterState::Patrolling,"director escalation cannot reawaken patrols");
    a.move_player(0,1);
    for(const auto&x:a.monsters)require(x.state==MonsterState::Hunting&&x.hint.max_row-x.hint.min_row+1==21,"next real movement broadcasts broader clue");
    require(log.str().find("Director: RELIEF started")!=std::string::npos&&log.str().find("RELIEF increased")!=std::string::npos,"director transitions logged");
    a.reset_game();require(a.director_hint_width()==5&&a.auto_recovery_target()==-1,"reset clears director and recovery state");
}

void test_director_recovers_without_flicker(){
    auto m=open_maze(41,41);m.player=m.id({11,11});m.monsters={monster(0,m.id({1,1}),NEVER)};
    m.move_player(0,1);oscillate(m,3900);m.advance_to(4000);
    require(m.director_hint_width()==13,"relief is active");
    for(int i=0;i<18;++i){m.advance_to(4200+i*200);m.move_player(0,1);}
    require(m.director_hint_width()==13,"early movement cannot cancel minimum relief hold");
    for(int i=0;i<14;++i){m.advance_to(7800+i*200);m.move_player(1,0);}
    require(m.director_hint_width()==5,"sustained escape restores normal precision after minimum hold");
    for(const auto&x:m.monsters)require(x.hint.max_col-x.hint.min_col+1==5,"new movement uses restored normal hints");
    auto tiny=open_maze(7,7);tiny.player=tiny.id({3,3});tiny.monsters={monster(0,tiny.id({1,1}),NEVER)};
    tiny.move_player(0,1);oscillate(tiny,3900);tiny.advance_to(4000);tiny.move_player(0,-1);
    const auto& h=tiny.monsters[0].hint;
    require(h.min_row>=0&&h.min_col>=0&&h.max_row<tiny.rows&&h.max_col<tiny.cols,"expanded hints clip safely on small mazes");
    require(h.contains(tiny.point(tiny.player)),"clipped broad hint includes player");
}

void test_director_event_partition_and_stall_regressions(){
    for(unsigned seed:{2u,10u,12u,27u,58u,60u,78u,93u}){
        Maze a(21,21,seed,nullptr),b(21,21,seed,nullptr);a.generate(5);b.generate(5);a.set_auto_solve(true);b.set_auto_solve(true);
        int last=a.player;Millis stationary=0,longest=0;
        for(Millis t=40;t<=60000&&a.outcome==Outcome::Playing;t+=40){
            a.advance_to(t);stationary=a.player==last?stationary+40:0;longest=std::max(longest,stationary);last=a.player;
        }
        b.advance_to(60000);
        require(a.automatic_steps>0,"previous zero-movement stall now advances");
        require(longest<=(2 * difficulty_settings(Difficulty::Medium).scent_ms)+AUTO_SOLVE_SPEED_MS,"old seeds no longer wait indefinitely");
        require(snapshot(a)==snapshot(b)&&a.director_hint_width()==b.director_hint_width(),"director and recovery deterministic across frame partitions");
    }
}

int director_regression_main(){try{
    test_movement_contract();test_no_exact_location_leak();test_waits_and_invalid_moves();test_complete_overlay();
    test_two_solution_generation();test_wait_for_scent_loss();test_regressions();test_logs();
    test_bounded_wait_and_committed_retreat();test_alternate_branch_backtracking();
    test_director_precision_and_privacy();test_director_recovers_without_flicker();test_director_event_partition_and_stall_regressions();
    std::cout<<"PASS: "<<checks<<" assertions across director, recovery, path, generation, and gameplay regressions.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
