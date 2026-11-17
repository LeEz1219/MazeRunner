#include "v542_tests.cpp"

void test_setup_inputs(){
    UiState ui;int r=0,c=0,n=0;
    require(ui.form.values(r,c,n)&&r==21&&c==31&&n==5,"initial form valid");
    for(auto d:{Control::Low,Control::Medium,Control::High})ui.activate(d);
    require(ui.form.difficulty==Difficulty::High,"difficulty buttons update profile");
    ui.activate(Control::Small);require(ui.form.values(r,c,n)&&r==33&&c==33,"small preset");
    ui.activate(Control::Large);require(ui.form.values(r,c,n)&&r==101&&c==101,"large preset");
    ui.activate(Control::Standard);require(ui.form.values(r,c,n)&&r==63&&c==63,"standard preset");
    ui.activate(Control::Width);ui.digit('2');ui.digit('0');
    ui.activate(Control::Height);ui.digit('1');ui.digit('0');
    ui.activate(Control::Count);ui.digit('0');
    require(ui.form.values(r,c,n)&&r==11&&c==21&&n==0,"normalize even custom sizes and allow zero monsters");
    for(const char* invalid:{"", "4", "502", "999", "-5", "5x", "1e2", "1000"}) {
        ui.form.width=invalid;require(!ui.form.values(r,c,n),"bad width rejected");
        ui.activate(Control::Start);require(ui.screen==Screen::Setup,"invalid form cannot start");
        require(!ui.form.error().empty(),"invalid input has onscreen explanation");
    }
    ui.form.width="501";ui.form.height="5";ui.form.count="10";
    ui.activate(Control::WidthPlus);ui.activate(Control::HeightMinus);ui.activate(Control::CountPlus);
    require(ui.form.values(r,c,n)&&r==5&&c==501&&n==10,"stepper endpoints clamped");
    ui.form.count="0";ui.activate(Control::CountMinus);require(ui.form.count=="0","count minimum clamped");
    ui.form.width="20";ui.activate(Control::WidthMinus);require(ui.form.width=="19","even width steps down to odd");
    ui.form.width="20";ui.activate(Control::WidthPlus);require(ui.form.width=="21","even width steps up to odd");
    ui.activate(Control::Count);ui.digit('1');ui.digit('1');
    require(!ui.form.values(r,c,n),"eleven monsters rejected");ui.erase();require(ui.form.count=="1","backspace edits field");
    ui.digit(0x0661);ui.digit('x');require(ui.form.count=="1","nondecimal unicode ignored");
    ui.form.width="20";ui.request_start();require(ui.screen==Screen::Loading&&ui.form.width=="21","start commits normalized values");
}

void test_pointer_and_keyboard_focus(){
    UiState ui;
    auto all=ui.widgets();std::set<Control> focusable;
    for(const auto& w:all){
        require(w.box.x>=0&&w.box.y>=0&&w.box.x+w.box.w<=UI_WIDTH&&w.box.y+w.box.h<=UI_HEIGHT,"all controls inside canvas");
        require(ui.hit(w.box.x+w.box.w/2,w.box.y+w.box.h/2)==w.id,"widget and hit region match");
        focusable.insert(w.id);
    }
    std::set<Control> visited;
    for(std::size_t i=0;i<all.size();++i){ui.tab(false);visited.insert(ui.focus);}
    require(visited==focusable,"Tab reaches every setup control");
    Control saved=ui.focus;ui.tab(true);ui.tab(false);require(saved==ui.focus,"Shift Tab reverses traversal");
    ui.pointer_button(true,60,170);ui.pointer_button(false,900,740);
    require(ui.form.difficulty==Difficulty::Medium,"dragging off a button cancels activation");
    ui.pointer_button(true,60,170);ui.pointer_button(false,70,175);
    require(ui.form.difficulty==Difficulty::Low,"complete pointer click selects Low");
    ui.form.width="";for(int i=0;i<20;++i){ui.tab(false);require(ui.focus!=Control::Start,"Tab skips disabled Start");}
    for(auto sizes:std::vector<std::array<int,4>>{{1280,960,1280,960},{800,600,1600,1200},{1400,600,1400,600},{600,1000,1200,2000}}){
        int ww=sizes[0],wh=sizes[1],fw=sizes[2],fh=sizes[3];UiTransform t(fw,fh);
        for(const auto& w:all){
            float x=w.box.x+w.box.w/2,y=w.box.y+w.box.h/2;
            auto p=t.pointer((t.x+x*t.scale)*ww/fw,(t.y+y*t.scale)*wh/fh,ww,wh,fw,fh);
            require(std::abs(p.first-x)<0.01f&&std::abs(p.second-y)<0.01f,"DPI and aspect transform preserves hit point");
        }
    }
    UiTransform minimized(0,0);require(minimized.pointer(1,1,0,0,0,0).first==-1,"minimized input safely rejected");
}

void test_screen_transitions(){
    Maze maze(21,21,42,nullptr);maze.generate(5);
    UiState ui;ui.activate(Control::Start);require(ui.screen==Screen::Loading,"Start enters Loading");
    ui.activate(Control::AutoSolve,&maze);require(!maze.auto_solve,"Loading rejects gameplay controls");
    ui.show(Screen::Playing);int player=maze.player;
    ui.activate(Control::AutoSolve,&maze);require(maze.auto_solve&&maze.player==player,"HUD toggle starts solver without displacement");
    ui.activate(Control::Pause,&maze);require(ui.screen==Screen::Paused,"HUD pause opens modal");
    ui.activate(Control::AutoSolve,&maze);require(maze.auto_solve,"paused modal blocks underlying HUD");
    ui.activate(Control::Resume,&maze);require(ui.screen==Screen::Playing,"resume returns to play");
    ui.show(Screen::Result);ui.activate(Control::Again,&maze);require(ui.screen==Screen::Loading,"result replay requests generation");
    ui.show(Screen::Result);ui.activate(Control::Setup,&maze);require(ui.screen==Screen::Setup,"result returns to settings");
    ui.activate(Control::Quit);require(ui.quit,"Quit is available without terminal");
}
int main(){try{
    require(v542_regression_main()==0,"all previous regression tests");
    test_setup_inputs();test_pointer_and_keyboard_focus();test_screen_transitions();
    std::cout<<"PASS: setup validation, all controls, text input, bounds, DPI hit testing, keyboard navigation, and screen transitions.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
