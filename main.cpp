#include <stdio.h>
#include <stdint.h>

#include <format>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gl/GL.h>

#include <vector>

#include "Wglext.h"
#include "glcorearb.h"

#include "gl_functions.h"
#include "polypartition.h"

#pragma comment(lib, "opengl32.lib")

//////////////////////////////////////////////////////////////////////

namespace
{
template <typename... args> constexpr void log(char const *fmt, args &&...arguments)
{
    std::string s = std::vformat(fmt, std::make_format_args(arguments...));
    puts(s.c_str());
}

struct vert
{
    float x, y;
    uint32_t color;
};

vert vertices[] = {
    100, 100, 0xff00ff00,    // ABGR
    700, 100, 0xffff0000,    //
    700, 500, 0xff0000ff,    //
    100, 500, 0xffff00ff,    //
};

GLushort indices[6] = {
    0, 1, 2,    //
    0, 2, 3,    //
};

char const *vertex_shader_source = R"-----(

#version 400
in vec2 positionIn;
in vec4 colorIn;
out vec4 fragmentColor;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(positionIn, 0.0f, 1.0f);
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

using matrix = float[16];

void make_ortho(matrix mat, int w, int h)
{
    mat[0] = 2.0f / w;
    mat[1] = 0.0f;
    mat[2] = 0.0f;
    mat[3] = -1.0f;
    mat[4] = 0.0f;
    mat[5] = 2.0f / h;
    mat[6] = 0.0f;
    mat[7] = -1.0f;
    mat[8] = 0.0f;
    mat[9] = 0.0f;
    mat[10] = -1.0f;
    mat[11] = 0.0f;
    mat[12] = 0.0f;
    mat[13] = 0.0f;
    mat[14] = 0.0f;
    mat[15] = 1.0f;
}

}    // namespace

//////////////////////////////////////////////////////////////////////

struct gl_program
{
    GLuint program_id{};
    GLuint vertex_shader_id{};
    GLuint fragment_shader_id{};
    GLuint projection_location{};

    matrix projection_matrix{};

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

        projection_location = glGetUniformLocation(program_id, "projection");
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    void resize(int w, int h)
    {
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
        glGenBuffers(1, &vbo_id);
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        return 0;
    }

