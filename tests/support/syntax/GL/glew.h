#pragma once
#include <cstddef>
using GLenum=unsigned int; using GLuint=unsigned int; using GLint=int; using GLsizei=int;
using GLboolean=unsigned char; using GLsizeiptr=std::ptrdiff_t;
constexpr unsigned GL_VERTEX_SHADER=1,GL_FRAGMENT_SHADER=2,GL_COMPILE_STATUS=3,GL_LINK_STATUS=4,GL_ARRAY_BUFFER=5,GL_FLOAT=6,GL_STATIC_DRAW=7,GL_STREAM_DRAW=8,GL_TRIANGLES=9,GL_COLOR_BUFFER_BIT=10,GL_NO_ERROR=0,GL_INVALID_ENUM=11;
constexpr unsigned GL_TRUE=1,GL_FALSE=0,GLEW_OK=0;
extern bool GLEW_VERSION_3_3,GLEW_VERSION_3_0,GLEW_VERSION_2_0,GLEW_VERSION_1_5,glewExperimental;
extern GLuint (*glCreateShader)(GLenum); extern GLuint (*glCreateProgram)();
extern void (*glGenVertexArrays)(GLsizei,GLuint*); extern void (*glGenBuffers)(GLsizei,GLuint*);
void glShaderSource(GLuint,GLsizei,const char* const*,const GLint*);void glCompileShader(GLuint);void glGetShaderiv(GLuint,GLenum,GLint*);void glGetShaderInfoLog(GLuint,GLsizei,GLsizei*,char*);void glDeleteShader(GLuint);
void glAttachShader(GLuint,GLuint);void glLinkProgram(GLuint);void glGetProgramiv(GLuint,GLenum,GLint*);void glGetProgramInfoLog(GLuint,GLsizei,GLsizei*,char*);void glDeleteProgram(GLuint);
void glBindVertexArray(GLuint);void glBindBuffer(GLenum,GLuint);void glVertexAttribPointer(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);void glEnableVertexAttribArray(GLuint);void glBufferData(GLenum,GLsizeiptr,const void*,GLenum);void glDeleteVertexArrays(GLsizei,const GLuint*);void glDeleteBuffers(GLsizei,const GLuint*);
void glViewport(GLint,GLint,GLsizei,GLsizei);void glClearColor(float,float,float,float);void glClear(GLenum);void glUseProgram(GLuint);void glDrawArrays(GLenum,GLint,GLsizei);GLenum glewInit();const unsigned char* glewGetErrorString(GLenum);GLenum glGetError();
#ifdef TEST_WAYLAND
#define GLEW_ERROR_NO_GLX_DISPLAY 4
#endif
constexpr unsigned GL_DEPTH_TEST=20,GL_LEQUAL=21,GL_BLEND=22,GL_CULL_FACE=23,GL_MULTISAMPLE=24,GL_DITHER=25,GL_DEPTH_BUFFER_BIT=256;
void glEnable(GLenum);void glDisable(GLenum);void glClearDepth(double);void glDepthFunc(GLenum);void glDepthMask(GLboolean);
GLint glGetUniformLocation(GLuint,const char*);void glUniformMatrix4fv(GLint,GLsizei,GLboolean,const float*);
