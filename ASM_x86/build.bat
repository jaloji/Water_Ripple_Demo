@echo off
C:\masm32\bin\ml /c /coff /Cp /IC:\masm32\include /IC:\masm32\units /Fo%1.obj %1.asm
if errorlevel 1 goto errasm
if not exist %1.rc goto norc
C:\masm32\bin\rc /iC:\masm32\include /fo%1.res %1.rc
if errorlevel 1 goto errrc
C:\masm32\bin\link /SUBSYSTEM:WINDOWS /LIBPATH:C:\masm32\lib /OUT:%1.exe %1.obj %1.res
if errorlevel 1 goto errlink
goto rc
:norc
C:\masm32\bin\link /SUBSYSTEM:WINDOWS /LIBPATH:C:\masm32\lib /OUT:%1.exe %1.obj
if errorlevel 1 goto errlink
:rc
echo OK > C:\masm32\error.txt
goto end
:errasm
pause
echo Error Masm > C:\masm32\error.txt
goto end
:errrc
pause
echo Error Rc > C:\masm32\error.txt
goto end
:errlink
pause
echo Error Link > C:\masm32\error.txt
goto end
:end
if exist %1.obj del %1.obj
if exist %1.res del %1.res