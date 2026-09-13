@echo off
REM Cross-platform build script for precomp-c (Windows CMD version)
REM Works on Windows with TCC

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set SRC_DIR=%SCRIPT_DIR%src
set INCLUDE_DIR=%SCRIPT_DIR%include
set CONTRIB_DIR=%SCRIPT_DIR%contrib
set TCC_DIR=%SCRIPT_DIR%.tcc

REM Detect architecture
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set ARCH=x86_64
if "%PROCESSOR_ARCHITECTURE%"=="x86" set ARCH=i686
if "%PROCESSOR_ARCHITEW6432%"=="AMD64" set ARCH=x86_64

REM Default values
set TCC_CMD=tcc
set BUILD_TYPE=release
set CFLAGS=

REM Parse arguments
:parse_args
if "%~1"=="" goto :end_parse
if /i "%~1"=="build" set BUILD_TYPE=release & shift & goto :parse_args
if /i "%~1"=="debug" set BUILD_TYPE=debug & shift & goto :parse_args
if /i "%~1"=="clean" goto :do_clean
if /i "%~1"=="help" goto :show_help
if /i "%~1"=="--skip-tcc" set SKIP_TCC_SETUP=1 & shift & goto :parse_args
shift
goto :parse_args

:end_parse

REM Check if TCC is available
where tcc >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Found system TCC
    goto :build
)

if exist "%TCC_DIR%\bin\tcc.exe" (
    set TCC_CMD=%TCC_DIR%\bin\tcc.exe
    echo [INFO] Found local TCC: !TCC_CMD!
    goto :build
)

if not "%SKIP_TCC_SETUP%"=="1" (
    echo [WARN] TCC not found. Please install TCC manually or use --skip-tcc
    echo [WARN] Download from: https://download.savannah.gnu.org/releases/tinycc/
    goto :error
)

:build
echo [INFO] Building precomp-c for Windows (%ARCH%) - %BUILD_TYPE% mode

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Setup compiler flags
set CFLAGS=-I%INCLUDE_DIR% -I%INCLUDE_DIR%\formats
set CFLAGS=!CFLAGS! -I%CONTRIB_DIR%\zlib -I%CONTRIB_DIR%\bzip2
set CFLAGS=!CFLAGS! -I%CONTRIB_DIR%\giflib -I%CONTRIB_DIR%\libjpeg -I%CONTRIB_DIR%\libpng
set CFLAGS=!CFLAGS! -D_WIN32 -D_CRT_SECURE_NO_WARNINGS

if "%BUILD_TYPE%"=="debug" (
    set CFLAGS=!CFLAGS! -g -DDEBUG
) else (
    set CFLAGS=!CFLAGS! -O2
)

REM Collect source files
set SOURCES=
for %%f in ("%SRC_DIR%\*.c") do set SOURCES=!SOURCES! %%f
if exist "%SRC_DIR%\formats" (
    for %%f in ("%SRC_DIR%\formats\*.c") do set SOURCES=!SOURCES! %%f
)

REM Add contrib libraries
if exist "%CONTRIB_DIR%\zlib" (
    for %%f in ("%CONTRIB_DIR%\zlib\*.c") do set SOURCES=!SOURCES! %%f
)
if exist "%CONTRIB_DIR%\bzip2" (
    for %%f in ("%CONTRIB_DIR%\bzip2\*.c") do set SOURCES=!SOURCES! %%f
)
if exist "%CONTRIB_DIR%\giflib" (
    for %%f in ("%CONTRIB_DIR%\giflib\*.c") do set SOURCES=!SOURCES! %%f
)
if exist "%CONTRIB_DIR%\libjpeg" (
    for %%f in ("%CONTRIB_DIR%\libjpeg\*.c") do set SOURCES=!SOURCES! %%f
)
if exist "%CONTRIB_DIR%\libpng" (
    for %%f in ("%CONTRIB_DIR%\libpng\*.c") do set SOURCES=!SOURCES! %%f
)

if "!SOURCES!"=="" (
    echo [ERROR] No source files found!
    goto :error
)

echo [INFO] Compiling...

REM Build executable
%TCC_CMD% !CFLAGS! -o "%BUILD_DIR%\precomp.exe" !SOURCES! > "%BUILD_DIR%\build.log" 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Build failed! Check %BUILD_DIR%\build.log for details
    type "%BUILD_DIR%\build.log"
    goto :error
)

echo [INFO] Build successful! Output: %BUILD_DIR%\precomp.exe
for %%A in ("%BUILD_DIR%\precomp.exe") do echo [INFO] Binary size: %%~zA bytes
goto :end

:do_clean
echo [INFO] Cleaning build artifacts...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo [INFO] Clean complete
goto :end

:show_help
echo precomp-c Build Script (Windows)
echo ==============================
echo.
echo Usage: %~nx0 [command] [options]
echo.
echo Commands:
echo   build       Build precomp ^(default^)
echo   debug       Build with debug symbols
echo   clean       Remove build artifacts
echo   help        Show this help message
echo.
echo Options:
echo   --skip-tcc  Skip TCC auto-setup
echo.
echo Environment Variables:
echo   CC          C compiler to use
echo.
goto :end

:error
exit /b 1

:end
exit /b 0
