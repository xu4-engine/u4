# Microsoft Developer Studio Project File - Name="xu4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=xu4 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xu4.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xu4.mak" CFG="xu4 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xu4 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "xu4 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xu4 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /w /W0 /GX /O2 /I "C:\Documents and Settings\Main\Desktop\xu4\zlib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D strncasecmp=strnicmp /D strcasecmp=stricmp /D snprintf=_snprintf /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 SDL.lib SDLmain.lib SDL_mixer.lib libxml2.lib zlibstat.lib user32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"LIBC" /libpath:"C:\Documents and Settings\Main\Desktop\xu4\zlib"

!ELSEIF  "$(CFG)" == "xu4 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D strncasecmp=strnicmp /D strcasecmp=stricmp /D snprintf=_snprintf /D vsnprintf=_vsnprintf /FD /GZ /c
# SUBTRACT CPP /WX /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 SDL.lib SDLmain.lib SDL_mixer.lib libxml2.lib zlib.lib user32.lib shell32.lib comdlg32.lib gdi32.lib kernel32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"LIBC" /out:"c:\xu4\xu4.exe" /libpath:"..\lib"
# SUBTRACT LINK32 /map
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc==== Installation ===
PostBuild_Cmds=@ECHO OFF	set U4PATH=c:\xu4	echo Install directory set to %U4PATH%	IF NOT EXIST %U4PATH% mkdir %U4PATH%	IF NOT EXIST %U4PATH% mkdir %U4PATH%\conf	IF NOT EXIST %U4PATH% mkdir %U4PATH%\conf\dtd	IF NOT EXIST %U4PATH% mkdir %U4PATH%\graphics	IF NOT EXIST %U4PATH% mkdir %U4PATH%\mid	IF NOT EXIST %U4PATH% mkdir %U4PATH%\sound	@ECHO ON	xcopy ..\lib\*.dll %U4PATH%\. /E /Y	xcopy ..\conf\*.xml %U4PATH%\conf\. /E /Y	xcopy ..\conf\dtd\*.dtd %U4PATH%\conf\dtd\. /E /Y	xcopy ..\graphics %U4PATH%\graphics\. /E /Y	xcopy ..\mid %U4PATH%\mid\. /E /Y	xcopy ..\sound %U4PATH%\sound\. /E /Y
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "xu4 - Win32 Release"
# Name "xu4 - Win32 Debug"
# Begin Group "lzw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\lzw\hash.c
# End Source File
# Begin Source File

SOURCE=..\src\lzw\hash.h
# End Source File
# Begin Source File

SOURCE=..\src\lzw\lzw.c
# End Source File
# Begin Source File

SOURCE=..\src\lzw\lzw.h
# End Source File
# Begin Source File

SOURCE=..\src\lzw\u4decode.cpp
# End Source File
# Begin Source File

SOURCE=..\src\lzw\u4decode.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\annotation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\annotation.h
# End Source File
# Begin Source File

SOURCE=..\src\armor.cpp
# End Source File
# Begin Source File

SOURCE=..\src\armor.h
# End Source File
# Begin Source File

SOURCE=..\src\camp.cpp
# End Source File
# Begin Source File

SOURCE=..\src\camp.h
# End Source File
# Begin Source File

SOURCE=..\src\city.cpp
# End Source File
# Begin Source File

SOURCE=..\src\city.h
# End Source File
# Begin Source File

SOURCE=..\src\codex.cpp
# End Source File
# Begin Source File

SOURCE=..\src\codex.h
# End Source File
# Begin Source File

SOURCE=..\src\combat.cpp
# End Source File
# Begin Source File

SOURCE=..\src\combat.h
# End Source File
# Begin Source File

SOURCE=..\src\context.h
# End Source File
# Begin Source File

SOURCE=..\src\death.cpp
# End Source File
# Begin Source File

SOURCE=..\src\death.h
# End Source File
# Begin Source File

SOURCE=..\src\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\src\debug.h
# End Source File
# Begin Source File

SOURCE=..\src\direction.cpp
# End Source File
# Begin Source File

SOURCE=..\src\direction.h
# End Source File
# Begin Source File

SOURCE=..\src\dngview.cpp
# End Source File
# Begin Source File

SOURCE=..\src\dngview.h
# End Source File
# Begin Source File

SOURCE=..\src\dungeon.cpp
# End Source File
# Begin Source File

SOURCE=..\src\dungeon.h
# End Source File
# Begin Source File

SOURCE=..\src\error.cpp
# End Source File
# Begin Source File

SOURCE=..\src\error.h
# End Source File
# Begin Source File

SOURCE=..\src\event.cpp
# End Source File
# Begin Source File

SOURCE=..\src\event.h
# End Source File
# Begin Source File

SOURCE=..\src\event_sdl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\game.cpp
# End Source File
# Begin Source File

SOURCE=..\src\game.h
# End Source File
# Begin Source File

SOURCE=..\src\image.h
# End Source File
# Begin Source File

SOURCE=..\src\image_sdl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\intro.cpp
# End Source File
# Begin Source File

SOURCE=..\src\intro.h
# End Source File
# Begin Source File

SOURCE=..\src\io.cpp
# End Source File
# Begin Source File

