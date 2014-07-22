// ================================================================================================
// BltTest: A simple benchmark for measuring BitBlt speed.
//          Minimal timing overhead, uses DIBSections.
//
// Copyright Michael Herf, 2000.  All Rights Reserved.
//
// Results may be published and used for comparison, 
//  but no liability for their accuracy or suitability is assumed.
//
// Results may differ based on mouse movement and system activity.  For best results:
//	1.  Run on a completely idle system (no network cable or modem connection)
//  2.  Keep the mouse pointer away from the window
//  3.  Let test run for several seconds before clicking.
//  4.  Run in "True Color" or "32-bit Color" mode.
//
// ------------------------------------------------------------------------------------------------
//
// Notes: this test was developed as a way to showcase vast performance differences
//  between popular video cards.  Among current high-end cards (AGP 2x-4x), there is a large
//  variation in 2-D video performance not captured by current benchmarks.
//  However, this variation is very important in video playback and for software rendering.
//
// For reference
//  my ATI Rage Fury on a 440LX motherboard (p2-300) gets about 142 MB/sec.
//  my TNT2 Ultra on a P3-600 with AGP-4X gets about 69 MB/sec.  
//
// Believe it! This is the problem.
//
// Some additional comments:
//  I could have used DDraw, but the performance seems to be similar with that, and it's more code.
//  32-bit rendering is good, and no one doing serious imaging should be using 16 bit.
//  This test should be important, and I don't understand why no one benchmarks this stuff!
//
// ================================================================================================

#include "stdafx.h"
#include <stdio.h>

static int done = 0;

const int kWidth  = 640;
const int kHeight = 480;

//LARGE_INTEGER tot;
//int framecount = 0;
double min = 100000000000;

// ================================================================================================
// A simple windowproc that allows us to exit the app, handle clicks, etc.
// ================================================================================================
LRESULT __stdcall WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

		case WM_DESTROY:
			done = 1;
			break;
		case WM_ERASEBKGND:
			return 0;
			break;

		case WM_LBUTTONUP:
			{
				char perf[1024];
				LARGE_INTEGER f;
				QueryPerformanceFrequency(&f);

				//double fps = (double)f.QuadPart * framecount / (double)tot.QuadPart;
				double fps = f.QuadPart / min;

				float size = kWidth*kHeight*4;
				//sprintf(perf, "Averaged %.1f frames per second (%.1f MB/sec bandwidth).", fps, fps * size / 1024 / 1024);
				sprintf(perf, "Best is %.1f frames per second (%.1f MB/sec bandwidth).", fps, fps * size / 1024 / 1024);
				if (MessageBox(hwnd, perf, "Performance Results (640x480 32-bit ARGB top-down DIB)", MB_OKCANCEL) == IDCANCEL) {
					done = 1;
				}

			}
			break;

		case WM_QUIT:
			done = 1;
			break;

		default:
			break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ================================================================================================
// Register a window class with win32
// ================================================================================================
static ATOM Register(HINSTANCE hInstance)
{
	HINSTANCE hinst = (HINSTANCE)hInstance;

	WNDCLASS wc;

	// register the main window class
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "BltTest";

	return RegisterClass(&wc);
}


// ================================================================================================
// Run!
// ================================================================================================
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HWND hwnd;
	HBITMAP hBitmap;
	HDC hdc;
	unsigned long *rawBits = NULL;

	Register(hInstance);

	SetCursorPos(0, 0);

	//tot.QuadPart = 0;

	// ------------------------------------------------------------------------------------------------
	// Create a window
	// ------------------------------------------------------------------------------------------------
	{
		hwnd = ::CreateWindowEx(WS_EX_TOPMOST, "BltTest", "", WS_POPUP | WS_EX_TOOLWINDOW,
		 					   256, 16, kWidth, kHeight,
							   (HWND)NULL, (HMENU)NULL, hInstance, LPVOID(NULL));

		::ShowWindow((HWND)hwnd, SW_SHOWNORMAL);

		if (!hwnd) return -1;
	}

	// ------------------------------------------------------------------------------------------------'
	// Create a bitmap
	// ------------------------------------------------------------------------------------------------
	{
		BITMAPINFO bmi;
		int bmihsize = sizeof(BITMAPINFOHEADER);
		memset(&bmi, 0, bmihsize);

		BITMAPINFOHEADER &h = bmi.bmiHeader;

		h.biSize		= bmihsize;
		h.biWidth		= kWidth;
		h.biHeight		= -kHeight;
		h.biPlanes		= 1;
		h.biBitCount	= 32;
		h.biCompression	= BI_RGB;

		hdc = CreateCompatibleDC(NULL);
		hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&rawBits, NULL, 0);

		if (!hdc || !hBitmap) return -1;
	}


	// ------------------------------------------------------------------------------------------------
	// Simple contents
	// ------------------------------------------------------------------------------------------------
	for (int y = 0; y < kHeight; y++) {
		unsigned long *p0 = rawBits + kWidth * y;
		for (int x = 0; x < kWidth; x++) {
			unsigned long g = (x ^ y) & 0xFF;
			p0[x] = (g << 16) + (g << 8) + g;
		}
	}

	// ------------------------------------------------------------------------------------------------
	// Where we'll draw
	// ------------------------------------------------------------------------------------------------
	HDC dc = (HDC)GetDC(hwnd);
	HBITMAP oldbitmap = (HBITMAP)SelectObject((HDC)hdc, hBitmap);

	// ------------------------------------------------------------------------------------------------
	// main loop
	// ------------------------------------------------------------------------------------------------
	while (!done) {

		int result = 1;
		MSG msg;

		while (result) {
			result = PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE);
			
			if (result != 0) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			//Sleep(1);
		}

		// Message dispatching done, so draw a frame and measure timing
		LARGE_INTEGER s1, s2;
		QueryPerformanceCounter(&s1);
		::BitBlt(dc, 0, 0, kWidth, kHeight, (HDC)hdc, 0, 0, SRCCOPY);
		::GdiFlush();
		QueryPerformanceCounter(&s2);

		LARGE_INTEGER diff;
		diff.QuadPart = s2.QuadPart - s1.QuadPart;
		if (diff.QuadPart < min) min = diff.QuadPart;
		//tot.QuadPart += s2.QuadPart - s1.QuadPart;
		//framecount ++;
	}

	SelectObject((HDC)hdc, oldbitmap);
	DeleteDC(dc);
	
	return 0;
}
