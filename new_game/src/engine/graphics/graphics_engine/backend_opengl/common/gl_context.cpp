// OpenGL 4.1 context creation via WGL on the existing Win32 HWND.

#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>

// WGL extension types for creating core profile contexts.
typedef HGLRC (WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_FLAGS_ARB             0x2094

// Pixel format extension types for modern pixel format selection.
typedef BOOL (WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const float*, UINT, int*, UINT*);
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023

extern volatile HWND hDDLibWindow;

// Combined GL function loader: wglGetProcAddress for extensions,
// GetProcAddress(opengl32) for GL 1.0-1.1 functions.
static HMODULE s_opengl32 = NULL;

static GLADapiproc gl_get_proc(const char* name)
{
    GLADapiproc proc = (GLADapiproc)wglGetProcAddress(name);
    if (!proc) {
        if (!s_opengl32) s_opengl32 = LoadLibraryA("opengl32.dll");
        if (s_opengl32) proc = (GLADapiproc)GetProcAddress(s_opengl32, name);
    }
    return proc;
}

static HDC   s_hdc   = NULL;
static HGLRC s_hglrc = NULL;
static int32_t s_width  = 640;
static int32_t s_height = 480;

// Helper: create a temporary context to get WGL extension function pointers.
static bool create_core_context(HDC hdc)
{
    // First, set a basic pixel format so we can create a temporary context.
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) return false;
    if (!SetPixelFormat(hdc, pf, &pfd)) return false;

    // Create a temporary legacy context to get wglCreateContextAttribsARB.
    HGLRC temp_ctx = wglCreateContext(hdc);
    if (!temp_ctx) return false;
    wglMakeCurrent(hdc, temp_ctx);

    auto wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    if (!wglCreateContextAttribsARB) {
        // Fallback: no ARB extension, use legacy context (won't be core profile).
        fprintf(stderr, "OpenGL: wglCreateContextAttribsARB not available, using legacy context\n");
        s_hglrc = temp_ctx;
        return true;
    }

    // Create the real 4.1 core profile context.
    int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    HGLRC core_ctx = wglCreateContextAttribsARB(hdc, NULL, attribs);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(temp_ctx);

    if (!core_ctx) {
        fprintf(stderr, "OpenGL: failed to create 4.1 core profile context\n");
        return false;
    }

    wglMakeCurrent(hdc, core_ctx);
    s_hglrc = core_ctx;
    return true;
}

bool gl_context_create(int32_t width, int32_t height)
{
    if (!hDDLibWindow) {
        fprintf(stderr, "OpenGL: hDDLibWindow is NULL\n");
        return false;
    }

    s_width = width;
    s_height = height;

    // Switch window style from WS_POPUP (set by host.cpp) to windowed with frame.
    // Matches D3D backend behavior in display.cpp OpenDisplay() for windowed mode.
    LONG style = GetWindowLong((HWND)hDDLibWindow, GWL_STYLE);
    style &= ~WS_POPUP;
    style |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;
    SetWindowLong((HWND)hDDLibWindow, GWL_STYLE, style);

    // Resize the window so the client area matches requested dimensions.
    RECT rc = { 0, 0, width, height };
    AdjustWindowRectEx(&rc, style, FALSE, GetWindowLong((HWND)hDDLibWindow, GWL_EXSTYLE));
    SetWindowPos((HWND)hDDLibWindow, HWND_TOP, 0, 0,
                 rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ShowWindow((HWND)hDDLibWindow, SW_SHOW);
    UpdateWindow((HWND)hDDLibWindow);

    s_hdc = GetDC((HWND)hDDLibWindow);
    if (!s_hdc) {
        fprintf(stderr, "OpenGL: GetDC failed\n");
        return false;
    }

    if (!create_core_context(s_hdc)) {
        fprintf(stderr, "OpenGL: context creation failed\n");
        ReleaseDC((HWND)hDDLibWindow, s_hdc);
        s_hdc = NULL;
        return false;
    }

    // Load GL function pointers via GLAD.
    int version = gladLoadGL((GLADloadfunc)gl_get_proc);
    if (!version) {
        fprintf(stderr, "OpenGL: GLAD failed to load GL functions\n");
        gl_context_destroy();
        return false;
    }

    fprintf(stderr, "OpenGL: %s\n", (const char*)glGetString(GL_VERSION));
    fprintf(stderr, "OpenGL: %s\n", (const char*)glGetString(GL_RENDERER));

    // Set initial viewport.
    glViewport(0, 0, width, height);

    return true;
}

void gl_context_destroy()
{
    if (s_hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(s_hglrc);
        s_hglrc = NULL;
    }
    if (s_hdc && hDDLibWindow) {
        ReleaseDC((HWND)hDDLibWindow, s_hdc);
        s_hdc = NULL;
    }
}

void gl_context_swap()
{
    if (s_hdc) {
        SwapBuffers(s_hdc);
    }
}

int32_t gl_context_get_width()  { return s_width; }
int32_t gl_context_get_height() { return s_height; }
