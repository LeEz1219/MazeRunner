#include "difficulty_tests.cpp"

void test_geometry_and_projection(){
    for(auto dims:std::vector<std::pair<int,int>>{{5,5},{13,19},{21,31},{5,101},{101,5},{125,125}}){
        Maze m(dims.first,dims.second,42,nullptr);m.generate(3);
        auto mesh=build_wall_mesh(m);require(!mesh.empty()&&mesh.size()%6==0,"walls consist of quad triangles");
        bool base=false,raised=false;
        for(const auto&v:mesh){
            require(v.x>=0&&v.x<=m.cols&&v.y>=0&&v.y<=m.rows,"wall stays inside maze bounds");
            require(v.z>=0&&v.z<=WALL_HEIGHT,"wall height is bounded");
            require(v.r==v.g&&v.g==v.b,"walls are neutral gray");base=base||v.z==0;raised=raised||v.z==WALL_HEIGHT;
        }
        require(base&&raised,"wall mesh has real height and vertical sides");
        for(auto viewport:std::vector<std::pair<int,int>>{{1200,900},{1600,600},{600,1000}}){
            auto a=scene_projection(m,viewport.first,viewport.second);
            require(std::abs(a[0]*viewport.first-a[9]*viewport.second/VIEW_SIN)<0.01,"horizontal and camera-vertical scales preserve aspect");
            for(const auto&v:mesh){
                float x=a[0]*v.x+a[12],y=a[5]*v.y+a[9]*v.z+a[13],z=a[6]*v.y+a[10]*v.z+a[14];
                require(std::abs(x)<=1&&std::abs(y)<=1&&std::abs(z)<1,"full view fits every wall in clip volume");
            }
        }
        std::vector<Vertex> actors;
        std::vector<const PixelSprite*> sprites{&KNIGHT_SPRITE,&EXIT_SPRITE};
        for(const auto& appearance:MONSTER_APPEARANCES) sprites.push_back(appearance.sprite);
        for(const auto*s:sprites){
            actors.clear();append_sprite(actors,m.point(m.player),*s);
            require(actors.size()>100&&actors.size()<20*20*6,"sprite is detailed and has transparent pixels");
            for(const auto&v:actors)require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"sprite coordinates finite");
        }
        auto expected=m;std::vector<Vertex> route;m.set_auto_solve(true);append_route(route,m);
        require(route.size()==(m.solution_overlay.size()-1)*12,"every full-route segment receives glow and center line");
        expected.set_auto_solve(true);m.advance_to(5000);expected.advance_to(5000);
        require(snapshot(m)==snapshot(expected),"mesh and sprite generation consume no gameplay RNG");
    }
    auto m=fixture(Difficulty::Low);bool threw=false;
    try{scene_projection(m,0,100);}catch(const std::invalid_argument&){threw=true;}
    require(threw,"zero-size projection rejected before division");
}

void test_distinct_monster_silhouettes(){
    require(MONSTER_APPEARANCES.size()>=MAX_MONSTERS,"every supported monster has its own variant");
    std::set<std::string> silhouettes,names;
    for(int id=0;id<MAX_MONSTERS;++id){
        const auto& appearance=monster_appearance(id);
        std::string silhouette;int eyes=0;
        for(auto row:*appearance.sprite)for(char pixel:row){silhouette+=pixel=='.'?'.':'#';eyes+=pixel=='E';}
        require(silhouettes.insert(silhouette).second,"each spawn has a different silhouette, not just a recolor");
        require(names.insert(std::string(appearance.name)).second,"each variant has a unique name");
        require(eyes>0,"all variants retain visible patrol/hunting eyes");
        std::vector<Vertex> normal,mirrored,hunting;
        append_sprite(normal,{5,5},*appearance.sprite);
        append_sprite(mirrored,{5,5},*appearance.sprite,true);
        append_sprite(hunting,{5,5},*appearance.sprite,false,true);
        require(normal.size()==mirrored.size()&&normal.size()==hunting.size(),"facing and hunt state preserve silhouette geometry");
        int changed=0;
        for(std::size_t i=0;i<normal.size();++i)changed+=normal[i].g!=hunting[i].g;
        require(changed==eyes*6,"hunting changes only the eye pixels");
    }
    for(auto difficulty:{Difficulty::Low,Difficulty::Medium,Difficulty::High}){
        Maze m(21,31,42,nullptr,difficulty);m.generate(MAX_MONSTERS);
        std::set<const PixelSprite*> appearances;
        for(const auto& monster:m.monsters)appearances.insert(monster_appearance(monster.id).sprite);
        require(m.monsters.size()==MAX_MONSTERS&&appearances.size()==MAX_MONSTERS,"ten-monster games spawn ten different appearances");
    }
}

int visual_regression_main(){try{
    require(difficulty_regression_main()==0,"difficulty and AI regression suite");
    test_geometry_and_projection();
    test_distinct_monster_silhouettes();
    std::cout<<"PASS: visual geometry and camera invariants plus all gameplay/difficulty regressions.\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
