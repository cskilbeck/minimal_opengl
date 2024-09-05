#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

#include "Wglext.h"
#include "glcorearb.h"

#include "gl_functions.h"

#pragma comment(lib, "opengl32.lib")

//////////////////////////////////////////////////////////////////////

char const *vertex_shader_source = R"-----(

#version 400
in vec3 positionIn;
in vec4 colorIn;
out vec4 fragmentColor;

uniform mat4 projection = mat4(1.0);
uniform mat4 model = mat4(1.0);

void main() {
    gl_Position = projection * model * vec4(positionIn, 1.0f);
    fragmentColor = colorIn;
}

)-----";

//////////////////////////////////////////////////////////////////////

char const *fragment_shader_source = R"-----(

#version 400
in vec4 fragmentColor;
out vec4 color;

void main() {
    color = fragmentColor;

})-----";

//////////////////////////////////////////////////////////////////////

struct gl_program
{
    GLuint program_id{};
    GLuint vertex_shader_id{};
    GLuint fragment_shader_id{};

    gl_program() = default;

    //////////////////////////////////////////////////////////////////////

    int check_shader(GLuint shader_id) const
    {
        GLint result;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
        if(result) {
            return 0;
        }
        GLsizei length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
        if(length != 0) {
            GLchar *infoLog = new GLchar[length];
            glGetShaderInfoLog(shader_id, length, &length, infoLog);
            printf("Error in shader: %s\n", infoLog);
            delete[] infoLog;
        } else {
            printf("Huh? Compile error but no log?\n");
        }
        return -1;
    }

    //////////////////////////////////////////////////////////////////////

    int validate(GLuint param) const
    {
        GLint result;
        glGetProgramiv(program_id, param, &result);
        if(result != FALSE) {
            return 0;
        }
        GLsizei length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);
        if(length != 0) {
            GLchar *infoLog = new GLchar[length];
            glGetProgramInfoLog(program_id, length, &length, infoLog);
            printf("Error in program: %s\n", infoLog);
            delete[] infoLog;
        } else if(param == GL_LINK_STATUS) {
            printf("glLinkProgram failed: Can not link program.\n");
        } else {
            printf("glValidateProgram failed: Can not execute shader program.\n");
        }
        return -1;
    }

    //////////////////////////////////////////////////////////////////////

    int init(char const *const vertex_shader, char const *const fragment_shader)
    {
        vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vertex_shader_id, 1, &vertex_shader, NULL);
        glShaderSource(fragment_shader_id, 1, &fragment_shader, NULL);

        glCompileShader(vertex_shader_id);
        glCompileShader(fragment_shader_id);

        int rc = check_shader(vertex_shader_id);
        if(rc != 0) {
            return rc;
        }

        rc = check_shader(fragment_shader_id);
        if(rc != 0) {
            return rc;
        }

        program_id = glCreateProgram();

        glAttachShader(program_id, vertex_shader_id);
        glAttachShader(program_id, fragment_shader_id);

        glLinkProgram(program_id);
        rc = validate(GL_LINK_STATUS);
        if(rc != 0) {
            return rc;
        }
        glValidateProgram(program_id);
        rc = validate(GL_VALIDATE_STATUS);
        if(rc != 0) {
            return rc;
        }
        glUseProgram(program_id);
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    void cleanup()
    {
    }
};

//////////////////////////////////////////////////////////////////////

struct gl_vertex_array
{
    GLuint vbo_id{};
    GLuint vao_id{};
    GLuint ibo_id{};

    gl_vertex_array() = default;

    //////////////////////////////////////////////////////////////////////

    int init(gl_program &program)
    {
        GLint positionLocation = glGetAttribLocation(program.program_id, "positionIn");
        GLint colorLocation = glGetAttribLocation(program.program_id, "colorIn");

        glGenBuffers(1, &vbo_id);
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);

        // x, y, z, r, g, b (triangle)
        float vertices[] = {
            0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, //
            1.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, //
            -1.0, -1.0, 0.0, 0.0, 0.0, 1.0, 1.0, //
        };

        GLushort indices[] = {
            0,1,2
        };

        glBindVertexArray(vao_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

        glEnableVertexAttribArray(positionLocation);
        glEnableVertexAttribArray(colorLocation);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(0));
        glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(3 * sizeof(float)));

        return 0;
    }
};

//////////////////////////////////////////////////////////////////////

