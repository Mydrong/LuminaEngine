@echo off
setlocal enableextensions

rem Any engine on this machine starts this, and the build tool hands it to the engine the .lproject names.

cd /d "%~dp0"
set "PROJECT_DIR=%CD%"

set "ENGINE_DIR="
for /f "usebackq delims=" %%E in (`powershell.exe -NoProfile -Command "$Store = Join-Path $env:LOCALAPPDATA 'Lumina\Engines.json'; if (Test-Path $Store) { (Get-Content $Store -Raw | ConvertFrom-Json).PSObject.Properties.Value | Where-Object { Test-Path (Join-Path $_ 'LuminaBuild.bat') } | Select-Object -First 1 }"`) do set "ENGINE_DIR=%%E"

if not defined ENGINE_DIR (
    echo No Lumina engine is known on this machine. Run Setup.bat in an engine checkout once, then try again.
    endlocal
    exit /b 1
)

call "%ENGINE_DIR%\LuminaBuild.bat" GenerateProjectFiles -Project="%PROJECT_DIR%" %*
if errorlevel 1 (
    echo.
    echo Project generation failed.
    endlocal
    exit /b 1
)

echo.
echo Solution generated.
endlocal
exit /b 0
