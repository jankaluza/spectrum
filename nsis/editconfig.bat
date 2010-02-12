@echo off

rem Set the proper path
pushd %~dp0
set mypath=%CD%
popd
cd "%mypath%"

set datastore=%APPDATA%\spectrum

set SPECTRUM_CFGDIR=%datastore%

if not exist spectrum.exe goto no-spectrum
if not exist "%SPECTRUM_CFGDIR%" goto make-folder
if not exist "%SPECTRUM_CFGDIR%\spectrum.cfg" goto install-config
goto run

:make-folder
mkdir "%SPECTRUM_CFGDIR%"
:install-config
copy spectrum.cfg "%SPECTRUM_CFGDIR%\spectrum.cfg"

:run
start notepad "%SPECTRUM_CFGDIR%\spectrum.cfg"
goto exit

:no-spectrum
echo File 'spectrum.exe' not found.
echo Current directory: %mypath%
pause

:exit
