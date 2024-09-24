#include <stdint.h>
#include <windows.h>

// Constant definitions
#define IDD_WATER_RIPPLE            1001
#define LOGO                        1002
#define MYICON                      1003

// WAVE_OBJECT structure definition
typedef struct WAVE_OBJECT {
HWND hWnd;              // Window handle
uint32_t dwFlag;           // Refer to the F_WO_xxx combination

// Rendering components
HDC hDcRender;
HBITMAP hBmpRender;
uint8_t* lpDIBitsSource;   // Original pixel data
uint8_t* lpDIBitsRender;   // Pixel data used for displaying on the screen
uint32_t* lpWave1;         // Water ripple energy data buffer 1
uint32_t* lpWave2;         // Water ripple energy data buffer 2

// Bitmap dimensions
uint32_t dwBmpWidth;
uint32_t dwBmpHeight;
uint32_t dwDIByteWidth;    // = (dwBmpWidth * 3 + 3) & ~3
uint32_t dwWaveByteWidth;  // = dwBmpWidth * 4
uint32_t dwRandom;

// Special Effect Parameters
uint32_t dwEffectType;
uint32_t dwEffectParam1;
uint32_t dwEffectParam2;
uint32_t dwEffectParam3;

// Used for boat effect
uint32_t dwEff2X;
uint32_t dwEff2Y;
int32_t dwEff2XAdd;
int32_t dwEff2YAdd;
uint32_t dwEff2Flip;

BITMAPINFO stBmpInfo;   // Bitmap information structure
} WAVE_OBJECT;

// Function prototype
INT_PTR CALLBACK DlgProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam);
int _WaveInit(WAVE_OBJECT* lpWaveObject, HWND hWnd, HBITMAP hBmp, DWORD dwSpeed, DWORD dwType);
void _WaveEffect(WAVE_OBJECT* lpWaveObject, uint32_t dwType, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3);
void _WaveUpdateFrame(WAVE_OBJECT* lpWaveObject, HDC _hDc, BOOL _bIfForce);
void _WaveFree(WAVE_OBJECT* lpWaveObject);

