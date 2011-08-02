
CC       = n:\bcc55\bin\bcc32.exe
LINK     = n:\bcc55\bin\ilink32.exe

CFLAGS   = -c -tWC -4
CPPFLAGS = -c -tWC -4 -B -Tt
LDFLAGS  = -Tpd -x -Gn -C -c
LEFLAGS  = -Tpe -x -Gn -C -c

C_OBJS   = fs4000-scsi.obj scsi_via_aspi.obj scsi_via_usb.obj

all: Fs4000.dll Fs4000.exe

Fs4000.exe: Fs4000.obj $(C_OBJS)
  $(LINK) $(LEFLAGS) @&&!
  C0X32 $**
  $*
  nul
  CW32 IMPORT32 LIBTIFFIMP
!
  @if exist $*.tds del $*.tds
  ren $< $<

Fs4000.dll: Fs4000.obj $(C_OBJS)
  $(LINK) $(LDFLAGS) @&&!
  C0D32 $**
  $*
  nul
  CW32 IMPORT32 LIBTIFFIMP
!
  @if exist $*.tds del $*.tds
  ren $< $<
  implib -c Fs4000imp.lib Fs4000.dll
  impdef    Fs4000imp.def Fs4000.dll

Fs4000.obj: Fs4000.cpp Fs4000*.inc Fs4000.new Fs4000.h scsi*.h
  $(CC) $(CPPFLAGS) Fs4000.cpp
  ren $< $<

fs4000-scsi.obj: fs4000-scsi.c fs4000-scsi.h
  $(CC) $(CFLAGS) fs4000-scsi.c

scsi_via_aspi.obj: scsi_via_aspi.c scsi_via_aspi.h
  $(CC) $(CFLAGS) scsi_via_aspi.c

scsi_via_usb.obj: scsi_via_usb.c scsi_via_usb.h
  $(CC) $(CFLAGS) scsi_via_usb.c

