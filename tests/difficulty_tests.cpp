#include "director_tests.cpp"

Maze fixture(Difficulty d,int rows=51,int cols=51){
    Maze m(rows,cols,123,nullptr,d);
    for(int r=1;r<m.rows-1;++r)for(int c=1;c<m.cols-1;++c)m.grid[m.id({r,c})]=PATH;
    m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();return m;
}

void test_difficulty_input(){
    for(auto entry:std::vector<std::pair<std::string,Difficulty>>{{"1",Difficulty::Low},{" LOW ",Difficulty::Low},{"2",Difficulty::Medium},{"Medium",Difficulty::Medium},{"3",Difficulty::High},{"hIgH",Difficulty::High}})
        require(parse_difficulty(entry.first)==entry.second,"difficulty number/name parser");
    for(std::string invalid:{"", "0", "4", "1oops", "easy", "--quiet"}){
        bool threw=false;try{parse_difficulty(invalid);}catch(const std::invalid_argument&){threw=true;}
        require(threw,"invalid explicit difficulty rejected");
    }
    std::ostringstream output;std::istringstream enter("\n15\n");
    require(prompt_difficulty(enter,output)==Difficulty::Medium,"Enter defaults to Medium");
    int height=0;enter>>height;require(height==15,"menu preserves following maze input");
    std::istringstream retry("4\ninvalid\n3\n");
    require(prompt_difficulty(retry,output)==Difficulty::High,"invalid menu input retries");
    std::istringstream closed;bool threw=false;
    try{prompt_difficulty(closed,output);}catch(const std::runtime_error&){threw=true;}
    require(threw,"EOF exits menu rather than looping forever");
    for(auto d:{Difficulty::Low,Difficulty::Medium,Difficulty::High}){
        auto m=fixture(d);m.monsters={monster(0,m.id({1,1}),NEVER)};
        auto text=m.configuration_summary();
        require(text.find(m.settings.name)!=std::string::npos&&text.find(std::to_string(m.settings.monster_move_ms))!=std::string::npos,"startup reports actual selected profile");
    }
}

