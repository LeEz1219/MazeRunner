#pragma once
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