static void center_window_on_default_monitor(HWND hwnd)
{
    RECT window_rect;
    GetWindowRect(hwnd, &window_rect);
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
    int x = (mi.rcMonitor.right - mi.rcMonitor.left - window_width) / 2;
    int y = (mi.rcMonitor.bottom - mi.rcMonitor.top - window_height) / 2;
    SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

//////////////////////////////////////////////////////////////////////

struct gl_window
{
    HGLRC render_context{};
    HWND hwnd{};
    HDC window_dc{};
    RECT normal_rect;
    bool fullscreen{};
    bool quit{};

    static constexpr char const *class_name = "GL_CONTEXT_WINDOW_CLASS";
    static constexpr char const *window_title = "GL Window";

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK wnd_proc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;

        switch(message) {

        case WM_SIZE:
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            clear();
            draw();
            swap();
            break;

        case WM_KEYDOWN:

            switch(wParam) {

            case VK_ESCAPE: {
                DestroyWindow(hwnd);
            } break;

            case VK_F11: {
                set_fullscreen_state(!fullscreen);
            } break;
            }
            break;

        case WM_CLOSE:
            // free_gl_resources();
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            wglMakeCurrent(window_dc, NULL);
            wglDeleteContext(render_context);
            ReleaseDC(hwnd, window_dc);
            PostQuitMessage(0);
            break;

        default:
            result = DefWindowProcA(hwnd, message, wParam, lParam);
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////

    void clear()
    {
        glClearColor(0.1f, 0.2f, 0.5f, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    //////////////////////////////////////////////////////////////////////

    void draw()
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, (GLvoid*)0);
    }

    //////////////////////////////////////////////////////////////////////

    void swap() const
    {
        SwapBuffers(window_dc);
    }

    //////////////////////////////////////////////////////////////////////

    void set_fullscreen_state(bool new_fullscreen)
    {
        fullscreen = new_fullscreen;

        RECT rc = normal_rect;
        HWND insert_after = HWND_NOTOPMOST;
        DWORD style = GetWindowLong(hwnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW;

        if(fullscreen) {
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
            rc = mi.rcMonitor;
            style &= ~WS_OVERLAPPEDWINDOW;
            GetWindowRect(hwnd, &normal_rect);
            insert_after = HWND_TOP;
        }

        int x = rc.left;
        int y = rc.top;
        int w = rc.right - x;
        int h = rc.bottom - y;

        SetWindowLongA(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, insert_after, x, y, w, h, SWP_FRAMECHANGED);
    }

    //////////////////////////////////////////////////////////////////////

    static LRESULT CALLBACK wnd_proc_proxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if(message == WM_CREATE) {
            LPCREATESTRUCTA c = reinterpret_cast<LPCREATESTRUCTA>(lParam);
            SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(c->lpCreateParams));
        }
        gl_window *d = reinterpret_cast<gl_window *>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
        if(d != nullptr) {
            return d->wnd_proc(message, wParam, lParam);
        }
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }

    //////////////////////////////////////////////////////////////////////

    int init()
    {
        HINSTANCE instance = GetModuleHandleA(nullptr);

        // register window class

        WNDCLASSEXA wcex;
        memset(&wcex, 0, sizeof(wcex));
        wcex.cbSize = sizeof(WNDCLASSEXA);
        wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcex.lpfnWndProc = (WNDPROC)wnd_proc_proxy;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = instance;
        wcex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = class_name;
        wcex.hIconSm = NULL;

        if(!RegisterClassExA(&wcex)) {
            return -1;
        }

        // create temp render_context

        HWND temp_hwnd = CreateWindowExA(0, class_name, "", 0, 0, 0, 1, 1, nullptr, nullptr, instance, nullptr);
        if(temp_hwnd == nullptr) {
            return -2;
        }

        HDC temp_dc = GetDC(temp_hwnd);
        if(temp_dc == nullptr) {
            return -3;
        }

        PIXELFORMATDESCRIPTOR pixelFormatDesc = { 0 };
        pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pixelFormatDesc.nVersion = 1;
        pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        pixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
        pixelFormatDesc.cColorBits = 32;
        pixelFormatDesc.cAlphaBits = 8;
        pixelFormatDesc.cDepthBits = 24;
        int temp_pixelFormat = ChoosePixelFormat(temp_dc, &pixelFormatDesc);
        if(temp_pixelFormat == 0) {
            return -4;
        }

        if(!SetPixelFormat(temp_dc, temp_pixelFormat, &pixelFormatDesc)) {
            return -5;
        }

        HGLRC temp_render_context = wglCreateContext(temp_dc);
        if(temp_render_context == nullptr) {
            return -6;
        }

        wglMakeCurrent(temp_dc, temp_render_context);

        // get some opengl function pointers

        init_gl_functions();

        // create actual window

        fullscreen = false;

        RECT rect = { 0, 0, 800, 600 };
        DWORD style = WS_OVERLAPPEDWINDOW;
        if(!AdjustWindowRect(&rect, style, false)) {
            return -7;
        }
        int window_width = rect.right - rect.left;
        int window_height = rect.bottom - rect.top;

        hwnd = CreateWindowExA(0, class_name, window_title, style, 0, 0, window_width, window_height, nullptr, nullptr, instance, this);
        if(hwnd == nullptr) {
            return -8;
        }

        window_dc = GetDC(hwnd);
        if(window_dc == nullptr) {
            return -9;
        }

        // create actual render context

        // clang-format off

        static constexpr int const pixelAttribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            0 };

        static constexpr int const contextAttributes[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 0,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0 };

        // clang-format on

        int pixelFormat;
        UINT numFormats;
        BOOL status = wglChoosePixelFormatARB(window_dc, pixelAttribs, nullptr, 1, &pixelFormat, &numFormats);
        if(!status || numFormats == 0) {
            return -10;
        }
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(window_dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if(!SetPixelFormat(window_dc, pixelFormat, &pfd)) {
            return -11;
        }
        render_context = wglCreateContextAttribsARB(window_dc, 0, contextAttributes);
        if(render_context == nullptr) {
            return -12;
        }

        // destroy temp window

        wglMakeCurrent(temp_dc, NULL);
        wglDeleteContext(temp_render_context);
        ReleaseDC(temp_hwnd, temp_dc);
        DestroyWindow(temp_hwnd);

        // done

        wglMakeCurrent(window_dc, render_context);

        wglSwapIntervalEXT(1);

        return 0;
    }
};

//////////////////////////////////////////////////////////////////////

int main(int, char **)
{
    gl_window window;
    window.init();

    gl_program program;
    program.init(vertex_shader_source, fragment_shader_source);

    gl_vertex_array verts{};
    verts.init(program);

    center_window_on_default_monitor(window.hwnd);

    ShowWindow(window.hwnd, SW_SHOW);

    MSG msg;
    bool quit{ false };
    do {
        while(PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        window.clear();
        window.draw();
        window.swap();

    } while(!quit);
}