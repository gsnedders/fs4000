@echo off
rem save32 *.* S:\FS4sav /a/u
rem xcopy  libtiff.dll   L:\cb3\fs4000 /D   > nul
rem xcopy  Fs4000.h      L:\cb3\fs4000 /D   > nul
rem xcopy  Fs4000-scsi.h L:\cb3\fs4000 /D   > nul
rem xcopy  Fs4000.dll    L:\cb3\fs4000 /D   > nul
rem xcopy  Fs4000imp.lib L:\cb3\fs4000 /D   > nul
rem xcopy  FsTiff.exe    L:\cb3\fs4000 /D   > nul
rem xcopy  L:\cb3\fs4000\FsHost.exe .\ /D   > nul
    if exist *.tds del *.tds
    if exist *.tds goto end
    L:\winzip\wzzip -ex -u -whs N:\Sites\Exetel\fs4.zip *.* -x-*.*
rem L:\winzip\wzzip -ex -u -whs N:\Sites\Exetel\fs4.zip *.exe *.dll *.txt -x-*.*
:end
