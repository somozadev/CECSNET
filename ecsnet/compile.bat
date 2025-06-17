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

REM Build dynamic library (.dll) + linker (.a)
if "%MODE%"=="3" (
    echo Building dynamic library...
    REM Building DLL first
    %CC% -shared -o %BUILD_DIR%\ecsnet.dll src\ecs.c src\ecs_builtin.c -Iinclude -lws2_32 -DECSNET_EXPORTS -Wl,--out-implib,%BUILD_DIR%\libecsnet.a
    if errorlevel 1 (
        echo ERROR compiling DLL
        exit /b 1
    )
     REM Move to build folder to call  gendef and dlltool
    pushd %BUILD_DIR%
    REM Generate .def from the DLL //// pacman -S mingw-w64-x86_64-tools is the command to install gendef from MINGW
    gendef ecsnet.dll 

    if errorlevel 1 (
        echo ERROR generating .def file
        popd
        exit /b 1
    )
   REM Create .lib file for Visual Studio Standards from the .def
    dlltool -D ecsnet.dll -d ecsnet.def -l ecsnet.lib
    if errorlevel 1 (
        echo ERROR generating .lib file
        popd
        exit /b 1
    )

    popd

    echo Dynamic library built: %BUILD_DIR%\ecsnet.dll
    echo Import library built: %BUILD_DIR%\ecsnet.lib
    
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