    int activate(gl_program &program)
    {
        glBindVertexArray(vao_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

        GLint positionLocation = glGetAttribLocation(program.program_id, "positionIn");
        GLint colorLocation = glGetAttribLocation(program.program_id, "colorIn");

        glEnableVertexAttribArray(positionLocation);
        glEnableVertexAttribArray(colorLocation);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vert) * 8192, nullptr, GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 8192, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(vert), (void *)(offsetof(vert, x)));
        glVertexAttribPointer(colorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vert), (void *)(offsetof(vert, color)));

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

    std::function<void(int, int)> on_draw{};
    std::function<void(int, int)> on_left_click{};
    std::function<void(int)> on_key_press{};

    static constexpr char const *class_name = "GL_CONTEXT_WINDOW_CLASS";
    static constexpr char const *window_title = "GL Window";

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK wnd_proc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;

        switch(message) {

        case WM_SIZE: {
            draw();
            swap();
        } break;

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            on_left_click(x, y);
        } break;

        case WM_KEYDOWN:

            switch(wParam) {

            case VK_ESCAPE: {
                DestroyWindow(hwnd);
            } break;

            default:
                on_key_press((int)wParam);
                break;
            }
            break;

        case WM_CLOSE:
            // free_gl_resources();
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            wglMakeCurrent(window_dc, NULL);
            wglDeleteContext(render_context);
            render_context = nullptr;
            ReleaseDC(hwnd, window_dc);
            PostQuitMessage(0);
            break;

        default:
            result = DefWindowProcA(hwnd, message, wParam, lParam);
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////

    void draw()
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right;
        int h = rc.bottom;
        on_draw(w, h);
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

        WNDCLASSEXA wcex{};
        memset(&wcex, 0, sizeof(wcex));
        wcex.cbSize = sizeof(WNDCLASSEXA);
        wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcex.lpfnWndProc = (WNDPROC)wnd_proc_proxy;
        wcex.hInstance = instance;
        wcex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.lpszClassName = class_name;

        if(!RegisterClassExA(&wcex)) {
            return -1;
        }

        // create temp render context

        HWND temp_hwnd = CreateWindowExA(0, class_name, "", 0, 0, 0, 1, 1, nullptr, nullptr, instance, nullptr);
        if(temp_hwnd == nullptr) {
            return -2;
        }

        HDC temp_dc = GetDC(temp_hwnd);
        if(temp_dc == nullptr) {
            return -3;
        }

        PIXELFORMATDESCRIPTOR temp_pixel_format_desc{};
        temp_pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        temp_pixel_format_desc.nVersion = 1;
        temp_pixel_format_desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        temp_pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
        temp_pixel_format_desc.cColorBits = 32;
        temp_pixel_format_desc.cAlphaBits = 8;
        temp_pixel_format_desc.cDepthBits = 24;
        int temp_pixelFormat = ChoosePixelFormat(temp_dc, &temp_pixel_format_desc);
        if(temp_pixelFormat == 0) {
            return -4;
        }

        if(!SetPixelFormat(temp_dc, temp_pixelFormat, &temp_pixel_format_desc)) {
            return -5;
        }

        HGLRC temp_render_context = wglCreateContext(temp_dc);
        if(temp_render_context == nullptr) {
            return -6;
        }

        // activate temp render context so we can...

        wglMakeCurrent(temp_dc, temp_render_context);

        // ...get some opengl function pointers

        init_gl_functions();

        // now opengl functions are available, create actual window

        fullscreen = false;

        RECT rect{ 0, 0, 800, 600 };
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

        static constexpr int const pixel_attributes[] = {
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

        static constexpr int const context_attributes[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 0,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0 };

        // clang-format on

        int pixel_format;
        UINT num_formats;
        BOOL status = wglChoosePixelFormatARB(window_dc, pixel_attributes, nullptr, 1, &pixel_format, &num_formats);
        if(!status || num_formats == 0) {
            return -10;
        }

        PIXELFORMATDESCRIPTOR pixel_format_desc{};
        DescribePixelFormat(window_dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format_desc);

        if(!SetPixelFormat(window_dc, pixel_format, &pixel_format_desc)) {
            return -11;
        }

        render_context = wglCreateContextAttribsARB(window_dc, 0, context_attributes);
        if(render_context == nullptr) {
            return -12;
        }

        // activate the true render context

        wglMakeCurrent(temp_dc, NULL);

        // destroy temp window

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

    std::vector<TPPLPoint> points;
    TPPLPolyList triangles;

    std::vector<vert> triangle_vertices;
    std::vector<GLushort> triangle_indices;

    window.on_key_press = [&](int k) {

        switch(k) {

        case VK_F11:
            window.set_fullscreen_state(!window.fullscreen);
            break;

        case 'C': {
            triangles.clear();
            triangle_vertices.clear();
            triangle_indices.clear();
            points.clear();
        } break;

        case 'T': {
            TPPLPoly poly;
            poly.Init((long)points.size());
            TPPLPoint *p = poly.GetPoints();
            memcpy(p, points.data(), sizeof(TPPLPoint) * points.size());
            TPPLPartition part;
            part.Triangulate_MONO(&poly, &triangles);

            triangle_vertices.clear();
            triangle_vertices.reserve(points.size());
            for(auto const &p : points) {
                triangle_vertices.emplace_back((float)p.x, (float)p.y, 0xff0000ff);
            }

            log("{} triangles", triangles.size());
            triangle_indices.clear();
            triangle_indices.reserve(triangles.size() * 3);
            for(auto const &t : triangles) {
                if(t.GetNumPoints() != 3) {
                    log("Huh?");
                } else {
                    for(int n = 0; n < 3; ++n) {
                        triangle_indices.push_back(t.GetPoint(n).id);
                    }
                }
            }
        } break;
        }
    };

    window.on_left_click = [&](int x, int y) {
        int n = (int)points.size();
        y = 600 - y;
        log("{},{},{}", x, y, n);
        points.emplace_back((float)x, (float)y, n);
    };

    window.on_draw = [&](int w, int h) {
        glViewport(0, 0, w, h);

        glClearColor(0.1f, 0.2f, 0.5f, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        matrix projection_matrix;
        make_ortho(projection_matrix, w, h);
        glUniformMatrix4fv(program.projection_location, 1, true, projection_matrix);

        verts.activate(program);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        GLushort *i = (GLushort *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        vert *v = (vert *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        memcpy(i, indices, sizeof(indices));
        memcpy(v, vertices, sizeof(vertices));
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid *)0);

        if(!triangle_vertices.empty()) {
            v = (vert *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            i = (GLushort *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
            memcpy(v, triangle_vertices.data(), triangle_vertices.size() * sizeof(vert));
            memcpy(i, triangle_indices.data(), triangle_indices.size() * sizeof(GLushort));
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glDrawElements(GL_TRIANGLES, (GLsizei)triangle_indices.size(), GL_UNSIGNED_SHORT, (GLvoid *)0);
        }

        if(points.size() >= 2) {
            v = (vert *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            i = (GLushort *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
            for(auto const &n : points) {
                *i = (GLushort)n.id;
                v->x = (float)n.x;
                v->y = (float)n.y;
                v->color = 0xffffffff;
                i += 1;
                v += 1;
            }
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glDrawElements(GL_LINE_STRIP, (GLsizei)points.size(), GL_UNSIGNED_SHORT, (GLvoid *)0);
        }
    };


    center_window_on_default_monitor(window.hwnd);

    ShowWindow(window.hwnd, SW_SHOW);

    MSG msg;
    bool quit{ false };
    while(!quit) {
        while(PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if(!quit) {
            window.draw();
            window.swap();
        }
    }
}