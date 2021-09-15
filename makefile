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
     
<<
   ILINK.EXE /NOFREE /PMTYPE:PM @PRESENT.@0
   RC PRESENT.RES PRESENT.EXE

{.}.rc.res:
   RC -r .\$*.RC

{.}.c.obj:
   ICC.EXE /c /I. /Ss .\$*.c

!include MAKEFILE.DEP