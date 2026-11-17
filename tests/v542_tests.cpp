// V5.4.2 feature regressions: start grace, hearing, searching, interceptors,
// corridor sight, timing-aware solver blocking, generation fallback, guards.
#include "visual_tests.cpp"

// Long single corridor: row 1 from the entrance to column 29, then down column
// 29 to the exit. Every interior cell is a chokepoint.
Maze corridor_maze(Difficulty d=Difficulty::Medium){
    Maze m(31,31,7,nullptr,d);
    for(int c=1;c<=29;++c)m.grid[m.id({1,c})]=PATH;
    for(int r=1;r<=29;++r)m.grid[m.id({r,29})]=PATH;
    m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();return m;
}

void test_start_grace(){
    for(auto d:{Difficulty::Low,Difficulty::Medium,Difficulty::High}){
        Maze m(33,33,11,nullptr,d);m.generate(10);
        std::vector<int> cells;for(const auto& x:m.monsters)cells.push_back(x.cell);
        m.advance_to(30000);
        require(m.outcome==Outcome::Playing&&!m.timer_started&&m.elapsed_ms==0,"idle player at the entrance is never caught before starting");
        for(std::size_t i=0;i<cells.size();++i)require(m.monsters[i].cell==cells[i],"monsters stay frozen before the first input");
        m.set_auto_solve(true);require(m.timer_started,"auto-solve also starts the game");
        m.advance_to(m.now+1000);
        int moved=0;for(std::size_t i=0;i<cells.size();++i)moved+=m.monsters[i].cell!=cells[i];
        require(moved>0,"monsters move once released");
    }
}

void test_hearing_and_per_monster_scent(){
    auto m=open_maze(41,41);m.player=m.id({20,20});
    m.monsters={monster(0,m.id({20,10}),NEVER),monster(1,m.id({1,1}),NEVER)};
    const int radius=m.hearing_radius();
    require(radius==m.settings.hearing_cells+m.settings.hearing_per_100*41/100,"hearing radius scales with map size");
    Millis last_heard=-1;
    for(int i=0;i<16;++i){
        m.advance_to(i*100);m.move_player(0,1);
        if(m.manhattan(m.monsters[0].cell,m.player)<=radius)last_heard=m.now;
    }
    require(last_heard>0&&last_heard<1500,"player walked out of the first monster's hearing range");
    require(m.monsters[1].state==MonsterState::Patrolling,"far monster never hears");
    m.advance_to(last_heard+m.settings.scent_ms-1);
    require(m.monsters[0].state==MonsterState::Hunting,"hunt persists until this monster's own scent expires");
    // Keep moving out of range: the player is active but this monster cannot hear.
    for(Millis t=m.now+50;t<last_heard+m.settings.scent_ms;t+=50){m.advance_to(t);m.move_player(t%100?1:-1,0);}
    m.advance_to(last_heard+m.settings.scent_ms);
    require(m.monsters[0].state==MonsterState::Searching,"a monster that stops hearing starts searching while the player keeps moving");
    m.monsters[0].next_move=m.now+1;m.advance_to(m.now+1);
    require(m.monsters[0].hint.contains(m.point(m.monsters[0].target)),"search targets stay inside the last clue area");
}

