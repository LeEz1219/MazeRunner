#pragma once
// Test harness ABI declarations: these call the real system OpenGL library.
#include <cstddef>
using GLenum=unsigned int; using GLuint=unsigned int; using GLint=int; using GLsizei=int;
using GLboolean=unsigned char; using GLsizeiptr=std::ptrdiff_t; using GLbitfield=unsigned int;
constexpr GLenum GL_VERTEX_SHADER=0x8B31,GL_FRAGMENT_SHADER=0x8B30,GL_COMPILE_STATUS=0x8B81,GL_LINK_STATUS=0x8B82;
constexpr GLenum GL_ARRAY_BUFFER=0x8892,GL_FLOAT=0x1406,GL_STATIC_DRAW=0x88E4,GL_STREAM_DRAW=0x88E0,GL_TRIANGLES=4;
constexpr GLenum GL_COLOR_BUFFER_BIT=0x4000,GL_DEPTH_BUFFER_BIT=0x100,GL_NO_ERROR=0,GL_INVALID_ENUM=0x500;
constexpr GLenum GL_DEPTH_TEST=0xB71,GL_LEQUAL=0x203,GL_BLEND=0xBE2,GL_CULL_FACE=0xB44,GL_MULTISAMPLE=0x809D,GL_DITHER=0xBD0;
constexpr GLenum GL_RGB=0x1907,GL_UNSIGNED_BYTE=0x1401,GL_VERSION=0x1F02,GL_RENDERER=0x1F01,GL_PACK_ALIGNMENT=0xD05,GL_DEPTH_BITS=0xD56;
constexpr GLboolean GL_TRUE=1,GL_FALSE=0;
extern "C" {
GLuint glCreateShader(GLenum);GLuint glCreateProgram();void glGenVertexArrays(GLsizei,GLuint*);void glGenBuffers(GLsizei,GLuint*);
void glShaderSource(GLuint,GLsizei,const char*const*,const GLint*);void glCompileShader(GLuint);void glGetShaderiv(GLuint,GLenum,GLint*);void glGetShaderInfoLog(GLuint,GLsizei,GLsizei*,char*);void glDeleteShader(GLuint);
void glAttachShader(GLuint,GLuint);void glLinkProgram(GLuint);void glGetProgramiv(GLuint,GLenum,GLint*);void glGetProgramInfoLog(GLuint,GLsizei,GLsizei*,char*);void glDeleteProgram(GLuint);
void glBindVertexArray(GLuint);void glBindBuffer(GLenum,GLuint);void glVertexAttribPointer(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);void glEnableVertexAttribArray(GLuint);void glBufferData(GLenum,GLsizeiptr,const void*,GLenum);void glDeleteVertexArrays(GLsizei,const GLuint*);void glDeleteBuffers(GLsizei,const GLuint*);
void glViewport(GLint,GLint,GLsizei,GLsizei);void glClearColor(float,float,float,float);void glClear(GLbitfield);void glUseProgram(GLuint);void glDrawArrays(GLenum,GLint,GLsizei);GLenum glGetError();
void glEnable(GLenum);void glDisable(GLenum);void glClearDepth(double);void glDepthFunc(GLenum);void glDepthMask(GLboolean);
GLint glGetUniformLocation(GLuint,const char*);void glUniformMatrix4fv(GLint,GLsizei,GLboolean,const float*);
const unsigned char* glGetString(GLenum);void glGetIntegerv(GLenum,GLint*);void glPixelStorei(GLenum,GLint);void glReadPixels(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);void glFinish();
}
