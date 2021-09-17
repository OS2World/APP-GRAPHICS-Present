# IBM Developer's Workframe/2 Make File Creation run at 12:29:23 on 04/08/92

# Make File Creation run in directory:
#   E:\C\SHEET;

.SUFFIXES:

.SUFFIXES: .c .rc

ALL: PRESENT.EXE \
     PRESENT.RES

PRESENT.EXE:  \
  PRESENT.OBJ \
  PRESENT.RES \
  PRESFONT.OBJ \
  MAKEFILE
   @REM @<<PRESENT.@0
     PRESENT.OBJ +
     PRESFONT.OBJ
     PRESENT.EXE
     PRESENT.DEF
     
<<
   ILINK.EXE /NOFREE /PMTYPE:PM @PRESENT.@0
   RC PRESENT.RES PRESENT.EXE
   dllrname.exe PRESENT.EXE CPPOM30=OS2OM30

{.}.rc.res:
   RC -r .\$*.RC

{.}.c.obj:
   ICC.EXE /c /I. /Ge+ /Gm+ /Gd+ .\$*.c

clean :
        @if exist *.obj del *.obj
        @if exist *.dll del *.dll
        @if exist *.exe del *.exe
        @if exist *.res del *.res

!include MAKEFILE.DEP