void test_interceptor_assignment(){
    // Open room (rows 1-13, cols 1-9) whose only way out is a corridor to the exit.
    auto build=[](int player_col){
        Maze m(15,31,3,nullptr,Difficulty::Medium);
        for(int r=1;r<=13;++r)for(int c=1;c<=9;++c)m.grid[m.id({r,c})]=PATH;
        for(int c=10;c<=29;++c)m.grid[m.id({7,c})]=PATH;
        for(int r=7;r<=13;++r)m.grid[m.id({r,29})]=PATH;
        m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();
        m.player=m.id({7,player_col});m.monsters={monster(0,m.id({7,20}),NEVER),monster(1,m.id({11,6}),NEVER)};
        m.move_player(0,1);return m;
    };
    auto a=build(2),b=build(3);
    require(a.monsters[0].hint.min_col==b.monsters[0].hint.min_col,"fixtures share one clue area");
    const auto& cutter=a.monsters[0];
    require(cutter.state==MonsterState::Hunting&&cutter.role==MonsterRole::Cutter,"corridor monster is sent to intercept");
    Point t=a.point(cutter.target);
    require(t.r==7&&t.c>=10&&t.c<=20,"interceptor waits at a chokepoint between the clue and the exit");
    require(!cutter.hint.contains(t),"interception point lies outside the clue area");
    require(a.monsters[1].role==MonsterRole::Chaser,"Medium sends at most one interceptor");
    require(a.monsters[0].target==b.monsters[0].target,"interception uses the clue area, not the exact player cell");
    Maze low(15,31,3,nullptr,Difficulty::Low);
    require(low.settings.cutters==0,"Low never intercepts");
}

void test_corridor_sight(){
    auto setup=[](Difficulty d,int monster_col,bool wall){
        Maze s(21,21,123,nullptr,d);
        for(int r=1;r<s.rows-1;++r)for(int c=1;c<s.cols-1;++c)s.grid[s.id({r,c})]=PATH;
        s.grid[s.entrance]=PATH;s.grid[s.exit_cell]=EXIT;
        if(wall)s.grid[s.id({10,12})]=WALL;
        s.rebuild_graph();s.reset_game();s.player=s.id({10,10});
        auto x=monster(0,s.id({10,monster_col}),10);x.target=s.id({1,1});x.next_replan=NEVER;s.monsters={x};
        s.set_auto_solve(true);s.set_auto_solve(false);  // start without moving: nobody hears anything
        s.advance_to(10);return s;
    };
    auto seen=setup(Difficulty::High,14,false);
    require(seen.monsters[0].sighted&&seen.monsters[0].state==MonsterState::Hunting,"High monster spots the player down a corridor");
    require(seen.monsters[0].target==seen.player||seen.monsters[0].cell==seen.player,"spotting gives the exact cell");
    require(!setup(Difficulty::High,12,false).monsters[0].sighted,"close quarters do not trigger sight");
    require(!setup(Difficulty::High,14,true).monsters[0].sighted,"walls block sight");
    require(!setup(Difficulty::Medium,14,false).monsters[0].sighted,"Medium has no corridor sight");
    // A hunter already following clues does not switch to exact tracking.
    Maze h(21,21,123,nullptr,Difficulty::High);
    for(int r=1;r<h.rows-1;++r)for(int c=1;c<h.cols-1;++c)h.grid[h.id({r,c})]=PATH;
    h.grid[h.entrance]=PATH;h.grid[h.exit_cell]=EXIT;h.rebuild_graph();h.reset_game();
    h.player=h.id({10,9});h.monsters={monster(0,h.id({10,14}),NEVER)};h.move_player(0,1);
    require(h.monsters[0].state==MonsterState::Hunting,"fixture monster heard the move");
    h.monsters[0].next_move=h.now+1;h.advance_to(h.now+1);
    require(!h.monsters[0].sighted,"sight is a discovery mechanic, not continuous tracking");
}

void test_timing_aware_blocking(){
    auto far=corridor_maze();far.player=far.id({1,2});
    auto x=monster(0,far.id({20,29}),NEVER);far.monsters={x};
    far.set_auto_solve(true);
    require(!far.overlay_avoids_monsters,"route through a distant monster is shown in amber");
    far.advance_to(40);
    require(far.automatic_steps==1&&far.player==far.id({1,3}),"a distant monster does not freeze the solver");
    auto near=corridor_maze();near.player=near.id({1,2});
    near.monsters={monster(0,near.id({1,10}),NEVER)};near.set_auto_solve(true);near.advance_to(40);
    require(near.automatic_steps==0,"a nearby monster blocking the only route makes the solver wait");
    require(near.auto_boldness()==0,"fresh solver has no loop escalation");
    auto parked=corridor_maze();parked.player=parked.id({1,2});
    parked.monsters={monster(0,parked.id({20,29}),NEVER)};parked.set_auto_solve(true);
    for(Millis t=40;t<=AUTO_STATIONARY_MS+200;t+=40)parked.advance_to(t);
    require(parked.player!=parked.id({1,2}),"solver advanced toward the distant monster");
    int stop=parked.player;parked.advance_to(parked.now+400);
    require(parked.manhattan(stop,parked.player)<=2,"a parked monster becomes a wall even at a distance");
}

