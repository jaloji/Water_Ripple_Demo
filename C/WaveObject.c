/*********************************************************************************
 * Water ripple effect common subroutine
 * by Luo Yunbin, http://asm.yeah.net, luoyunbin@sina.com
 * Version 1.0.041019 --- Initial version
 *********************************************************************************
 * To use this effect, include WaveObject.asm in your source code.
 * Then, you can call the functions as follows:
 *********************************************************************************/

 /**
  * 1. Create a water ripple object:
  *    To draw on a window, first create a water ripple object (this function allocates some buffers)
  *
  *    _WaveInit(&lpWaveObject, hWnd, hBmp, dwTime, dwType);
  *       lpWaveObject --> Pointer to an empty WAVE_OBJECT structure
  *       hWnd --> The window on which the water ripple effect will be drawn, the rendered image will be drawn to the client area of the window
  *       hBmp --> Background image, the drawing range is the same as the size of the background image
  *       dwTime --> Refresh interval (milliseconds), recommended value: 10~30
  *       dwType --> =0 indicates circular water ripples, =1 indicates elliptical water ripples (used for perspective effects)
  *       Return value: 0 (success, object initialized), 1 (failure)
  */

  /**
   * 2. If the _WaveInit function returns successfully, the object is initialized,
   *    and you can pass the object to various functions below to achieve different effects.
   *    The lpWaveObject parameter in the following functions points to the WAVE_OBJECT structure initialized by the _WaveInit function.
   *
   *    a) "Throw a stone" at a specified position, causing ripples:
   *       _WaveDropStone(&lpWaveObject, dwPosX, dwPosY, dwStoneSize, dwStoneWeight);
   *          dwPosX, dwPosY --> Position where the stone is thrown
   *          dwStoneSize --> Stone size, i.e., initial point size, recommended value: 0~5
   *          dwStoneWeight --> Stone weight, determines the range of the ripple, recommended value: 10~1000
   *
   *    b) Automatically display special effects:
   *       _WaveEffect(&lpWaveObject, dwEffectType, dwParam1, dwParam2, dwParam3);
   *          dwParam1, dwParam2, dwParam3 --> Effect parameters, meaning varies for different effect types
   *          dwEffectType --> Effect type
   *             0 --> Turn off the effect
   *             1 --> Rain:
   *                    Param1 = Intensity speed (0 is the densest, larger values are sparser), recommended value: 0~30
   *                    Param2 = Maximum raindrop diameter, recommended value: 0~5
   *                    Param3 = Maximum raindrop weight, recommended value: 50~250
   *             2 --> Motorboat:
   *                    Param1 = Speed (0 is the slowest, larger values are faster), recommended value: 0~8
   *                    Param2 = Boat size, recommended value: 0~4
   *                    Param3 = Range of water ripple diffusion, recommended value: 100~500
   *             3 --> Wind waves:
   *                    Param1 = Density (larger is denser), recommended value: 50~300
   *                    Param2 = Size, recommended value: 2~5
   *                    Param3 = Energy, recommended value: 5~10
   *
   *    c) Force update of the window client area (used to force update the client area in the window's WM_PAINT message):
   *     case WM_PAINT:
   *        hDc = BeginPaint(hWin, &stPs);
   *        hMemDC = CreateCompatibleDC(hDc);
   *        SelectObject(hMemDC, hBitmap);
   *        GetClientRect(hWin, &stRect);
   *        BitBlt(hDc, 10, 10, stRect.right, stRect.bottom, hMemDC, 0, 0, MERGECOPY);
   *        updelete = (HDC)DeleteDC(hMemDC);
   *        _WaveUpdateFrame(&stWaveObj, updelete, TRUE);
   *        EndPaint(hWin, &stPs);
   *        return 0;
   */

   /**
    * 3. Release the water ripple object:
    *    After use, the water ripple object must be released (this function releases allocated buffer memory and other resources)
    *    _WaveFree(lpWaveObject);
    *    lpWaveObject --> Pointer to the WAVE_OBJECT structure
    *********************************************************************************
    * Implementation details:
    *
    * 1. Characteristics of water ripples:
    *    a) Diffusion: The wave at each point spreads to its surrounding positions.
    *    b) Attenuation: Each diffusion loses a small amount of energy (otherwise the water ripple will oscillate indefinitely).
    *
    * 2. To save the energy distribution maps at two moments, the object defines 2 buffers Wave1 and Wave2
    *    (saved in the buffers pointed to by lpWave1 and lpWave2). Wave1 is the current data, and Wave2 is
    *    the data of the last frame. Each time during rendering, based on the above two characteristics, the new
    *    energy distribution maps are calculated from the data of Wave1, saved to Wave2, and then Wave1 and Wave2 are swapped,
    *    such that Wave1 always contains the latest data.
    *       The calculation method is: the energy at a certain point = the average value of the last energy of the surrounding points * attenuation coefficient.
    *    Taking the average value of the surrounding points reflects the spreading characteristics, and multiplying by the attenuation coefficient reflects the attenuation characteristics.
    *       This part of the code is implemented in the _WaveSpread subroutine.
    *
    * 3. The object saves the data of the original bitmap in lpDIBitsSource. Each time during rendering, a new bitmap is generated from the energy distribution data saved in Wave1.
    *    Visually, if the energy at a certain point is larger (the water ripple is larger), the scene refracted by the light will be farther away.
    *       The algorithm is: for point (x, y), find this point in Wave1, calculate the wave energy difference of adjacent points
    *    (two data values, Dx and Dy), then the new bitmap pixel (x, y) = original bitmap pixel (x+Dx, y+Dy).
    *    This algorithm reflects that the size of the energy affects the offset of pixel refraction.
    *       This part of the code is implemented in the _WaveRender subroutine.
    *
    * 4. The algorithm for throwing stones is easy to understand. Set the energy value of a certain point in Wave1 to a non-zero value;
    *    the larger the value, the greater the energy of the stone thrown. If the stone is large, set all the points around that point to a non-zero value.
    *********************************************************************************/