void test_movement_and_hint_profiles(){
    for(auto d:{Difficulty::Low,Difficulty::Medium,Difficulty::High}){
        auto m=fixture(d);m.player=m.id({25,25});m.monsters={monster(0,m.id({20,20}),NEVER)};
        m.move_player(0,1);
        require(m.monsters[0].state==MonsterState::Hunting&&m.monsters[0].hint.revision==1,"first movement alerts every difficulty immediately");
        require(m.monsters[0].hint.max_row-m.monsters[0].hint.min_row+1==m.settings.hint_area_size,"normal hint uses selected area");
        for(int t=40;t<=200;t+=40){
            m.advance_to(t);m.move_player(0,(t/40)%2?-1:1);
            auto revision=m.monsters[0].hint.revision;
            require(revision==(d==Difficulty::Low?(t<200?1u:2u):std::uint64_t(1+t/40)),"moving update throttle matches difficulty");
        }
        // The last move at 240 ms is suppressed on Low, but must extend scent.
        m.advance_to(240);m.move_player(0,1);
        auto revision=m.monsters[0].hint.revision;
        m.advance_to(240+m.settings.scent_ms-1);
        require(m.monsters[0].state==MonsterState::Hunting,"scent tracks last displacement, not last report");
        require(m.monsters[0].hint.revision==revision,"no delayed hint flush while stationary");
        m.advance_to(240+m.settings.scent_ms);
        require(m.monsters[0].state==MonsterState::Searching,"profile scent expires at exact deadline");
        m.move_player(0,-1);require(m.monsters[0].state==MonsterState::Hunting&&m.monsters[0].hint.revision>revision,"first resumed movement immediately alerts again");
        auto moving=fixture(d);moving.player=moving.id({45,45});
        auto x=monster(0,moving.id({1,1}),100);x.target=moving.id({1,40});x.next_replan=NEVER;
        auto frozen=fixture(d);frozen.player=frozen.id({45,45});frozen.monsters={x};
        frozen.advance_to(1000);require(frozen.monsters[0].cell==x.cell&&frozen.monsters[0].next_move==100,"monsters stay frozen until the first input");
        moving.monsters={x};moving.set_auto_solve(true);moving.set_auto_solve(false);
        require(moving.timer_started,"enabling auto-solve releases patrols before a real step or radio report");
        moving.advance_to(99);require(moving.monsters[0].cell==x.cell,"monster waits for its deadline");
        moving.advance_to(100);require(moving.monsters[0].cell==moving.id({1,2}),"monster moves at deadline");
        require(moving.monsters[0].next_move==100+moving.settings.monster_move_ms*PATROL_PACE_PERCENT/100,"monster schedules its patrol pace");
        const Millis patrol=moving.settings.monster_move_ms*PATROL_PACE_PERCENT/100;
        moving.advance_to(100+patrol-1);require(moving.monsters[0].cell==moving.id({1,2}),"no early second monster movement");
        moving.advance_to(100+patrol);require(moving.monsters[0].cell==moving.id({1,3}),"second movement respects patrol pace");
        auto threat=fixture(d);threat.player=threat.id({25,25});
        threat.monsters={monster(0,threat.id({25,26}),10),monster(1,threat.exit_cell,NEVER)};
        auto route=threat.plan_auto_route();require(route.size()>1&&route[1]!=threat.player,"imminent threat overrides scent wait at every speed");
        auto waiting=fixture(d);waiting.monsters={monster(0,waiting.exit_cell,NEVER)};
        waiting.player=waiting.id({47,47});waiting.move_player(0,1);int start=waiting.player;waiting.set_auto_solve(true);
        waiting.advance_to(waiting.settings.scent_ms);
        require(waiting.player==start&&waiting.monsters[0].state!=MonsterState::Hunting,"auto wait sheds profile-specific scent");
        waiting.advance_to(2*waiting.settings.scent_ms+40);
        require(waiting.player!=start,"profile wait budget is bounded");
        bool reached=false;
        for(Millis t=waiting.now+40;t<6000;t+=40){
            waiting.advance_to(t);
            if(waiting.auto_recovery_target()==waiting.player){
                int rested=waiting.player;Millis arrived=waiting.now;
                waiting.advance_to(arrived+waiting.settings.scent_ms);
                require(waiting.player==rested&&waiting.monsters[0].state!=MonsterState::Hunting,"post-retreat pause adapts to scent duration");
                reached=true;break;
            }
        }
        require(reached,"post-retreat scent test reaches a committed waypoint");
    }
}

void test_director_profiles(){
    for(auto d:{Difficulty::Low,Difficulty::Medium,Difficulty::High}){
        auto m=fixture(d);m.player=m.id({25,25});m.monsters={monster(0,m.id({20,20}),NEVER)};m.move_player(0,1);
        int first_tick=((m.settings.confinement_ms+DIRECTOR_TICK_MS-1)/DIRECTOR_TICK_MS)*DIRECTOR_TICK_MS;
        oscillate(m,first_tick-100);m.advance_to(first_tick-1);
        require(m.director_hint_width()==m.settings.hint_area_size,"relief does not start early");
        m.monsters[0].target=m.id({25,25});m.advance_to(first_tick);
        require(m.director_hint_width()==m.settings.hint_area_size+2*m.settings.padding_step,"confinement widens selected profile");
        require(m.monsters[0].target==-1,"expansion invalidates tighter target");
        oscillate(m,first_tick+m.settings.relief_ms-100);m.advance_to(first_tick+m.settings.relief_ms-1);
        require(m.director_hint_width()==m.settings.hint_area_size+2*m.settings.padding_step,"relief stage holds for profile duration");
        m.advance_to(first_tick+m.settings.relief_ms);
        require(m.director_hint_width()==m.settings.hint_area_size+4*m.settings.padding_step,"persistent confinement escalates selected profile");
        int width=m.director_hint_width();m.advance_to(m.now+2*m.settings.relief_ms);
        require(m.director_hint_width()==width,"idle time after confinement does not escalate further");
        oscillate(m,first_tick+8*m.settings.relief_ms-100);m.advance_to(first_tick+8*m.settings.relief_ms);
        require(m.director_hint_width()==m.settings.hint_area_size+2*m.settings.max_padding,"relief respects maximum uncertainty");
        m.reset_game();require(m.director_hint_width()==m.settings.hint_area_size,"reset restores profile precision");
        for(auto dims:std::vector<std::pair<int,int>>{{5,5},{5,51},{51,5}}){
            auto small=fixture(d,dims.first,dims.second);small.monsters={monster(0,small.id({3,1}),NEVER)};
            small.move_player(0,1);const auto&hint=small.monsters[0].hint;
            require(hint.min_row>=0&&hint.min_col>=0&&hint.max_row<small.rows&&hint.max_col<small.cols,"hint clipping never produces negative bounds");
            require(hint.contains(small.point(small.player)),"clipped hint contains player");
        }
    }
}

