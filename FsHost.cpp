//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("FsHost.res");
USELIB("Fs4000imp.lib");
USEFORM("FsHostUnit1.cpp", BaseForm);
USEFORM("FsHostUnit2.cpp", AboutBox);
//---------------------------------------------------------------------------
WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int)
 {
  try
   {
    Application->Initialize();
    Application->CreateForm(__classid(TBaseForm), &BaseForm);
                 Application->Run();
   }
  catch (Exception &exception)
   {
    Application->ShowException(&exception);
   }
  return 0;
 }
//---------------------------------------------------------------------------
