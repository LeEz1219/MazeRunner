#define MAZERUNNER_NO_MAIN
#define MAZERUNNER_APP_TEST
#include "../src/mazerunnerV5.4.3c.cpp"
#include "egl_test_context.h"
#include <thread>
#include <tuple>

// Inject window input into production callbacks; all rendering uses real EGL/GL.
struct GLFWwindow { void* app=nullptr; double x=0,y=0; int width=1280,height=960,fw=1280,fh=960; };
void* glfwGetWindowUserPointer(GLFWwindow* w){return w->app;}
void glfwGetCursorPos(GLFWwindow* w,double* x,double* y){*x=w->x;*y=w->y;}
void glfwGetWindowSize(GLFWwindow* w,int* x,int* y){*x=w->width;*y=w->height;}
void glfwGetFramebufferSize(GLFWwindow* w,int* x,int* y){*x=w->fw;*y=w->fh;}
void key(GLFWwindow& w,int k,int mods=0,int action=GLFW_PRESS){key_callback(&w,k,0,action,mods);}
void click(GLFWwindow& w,Control id){
    auto* app=static_cast<GameApplication*>(w.app);
    for(const auto& c:app->ui.widgets(app->maze.get()))if(c.id==id){
        UiTransform t(w.fw,w.fh);
        w.x=(t.x+(c.box.x+c.box.w/2)*t.scale)*w.width/w.fw;
        w.y=(t.y+(c.box.y+c.box.h/2)*t.scale)*w.height/w.fh;
        mouse_callback(&w,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        mouse_callback(&w,GLFW_MOUSE_BUTTON_LEFT,GLFW_RELEASE,0);return;
    }
    throw std::runtime_error("Missing click control");
}
void ready(GameApplication& app,bool focused=true){
    auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    while(app.generation.valid()&&std::chrono::steady_clock::now()<limit){
        app.finish_generation(focused);std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(app.maze&&app.renderer&&!app.generation.valid(),"asynchronous maze ready");
    require(app.maze->now==0,"loading does not advance simulation time");
}
void test_held_collision_clock() {
    auto play=[](Millis arrival,const std::vector<Millis>& frames) {
        GameApplication app(11,true,true,false,Difficulty::Medium);
        GLFWwindow window;window.app=&app;
        app.maze=std::make_unique<Maze>(31,31,11,nullptr,Difficulty::Medium);
        auto& m=*app.maze;
        for(int r=1;r<30;++r) for(int c=1;c<30;++c) m.grid[m.id({r,c})]=PATH;
        m.grid[m.entrance]=PATH;m.grid[m.exit_cell]=EXIT;m.rebuild_graph();m.reset_game();
        m.player=m.id({1,10});
        Monster hunter;hunter.cell=m.id({2,11});hunter.next_move=arrival;
        m.monsters={hunter};
        app.renderer=std::make_unique<Renderer>(m);
        app.ui.show(Screen::Playing);
        key(window,GLFW_KEY_D);
        auto& active=m.monsters.front();
        active.target=m.id({1,11});active.planned_revision=active.hint.revision;active.next_replan=NEVER;
        for(Millis delta:frames) app.tick(delta);
        return std::make_tuple(m.outcome,m.now,m.point(m.player).c,m.manual_steps,active.cell);
    };
    auto escaped=play(230,{250});
    require(escaped==play(230,{220,30}) && escaped==play(230,{119,100,12,19}),
            "frame partition cannot change whether the held step escapes");
    require(std::get<0>(escaped)==Outcome::Playing && std::get<2>(escaped)==12,
            "the 220 ms repeat moves before the 230 ms monster");
    auto caught=play(220,{250});
    require(caught==play(220,{219,1,30}) && std::get<0>(caught)==Outcome::Caught &&
            std::get<1>(caught)==220 && std::get<2>(caught)==11,"monsters win an exact repeat-time tie");
    require(play(230,{500})==play(230,{250,250}),"large frame keeps every elapsed simulation deadline");
}
int main(){try{
    Offscreen egl(1280,960);
    test_held_collision_clock();
    {
        GameApplication app(42,true,true,false,Difficulty::High);GLFWwindow w;w.app=&app;
        require(app.ui.screen==Screen::Setup&&!app.maze,"launch shows setup without terminal input");
        require(app.ui.form.difficulty==Difficulty::High,"CLI difficulty preselects card");
        app.draw(1280,960);click(w,Control::Low);click(w,Control::Small);
        click(w,Control::Width);character_callback(&w,'2');character_callback(&w,'0');
        key(w,GLFW_KEY_ENTER);
        require(app.ui.screen==Screen::Setup&&app.ui.focus==Control::Start,"Enter commits field without starting");
        click(w,Control::Height);key(w,GLFW_KEY_ESCAPE);
        require(!app.ui.quit&&app.ui.screen==Screen::Setup&&app.ui.focus==Control::Start,"Esc leaves a text field without quitting");
        key(w,GLFW_KEY_ESCAPE);require(!app.ui.quit&&app.ui.screen==Screen::Setup,"Esc on setup never quits");
        key(w,GLFW_KEY_ENTER);require(app.ui.screen==Screen::Loading,"keyboard Start enters loading");
        app.draw(1280,960);app.begin_generation();app.begin_generation();ready(app);
        require(app.maze->cols==21&&app.maze->rows==33&&app.maze->monsters.size()==5,"form transferred to maze");
        require(app.maze->difficulty==Difficulty::Low,"selected difficulty reaches simulation");
        auto first_grid=app.maze->grid;
        app.tick(100);auto now=app.maze->now;require(now==100,"active play advances");
        click(w,Control::Pause);app.tick(1000);require(app.maze->now==now,"pause freezes simulation");
        auto player=app.maze->player;key(w,GLFW_KEY_D);key(w,GLFW_KEY_G);
        require(app.maze->player==player&&!app.maze->auto_solve,"modal blocks movement and auto-solve");
        app.draw(1280,960);click(w,Control::Resume);key(w,GLFW_KEY_G);
        require(app.maze->auto_solve,"G enables auto-solve");
        key(w,GLFW_KEY_G,0,GLFW_REPEAT);require(app.maze->auto_solve,"held key does not repeatedly toggle");
        click(w,Control::AutoSolve);require(!app.maze->auto_solve,"HUD toggles auto-solve off");
        {   // Hold-to-move on an opened copy of the terrain, with no monsters.
            auto& m=*app.maze;
            for(int r=1;r<m.rows-1;++r)for(int c=1;c<m.cols-1;++c)m.grid[m.id({r,c})]=PATH;
            m.rebuild_graph();m.monsters.clear();int start=m.player;
            key(w,GLFW_KEY_D);require(m.player==start+1,"first press moves immediately");
            app.tick(200);require(m.player==start+1,"hold waits for the repeat delay");
            app.tick(20);require(m.player==start+2,"holding keeps walking");
            app.tick(240);require(m.player==start+4,"repeat cadence is steady");
            key(w,GLFW_KEY_D,0,GLFW_RELEASE);app.tick(500);require(m.player==start+4,"release stops walking");
            key(w,GLFW_KEY_D,0,GLFW_REPEAT);require(m.player==start+4,"OS key-repeat events are ignored");
            key(w,GLFW_KEY_D);click(w,Control::Pause);click(w,Control::Resume);
            int paused=m.player;app.tick(300);require(m.player==paused,"pause and resume without a tick clear the hold");
            key(w,GLFW_KEY_D);click(w,Control::AutoSolve);click(w,Control::AutoSolve);
            int toggled=m.player;app.tick(300);require(m.player==toggled,"an auto-solve toggle cannot resurrect the old hold");
            key(w,GLFW_KEY_S);focus_callback(&w,0);focus_callback(&w,1);click(w,Control::Resume);
            int held=m.player;app.tick(1000);require(m.player==held,"focus loss cancels a held direction");
            now=m.now;
        }
        scroll_callback(&w,0,2);require(app.renderer->zoom_factor()>1,"mouse wheel zoom");
        key(w,GLFW_KEY_HOME);require(app.renderer->zoom_factor()==1,"Home fits map");
        focus_callback(&w,0);require(app.ui.screen==Screen::Paused,"focus loss pauses");
        app.tick(250);require(app.maze->now==now,"focus pause freezes time");
        focus_callback(&w,1);require(app.ui.screen==Screen::Paused,"focus return keeps explicit resume");
        key(w,GLFW_KEY_ESCAPE);app.tick(20);require(app.maze->now==now+20,"resume excludes paused time");
        key(w,GLFW_KEY_ESCAPE);click(w,Control::Setup);app.discard_game();
        require(!app.maze&&!app.renderer&&app.ui.screen==Screen::Setup,"New Game releases map and returns setup");
        click(w,Control::Start);app.begin_generation();ready(app,false);
        require(app.ui.screen==Screen::Paused&&app.maze->grid==first_grid,"fixed seed replays and unfocused loading pauses");
        click(w,Control::Setup);app.discard_game();
        click(w,Control::Count);key(w,GLFW_KEY_A,GLFW_MOD_CONTROL);character_callback(&w,'0');
        click(w,Control::Start);app.begin_generation();ready(app);
        require(app.maze->monsters.empty(),"zero-monster mode transferred");
        click(w,Control::AutoSolve);
        for(int i=0;i<1000&&app.ui.screen==Screen::Playing;++i)app.tick(100);
        require(app.ui.screen==Screen::Result&&app.maze->outcome==Outcome::Won,"solver escape opens result UI");
        now=app.maze->now;app.tick(250);require(app.maze->now==now,"result freezes simulation");
        app.draw(1280,960);click(w,Control::Again);app.begin_generation();ready(app);
        app.maze->outcome=Outcome::Caught;app.tick(0);
        require(app.ui.screen==Screen::Result,"capture opens result UI");app.draw(1280,960);
        click(w,Control::Setup);app.discard_game();
        // 2x framebuffer scale tests actual pointer callbacks, not only transforms.
        w.width=640;w.height=480;click(w,Control::High);
        require(app.ui.form.difficulty==Difficulty::High,"high-DPI callback hits correct card");
        app.ui.show(Screen::Loading);
        app.generation=std::async(std::launch::async,[]()->std::unique_ptr<Maze>{throw std::runtime_error("injected generation failure");});
        app.generation.wait();app.finish_generation(true);
        require(app.ui.screen==Screen::Setup&&!app.ui.message.empty(),"worker failure returns usable setup with explanation");
        app.draw(1280,960);click(w,Control::Quit);require(app.ui.quit,"quit is available through UI");
    }
    {
        GameApplication app(42,false,true,false,Difficulty::Medium);
        app.ui.request_start();app.begin_generation();ready(app);auto first=app.maze->seed;
        app.ui.show(Screen::Result);app.ui.activate(Control::Again,app.maze.get());app.begin_generation();ready(app);
        require(app.maze->seed!=first,"ordinary replay chooses a fresh maze seed");
        app.ui.show(Screen::Paused);app.draw(0,0);
    }
    require(glGetError()==GL_NO_ERROR,"all app draw and destruction paths free of GL errors");
    std::cout<<"PASS: real-GL application lifecycle, async generation, input callbacks, pause clocks, focus loss, replay seeds, results, DPI, and worker failure recovery.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