// Count actual simple routes on small graphs; independent of block/cycle ranks.
int enumerate_paths(const Maze&m,int u,std::vector<unsigned char>&seen,int remaining){
    if(u==m.exit_cell)return 1;
    seen[u]=1;int count=0;
    for(int v:m.neighbors(u))if(v>=0&&!seen[v]){
        count+=enumerate_paths(m,v,seen,remaining-count);if(count>=remaining)break;
    }
    seen[u]=0;return count;
}
void test_diversity_certificates(){
    // Every 3x3 interior subgraph catches articulation and off-route cycle errors.
    for(unsigned mask=0;mask<512;++mask){
        Maze m(5,5,0,nullptr,Difficulty::Low);
        for(int i=0;i<9;++i)if(mask&(1u<<i))m.grid[m.id({1+i/3,1+i%3})]=PATH;
        m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();
        std::vector<unsigned char> seen(m.grid.size(),0);int actual=enumerate_paths(m,m.entrance,seen,4);
        require(m.solution_diversity()<=actual,"cycle-rank certificate never overstates actual simple solutions");
        require(m.solution_diversity(true)<=m.solution_diversity(),"useful-route filter is conservative");
    }
    for(unsigned seed=0;seed<40;++seed){
        Maze m(5+2*(seed%2),5+2*((seed/2)%2),seed,nullptr,Difficulty::Low);m.generate(0);
        std::vector<unsigned char> seen(m.grid.size(),0);
        require(enumerate_paths(m,m.entrance,seen,4)>=4,"Low generation has four independently enumerated solutions");
    }
    auto m=fixture(Difficulty::Low);m.player=m.entrance;
    require(m.solution_diversity(true)==4,"dense graph lower bound saturates at four");
}

void test_all_profile_simulation_invariants(){
    for(auto d:{Difficulty::Low,Difficulty::Medium,Difficulty::High})for(unsigned seed=0;seed<12;++seed){
        Maze a(21,25,seed,nullptr,d),b(21,25,seed,nullptr,d);a.generate(5);b.generate(5);a.set_auto_solve(true);b.set_auto_solve(true);
        for(Millis time=20;time<=12000&&a.outcome==Outcome::Playing;time+=20){
            int before=a.player;a.advance_to(time);
            require(a.manhattan(before,a.player)<=1,"player movement remains cardinal at all speeds");
            std::set<int> occupied;
            for(const auto&x:a.monsters){require(a.walkable(x.cell),"monster remains on terrain");require(occupied.insert(x.cell).second,"no monster stacking");}
            if(a.outcome==Outcome::Playing)require(!occupied.count(a.player),"playing state cannot hide a collision");
            check_overlay(a);
        }
        b.advance_to(12000);require(snapshot(a)==snapshot(b)&&a.director_hint_width()==b.director_hint_width(),"all profiles deterministic across frame partitions");
        Maze free(11,15,seed,nullptr,d);free.generate(0);int optimum=bfs(free,free.player,free.exit_cell);
        free.set_auto_solve(true);free.advance_to(10000);
        require(free.outcome==Outcome::Won&&free.automatic_steps==optimum,"no-monster solver remains optimal on every difficulty");
        require(free.elapsed_ms==optimum*AUTO_SOLVE_SPEED_MS,"player speed is unchanged on every difficulty");
    }
}

int difficulty_regression_main(){try{
    require(director_regression_main()==0,"V5.0 gameplay regressions remain valid on Medium");
    test_difficulty_input();test_movement_and_hint_profiles();test_director_profiles();
    test_diversity_certificates();test_all_profile_simulation_invariants();
    std::cout<<"PASS: "<<checks<<" assertions including all difficulty profiles, input, timing, hints, diversity, and determinism.\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