SOURCE=..\src\io.h
# End Source File
# Begin Source File

SOURCE=..\src\item.cpp
# End Source File
# Begin Source File

SOURCE=..\src\item.h
# End Source File
# Begin Source File

SOURCE=..\src\location.cpp
# End Source File
# Begin Source File

SOURCE=..\src\location.h
# End Source File
# Begin Source File

SOURCE=..\src\map.cpp
# End Source File
# Begin Source File

SOURCE=..\src\map.h
# End Source File
# Begin Source File

SOURCE=..\src\maploader.cpp
# End Source File
# Begin Source File

SOURCE=..\src\maploader.h
# End Source File
# Begin Source File

SOURCE=..\src\mapmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mapmgr.h
# End Source File
# Begin Source File

SOURCE=..\src\menu.cpp
# End Source File
# Begin Source File

SOURCE=..\src\menu.h
# End Source File
# Begin Source File

SOURCE=..\src\monster.cpp
# End Source File
# Begin Source File

SOURCE=..\src\monster.h
# End Source File
# Begin Source File

SOURCE=..\src\moongate.cpp
# End Source File
# Begin Source File

SOURCE=..\src\moongate.h
# End Source File
# Begin Source File

SOURCE=..\src\movement.cpp
# End Source File
# Begin Source File

SOURCE=..\src\movement.h
# End Source File
# Begin Source File

SOURCE=..\src\music.cpp
# End Source File
# Begin Source File

SOURCE=..\src\music.h
# End Source File
# Begin Source File

SOURCE=..\src\names.cpp
# End Source File
# Begin Source File

SOURCE=..\src\names.h
# End Source File
# Begin Source File

SOURCE=..\src\object.cpp
# End Source File
# Begin Source File

SOURCE=..\src\object.h
# End Source File
# Begin Source File

SOURCE=..\src\person.cpp
# End Source File
# Begin Source File

SOURCE=..\src\person.h
# End Source File
# Begin Source File

SOURCE=..\src\player.cpp
# End Source File
# Begin Source File

SOURCE=..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\src\portal.cpp
# End Source File
# Begin Source File

SOURCE=..\src\portal.h
# End Source File
# Begin Source File

SOURCE=..\src\rle.cpp
# End Source File
# Begin Source File

SOURCE=..\src\rle.h
# End Source File
# Begin Source File

SOURCE=..\src\savegame.cpp
# End Source File
# Begin Source File

SOURCE=..\src\savegame.h
# End Source File
# Begin Source File

SOURCE=..\src\scale.cpp
# End Source File
# Begin Source File

SOURCE=..\src\scale.h
# End Source File
# Begin Source File

SOURCE=..\src\screen.cpp
# End Source File
# Begin Source File

SOURCE=..\src\screen.h
# End Source File
# Begin Source File

SOURCE=..\src\screen_sdl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\script.cpp
# End Source File
# Begin Source File

SOURCE=..\src\script.h
# End Source File
# Begin Source File

SOURCE=..\src\settings.cpp
# End Source File
# Begin Source File

SOURCE=..\src\settings.h
# End Source File
# Begin Source File

SOURCE=..\src\shrine.cpp
# End Source File
# Begin Source File

SOURCE=..\src\shrine.h
# End Source File
# Begin Source File

SOURCE=..\src\sound.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sound.h
# End Source File
# Begin Source File

SOURCE=..\src\spell.cpp
# End Source File
# Begin Source File

SOURCE=..\src\spell.h
# End Source File
# Begin Source File

SOURCE=..\src\stats.cpp
# End Source File
# Begin Source File

SOURCE=..\src\stats.h
# End Source File
# Begin Source File

SOURCE=..\src\tile.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tile.h
# End Source File
# Begin Source File

SOURCE=..\src\tileanim.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tileanim.h
# End Source File
# Begin Source File

SOURCE=..\src\tileset.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tileset.h
# End Source File
# Begin Source File

SOURCE=..\src\types.h
# End Source File
# Begin Source File

SOURCE=..\src\u4.cpp
# End Source File
# Begin Source File

SOURCE=..\src\u4.h
# End Source File
# Begin Source File

SOURCE=..\src\u4_sdl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\u4_sdl.h
# End Source File
# Begin Source File

SOURCE=..\src\u4file.cpp
# End Source File
# Begin Source File

SOURCE=..\src\u4file.h
# End Source File
# Begin Source File

SOURCE=..\src\unzip.c
# End Source File
# Begin Source File

SOURCE=..\src\unzip.h
# End Source File
# Begin Source File

SOURCE=..\src\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\src\utils.h
# End Source File
# Begin Source File

SOURCE=..\src\vc6.h
# End Source File
# Begin Source File

SOURCE=..\src\weapon.cpp
# End Source File
# Begin Source File

SOURCE=..\src\weapon.h
# End Source File
# Begin Source File

SOURCE=..\src\xml.cpp
# End Source File
# Begin Source File

SOURCE=..\src\xml.h
# End Source File
# Begin Source File

SOURCE=..\src\xu4.ico
# End Source File
# Begin Source File

SOURCE=..\src\xu4.rc
# End Source File
# End Target
# End Project
