@ECHO OFF

REM See if the install path has been set

if "x%1"=="x" (
    @set U4PATH > nul
    if ERRORLEVEL 1 goto usage
) ELSE (
    @set U4PATH=%1
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
echo Usage: install PATH
echo.
echo Note: You may also set the U4PATH environment variable
echo       before calling this script for the target directory.
echo.

:end
