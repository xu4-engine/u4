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
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "C:\Documents and Settings\Main\Desktop\xu4\zlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D strncasecmp=strnicmp /D strcasecmp=stricmp /D snprintf=_snprintf /FR /FD /GZ /c
# SUBTRACT CPP /WX /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 SDL.lib SDLmain.lib SDL_mixer.lib libxml2.lib zlibstat.lib user32.lib shell32.lib comdlg32.lib gdi32.lib kernel32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBC" /out:"C:\Documents and Settings\Main\Desktop\xu4\u4\vc6\Debug\xu4.exe" /pdbtype:sept /libpath:"C:\Documents and Settings\Main\Desktop\xu4\zlib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=set U4ZIPFILE=ultima4.zip
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

SOURCE=..\src\lzw\u4decode.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\annotation.c
# End Source File
# Begin Source File

SOURCE=..\src\annotation.h
# End Source File
# Begin Source File

SOURCE=..\src\area.h
# End Source File
# Begin Source File

SOURCE=..\src\armor.c
# End Source File
# Begin Source File

SOURCE=..\src\armor.h
# End Source File
# Begin Source File

SOURCE=..\src\camp.c
# End Source File
# Begin Source File

SOURCE=..\src\camp.h
# End Source File
# Begin Source File

SOURCE=..\src\city.h
# End Source File
# Begin Source File

SOURCE=..\src\codex.c
# End Source File
# Begin Source File

SOURCE=..\src\codex.h
# End Source File
# Begin Source File

SOURCE=..\src\combat.c
# End Source File
# Begin Source File

SOURCE=..\src\combat.h
# End Source File
# Begin Source File

SOURCE=..\src\context.h
# End Source File
# Begin Source File

SOURCE=..\src\death.c
# End Source File
# Begin Source File

SOURCE=..\src\death.h
# End Source File
# Begin Source File

SOURCE=..\src\debug.c
# End Source File
# Begin Source File

SOURCE=..\src\debug.h
# End Source File
# Begin Source File

SOURCE=..\src\direction.c
# End Source File
# Begin Source File

SOURCE=..\src\direction.h
# End Source File
# Begin Source File

SOURCE=..\src\dngview.c
# End Source File
# Begin Source File

SOURCE=..\src\dngview.h
# End Source File
# Begin Source File

SOURCE=..\src\dungeon.c
# End Source File
# Begin Source File

SOURCE=..\src\dungeon.h
# End Source File
# Begin Source File

SOURCE=..\src\error.c
# End Source File
# Begin Source File

SOURCE=..\src\error.h
# End Source File
# Begin Source File

SOURCE=..\src\event.c
# End Source File
# Begin Source File

SOURCE=..\src\event.h
# End Source File
# Begin Source File

SOURCE=..\src\event_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\game.c
# End Source File
# Begin Source File

SOURCE=..\src\game.h
# End Source File
# Begin Source File

SOURCE=..\src\image.h
# End Source File
# Begin Source File

SOURCE=..\src\image_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\intro.c
# End Source File
# Begin Source File

SOURCE=..\src\intro.h
# End Source File
# Begin Source File

SOURCE=..\src\io.c
# End Source File
# Begin Source File

SOURCE=..\src\io.h
# End Source File
# Begin Source File

SOURCE=..\src\item.c
# End Source File
# Begin Source File

SOURCE=..\src\item.h
# End Source File
# Begin Source File

SOURCE=..\src\list.c
# End Source File
# Begin Source File

SOURCE=..\src\list.h
# End Source File
# Begin Source File

SOURCE=..\src\location.c
# End Source File
# Begin Source File

SOURCE=..\src\location.h
# End Source File
# Begin Source File

SOURCE=..\src\map.c
# End Source File
# Begin Source File

SOURCE=..\src\map.h
# End Source File
# Begin Source File

SOURCE=..\src\mapmgr.c
# End Source File
# Begin Source File

SOURCE=..\src\mapmgr.h
# End Source File
# Begin Source File

SOURCE=..\src\menu.c
# End Source File
# Begin Source File

SOURCE=..\src\menu.h
# End Source File
# Begin Source File

SOURCE=..\src\monster.c
# End Source File
# Begin Source File

SOURCE=..\src\monster.h
# End Source File
# Begin Source File

SOURCE=..\src\moongate.c
# End Source File
# Begin Source File

SOURCE=..\src\moongate.h
# End Source File
# Begin Source File

SOURCE=..\src\movement.c
# End Source File
# Begin Source File

SOURCE=..\src\movement.h
# End Source File
# Begin Source File

SOURCE=..\src\music.c
# End Source File
# Begin Source File

SOURCE=..\src\music.h
# End Source File
# Begin Source File

SOURCE=..\src\names.c
# End Source File
# Begin Source File

SOURCE=..\src\names.h
# End Source File
# Begin Source File

SOURCE=..\src\object.h
# End Source File
# Begin Source File

SOURCE=..\src\person.c
# End Source File
# Begin Source File

SOURCE=..\src\person.h
# End Source File
# Begin Source File

SOURCE=..\src\player.c
# End Source File
# Begin Source File

SOURCE=..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\src\portal.c
# End Source File
# Begin Source File

SOURCE=..\src\portal.h
# End Source File
# Begin Source File

SOURCE=..\src\rle.c
# End Source File
# Begin Source File

SOURCE=..\src\rle.h
# End Source File
# Begin Source File

SOURCE=..\src\savegame.c
# End Source File
# Begin Source File

SOURCE=..\src\savegame.h
# End Source File
# Begin Source File

SOURCE=..\src\scale.c
# End Source File
# Begin Source File

SOURCE=..\src\scale.h
# End Source File
# Begin Source File

SOURCE=..\src\screen.c
# End Source File
# Begin Source File

SOURCE=..\src\screen.h
# End Source File
# Begin Source File

SOURCE=..\src\screen_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\settings.c
# End Source File
# Begin Source File

SOURCE=..\src\settings.h
# End Source File
# Begin Source File

SOURCE=..\src\shrine.c
# End Source File
# Begin Source File

SOURCE=..\src\shrine.h
# End Source File
# Begin Source File

SOURCE=..\src\sound.c
# End Source File
# Begin Source File

SOURCE=..\src\sound.h
# End Source File
# Begin Source File

SOURCE=..\src\spell.c
# End Source File
# Begin Source File

SOURCE=..\src\spell.h
# End Source File
# Begin Source File

SOURCE=..\src\stats.c
# End Source File
# Begin Source File

SOURCE=..\src\stats.h
# End Source File
# Begin Source File

SOURCE=..\src\ttype.c
# End Source File
# Begin Source File

SOURCE=..\src\ttype.h
# End Source File
# Begin Source File

SOURCE=..\src\u4.c
# End Source File
# Begin Source File

SOURCE=..\src\u4.h
# End Source File
# Begin Source File

SOURCE=..\src\u4_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\u4_sdl.h
# End Source File
# Begin Source File

SOURCE=..\src\u4file.c
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

SOURCE=..\src\utils.c
# End Source File
# Begin Source File

SOURCE=..\src\utils.h
# End Source File
# Begin Source File

SOURCE=..\src\vendor.c
# End Source File
# Begin Source File

SOURCE=..\src\vendor.h
# End Source File
# Begin Source File

SOURCE=..\src\weapon.c
# End Source File
# Begin Source File

SOURCE=..\src\weapon.h
# End Source File
# Begin Source File

SOURCE=..\src\xml.c
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
