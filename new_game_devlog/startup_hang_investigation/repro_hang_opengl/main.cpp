// Minimal Win32 + OpenGL hang repro — analogue of OpenChaos stars path.
//
// Pure Win32 + WGL, no external dependencies. Links only opengl32/gdi32/user32.
//
// Reproduces the startup_hang bug triggered by the stars render path in the
// game (ge_lock_screen → glReadPixels → CPU pixel plot → glGenTextures /
// glTexImage2D / fullscreen quad / glDeleteTextures → swap).
//
// NOTE: hardware-specific — on the author's box (Windows 11 + NVIDIA) it
// reproduces at READBACK_CYCLES_PER_FRAME=8. On other machines with the same
// binary it may not reproduce at all. Same readback pattern reproduces the
// hang on DirectX (via DirectDraw Lock) — not OpenGL-specific.
//
// Per frame:
//   1. glClear backbuffer.
//   2. Draw a colored triangle (populate backbuffer with real render output).
//   3. Repeat CYCLES times:
//      a. glReadPixels full backbuffer (sync GPU→CPU).
//      b. Y-flip + scattered CPU writes (~500 random pixels, mimics SKY_draw_stars).
//      c. glGenTextures + glBindTexture + glTexParameteri×2 + glTexImage2D.
//      d. Draw fullscreen textured quad (writeback step).
//      e. glDeleteTextures.
//   4. SwapBuffers.
//
// Context: OpenGL 4.1 core profile, MSAA 4×, 640×480, SwapInterval=2.

// Number of lock/unlock cycles per frame. 8 reproduces reliably on affected
// hardware. Lower values are marginal; >=2 is the threshold for hang to occur
// at all. Tunable if repro doesn't hit on a given machine.
#define READBACK_CYCLES_PER_FRAME 8

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

static const int WIDTH  = 640;
static const int HEIGHT = 480;

// ---- WGL extension constants ----
#define WGL_DRAW_TO_WINDOW_ARB             0x2001
#define WGL_ACCELERATION_ARB               0x2003
#define WGL_SUPPORT_OPENGL_ARB             0x2010
#define WGL_DOUBLE_BUFFER_ARB              0x2011
#define WGL_PIXEL_TYPE_ARB                 0x2013
#define WGL_COLOR_BITS_ARB                 0x2014
#define WGL_DEPTH_BITS_ARB                 0x2022
#define WGL_STENCIL_BITS_ARB               0x2023
#define WGL_FULL_ACCELERATION_ARB          0x2027
#define WGL_TYPE_RGBA_ARB                  0x202B
#define WGL_SAMPLE_BUFFERS_ARB             0x2041
#define WGL_SAMPLES_ARB                    0x2042
#define WGL_CONTEXT_MAJOR_VERSION_ARB      0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB      0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB       0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB   0x00000001

// ---- GL 2.0+ constants (missing from legacy gl.h on Windows) ----
#define GL_ARRAY_BUFFER        0x8892
#define GL_STATIC_DRAW         0x88E4
#define GL_VERTEX_SHADER       0x8B31
#define GL_FRAGMENT_SHADER     0x8B30
#define GL_COMPILE_STATUS      0x8B81
#define GL_LINK_STATUS         0x8B82
#define GL_TEXTURE0            0x84C0
#define GL_SAMPLES             0x80A9
#define GL_RGBA8               0x8058

// ---- WGL extension function pointers ----
typedef BOOL  (WINAPI *PFNWGLCHOOSEPIXELFORMATARB)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARB)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI *PFNWGLSWAPINTERVALEXT)(int);

static PFNWGLCHOOSEPIXELFORMATARB     p_wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARB  p_wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXT          p_wglSwapIntervalEXT;

