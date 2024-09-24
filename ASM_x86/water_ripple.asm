.486
.model	flat, stdcall
option	casemap :none   ; case sensitive

include water_ripple.inc

szCap                          db       "Water Ripple Demo",0
szTitle                        db       "Error",0
szError                        db       "An error has occured",0

.data?
hInstance                      dd           ?
stWaveObj                      WAVE_OBJECT <?>
xWin                           dd           ?
hBitmap                        dd           ?
bitmp                          dd           ?
szName                         db   256 dup(?)
szSerial                       db   256 dup(?)
LenName                        db   256 dup(?)


.code
start:
        invoke GetModuleHandle, NULL
           MOV hInstance, EAX
        invoke DialogBoxParam, hInstance, IDD_WATER_RIPPLE, 0, ADDR DlgProc, 0
        invoke ExitProcess, EAX
;###############################################################################################################################################################
DlgProc	proc hWin:DWORD,uMsg:DWORD,wParam:DWORD,lParam:DWORD
;###############################################################################################################################################################
        local @stPs:PAINTSTRUCT,@hDc,@stRect:RECT
        local @stBmp:BITMAP
        local hMemDC:HDC
		
		
.if uMsg==WM_INITDIALOG
    invoke LoadBitmap,hInstance,1002
        MOV hBitmap,EAX
        PUSH hBitmap
    invoke _WaveInit,addr stWaveObj,hWin,hBitmap,30,0
	;invoke _WaveInit,addr stWaveObj,hWin,hBitmap,30,1
	
.if EAX
    invoke MessageBox,hWin,addr szError,addr szTitle,MB_OK or MB_ICONSTOP
        CALL _Quit
.else
.endif

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        POP hBitmap
    invoke DeleteObject,hBitmap
	invoke _WaveEffect,addr stWaveObj,1,5,4,250 ; Rain
    ;invoke _WaveEffect,addr stWaveObj,2,4,2,400  ; Motorboat
    ;invoke _WaveEffect,addr stWaveObj,3,100,3,7 ; Wind Waves
	
.elseif	uMsg ==	WM_PAINT
    invoke BeginPaint,hWin,addr @stPs
        MOV @hDc,EAX
    invoke CreateCompatibleDC,@hDc
        MOV hMemDC,EAX
    invoke SelectObject,hMemDC,hBitmap
    invoke GetClientRect,hWin,addr @stRect
    invoke BitBlt,@hDc,10,10,@stRect.right,@stRect.bottom,hMemDC,0,0,MERGECOPY
    invoke DeleteDC,hMemDC
    invoke _WaveUpdateFrame,addr stWaveObj,eax,TRUE
    invoke EndPaint,hWin,addr @stPs
        XOR EAX,EAX
        XOR ECX,ECX
        RET
	
.elseif uMsg==WM_DESTROY
    invoke DeleteObject,hBitmap
    invoke PostQuitMessage,NULL

.elseif	uMsg == WM_CLOSE
        CALL _Quit
    invoke EndDialog,xWin,0		
.endif		
	    XOR EAX,EAX
        RET
	
DlgProc	endp

_Quit proc

    invoke _WaveFree,addr stWaveObj
    invoke DestroyWindow,xWin
    invoke PostQuitMessage,NULL
        RET
_Quit endp
end start
