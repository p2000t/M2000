@echo off
if "%~1"=="" (
  echo Drag-and-drop a .cas program file on 'BootFromTape.bat' to start the program in the M2000 emulator directly
  pause&exit
)
cd %~dp0
m2000.exe -ram 64k -tape "%~1" -boot 1 -video 1