// ---- GL 2.0+ function pointers (loaded via wglGetProcAddress) ----
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void   (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void   (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void   (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef GLuint (APIENTRY *PFN_glCreateProgram)();
typedef void   (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void   (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void   (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef void   (APIENTRY *PFN_glUseProgram)(GLuint);
typedef void   (APIENTRY *PFN_glDeleteShader)(GLuint);
typedef void   (APIENTRY *PFN_glGenVertexArrays)(GLsizei, GLuint*);
typedef void   (APIENTRY *PFN_glBindVertexArray)(GLuint);
typedef void   (APIENTRY *PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void   (APIENTRY *PFN_glBindBuffer)(GLenum, GLuint);
typedef void   (APIENTRY *PFN_glBufferData)(GLenum, ptrdiff_t, const void*, GLenum);
typedef void   (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);
typedef void   (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef GLint  (APIENTRY *PFN_glGetUniformLocation)(GLuint, const char*);
typedef void   (APIENTRY *PFN_glUniform1i)(GLint, GLint);
typedef void   (APIENTRY *PFN_glActiveTexture)(GLenum);

static PFN_glCreateShader   p_glCreateShader;
static PFN_glShaderSource   p_glShaderSource;
static PFN_glCompileShader  p_glCompileShader;
static PFN_glGetShaderiv    p_glGetShaderiv;
static PFN_glGetShaderInfoLog p_glGetShaderInfoLog;
static PFN_glCreateProgram  p_glCreateProgram;
static PFN_glAttachShader   p_glAttachShader;
static PFN_glLinkProgram    p_glLinkProgram;
static PFN_glGetProgramiv   p_glGetProgramiv;
static PFN_glGetProgramInfoLog p_glGetProgramInfoLog;
static PFN_glUseProgram     p_glUseProgram;
static PFN_glDeleteShader   p_glDeleteShader;
static PFN_glGenVertexArrays p_glGenVertexArrays;
static PFN_glBindVertexArray p_glBindVertexArray;
static PFN_glGenBuffers     p_glGenBuffers;
static PFN_glBindBuffer     p_glBindBuffer;
static PFN_glBufferData     p_glBufferData;
static PFN_glEnableVertexAttribArray p_glEnableVertexAttribArray;
static PFN_glVertexAttribPointer p_glVertexAttribPointer;
static PFN_glGetUniformLocation p_glGetUniformLocation;
static PFN_glUniform1i      p_glUniform1i;
static PFN_glActiveTexture  p_glActiveTexture;

static void* gl_get_proc(const char* name)
{
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 || p == (void*)1 || p == (void*)2 || p == (void*)3 || p == (void*)-1) {
        HMODULE mod = GetModuleHandleA("opengl32.dll");
        if (!mod) mod = LoadLibraryA("opengl32.dll");
        p = (void*)GetProcAddress(mod, name);
    }
    return p;
}

#define LOAD(name) p_##name = (PFN_##name)gl_get_proc(#name)

static void load_gl_funcs()
{
    LOAD(glCreateShader);   LOAD(glShaderSource);   LOAD(glCompileShader);
    LOAD(glGetShaderiv);    LOAD(glGetShaderInfoLog);
    LOAD(glCreateProgram);  LOAD(glAttachShader);   LOAD(glLinkProgram);
    LOAD(glGetProgramiv);   LOAD(glGetProgramInfoLog);
    LOAD(glUseProgram);     LOAD(glDeleteShader);
    LOAD(glGenVertexArrays);LOAD(glBindVertexArray);
    LOAD(glGenBuffers);     LOAD(glBindBuffer);     LOAD(glBufferData);
    LOAD(glEnableVertexAttribArray); LOAD(glVertexAttribPointer);
    LOAD(glGetUniformLocation); LOAD(glUniform1i);  LOAD(glActiveTexture);
}

// ---- Shader helpers ----

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint s = p_glCreateShader(type);
    p_glShaderSource(s, 1, &src, NULL);
    p_glCompileShader(s);
    GLint ok = 0;
    p_glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        p_glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[repro] shader compile FAIL: %s\n", log);
        exit(2);
    }
    return s;
}

static GLuint link_program(GLuint vs, GLuint fs)
{
    GLuint p = p_glCreateProgram();
    p_glAttachShader(p, vs);
    p_glAttachShader(p, fs);
    p_glLinkProgram(p);
    GLint ok = 0;
    p_glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        p_glGetProgramInfoLog(p, sizeof(log), NULL, log);
        fprintf(stderr, "[repro] program link FAIL: %s\n", log);
        exit(2);
    }
    return p;
}

// ---- Shader sources ----

static const char* VS_TRI =
    "#version 410 core\n"
    "layout(location=0) in vec2 a_pos;\n"
    "layout(location=1) in vec3 a_color;\n"
    "out vec3 v_color;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_color = a_color; }\n";

static const char* FS_TRI =
    "#version 410 core\n"
    "in vec3 v_color;\n"
    "out vec4 frag;\n"
    "void main() { frag = vec4(v_color, 1.0); }\n";

static const char* VS_QUAD =
    "#version 410 core\n"
    "layout(location=0) in vec2 a_pos;\n"
    "layout(location=1) in vec2 a_uv;\n"
    "out vec2 v_uv;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";

static const char* FS_QUAD =
    "#version 410 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 frag;\n"
    "void main() { frag = texture(u_tex, v_uv); }\n";

// ---- Window proc ----

static bool g_quit = false;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CLOSE:
    case WM_DESTROY:
        g_quit = true;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (w == VK_ESCAPE) { g_quit = true; PostQuitMessage(0); return 0; }
        break;
    }
    return DefWindowProcW(h, m, w, l);
}

// ---- Load WGL extensions via dummy context ----

static bool load_wgl_extensions()
{
    WNDCLASSW wcd = {};
    wcd.lpfnWndProc   = DefWindowProcW;
    wcd.hInstance     = GetModuleHandleW(NULL);
    wcd.lpszClassName = L"OCReproDummy";
    RegisterClassW(&wcd);

    HWND dw = CreateWindowExW(0, wcd.lpszClassName, L"dummy",
        WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL, wcd.hInstance, NULL);
    HDC ddc = GetDC(dw);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize         = sizeof(pfd);
    pfd.nVersion      = 1;
    pfd.dwFlags       = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType    = PFD_TYPE_RGBA;
    pfd.cColorBits    = 32;
    pfd.cDepthBits    = 24;
    pfd.cStencilBits  = 8;
    pfd.iLayerType    = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(ddc, &pfd);
    SetPixelFormat(ddc, pf, &pfd);

    HGLRC drc = wglCreateContext(ddc);
    wglMakeCurrent(ddc, drc);

    p_wglChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARB)   wglGetProcAddress("wglChoosePixelFormatARB");
    p_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARB)wglGetProcAddress("wglCreateContextAttribsARB");
    p_wglSwapIntervalEXT         = (PFNWGLSWAPINTERVALEXT)        wglGetProcAddress("wglSwapIntervalEXT");

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(drc);
    ReleaseDC(dw, ddc);
    DestroyWindow(dw);
    UnregisterClassW(wcd.lpszClassName, wcd.hInstance);

    return p_wglChoosePixelFormatARB && p_wglCreateContextAttribsARB;
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    setvbuf(stderr, NULL, _IONBF, 0);

    if (!load_wgl_extensions()) {
        fprintf(stderr, "WGL extensions unavailable\n"); return 1;
    }

    // Real window.
    HINSTANCE hinst = GetModuleHandleW(NULL);
    WNDCLASSW wc = {};
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"OCReproMain";
    RegisterClassW(&wc);

    RECT rc = { 0, 0, WIDTH, HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName,
        L"OC stars-analogue hang repro",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hinst, NULL);
    ShowWindow(hwnd, SW_SHOW);
    HDC hdc = GetDC(hwnd);

    // Choose multisample pixel format.
    int pf_attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        WGL_SAMPLE_BUFFERS_ARB, 1,
        WGL_SAMPLES_ARB,        4,
        0
    };
    int pf = 0;
    UINT num = 0;
    if (!p_wglChoosePixelFormatARB(hdc, pf_attribs, NULL, 1, &pf, &num) || num == 0) {
        fprintf(stderr, "wglChoosePixelFormatARB failed\n"); return 1;
    }
    PIXELFORMATDESCRIPTOR pfd = {};
    DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    // Core 4.1 context.
    int ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    HGLRC glrc = p_wglCreateContextAttribsARB(hdc, NULL, ctx_attribs);
    if (!glrc) { fprintf(stderr, "wglCreateContextAttribsARB failed\n"); return 1; }
    wglMakeCurrent(hdc, glrc);

    if (p_wglSwapIntervalEXT) p_wglSwapIntervalEXT(2);

    load_gl_funcs();

    fprintf(stderr, "[repro] GL vendor:   %s\n", glGetString(GL_VENDOR));
    fprintf(stderr, "[repro] GL renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, "[repro] GL version:  %s\n", glGetString(GL_VERSION));

    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    fprintf(stderr, "[repro] MSAA samples: %d\n", samples);
    fprintf(stderr, "[repro] CYCLES=%d\n", READBACK_CYCLES_PER_FRAME);

    glViewport(0, 0, WIDTH, HEIGHT);

    // Build programs.
    GLuint vs_tri  = compile_shader(GL_VERTEX_SHADER,   VS_TRI);
    GLuint fs_tri  = compile_shader(GL_FRAGMENT_SHADER, FS_TRI);
    GLuint prog_tri = link_program(vs_tri, fs_tri);
    p_glDeleteShader(vs_tri); p_glDeleteShader(fs_tri);

    GLuint vs_q = compile_shader(GL_VERTEX_SHADER,   VS_QUAD);
    GLuint fs_q = compile_shader(GL_FRAGMENT_SHADER, FS_QUAD);
    GLuint prog_quad = link_program(vs_q, fs_q);
    p_glDeleteShader(vs_q); p_glDeleteShader(fs_q);
    GLint u_tex = p_glGetUniformLocation(prog_quad, "u_tex");

    // Triangle VAO/VBO.
    float tri_verts[] = {
        -0.8f, -0.8f,  1.0f, 0.2f, 0.2f,
         0.8f, -0.8f,  0.2f, 1.0f, 0.2f,
         0.0f,  0.8f,  0.2f, 0.2f, 1.0f,
    };
    GLuint vao_tri = 0, vbo_tri = 0;
    p_glGenVertexArrays(1, &vao_tri); p_glBindVertexArray(vao_tri);
    p_glGenBuffers(1, &vbo_tri); p_glBindBuffer(GL_ARRAY_BUFFER, vbo_tri);
    p_glBufferData(GL_ARRAY_BUFFER, sizeof(tri_verts), tri_verts, GL_STATIC_DRAW);
    p_glEnableVertexAttribArray(0);
    p_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    p_glEnableVertexAttribArray(1);
    p_glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    // Fullscreen quad VAO/VBO.
    float quad_verts[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    GLuint vao_q = 0, vbo_q = 0;
    p_glGenVertexArrays(1, &vao_q); p_glBindVertexArray(vao_q);
    p_glGenBuffers(1, &vbo_q); p_glBindBuffer(GL_ARRAY_BUFFER, vbo_q);
    p_glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    p_glEnableVertexAttribArray(0);
    p_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    p_glEnableVertexAttribArray(1);
    p_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    p_glBindVertexArray(0);

    // CPU screen buffer.
    unsigned char* s_dummy_screen = (unsigned char*)calloc(1, WIDTH * HEIGHT * 4);
    const int s_screen_w = WIDTH, s_screen_h = HEIGHT;
    const int s_screen_pitch = WIDTH * 4;

    ULONGLONG start_ms      = GetTickCount64();
    ULONGLONG last_log_ms   = start_ms;
    int frame_count         = 0;
    int frame_since_log     = 0;
    int max_ms_since_log    = 0;

    unsigned int rng = 0x1234ABCDu;

    MSG msg;
    while (!g_quit) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_quit = true;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (g_quit) break;

        ULONGLONG frame_begin = GetTickCount64();

        // 1. Clear.
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 2. Triangle — make the backbuffer non-trivial so the driver can't
        //    elide readpixels as a no-op against a cleared surface.
        p_glUseProgram(prog_tri);
        p_glBindVertexArray(vao_tri);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 3. Repeated read+writeback — core of the repro.
        for (int cyc = 0; cyc < READBACK_CYCLES_PER_FRAME; cyc++) {
            // a. glReadPixels — sync GPU→CPU read of the whole backbuffer.
            glReadPixels(0, 0, s_screen_w, s_screen_h,
                         GL_RGBA, GL_UNSIGNED_BYTE, s_dummy_screen);

            // b. CPU work on the readback buffer.
            //    Y-flip (1.2 MB sequential read+write) + scattered random
            //    writes (~500 pixels over the full buffer). Scattered pattern
            //    is what differentiates — sequential writes alone don't
            //    trigger. See FINDINGS.md.
            unsigned char* top = s_dummy_screen;
            unsigned char* bot = s_dummy_screen + (s_screen_h - 1) * s_screen_pitch;
            while (top < bot) {
                for (int x = 0; x < s_screen_pitch; x += 16) {
                    int chunk = (s_screen_pitch - x < 16) ? (s_screen_pitch - x) : 16;
                    unsigned char tmp[16];
                    memcpy(tmp, top + x, chunk);
                    memcpy(top + x, bot + x, chunk);
                    memcpy(bot + x, tmp, chunk);
                }
                top += s_screen_pitch;
                bot -= s_screen_pitch;
            }
            for (int i = 0; i < 500; i++) {
                rng = rng * 1664525u + 1013904223u;
                int px = (int)((rng >> 8)  % s_screen_w);
                int py = (int)((rng >> 16) % s_screen_h);
                unsigned char* p = s_dummy_screen + py * s_screen_pitch + px * 4;
                p[0] = p[1] = p[2] = 255; p[3] = 255;
            }

            // c. Create a temp texture and upload the modified buffer.
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_screen_w, s_screen_h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, s_dummy_screen);

            // d. Draw fullscreen quad with the uploaded texture — forces the
            //    driver to actually commit the upload.
            p_glUseProgram(prog_quad);
            p_glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            p_glUniform1i(u_tex, 0);
            p_glBindVertexArray(vao_q);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // e. Delete the temp texture.
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &tex);
        }

        // 4. Swap (VSync-blocked — required for the hang to trigger).
        SwapBuffers(hdc);

        ULONGLONG frame_end = GetTickCount64();
        int frame_ms = (int)(frame_end - frame_begin);
        if (frame_ms > max_ms_since_log) max_ms_since_log = frame_ms;

        frame_count++;
        frame_since_log++;

        if (frame_end - last_log_ms >= 1000) {
            ULONGLONG total_ms = frame_end - start_ms;
            float fps = frame_since_log * 1000.0f / (float)(frame_end - last_log_ms);
            fprintf(stderr,
                "[repro] t=%4llus frame=%6d fps=%5.1f max_frame_ms=%4d%s\n",
                (unsigned long long)(total_ms / 1000), frame_count, fps, max_ms_since_log,
                max_ms_since_log > 80 ? "  <-- HANG" : "");
            wchar_t title[160];
            _snwprintf(title, 160,
                L"OC repro  t=%llus  frame=%d  fps=%.1f  max_ms=%d%s",
                (unsigned long long)(total_ms / 1000), frame_count, fps, max_ms_since_log,
                max_ms_since_log > 80 ? L"  HANG" : L"");
            SetWindowTextW(hwnd, title);
            last_log_ms      = frame_end;
            frame_since_log  = 0;
            max_ms_since_log = 0;
        }
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(glrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hinst);
    free(s_dummy_screen);
    return 0;
}
