@echo off
echo ==============================
echo ECS-NET Compiler
echo ==============================
echo.
echo Select build mode:
echo [1] Executable (.exe)
echo [2] Static Library (.a)
echo [3] Dynamic Library (.dll)
echo [4] Clean build folder
set /p MODE=Enter 1, 2, 3 or 4: 
echo.


REM Base configuration
set CC=gcc
set AR=ar
set CFLAGS=-Wall -Iinclude
set BUILD_DIR=build
set SOURCES=src\main.c src\net\net_socket.c src\net\connection_manager.c

if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

REM Build executable
if "%MODE%"=="1" (
    echo Building executable...
    %CC% %CFLAGS% %SOURCES% -o %BUILD_DIR%\ecsnet.exe -lws2_32 -mconsole
    if errorlevel 1 (
        echo ERROR compiling executable
        exit /b 1
    )
    echo Executable built: %BUILD_DIR%\ecsnet.exe 
    exit /b 0
)

REM Build static library (.a)
if "%MODE%"=="2" (
    echo Building static library...
    %CC% %CFLAGS% -c src\ecs.c -o %BUILD_DIR%\ecs.o
    %CC% %CFLAGS% -c src\net.c -o %BUILD_DIR%\net.o
    %AR% rcs %BUILD_DIR%\libecsnet.a %BUILD_DIR%\ecs.o %BUILD_DIR%\net.o
    if errorlevel 1 (
        echo ERROR building static library
        exit /b 1
    )
    echo Static library built: %BUILD_DIR%\libecsnet.a
    exit /b 0
)

REM Build dynamic library (.dll)
if "%MODE%"=="3" (
    echo Building dynamic library...
    %CC% -shared -o %BUILD_DIR%\ecsnet.dll src\ecs.c src\net.c -Iinclude -lws2_32
    if errorlevel 1 (
        echo ERROR building DLL
        exit /b 1
    )
    echo Dynamic library built: %BUILD_DIR%\ecsnet.dll
    exit /b 0
)
REM Clear build folder
if "%MODE%"=="4" (
    echo Cleaning build folder...
    del /q %BUILD_DIR%\*.exe >nul 2>&1
    del /q %BUILD_DIR%\*.o >nul 2>&1
    del /q %BUILD_DIR%\*.a >nul 2>&1
    del /q %BUILD_DIR%\*.dll >nul 2>&1
    echo Build folder cleaned.
    exit /b 0
)

echo Invalid option selected.
exit /b 1
