#define MAZERUNNER_HEADLESS
#define MAZERUNNER_NO_MAIN
#include "../src/mazerunnerV5.4.3c.cpp"
#include <set>

int checks=0;
void require(bool value,const char* label){++checks;if(!value)throw std::runtime_error(std::string("FAILED: ")+label);}
Maze open_maze(int rows=31,int cols=31,std::ostream* logs=nullptr){
    Maze m(rows,cols,123,logs);
    for(int r=1;r<m.rows-1;++r)for(int c=1;c<m.cols-1;++c)m.grid[m.id({r,c})]=PATH;
    m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();return m;
}
Monster monster(int id,int cell,Millis next=140){Monster m;m.id=id;m.cell=cell;m.next_move=next;return m;}
// Exact unit-cost A* used only as an independent oracle (moved out of the game in V5.4.2).
bool astar(const Maze& m,int start,int goal,std::vector<int>& path,const std::vector<int>& blocked={}){
    path.clear();
    auto occupied=[&](int u){return std::find(blocked.begin(),blocked.end(),u)!=blocked.end();};
    if(!m.walkable(start)||!m.walkable(goal)||occupied(start)||occupied(goal))return false;
    struct Node{int u,g,h;bool operator<(const Node&b)const{if(g+h!=b.g+b.h)return g+h>b.g+b.h;if(h!=b.h)return h>b.h;return u>b.u;}};
    std::vector<int> g(m.grid.size(),INF_DISTANCE),parent(m.grid.size(),-1);std::priority_queue<Node> open;
    g[start]=0;open.push({start,0,m.manhattan(start,goal)});
    while(!open.empty()){
        Node current=open.top();open.pop();if(current.g!=g[current.u])continue;
        if(current.u==goal){for(int u=goal;u>=0;u=parent[u])path.push_back(u);std::reverse(path.begin(),path.end());return true;}
        for(int v:m.neighbors(current.u)){if(v<0||occupied(v))continue;int cost=current.g+1;if(g[v]<=cost)continue;
            g[v]=cost;parent[v]=current.u;open.push({v,cost,m.manhattan(v,goal)});}
    }
    return false;
}
void check_overlay(const Maze& m){
    if(!m.auto_solve||m.outcome!=Outcome::Playing)return;
    require(!m.solution_overlay.empty(),"full overlay exists while playing");
    require(m.id(m.solution_overlay.front())==m.player,"overlay starts at current player");
    require(m.id(m.solution_overlay.back())==m.exit_cell,"overlay ends at exit");
    std::set<int> route;
    for(size_t i=0;i<m.solution_overlay.size();++i){
        int u=m.id(m.solution_overlay[i]);require(m.walkable(u),"overlay follows terrain");
        require(route.insert(u).second,"overlay contains no cycles");
        if(i)require(m.manhattan(u,m.id(m.solution_overlay[i-1]))==1,"overlay cardinal continuity");
        if(m.overlay_avoids_monsters)for(const auto& x:m.monsters)require(x.cell!=u,"clear overlay avoids all monsters");
    }
}
void test_movement_contract(){
    auto m=open_maze();m.player=m.id({15,15});m.monsters={monster(0,m.id({11,13}),10),monster(1,m.id({1,1}),100)};
    auto first=m.monsters[0].cell;m.advance_to(300);
    require(m.monsters[0].cell==first&&m.monsters[1].cell==m.id({1,1}),"monsters wait for the player's first input");
    require(!m.timer_started&&m.elapsed_ms==0,"waiting at the entrance is not timed");
    for(const auto& x:m.monsters)require(x.state==MonsterState::Patrolling&&x.hint.revision==0,"spawn has no player knowledge");
    require(m.move_player(0,1),"successful manual displacement");
    require(m.monsters[0].next_move==310&&m.monsters[1].next_move==400,"release keeps spawn staggering relative to first input");
    const auto& near=m.monsters[0];
    require(m.manhattan(near.cell,m.player)<=m.hearing_radius(),"fixture monster is within hearing");
    require(near.state==MonsterState::Hunting,"monster within hearing range hunts");
    require(near.hint.revision==1&&near.hint.contains(m.point(m.player)),"hint includes player region");
    require(near.hint.max_row-near.hint.min_row==4&&near.hint.max_col-near.hint.min_col==4,"hint gives 5x5 region");
    require(near.planned_revision==0,"hint remains pending until own move");
    require(m.manhattan(m.monsters[1].cell,m.player)>m.hearing_radius(),"fixture monster is out of hearing");
    require(m.monsters[1].state==MonsterState::Searching&&m.monsters[1].scent_until==NEVER&&m.monsters[1].hint.contains(m.point(m.player)),
            "monster beyond hearing receives an actionable director sector without scent");
    for(auto& x:m.monsters)x.next_move=NEVER;
    const Millis scent=m.settings.scent_ms,search=m.settings.search_ms;
    m.advance_to(300+scent-1);require(m.monsters[0].state==MonsterState::Hunting,"hunt stays active until timeout");
    m.advance_to(300+scent);
    require(m.monsters[0].state==MonsterState::Searching&&m.monsters[0].hint.revision==1,"exact scent deadline starts a search of the last clue area");
    m.advance_to(300+scent+search-1);require(m.monsters[0].state==MonsterState::Searching,"search lasts the profile duration");
    m.advance_to(300+scent+search);
    require(m.monsters[0].state==MonsterState::Patrolling&&m.monsters[0].hint.revision==0,"search ends at its deadline");
    Millis t=m.now;require(m.move_player(0,-1),"resume movement");
    require(m.monsters[0].state==MonsterState::Hunting&&m.monsters[0].hint.revision==2,"new movement resumes hunt");
    m.advance_to(t+40);m.move_player(-1,0);m.advance_to(t+40+scent-1);
    require(m.monsters[0].state==MonsterState::Hunting,"fresh movement extends timeout");
    m.advance_to(t+40+scent);require(m.monsters[0].state==MonsterState::Searching,"extended timeout eventually expires");
    require(m.monsters[1].state!=MonsterState::Hunting&&m.monsters[1].scent_until==NEVER,"radio never grants physical hearing");
}
void test_no_exact_location_leak(){
    auto a=open_maze(),b=open_maze();a.player=a.id({11,11});b.player=b.id({13,13});
    for(auto* m:{&a,&b})m->monsters={monster(0,m->id({5,5}),10),monster(1,m->id({5,7}),100)};
    a.move_player(0,1);b.move_player(0,1);
    for(auto* m:{&a,&b})for(const auto& x:m->monsters)require(x.state==MonsterState::Hunting,"both monsters hear in both games");
    require(a.player!=b.player,"players at different cells");
    a.advance_to(500);b.advance_to(500);
    for(size_t i=0;i<a.monsters.size();++i){
        const auto& x=a.monsters[i];const auto& y=b.monsters[i];
        require(x.hint.min_row==y.hint.min_row&&x.hint.min_col==y.hint.min_col,"same coarse region");
        require(x.target==y.target&&x.cell==y.cell,"same hint produces identical monster decisions despite different exact player position");
        require(x.hint.contains(a.point(x.target)),"hunting target stays inside hinted area");
        require(x.planned_revision==x.hint.revision,"staggered monster consumed latest hint");
    }
    auto edge=open_maze(31,31);edge.player=edge.id({29,29});edge.monsters={monster(0,edge.id({25,25}),NEVER)};
    edge.move_player(0,1);const auto& hint=edge.monsters[0].hint;
    require(hint.max_col-hint.min_col==4&&hint.max_row-hint.min_row==4,"edge hint keeps full uncertainty area");
    require(hint.contains(edge.point(edge.player)),"edge region includes player");
    auto hidden=open_maze(41,41);hidden.player=hidden.id({37,37});
    hidden.grid[hidden.id({20,20})]=WALL;hidden.rebuild_graph();
    hidden.monsters={monster(0,hidden.id({1,1}),10),monster(1,hidden.id({33,33}),10)};
    hidden.move_player(0,1);
    require(hidden.monsters[0].state==MonsterState::Searching&&hidden.monsters[0].scent_until==NEVER,"far monster has only a radio clue");
    require(hidden.monsters[1].state==MonsterState::Hunting,"nearby monster hears along an open path");
}
void test_waits_and_invalid_moves(){
    auto m=open_maze();m.player=m.id({11,11});m.monsters={monster(0,m.id({5,5}),NEVER)};
    m.move_player(0,1);int revision=static_cast<int>(m.monsters[0].hint.revision);
    require(revision==1,"fixture monster heard the move");
    for(int v:m.neighbors(m.player))if(v>=0)m.grid[v]=WALL;
    m.rebuild_graph();m.advance_to(100);
    require(!m.move_player(0,1),"blocked movement rejected");
    require(static_cast<int>(m.monsters[0].hint.revision)==revision,"blocked input does not broadcast");
    m.set_auto_solve(true);m.advance_to(560);
    require(m.automatic_steps==0,"auto only waited");
    require(m.monsters[0].state!=MonsterState::Hunting,"auto waits do not keep hunt alive");
    require(m.timer_started&&m.elapsed_ms==560,"waiting timer continues");
}
void test_complete_overlay(){
    auto m=open_maze();m.set_auto_solve(true);check_overlay(m);
    require(m.solution_overlay.size()>static_cast<size_t>(AUTO_HORIZON+1),"overlay exceeds tactical horizon");
    for(int t=40;t<=400;t+=40){m.advance_to(t);check_overlay(m);}
    m.set_auto_solve(false);require(m.solution_overlay.empty(),"toggle off clears overlay");
    Maze detour(9,31,0,nullptr);std::fill(detour.grid.begin(),detour.grid.end(),WALL);
    for(int c=1;c<=29;++c){detour.grid[detour.id({1,c})]=PATH;detour.grid[detour.id({7,c})]=PATH;}
    for(int r=1;r<=7;++r){detour.grid[detour.id({r,1})]=PATH;detour.grid[detour.id({r,29})]=PATH;}
    detour.grid[detour.id({2,20})]=PATH;detour.grid[detour.entrance]=PATH;detour.grid[detour.exit_cell]=EXIT;
    detour.rebuild_graph();detour.player=detour.id({1,3});
    auto x=monster(0,detour.id({2,20}),10);x.target=detour.id({1,20});x.next_replan=NEVER;
    detour.monsters={x};detour.set_auto_solve(true);check_overlay(detour);
    require(detour.solution_overlay[1]==Point{1,4},"initial full route uses shorter upper passage");
    auto before=detour.player;detour.advance_to(10);check_overlay(detour);
    require(detour.player==before,"player has not moved before monster event");
    require(detour.monsters[0].cell==detour.id({1,20}),"monster entered upper route");
    require(detour.solution_overlay[1]==Point{1,2},"monster-only movement immediately replans entire overlay");
    auto blocked=open_maze();blocked.monsters={monster(0,blocked.exit_cell,NEVER)};blocked.set_auto_solve(true);
    check_overlay(blocked);require(!blocked.overlay_avoids_monsters,"blocked goal explicitly marks fallback overlay");
    require(blocked.solution_overlay.size()>static_cast<size_t>(AUTO_HORIZON+1),"blocked fallback still reaches exit");
}
int bfs(const Maze&m,int start,int goal,const std::vector<int>&blocked={}){
    if(std::find(blocked.begin(),blocked.end(),start)!=blocked.end())return -1;
    std::vector<int>d(m.grid.size(),-1);std::queue<int>q;q.push(start);d[start]=0;
    while(!q.empty()){int u=q.front();q.pop();if(u==goal)return d[u];for(int v:m.neighbors(u))
        if(v>=0&&d[v]<0&&std::find(blocked.begin(),blocked.end(),v)==blocked.end()){d[v]=d[u]+1;q.push(v);}}
    return -1;
}
std::string snapshot(const Maze&m){std::ostringstream s;s<<m.player<<' '<<m.now<<' '<<int(m.outcome)<<' '<<m.automatic_steps;
    for(const auto&x:m.monsters)s<<' '<<x.cell<<' '<<x.target<<' '<<int(x.state)<<' '<<x.next_move<<' '<<x.hint.revision<<' '<<x.planned_revision;
    return s.str();}
