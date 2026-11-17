#define MAZERUNNER_NO_MAIN
#define MAZERUNNER_APP_TEST
#include "../src/mazerunnerV5.4.1.cpp"
#include "egl_test_context.h"
#include <thread>

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
int main(){try{
    Offscreen egl(1280,960);
    {
        GameApplication app(42,true,true,false,Difficulty::High);GLFWwindow w;w.app=&app;
        require(app.ui.screen==Screen::Setup&&!app.maze,"launch shows setup without terminal input");
        require(app.ui.form.difficulty==Difficulty::High,"CLI difficulty preselects card");
        app.draw(1280,960);click(w,Control::Low);click(w,Control::Small);
        click(w,Control::Width);character_callback(&w,'2');character_callback(&w,'0');
        key(w,GLFW_KEY_ENTER);
        require(app.ui.screen==Screen::Setup&&app.ui.focus==Control::Start,"Enter commits field without starting");
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
