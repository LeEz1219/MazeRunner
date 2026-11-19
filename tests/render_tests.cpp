#define MAZERUNNER_NO_MAIN
#define MAZERUNNER_RENDER_TEST
#include "../src/mazerunnerV5.4.3c.cpp"
#include <fstream>
extern "C" {
void* eglGetPlatformDisplay(unsigned,void*,const std::intptr_t*);
unsigned eglInitialize(void*,int*,int*);unsigned eglBindAPI(unsigned);
unsigned eglChooseConfig(void*,const int*,void**,int,int*);
unsigned eglGetConfigAttrib(void*,void*,int,int*);
void* eglCreatePbufferSurface(void*,void*,const int*);void* eglCreateContext(void*,void*,void*,const int*);
unsigned eglMakeCurrent(void*,void*,void*,void*);unsigned eglDestroyContext(void*,void*);
unsigned eglDestroySurface(void*,void*);unsigned eglTerminate(void*);
}
void require(bool value,const char*label){if(!value)throw std::runtime_error(label);}
struct Offscreen {
    void* display=nullptr;void* surface=nullptr;void* context=nullptr;
    Offscreen(int width,int height){
        display=eglGetPlatformDisplay(0x31DD,nullptr,nullptr);int major=0,minor=0;
        require(eglInitialize(display,&major,&minor),"EGL initialization failed");
        require(eglBindAPI(0x30A2),"OpenGL binding failed");
        const int attributes[]={0x3033,1,0x3040,8,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3025,24,0x3038};
        void* config=nullptr;int count=0;
        require(eglChooseConfig(display,attributes,&config,1,&count)&&count==1,"EGL config failed");
        const int size[]={0x3057,width,0x3056,height,0x3038};
        surface=eglCreatePbufferSurface(display,config,size);
        const int settings[]={0x3098,3,0x30FB,3,0x30FD,1,0x3038};
        context=eglCreateContext(display,config,nullptr,settings);
        require(surface&&context&&eglMakeCurrent(display,surface,surface,context),"EGL context failed");
        int bits=0;require(eglGetConfigAttrib(display,config,0x3025,&bits)&&bits>=24,"Need actual depth buffer");
    }
    ~Offscreen(){eglMakeCurrent(display,nullptr,nullptr,nullptr);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);}
};
Maze showcase(){
    Maze m(13,19,42,nullptr);m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;
    for(int r:{1,5,9,11})for(int c=1;c<=17;++c)m.grid[m.id({r,c})]=PATH;
    for(int c:{1,5,9,13,17})for(int r=1;r<=11;++r)m.grid[m.id({r,c})]=PATH;
    for(int c=5;c<=9;++c)m.grid[m.id({3,c})]=PATH;
    for(int c=9;c<=17;++c)m.grid[m.id({7,c})]=PATH;
    m.rebuild_graph();m.player=m.id({5,5});
    for(Point p:std::vector<Point>{{5,9},{5,13},{9,13}}){Monster x;x.id=int(m.monsters.size());x.cell=m.id(p);x.next_move=NEVER;x.state=MonsterState::Hunting;m.monsters.push_back(x);}
    m.set_auto_solve(true);m.now=250;return m;
}
Maze monster_gallery(){
    Maze m(9,15,42,nullptr);m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;
    for(int r:{1,3,5,7})for(int c=1;c<=13;++c)m.grid[m.id({r,c})]=PATH;
    for(int c:{1,3,5,7,9,11,13})for(int r=1;r<=7;++r)m.grid[m.id({r,c})]=PATH;
    m.rebuild_graph();m.player=m.id({7,7});
    for(int i=0;i<MAX_MONSTERS;++i){
        Monster x;x.id=i;x.cell=m.id({i<5?3:5,3+2*(i%5)});x.next_move=NEVER;
        x.state=MonsterState::Hunting;m.monsters.push_back(x);
    }
    m.now=250;return m;
}
std::vector<unsigned char> capture(int width,int height,const std::string&path){
    std::vector<unsigned char> image(width*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glFinish();
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,image.data());require(glGetError()==GL_NO_ERROR,"OpenGL readback error");
    if(!path.empty()){std::ofstream file(path,std::ios::binary);file<<"P6\n"<<width<<' '<<height<<"\n255\n";
        for(int y=height-1;y>=0;--y)file.write(reinterpret_cast<const char*>(image.data()+y*width*3),width*3);}
    return image;
}
// The scan diamond sits inside the selected corridor arm for one move interval.
void test_scan_marker() {
    auto maze = monster_gallery();
    auto& monster = maze.monsters.front();
    monster.state = MonsterState::Searching;
    monster.scan_until = maze.now + 1;
    Renderer renderer(maze);
    auto marker_x = [&]() {
        renderer.draw(1200,900,maze);
        auto pixels = capture(1200,900,"");
        int count = 0, sum = 0;
        for (std::size_t i = 0; i < pixels.size(); i += 3) {
            if (pixels[i] == 255 && std::abs(int(pixels[i+1])-173) <= 1 &&
                std::abs(int(pixels[i+2])-48) <= 1) {
                sum += int((i / 3) % 1200);
                ++count;
            }
        }
        return count ? sum / count : -1;
    };
    monster.gaze = 2;
    int left = marker_x();
    monster.gaze = 3;
    require(left >= 0 && marker_x() > left, "scan marker follows the selected corridor arm");
    maze.now = monster.scan_until;
    require(marker_x() == -1, "scan marker disappears at the end of the pause");
}
int main(){try{
    Offscreen egl(1600,1200);
    test_scan_marker();
    std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<" | "<<glGetString(GL_RENDERER)<<'\n';
    {
        UiState ui;UiRenderer menu;
        menu.draw(1280,960,ui);capture(1280,960,"artifacts/screenshots/setup.ppm");
        for (auto choice : std::vector<std::pair<Control,int>>{{Control::Small,33},{Control::Standard,63},{Control::Large,101}}) {
            ui.activate(choice.first);
            int rows=0,cols=0,count=0;
            require(ui.form.values(rows,cols,count)&&rows==choice.second&&cols==choice.second,"preset dimensions");
            int selected=0;for(const auto& w:ui.widgets())if(w.id==Control::Small||w.id==Control::Standard||w.id==Control::Large)selected+=w.selected;
            require(selected==1,"exactly one matching preset highlighted");
            menu.draw(1280,960,ui);capture(1280,960,"artifacts/screenshots/preset-"+std::to_string(choice.second)+".ppm");
        }
        ui=UiState{};
        menu.draw(800,600,ui);capture(800,600,"artifacts/screenshots/setup-small.ppm");
        menu.draw(1600,600,ui);capture(1600,600,"artifacts/screenshots/setup-wide.ppm");
        menu.draw(600,1000,ui);capture(600,1000,"artifacts/screenshots/setup-portrait.ppm");
        ui.form.width="999";ui.select(Control::Width);
        menu.draw(1280,960,ui);capture(1280,960,"artifacts/screenshots/setup-invalid.ppm");
        ui.show(Screen::Loading);menu.draw(1280,960,ui);capture(1280,960,"artifacts/screenshots/loading.ppm");
        Maze game(21,31,8,nullptr);game.generate(5);game.set_auto_solve(true);game.advance_to(12400);
        require(std::string(game.director_phase())=="RELIEF" && game.radio_area_label()=="21x25" && game.director_reports()>0,
                "HUD capture uses a real widened director report");
        require(game.withdrawing() && game.withdrawal_steps()>0, "HUD capture shows a physical withdrawal");
        for(auto screen:{Screen::Playing,Screen::Paused,Screen::Result}) {
            ui.show(screen);
            if(screen==Screen::Result) {
                game.generate(0);game.set_auto_solve(true);game.advance_to(120000);
                require(game.outcome==Outcome::Won,"real zero-monster escape for results screenshot");
            }
            Renderer graphics(game);
            UiTransform t(1280,960);int bottom=int(t.y+702*t.scale),top=int(t.y+94*t.scale);
            graphics.draw(1280,bottom-top,game,960-bottom);menu.draw(1280,960,ui,&game);
            capture(1280,960,screen==Screen::Playing?"artifacts/screenshots/hud.ppm":screen==Screen::Paused?"artifacts/screenshots/pause.ppm":"artifacts/screenshots/result.ppm");
        }
        menu.draw(0,0,ui,&game);require(glGetError()==GL_NO_ERROR,"UI real renderer failed");
    }
    auto maze=showcase();
    {
        Renderer renderer(maze);renderer.draw(1200,900,maze);
        require(glGetError()==GL_NO_ERROR,"Showcase draw failed");auto pixels=capture(1200,900,"artifacts/screenshots/showcase.ppm");
        int black=0,gray=0,red=0,blue=0,purple=0,cyan=0;
        for(size_t i=0;i<pixels.size();i+=3){int r=pixels[i],g=pixels[i+1],b=pixels[i+2];
            black+=r==0&&g==0&&b==0;gray+=r==g&&g==b&&r>25;
            red+=r>200&&g<70&&b<90;blue+=b>170&&r<100&&g<150;purple+=r>140&&b>140&&g<110;cyan+=b>160&&g>150&&r<130;}
        require(black>200000&&gray>200000,"Floor/wall palette not black and gray");
        require(red>10&&blue>20&&purple>50&&cyan>50,"One or more actor sprites hidden or missing");
        renderer.draw(1600,600,maze);capture(1600,600,"artifacts/screenshots/wide.ppm");
        renderer.draw(600,1000,maze);capture(600,1000,"artifacts/screenshots/portrait.ppm");
        renderer.change_zoom(2);renderer.draw(1200,900,maze);capture(1200,900,"artifacts/screenshots/zoom.ppm");
        for(int i=0;i<20;++i)renderer.change_zoom(4);
        require(renderer.zoom_factor()==64,"Zoom maximum failed");
        renderer.reset_view();require(renderer.zoom_factor()==1,"Fit reset failed");
        renderer.draw(0,0,maze);require(glGetError()==GL_NO_ERROR,"Minimized framebuffer failed");
    }
    {
        auto gallery=monster_gallery();Renderer renderer(gallery);
        renderer.draw(1500,1000,gallery);auto all=capture(1500,1000,"artifacts/screenshots/monsters.ppm");
        for(int i=0;i<MAX_MONSTERS;++i){
            auto omitted=gallery;omitted.monsters.erase(omitted.monsters.begin()+i);
            Renderer omitted_renderer(omitted);
            omitted_renderer.draw(1500,1000,omitted);auto fewer=capture(1500,1000,"");
            int changed=0;for(std::size_t j=0;j<all.size();j+=3)
                changed+=all[j]!=fewer[j]||all[j+1]!=fewer[j+1]||all[j+2]!=fewer[j+2];
            require(changed>500,"Registered monster missing or hidden in the real renderer");
        }
        renderer.draw(1500,1000,gallery);capture(1500,1000,"artifacts/screenshots/monsters.ppm");
        require(glGetError()==GL_NO_ERROR,"Ten-monster gallery rendering failed");
    }
    {
        Maze actual(21,31,42,nullptr,Difficulty::Medium);actual.generate(6);actual.set_auto_solve(true);actual.advance_to(1000);
        Renderer renderer(actual);renderer.draw(1200,900,actual);capture(1200,900,"artifacts/screenshots/game.ppm");
        require(glGetError()==GL_NO_ERROR,"Generated maze rendering failed");
    }
    {
        Maze largest(501,501,42,nullptr,Difficulty::Low);largest.generate(10);largest.set_auto_solve(true);
        Renderer renderer(largest);renderer.draw(800,600,largest);glFinish();require(glGetError()==GL_NO_ERROR,"501x501 mesh render failed");
    }
    std::cout<<"PASS: real OpenGL shader compilation, depth rendering, palette, sprites, aspect ratios, zoom, minimized view, and maximum-size mesh.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
