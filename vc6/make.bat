@ECHO OFF

@if "x%1"=="x" goto usage

@if "%1"=="install" goto install
@if "%1" NEQ "clean" goto usage

:clean

for %%i IN (., dumpsavegame, tlkconv, u4dec, u4enc) DO (
    echo === Cleaning %%i ===
    @del /Q %%i\Debug\* > nul
    @del /Q %%i\Release\* > nul
    @del %%i\*.opt > nul
    @del %%i\*.plg > nul
    @rmdir /S /Q %%i\Debug > nul
    @rmdir /S /Q %%i\Release > nul
    echo.
)
goto end

:install
REM See if the install path has been set

if "x%2"=="x" (
    @set U4PATH > nul
    if ERRORLEVEL 1 goto usage
) ELSE (
    @set U4PATH=%2
)

REM Create the necessary directories

IF NOT EXIST %U4PATH% mkdir %U4PATH%
IF EXIST %U4PATH% (
    echo Install directory set to %U4PATH%
    IF NOT EXIST %U4PATH% mkdir %U4PATH%
    IF NOT EXIST %U4PATH%\conf mkdir %U4PATH%\conf
    IF NOT EXIST %U4PATH%\conf\dtd mkdir %U4PATH%\conf\dtd
    IF NOT EXIST %U4PATH%\graphics mkdir %U4PATH%\graphics
    IF NOT EXIST %U4PATH%\mid mkdir %U4PATH%\mid
    IF NOT EXIST %U4PATH%\sound mkdir %U4PATH%\sound

REM Copy all files

    @ECHO ON
    xcopy ..\lib\*.dll %U4PATH%\. /E /D /Y
    xcopy ..\conf\*.xml %U4PATH%\conf\. /E /Y
    xcopy ..\conf\dtd\*.dtd %U4PATH%\conf\dtd\. /E /Y
    xcopy ..\graphics %U4PATH%\graphics\. /E /D /Y
    xcopy ..\mid %U4PATH%\mid\. /E /D /Y
    xcopy ..\sound %U4PATH%\sound\. /E /D /Y
) ELSE (
    echo An error occurred while setting the xu4 installation path.
)

@goto end

:usage
echo.
echo Usage: install ^<mode^> PATH
echo.
echo Where ^<mode^> is one of the following:
echo       install - installs the necessary xu4 files
echo       clean   - deletes all unnecessary vc6 build files
echo.
echo If installing, PATH is the desired installation path.
echo.
echo Note: You may also set the U4PATH environment variable
echo       before calling this script to replace the PATH
echo       variable
echo.

:end
