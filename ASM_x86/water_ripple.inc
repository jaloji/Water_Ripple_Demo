uselib  MACRO   libname
include    libname.inc    
includelib libname.lib
ENDM

;uselib     user32
;uselib     kernel32
include    \masm32\include\user32.inc
include    \masm32\include\kernel32.inc
include	   \masm32\include\windows.inc
include    \masm32\include\comctl32.inc
include    \masm32\include\winmm.inc
include	   \masm32\include\gdi32.inc
include	   \masm32\macros\macros.asm
include	   WaveObject.asm
includelib \masm32\lib\user32.lib
includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\gdi32.lib
includelib \masm32\lib\comctl32.lib
includelib \masm32\lib\winmm.lib


; Prototype des fonctions : 
DlgProc	PROTO :DWORD,:DWORD,:DWORD,:DWORD


 ; Définition des constantes ici : 
.const
IDD_WATER_RIPPLE           equ 1001
LOGO                       equ 1002
ICON                       equ 1003

