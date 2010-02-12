@echo off
FOR /F "delims=" %%A IN ('dir /a /on /b "%APPDATA%\spectrum\*.cfg"') DO (
start spectrum.exe "%%A"
)
exit
