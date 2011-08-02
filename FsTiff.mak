
CC       = n:\bcc55\bin\bcc32.exe
LINK     = n:\bcc55\bin\ilink32.exe

CPPFLAGS = -c -tWC -4
LFLAGS   = -Tpe -x -Gn -C -c

FsTiff.exe: FsTiff.obj
  $(LINK) $(LFLAGS) @&&!
  C0X32 $**
  $*
  nul
  CW32 IMPORT32 LIBTIFFIMP
!
  @if exist $*.tds del $*.tds
  ren $< $<

FsTiff.obj: FsTiff.cpp Fs4000.h
  $(CC) $(CPPFLAGS) FsTiff.cpp
  ren $< $<