#pragma warning( disable : 4146)

#include <stdint.h>
#include <windows.h>
#include <stdbool.h>
#include "water_ripple.h"

#ifndef WAVEOBJ_INC
#define WAVEOBJ_INC 1

// Flags
#define F_WO_ACTIVE       0x0001
#define F_WO_NEED_UPDATE  0x0002
#define F_WO_EFFECT       0x0004
#define F_WO_ELLIPSE      0x0008

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Random Number Generation Subroutine
// Input: Maximum value of the desired random number, Output: Random number
// Based on:
// 1. Mathematical formula Rnd = (Rnd * I + J) mod K cyclically generates pseudo-random numbers within K times without repetition,
//    but K, I, J must be prime numbers.
// 2. 2^(2n-1)-1 is guaranteed to be a prime number (i.e., 2 raised to the power of an odd number minus 1).
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
uint16_t _WaveRandom16(WAVE_OBJECT* lpWaveObject) {
    uint32_t result; // eax

    result = lpWaveObject->dwRandom;
    uint64_t temp = (0x7FFF * (uint64_t)result) + 0x7FF;
    lpWaveObject->dwRandom = temp % 0x7FFFFFFF;
    
    return (uint16_t)lpWaveObject->dwRandom;
}

uint32_t _WaveRandom(WAVE_OBJECT* lpWaveObject, uint32_t dwMax) {
    uint16_t eax = _WaveRandom16(lpWaveObject);
    uint16_t edx = _WaveRandom16(lpWaveObject);
    
    uint32_t result = ((uint32_t)eax << 16) | edx;  // Combine two 16-bit values into a 32-bit value
    
    if (dwMax != 0) {
        result %= dwMax;
    }
    
    return result;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Wave Energy Diffusion
// Algorithm:
// Wave2(x, y) = (Wave1(x+1, y) + Wave1(x-1, y) + Wave1(x, y+1) + Wave1(x, y-1))/2 - Wave2(x, y)
// Wave2(x, y) = Wave2(x, y) - (Wave2(x, y) >> 5)
// xchg Wave1, Wave2
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveSpread(WAVE_OBJECT* lpWaveObject) {
    if (!(lpWaveObject->dwFlag & F_WO_ACTIVE)) return;

    uint32_t* wave1 = lpWaveObject->lpWave1;
    uint32_t* wave2 = lpWaveObject->lpWave2;
    uint32_t width = lpWaveObject->dwWaveByteWidth / sizeof(uint32_t);
    uint32_t height = lpWaveObject->dwBmpHeight;
    uint32_t maxIndex = (height - 1) * width;
    uint32_t i = lpWaveObject->dwBmpWidth;

    while (i < maxIndex) {
        if (lpWaveObject->dwFlag & F_WO_ELLIPSE) {
            int32_t value = 3 * (wave1[i - 1] + wave1[i + 1]) +
                2 * (wave1[i - 2] + wave1[i + 2]) +
                2 * (wave1[i - 3] + wave1[i + 3]);

            value += 8 * (wave1[i - width] + wave1[i + width]);
            value = (value >> 4) - wave2[i];

            int32_t delta = value >> 5;
            value -= delta;

            wave2[i] = value;
        }
        else {
            int32_t value = wave1[i - 1] + wave1[i + 1] + wave1[i - width] + wave1[i + width];

            value = (value >> 1) - wave2[i];

            int32_t delta = value >> 5;
            value -= delta;

            wave2[i] = value;
        }
        i++;
    }

    lpWaveObject->lpWave1 = wave2;
    lpWaveObject->lpWave2 = wave1;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// esi -> edi, ecx = line width
// return = (4 * Pixel(x, y) + 3 * Pixel(x - 1, y) + 3 * Pixel(x + 1, y) + 3 * Pixel(x, y + 1) + 3 * Pixel(x, y - 1)) / 16
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveGetPixel(uint8_t* src, uint8_t* dest, int32_t width) {
    uint32_t sum = 0;
    uint32_t pix;

    // 4 * Pixel(x, y)
    pix = src[0];
    pix <<= 2;
    sum += pix;

    // 3 * Pxl(x-1,y)
    pix = src[-3];
    pix *= 3;
    sum += pix;

    // 3 * Pxl(x+1,y)
    pix = src[3];
    pix *= 3;
    sum += pix;

    // 3 * Pxl(x,y+1)
    pix = src[width];
    pix *= 3;
    sum += pix;

    // 3 * Pxl(x,y-1)
    pix = src[-width];
    pix *= 3;
    sum += pix;

    // / 16
    sum >>= 4;

    *dest = (uint8_t)sum;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//Rendering subroutine, renders the new frame data into lpDIBitsRender
//Algorithm:
//posx = Wave1(x - 1, y) - Wave1(x + 1, y) + x
//posy = Wave1(x, y - 1) - Wave1(x, y + 1) + y
//SourceBmp(x, y) = DestBmp(posx, posy)
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveRender(WAVE_OBJECT* lpWaveObject) {
    int dwFlag = 0;
    if (!(lpWaveObject->dwFlag & F_WO_ACTIVE)) return;

    lpWaveObject->dwFlag |= F_WO_NEED_UPDATE;
    uint32_t* wave1 = lpWaveObject->lpWave1;
    uint32_t ByteWidth = lpWaveObject->dwDIByteWidth;
    uint32_t width = lpWaveObject->dwBmpWidth;
    uint32_t height = lpWaveObject->dwBmpHeight;

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 0; x + 1 < width; ++x) {
            // PosY = i + energy above pixel - energy below pixel
            // PosX = j + energy left of pixel - energy right of pixel
            int posY = y + wave1[(y - 1) * width + x] - wave1[(y + 1) * width + x];
            
            int posX = x + wave1[y * width + x - 1] - wave1[y * width + x + 1];

            if (posX >= 0 && posX < width && posY >= 0 && posY < height) {
                // ptrSource = dwPosY * dwDIByteWidth + dwPosX * 3
                // ptrDest = i * dwDIByteWidth + j * 3
                uint8_t* src = lpWaveObject->lpDIBitsSource + (posY * ByteWidth) + (posX * 3);
                uint8_t* dest = lpWaveObject->lpDIBitsRender + (y * ByteWidth) + (x * 3);
                
                // Render pixel[ptrDest] = Original pixel[ptrSource]
                if ((posY * ByteWidth) + (posX * 3) == (y * ByteWidth) + (x * 3)) {
                    uint16_t tempWord = *(uint16_t*)src;  // Load 2 bytes from src
                    src += 2;
                    dest[0] = (uint8_t)tempWord;          // Copy the first byte
                    dest[1] = (uint8_t)(tempWord >> 8);   // Copy the second byte
                    dest[2] = *src;                       // Copy the third byte
                }
                // If the source pixel and destination pixel are different, it indicates that the activity is still ongoing
                else {
                    dwFlag |= 1;
                    _WaveGetPixel(src, dest, lpWaveObject->dwDIByteWidth);
                    _WaveGetPixel(src + 1, dest + 1, lpWaveObject->dwDIByteWidth);
                    _WaveGetPixel(src + 2, dest + 2, lpWaveObject->dwDIByteWidth);
                }
            }
        }
    }
    // Copy the rendered pixel data to hDc (Device Context)
    SetDIBits(lpWaveObject->hDcRender, lpWaveObject->hBmpRender, 0, lpWaveObject->dwBmpHeight, lpWaveObject->lpDIBitsRender, &lpWaveObject->stBmpInfo, DIB_RGB_COLORS);

    if (!dwFlag) {
        lpWaveObject->dwFlag &= ~F_WO_ACTIVE;
    }
}

void _WaveUpdateFrame(WAVE_OBJECT* lpWaveObject, HDC _hDc, BOOL _bIfForce) {
    if (_bIfForce || (lpWaveObject->dwFlag & F_WO_NEED_UPDATE) != 0) {
        BitBlt(_hDc, 0, 0, lpWaveObject->dwBmpWidth, lpWaveObject->dwBmpHeight, (HDC)lpWaveObject->hDcRender, 0, 0, SRCCOPY);
        lpWaveObject->dwFlag &= ~F_WO_NEED_UPDATE;
    }
}

void _WaveDropStone(WAVE_OBJECT* lpWaveObject, uint32_t dwX, uint32_t dwY, uint32_t dwSize, uint32_t dwWeight) {
    // Calculate Range
    uint32_t halfSize = dwSize >> 1;

    uint32_t startX = dwX - halfSize;
    uint32_t endX = dwX + halfSize;
    uint32_t startY = dwY - halfSize;
    uint32_t endY = dwY + halfSize;

    if (lpWaveObject->dwFlag & F_WO_ELLIPSE) {
        halfSize = dwSize >> 2;
        endY = dwY + halfSize;
        startY = dwY - halfSize;
    }

    uint32_t x = startX;
    dwSize = (dwSize * 2 > 1) ? dwSize : 1;
    // Check the Validity of the Range
    if (endX + 1 < lpWaveObject->dwBmpWidth && startX >= 1) {

        if (endY + 1 < lpWaveObject->dwBmpHeight && startY >= 1) {

            // Set the energy of points within the range to dwWeight
            while (x <= endX) {
                uint32_t y = startY;
                while (y  <= endY) {
                    int32_t dx = x - dwX;
                    int32_t dy = y - dwY;
                    if ((dx * dx + dy * dy) <= (dwSize * dwSize)) {
                        lpWaveObject->lpWave1[y * lpWaveObject->dwBmpWidth + x] = dwWeight;
                    }
                ++y;
                }
            ++x;
            }
        }
    }
    lpWaveObject->dwFlag |= F_WO_ACTIVE; 
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Timer procedure for calculating diffusion data, rendering bitmaps, updating the window, and handling special effects
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveTimerProc(HWND hWnd, UINT uMsg, WAVE_OBJECT* lpWaveObject, DWORD dwTime) {

    _WaveSpread(lpWaveObject);
    _WaveRender(lpWaveObject);

    if (lpWaveObject->dwFlag & F_WO_NEED_UPDATE) {
        HDC hdc = GetDC(lpWaveObject->hWnd);
        _WaveUpdateFrame(lpWaveObject, hdc, FALSE);
        ReleaseDC(lpWaveObject->hWnd, hdc);
    }

    // Special Effect Processing
    if ((lpWaveObject->dwFlag & F_WO_EFFECT) != 0) {
        switch (lpWaveObject->dwEffectType) {
        // Type = 1 Raindrops, Param1 = Speed (0 is the fastest, larger values are slower), Param2 = Raindrop Size, Param3 = Energy
        case 1: {
            if (!lpWaveObject->dwEffectParam1 || !_WaveRandom(lpWaveObject, lpWaveObject->dwEffectParam1)) {
                int x = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpWidth - 2) + 1;
                int y = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpHeight - 2) + 1;
                int size = _WaveRandom(lpWaveObject, lpWaveObject->dwEffectParam2) + 1;
                int energy = _WaveRandom(lpWaveObject, lpWaveObject->dwEffectParam3) + 50;

                _WaveDropStone(lpWaveObject, x, y, size, energy);
            }
            break;
        }
        // Type = 2 Boat, Param1 = Speed (0 is the fastest, larger values are faster), Param2 = Size, Param3 = Energy
        case 2: {
            if ((++lpWaveObject->dwEff2Flip & 1) == 0) {
                int x = lpWaveObject->dwEff2XAdd + lpWaveObject->dwEff2X;
                int y = lpWaveObject->dwEff2YAdd + lpWaveObject->dwEff2Y;
                if (x < 1)
                {
                    x = 1 - x;
                    lpWaveObject->dwEff2XAdd = -lpWaveObject->dwEff2XAdd;
                }
                if (y < 1)
                {
                    y = 1 - y;
                    lpWaveObject->dwEff2YAdd = -lpWaveObject->dwEff2YAdd;
                }
                if (x >= (lpWaveObject->dwBmpWidth - 1))
                {
                    x = lpWaveObject->dwBmpWidth - 1 - (x - (lpWaveObject->dwBmpWidth - 1));
                    lpWaveObject->dwEff2XAdd = -lpWaveObject->dwEff2XAdd;
                }
                if (y >= (lpWaveObject->dwBmpHeight - 1))
                {
                    y = lpWaveObject->dwBmpHeight - 1 - (y - (lpWaveObject->dwBmpHeight - 1));
                    lpWaveObject->dwEff2YAdd = -lpWaveObject->dwEff2YAdd;
                }
                lpWaveObject->dwEff2X = x;
                lpWaveObject->dwEff2Y = y;
                _WaveDropStone(lpWaveObject, x, y, lpWaveObject->dwEffectParam2, lpWaveObject->dwEffectParam3);
            }
            break;
        }
        // Type = 3 Waves, Param1 = Density, Param2 = Size, Param3 = Energy
        case 3: {
            for (int i = 0; i <= lpWaveObject->dwEffectParam1; ++i) {
                int x = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpWidth - 2) + 1;
                int y = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpHeight - 2) + 1;
                int size = _WaveRandom(lpWaveObject, lpWaveObject->dwEffectParam2) + 1;
                int energy = _WaveRandom(lpWaveObject, lpWaveObject->dwEffectParam3);

                _WaveDropStone(lpWaveObject, x, y, size, energy);
            }
            break;
            }
        }
    }
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Release the object
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveFree(WAVE_OBJECT* lpWaveObject) {
    if (lpWaveObject->hDcRender)
        DeleteDC(lpWaveObject->hDcRender);

    if (lpWaveObject->hBmpRender)
        DeleteObject(lpWaveObject->hBmpRender);

    if (lpWaveObject->lpDIBitsSource)
        GlobalFree(lpWaveObject->lpDIBitsSource);

    if (lpWaveObject->lpDIBitsRender)
        GlobalFree(lpWaveObject->lpDIBitsRender);

    if (lpWaveObject->lpWave1)
        GlobalFree(lpWaveObject->lpWave1);

    if (lpWaveObject->lpWave2)
        GlobalFree(lpWaveObject->lpWave2);

    KillTimer(lpWaveObject->hWnd, (UINT_PTR)lpWaveObject);
    RtlZeroMemory(lpWaveObject, sizeof(WAVE_OBJECT));
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Initialize the object
// Parameters: _lpWaveObject = Pointer to WAVE_OBJECT
// Returns: eax = 0 Success, = 1 Failure
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int _WaveInit(WAVE_OBJECT* lpWaveObject, HWND hWnd, HBITMAP hBmp, DWORD dwSpeed, DWORD dwType) {
    BITMAP stBmp;
    int dwReturn = 0;  // Return 0 for success, 1 for failure

    // Zero out the wave object structure
    RtlZeroMemory(lpWaveObject, sizeof(WAVE_OBJECT));

    // Set the elliptical flag if dwType is non-zero
    if (dwType) {
        lpWaveObject->dwFlag |= F_WO_ELLIPSE;
    }

    // Assign window handle and set random seed
    lpWaveObject->hWnd = hWnd;
    lpWaveObject->dwRandom = GetTickCount();

    // Retrieve bitmap dimensions
    if (!GetObject(hBmp, sizeof(BITMAP), &stBmp)) {
        dwReturn = 1;
        return dwReturn;
    }

    lpWaveObject->dwBmpHeight = stBmp.bmHeight;
    if (lpWaveObject->dwBmpHeight <= 3) {
        dwReturn = 1;
        return dwReturn;
    }

    lpWaveObject->dwBmpWidth = stBmp.bmWidth;
    if (lpWaveObject->dwBmpWidth <= 3) {
        dwReturn = 1;
        return dwReturn;
    }

    // Set wave byte width and DI byte width
    lpWaveObject->dwWaveByteWidth = lpWaveObject->dwBmpWidth * 4;
    lpWaveObject->dwDIByteWidth = ((lpWaveObject->dwBmpWidth * 3) + 3) & ~3;

    // Create a bitmap for rendering
    HDC hDC = GetDC(hWnd);
    lpWaveObject->hDcRender = CreateCompatibleDC(hDC);
    lpWaveObject->hBmpRender = CreateCompatibleBitmap(hDC, lpWaveObject->dwBmpWidth, lpWaveObject->dwBmpHeight);
    SelectObject(lpWaveObject->hDcRender, lpWaveObject->hBmpRender);

    // Allocate wave energy buffers
    size_t waveBufferSize = lpWaveObject->dwWaveByteWidth * lpWaveObject->dwBmpHeight;
    lpWaveObject->lpWave1 = (uint32_t*)GlobalAlloc(GPTR, waveBufferSize);
    lpWaveObject->lpWave2 = (uint32_t*)GlobalAlloc(GPTR, waveBufferSize);

    // Allocate pixel buffers
    size_t pixelBufferSize = lpWaveObject->dwDIByteWidth * lpWaveObject->dwBmpHeight;
    lpWaveObject->lpDIBitsSource = (uint8_t*)GlobalAlloc(GPTR, pixelBufferSize);
    lpWaveObject->lpDIBitsRender = (uint8_t*)GlobalAlloc(GPTR, pixelBufferSize);

    // Set up BITMAPINFO for original pixel data
    lpWaveObject->stBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    lpWaveObject->stBmpInfo.bmiHeader.biWidth = lpWaveObject->dwBmpWidth;
    lpWaveObject->stBmpInfo.bmiHeader.biHeight = -(lpWaveObject->dwBmpHeight);
    lpWaveObject->stBmpInfo.bmiHeader.biPlanes = 1;
    lpWaveObject->stBmpInfo.bmiHeader.biBitCount = 24;
    lpWaveObject->stBmpInfo.bmiHeader.biCompression = BI_RGB;
    lpWaveObject->stBmpInfo.bmiHeader.biSizeImage = 0;

    // Retrieve the original pixel data
    HDC hBmpDC = CreateCompatibleDC(hDC);
    SelectObject(hBmpDC, hBmp);

    GetDIBits(hBmpDC, hBmp, 0, lpWaveObject->dwBmpHeight, lpWaveObject->lpDIBitsSource, &lpWaveObject->stBmpInfo, DIB_RGB_COLORS);
    GetDIBits(hBmpDC, hBmp, 0, lpWaveObject->dwBmpHeight, lpWaveObject->lpDIBitsRender, &lpWaveObject->stBmpInfo, DIB_RGB_COLORS);
    DeleteDC(hBmpDC);

    // Verify allocation success
    if (!lpWaveObject->lpWave1 || !lpWaveObject->lpWave2 || !lpWaveObject->lpDIBitsSource || !lpWaveObject->lpDIBitsRender || !lpWaveObject->hDcRender) {
        _WaveFree(lpWaveObject);
        dwReturn = 1;
    }

    // Set up a timer for the wave simulation
    SetTimer(hWnd, (UINT_PTR)lpWaveObject, dwSpeed, (TIMERPROC)_WaveTimerProc);

    // Activate and mark the object for updating
    lpWaveObject->dwFlag |= (F_WO_ACTIVE | F_WO_NEED_UPDATE);

    // Render the initial frame
    _WaveRender(lpWaveObject);
    HDC hWndDC = GetDC(lpWaveObject->hWnd);
    _WaveUpdateFrame(lpWaveObject, hWndDC, TRUE);
    ReleaseDC(lpWaveObject->hWnd, hWndDC);

    return dwReturn;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Some special effects
// Input: _dwType = 0    Close the special effect
//        _dwType <> 0    Enable the special effect, specific parameters as described above
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void _WaveEffect(WAVE_OBJECT* lpWaveObject, uint32_t dwType, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3) {

    // Check the type of the effect
    if (dwType)
    {
        // Boat special effect
        if (dwType == 2)
        {
            lpWaveObject->dwEff2XAdd = dwParam1;
            lpWaveObject->dwEff2YAdd = dwParam1;
            lpWaveObject->dwEff2X = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpWidth - 2) + 1;
            lpWaveObject->dwEff2Y = _WaveRandom(lpWaveObject, lpWaveObject->dwBmpHeight - 2) + 1;
        }
        lpWaveObject->dwEffectType = dwType;
        lpWaveObject->dwEffectParam1 = dwParam1;
        lpWaveObject->dwEffectParam2 = dwParam2;
        lpWaveObject->dwEffectParam3 = dwParam3;
        lpWaveObject->dwFlag |= F_WO_EFFECT;
    }
    // Turn off the special effect
    else
    {
        lpWaveObject->dwFlag &= ~F_WO_EFFECT;
        lpWaveObject->dwEffectType = 0;
    }
}

#endif