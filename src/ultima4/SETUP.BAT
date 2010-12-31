@echo off
setm
IF ERRORLEVEL 1 GOTO ABORTED2
GOTO END
:ABORTED2
echo Midi Music Driver configuration was not completed.
GOTO FINI
:END
avpatch
call switch
echo Configuration Complete!
echo Thank you for using The Ultima 4 upgrade patch v1.1
echo
echo run 'ultima.com' to play Ultima 4 with sound
echo run 'switch.bat' to disable/reenable the patch
:FINI
