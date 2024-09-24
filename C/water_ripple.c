#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "water_ripple.h"

#define szCap                  "Water Ripple Demo"
#define szTitle                "Error"
#define szError                "An error has occured"

WAVE_OBJECT stWaveObj;
HBITMAP hBitmap;

void _Quit(HWND xWin) {
    _WaveFree(&stWaveObj);
    DestroyWindow(xWin);
    PostQuitMessage(0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInstance = GetModuleHandle(NULL);
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_WATER_RIPPLE), NULL, DlgProc, NULL);
}

INT_PTR CALLBACK DlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT stPs;
    HDC hDc;
    RECT stRect;
    HDC hMemDC;
    HDC updelete;

    switch (uMsg) {
    case WM_INITDIALOG:
        hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(LOGO));
        
        // Elliptical water ripples (used for perspective effects)
        //if (_WaveInit(&stWaveObj, hWin, hBitmap, 30, 1)) {
        // Circular water ripples
        if (_WaveInit(&stWaveObj, hWin, hBitmap, 30, 0)) {
            MessageBox(hWin, _T(szError), _T(szTitle), MB_OK | MB_ICONSTOP);
            _Quit(hWin);
        }
        
        DeleteObject(hBitmap);
        _WaveEffect(&stWaveObj, 1, 5, 4, 250); // Rain
        //_WaveEffect(&stWaveObj, 2, 4, 2, 400); // Motorboat
        //_WaveEffect(&stWaveObj, 3, 100, 3, 7); // Wind Waves
        break;

    case WM_PAINT:
        hDc = BeginPaint(hWin, &stPs);
        hMemDC = CreateCompatibleDC(hDc);
        SelectObject(hMemDC, hBitmap);
        GetClientRect(hWin, &stRect);
        BitBlt(hDc, 10, 10, stRect.right, stRect.bottom, hMemDC, 0, 0, MERGECOPY);
        updelete = (HDC)DeleteDC(hMemDC);
        _WaveUpdateFrame(&stWaveObj, updelete, TRUE);
        EndPaint(hWin, &stPs);
        return 0;

    case WM_DESTROY:
        DeleteObject(hBitmap);
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        _Quit(hWin);
        EndDialog(hWin, 0);
        return 0;
    }

    return 0;
}