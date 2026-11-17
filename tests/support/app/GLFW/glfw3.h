#pragma once
struct GLFWwindow;
constexpr int GLFW_PRESS=1,GLFW_KEY_ESCAPE=2,GLFW_KEY_Q=3,GLFW_KEY_G=4,GLFW_KEY_W=5,GLFW_KEY_S=6,GLFW_KEY_A=7,GLFW_KEY_D=8,GLFW_TRUE=1,GLFW_CONTEXT_VERSION_MAJOR=9,GLFW_CONTEXT_VERSION_MINOR=10,GLFW_OPENGL_PROFILE=11,GLFW_OPENGL_CORE_PROFILE=12,GLFW_OPENGL_FORWARD_COMPAT=13;
void glfwGetFramebufferSize(GLFWwindow*,int*,int*);void glfwSetWindowShouldClose(GLFWwindow*,int);void* glfwGetWindowUserPointer(GLFWwindow*);int glfwInit();void glfwWindowHint(int,int);GLFWwindow* glfwCreateWindow(int,int,const char*,void*,GLFWwindow*);void glfwTerminate();void glfwMakeContextCurrent(GLFWwindow*);void glfwSwapInterval(int);void glfwSetWindowUserPointer(GLFWwindow*,void*);void glfwSetKeyCallback(GLFWwindow*,void(*)(GLFWwindow*,int,int,int,int));int glfwWindowShouldClose(GLFWwindow*);void glfwPollEvents();void glfwSwapBuffers(GLFWwindow*);void glfwDestroyWindow(GLFWwindow*);
#ifdef TEST_WAYLAND
#define GLFW_PLATFORM_WAYLAND 42
int glfwGetPlatform();
#endif
constexpr int GLFW_KEY_HOME=30,GLFW_KEY_EQUAL=31,GLFW_KEY_KP_ADD=32,GLFW_KEY_MINUS=33,GLFW_KEY_KP_SUBTRACT=34,GLFW_DEPTH_BITS=35,GLFW_SAMPLES=36;
void glfwSetScrollCallback(GLFWwindow*,void(*)(GLFWwindow*,double,double));
constexpr int GLFW_RELEASE=0,GLFW_REPEAT=2,GLFW_MOD_SHIFT=1,GLFW_MOD_CONTROL=2;
constexpr int GLFW_KEY_TAB=50,GLFW_KEY_BACKSPACE=51,GLFW_KEY_DELETE=52,GLFW_KEY_UP=53,GLFW_KEY_DOWN=54,GLFW_KEY_LEFT=55,GLFW_KEY_RIGHT=56,GLFW_KEY_ENTER=57,GLFW_KEY_KP_ENTER=58,GLFW_KEY_SPACE=59;
constexpr int GLFW_MOUSE_BUTTON_LEFT=0,GLFW_DONT_CARE=-1,GLFW_FOCUSED=60;
void glfwGetCursorPos(GLFWwindow*,double*,double*);void glfwGetWindowSize(GLFWwindow*,int*,int*);
void glfwSetWindowSizeLimits(GLFWwindow*,int,int,int,int);int glfwGetWindowAttrib(GLFWwindow*,int);
void glfwSetCharCallback(GLFWwindow*,void(*)(GLFWwindow*,unsigned int));
void glfwSetMouseButtonCallback(GLFWwindow*,void(*)(GLFWwindow*,int,int,int));
void glfwSetWindowFocusCallback(GLFWwindow*,void(*)(GLFWwindow*,int));void glfwWaitEventsTimeout(double);
