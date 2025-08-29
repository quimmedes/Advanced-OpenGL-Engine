#pragma once  
#include <windows.h>  
#include <iostream>  


#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB  
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091  
#endif  
#ifndef WGL_CONTEXT_MINOR_VERSION_ARB  
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092  
#endif  
#ifndef WGL_CONTEXT_PROFILE_MASK_ARB  
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126  
#endif  
#ifndef WGL_CONTEXT_CORE_PROFILE_BIT_ARB  
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001  
#endif  

extern HDC hDCGlobal;  
extern HGLRC hRCGlobal;  
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);  

class WindowWin {  

public:  

	WindowWin();
	~WindowWin();
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow);  
	bool CreateModernContext(HWND hWnd);  
	bool Init();  
	bool Tick();

};