// Independent witness check: remove an edge from one path and look for another.
bool independent_two_paths(Maze& m){
    std::vector<int> path;if(!astar(m,m.entrance,m.exit_cell,path))return false;
    for(size_t cut=1;cut<path.size();++cut){
        int a=path[cut-1],b=path[cut];std::vector<unsigned char>seen(m.grid.size(),0);std::queue<int>q;
        q.push(m.entrance);seen[m.entrance]=1;
        while(!q.empty()){
            int u=q.front();q.pop();if(u==m.exit_cell)return true;
            for(int v:m.neighbors(u))if(v>=0&&!seen[v]&&!((u==a&&v==b)||(u==b&&v==a))){seen[v]=1;q.push(v);}
        }
    }return false;
}
void test_two_solution_generation(){
    Maze single(9,9,0,nullptr);
    for(int r=1;r<=7;++r)single.grid[single.id({r,1})]=PATH;
    for(int c=1;c<=7;++c)single.grid[single.id({7,c})]=PATH;
    single.grid[single.entrance]=PATH;single.grid[single.exit_cell]=EXIT;single.rebuild_graph();
    require(!single.has_at_least_two_solutions(),"single corridor rejected");
    for(Point p:std::vector<Point>{{3,2},{3,3},{3,4},{3,5},{4,4},{4,5}})single.grid[single.id(p)]=PATH;
    single.rebuild_graph();
    require(!single.has_at_least_two_solutions(),"off-route cycle does not create second solution");
    require(!independent_two_paths(single),"independent witness also rejects side cycle");
    for(int c=1;c<=7;++c)single.grid[single.id({1,c})]=PATH;
    for(int r=1;r<=7;++r)single.grid[single.id({r,7})]=PATH;
    single.rebuild_graph();require(single.has_at_least_two_solutions()&&independent_two_paths(single),"actual alternate route accepted");
    single.grid[single.exit_cell]=WALL;single.rebuild_graph();require(!single.has_at_least_two_solutions(),"unreachable exit rejected");
    bool retried=false;
    for(unsigned seed=0;seed<80;++seed){
        std::ostringstream log;Maze m(5+seed%15,5+seed%17,seed,&log);m.generate(0);
        require(m.has_at_least_two_solutions(),"generated maze passes bridge check");
        require(independent_two_paths(m),"generated maze has independent alternate-path witness");
        retried=retried || log.str().find("regenerating")!=std::string::npos;
        require(log.str().find("TWO distinct")!=std::string::npos,"generation validation logged");
    }
    require(retried,"regeneration branch exercised across fixed seeds");
    Maze large(501,501,42,nullptr);large.generate(10);require(large.has_at_least_two_solutions(),"maximum size validated without recursion");
}
void test_wait_for_scent_loss(){
    std::ostringstream log;auto m=open_maze(31,31,&log);m.monsters={monster(0,m.exit_cell,NEVER)};
    m.player=m.id({27,27});m.move_player(0,1);int position=m.player;m.set_auto_solve(true);m.advance_to(560);
    require(m.player==position&&m.automatic_steps==0,"route blocked by a nearby monster prefers standing still");
    require(m.monsters[0].state!=MonsterState::Hunting,"waiting lets hunting scent expire");
    require(m.elapsed_ms==560,"waiting preserves elapsed clock");check_overlay(m);
    require(!m.overlay_avoids_monsters,"waiting retains full blocked overlay");
    require(log.str().find("standing still so monsters lose")!=std::string::npos,"waiting reason logged");
    m.monsters.clear();m.advance_to(600);require(m.player!=position,"auto resumes when blocking monster clears");check_overlay(m);
    auto danger=open_maze();danger.player=danger.id({11,11});
    auto approaching=monster(0,danger.id({11,12}),10);approaching.target=danger.player;approaching.next_replan=NEVER;
    danger.monsters={approaching,monster(1,danger.exit_cell,NEVER)};
    auto route=danger.plan_auto_route();require(route.size()>1&&route[1]!=danger.player,"imminent capture overrides waiting");
}
void test_regressions(){
    for(unsigned seed=0;seed<30;++seed){
        Maze m(9+seed%15,9+seed%17,seed,nullptr);m.generate(3);
        std::vector<int>blocked;for(auto x:m.monsters)blocked.push_back(x.cell);
        for(auto obstacles:std::vector<std::vector<int>>{{},blocked}){
            std::vector<int>path;bool found=astar(m,m.player,m.exit_cell,path,obstacles);int d=bfs(m,m.player,m.exit_cell,obstacles);
            require(found==(d>=0),"A* reachability matches independent BFS");if(found)require(path.size()==static_cast<size_t>(d+1),"A* length optimal");
        }
        m.monsters.clear();int d=bfs(m,m.player,m.exit_cell);m.set_auto_solve(true);check_overlay(m);m.advance_to(100000);
        require(m.outcome==Outcome::Won&&m.automatic_steps==d,"no-monster optimal escape");
        require(m.elapsed_ms==d*AUTO_SOLVE_SPEED_MS,"timer final sample exact");auto time=m.elapsed_ms;m.advance_to(200000);require(m.elapsed_ms==time,"final time frozen");
    }
    auto m=open_maze();m.player=m.id({29,29});m.monsters={monster(0,m.exit_cell,NEVER)};m.move_player(0,1);
    require(m.outcome==Outcome::Caught,"occupied exit remains fatal");
    for(unsigned seed=0;seed<20;++seed){
        Maze a(21,25,seed,nullptr),b(21,25,seed,nullptr);a.generate(5);b.generate(5);a.set_auto_solve(true);b.set_auto_solve(true);
        for(Millis time=0;time<=10000&&a.outcome==Outcome::Playing;time+=20){
            a.advance_to(time);std::set<int>occupied;for(auto x:a.monsters){require(a.walkable(x.cell),"monster on walkable terrain");require(occupied.insert(x.cell).second,"no monster stacking");}
            require(a.walkable(a.player),"player on walkable terrain");check_overlay(a);
        }
        b.advance_to(10000);require(snapshot(a)==snapshot(b),"frame partition does not affect deterministic state");
    }
}
void test_logs(){
    std::ostringstream logs;Maze m(31,31,42,&logs);m.generate(2);
    for(int u=0;u<static_cast<int>(m.grid.size());++u)
        if(m.walkable(u)&&m.manhattan(u,m.id({1,1}))>=3&&m.manhattan(u,m.id({1,1}))<=8){m.monsters[0].cell=u;break;}
    m.move_player(0,1);m.advance_to(560);
    auto solo=open_maze(15,15,&logs);solo.set_auto_solve(true);solo.advance_to(200);solo.set_auto_solve(false);
    auto quiet=open_maze(15,15,&logs);quiet.player=quiet.id({7,7});quiet.monsters={monster(0,quiet.id({7,10}),NEVER)};
    quiet.move_player(0,1);quiet.advance_to(quiet.settings.scent_ms+quiet.settings.search_ms);
    auto text=logs.str();
    for(const char* marker:{"spawned at","Player started | Monsters released","Player MANUAL","Movement hint #","Heard by","ROAMING -> HUNTING","HUNTING -> SEARCHING","SEARCHING -> ROAMING","area target","Auto-solve activated","Full path to exit updated","Player AUTO","Auto-solve deactivated"})
        require(text.find(marker)!=std::string::npos,marker);
    require(text.find('\r')==std::string::npos,"logs append without carriage-return overwrites");
    m.log_status();require(logs.str().find("Manual steps:")!=std::string::npos,"player statistics logged");
    auto default_logging=open_maze(9,9,&logs);require(default_logging.log_output!=nullptr,"log stream supported");
}
int main(){try{test_movement_contract();test_no_exact_location_leak();test_waits_and_invalid_moves();test_complete_overlay();test_two_solution_generation();test_wait_for_scent_loss();test_regressions();test_logs();std::cout<<"PASS: "<<checks<<" checks; movement hints, idle transitions, complete overlays, scent-loss waiting, two-solution generation, and logs.\n";return 0;}
catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
