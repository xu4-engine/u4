@CLS
@ECHO OFF
@ECHO                    ULTIMA IV - QUEST OF THE AVATAR
@ECHO ...........................................................................
@ECHO    This program will install Ultima IV to your hard drive. The directory
@ECHO    will be C:\ULTIMA4. If this is okay, hit the SPACEBAR. If it is not
@ECHO    hit CTRL-C and when asked to TERMINATE BATCH JOB(Y/N) type in Y.
@ECHO ...........................................................................
@PAUSE
@MD C:\ULTIMA4
@COPY *.* C:\ULTIMA4
@C:
@CD\ULTIMA4
@CLS
@ECHO ...........................................................................
@ECHO    Ultima IV is now installed to your hard drive at C:\ULTIMA4. Hit any
@ECHO    key to begin playing Ultima IV: Quest of the Avatar. Enjoy!
@ECHO ...........................................................................
@PAUSE
@ULTIMA
