#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

#include "Wglext.h"
#include "glcorearb.h"

#include "gl_functions.h"

//////////////////////////////////////////////////////////////////////

namespace
{
    template <typename T> void get_proc(char const *function_name, T &function_pointer)
    {
        function_pointer = reinterpret_cast<T>(wglGetProcAddress(function_name));
        if(function_pointer == nullptr) {
            fprintf(stderr, "ERROR: Can't get proc address for %s\n", function_name);
            ExitProcess(1);
        }
    }
}    // namespace

#define GET_PROC(x) get_proc(#x, x)

namespace gl_functions
{
    void init()
    {
        GET_PROC(glCreateShader);
        GET_PROC(glShaderSource);
        GET_PROC(glCompileShader);
        GET_PROC(glGetShaderiv);
        GET_PROC(glGetShaderInfoLog);
        GET_PROC(glCreateProgram);
        GET_PROC(glAttachShader);
        GET_PROC(glLinkProgram);
        GET_PROC(glValidateProgram);
        GET_PROC(glGetProgramiv);
        GET_PROC(glGenBuffers);
        GET_PROC(glGenVertexArrays);
        GET_PROC(glGetAttribLocation);
        GET_PROC(glBindVertexArray);
        GET_PROC(glEnableVertexAttribArray);
        GET_PROC(glVertexAttribPointer);
        GET_PROC(glBindBuffer);
        GET_PROC(glBufferData);
        GET_PROC(glGetVertexAttribPointerv);
        GET_PROC(glUseProgram);
        GET_PROC(glDeleteVertexArrays);
        GET_PROC(glDeleteBuffers);
        GET_PROC(glDeleteProgram);
        GET_PROC(glDeleteShader);

        GET_PROC(wglChoosePixelFormatARB);
        GET_PROC(wglCreateContextAttribsARB);
        GET_PROC(wglSwapIntervalEXT);
    }
}    // namespace gl_functions

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;

PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
