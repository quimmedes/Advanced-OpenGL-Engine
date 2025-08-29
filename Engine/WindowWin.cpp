#include "WindowWin.hpp"
#include "App.hpp"
#include <iostream>

// Global variables for window and OpenGL context
HDC hDCGlobal = NULL;
HGLRC hRCGlobal = NULL;

WindowWin::WindowWin() {
    // Constructor can be used for initialization if needed
}

WindowWin::~WindowWin() {
    // Clean up OpenGL context and device context
    if (hRCGlobal) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRCGlobal);
        hRCGlobal = NULL;
    }
    if (hDCGlobal) {
        ReleaseDC(NULL, hDCGlobal);
        hDCGlobal = NULL;
    }
}

bool WindowWin::CreateModernContext(HWND hWnd) {
	hDCGlobal = GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hDCGlobal, &pfd);
    if (!pf) return false;
    if (!SetPixelFormat(hDCGlobal, pf, &pfd)) return false;
    
    HGLRC tempContext = wglCreateContext(hDCGlobal);
    if (!tempContext) return false;
    if (!wglMakeCurrent(hDCGlobal, tempContext)) return false;
    
    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB)
    {
        int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 2,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0,0
        };
        hRCGlobal = wglCreateContextAttribsARB(hDCGlobal, 0, attribs);
        if (!hRCGlobal) return false;
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        if (!wglMakeCurrent(hDCGlobal, hRCGlobal)) return false;
    }
    else
    {
        hRCGlobal = tempContext;
    }
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            ValidateRect(hWnd, NULL);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

HWND hWndGlobal = NULL;

int WindowWin::WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OpenGLWindowClass";
    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"Error Registering Window Class", L"Erro", MB_OK | MB_ICONERROR);
        return -1;
    }

    hWndGlobal = CreateWindow(
        L"OpenGLWindowClass",
        L"Zero Game Engine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1940,
        1080,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    if (!hWndGlobal)
    {
        MessageBox(0, L"Error creating window.", L"Erro", MB_OK | MB_ICONERROR);
        return -1;
    }

    if (!CreateModernContext(hWndGlobal))
    {
        MessageBox(0, L"Error creating context opengl", L"Erro", MB_OK | MB_ICONERROR);
        return -1;
    }

    return 0;
}

bool WindowWin::Tick() {
    MSG msg;
    bool done, result;


    // Initialize the message structure.
    ZeroMemory(&msg, sizeof(MSG));

    // Loop until there is a quit message from the window or the user.
    done = false;
    while (!done)
    {
        // Handle the windows messages.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If windows signals to end the application then exit out.
        if (msg.message == WM_QUIT)
        {
            done = true;
        }
        else
        {
            // Otherwise do the frame processing.
            result = Engine::app->Tick();
            if (!result)
            {
                done = true;
            }
        }

    }
    return static_cast<int>(msg.wParam);
}

bool WindowWin::Init() {

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WinMain(hInstance, NULL, NULL, SW_SHOWDEFAULT);
    return true;
}