void test_generation_fallback_and_guards(){
    Maze single(9,9,0,nullptr,Difficulty::Low);
    for(int r=1;r<=7;++r)single.grid[single.id({r,1})]=PATH;
    for(int c=1;c<=7;++c)single.grid[single.id({7,c})]=PATH;
    single.grid[single.entrance]=PATH;single.grid[single.exit_cell]=EXIT;single.rebuild_graph();
    require(!single.route_diversity_met(),"fixture fails the Low requirement");
    single.add_connections_until_diverse();
    require(single.route_diversity_met(),"fallback always reaches the profile requirement");
    for(int r=0;r<single.rows;++r)for(int c=0;c<single.cols;++c)
        if(r==0||c==0||r==single.rows-1||c==single.cols-1){
            int u=single.id({r,c});
            require(u==single.entrance||u==single.exit_cell||single.grid[u]==WALL,"fallback keeps the outer wall");
        }
    require(single.grid[single.exit_cell]==EXIT,"fallback preserves the exit");
#ifndef NDEBUG
    auto m=open_maze();m.grid[m.id({5,5})]=WALL;bool threw=false;
    try{m.advance_to(10);}catch(const std::logic_error&){threw=true;}
    require(threw,"terrain edits without rebuild_graph() are detected");
    m.rebuild_graph();m.advance_to(10);
#endif
    bool range=false;
    try{parse_number("99999999999999999999999",1000);}catch(const std::runtime_error& e){range=std::string(e.what()).find("out of range")!=std::string::npos;}
    require(range,"huge numbers report a readable range error");
}

void test_bounded_threat_fields(){
    for(unsigned seed:{1u,5u,9u}){
        Maze m(61,61,seed,nullptr);m.generate(0);
        for(int source:{m.entrance,m.exit_cell,m.id({31,31})}){
            if(!m.walkable(source))continue;
            auto field=m.local_field(source);
            for(int u=0;u<static_cast<int>(m.grid.size());++u){
                if(!m.walkable(u))continue;
                int exact=bfs(m,source,u);
                if(exact<=THREAT_RADIUS)require(field->at(u)==exact,"bounded field is exact within its radius");
                else require(field->at(u)==THREAT_RADIUS+1&&field->at(u)<=exact,"bounded field is a lower bound beyond its radius");
            }
        }
    }
}

void test_searching_eyes(){
    for(int id=0;id<MAX_MONSTERS;++id){
        const auto& sprite=*monster_appearance(id).sprite;int eyes=0;
        for(auto row:sprite)for(char pixel:row)eyes+=pixel=='E';
        std::vector<Vertex> calm,searching;
        append_sprite(calm,{5,5},sprite);append_sprite(searching,{5,5},sprite,false,false,0.0f,true);
        int changed=0;for(std::size_t i=0;i<calm.size();++i)changed+=calm[i].g!=searching[i].g;
        require(calm.size()==searching.size()&&changed==eyes*6,"searching changes only the eye pixels");
    }
}

int v542_regression_main(){try{
    require(visual_regression_main()==0,"visual and earlier regression suites");
    test_start_grace();test_hearing_and_per_monster_scent();test_interceptor_assignment();
    test_corridor_sight();test_timing_aware_blocking();test_generation_fallback_and_guards();
    test_bounded_threat_fields();test_searching_eyes();
    std::cout<<"PASS: V5.4.2 start grace, hearing, searching, interceptors, sight, timing-aware blocking, fallback, and guards.\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
