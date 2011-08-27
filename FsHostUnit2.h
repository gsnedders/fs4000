//---------------------------------------------------------------------------
#ifndef FsHostUnit2H
#define FsHostUnit2H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TAboutBox : public TForm
{
__published:	// IDE-managed Components
        TButton *CloseButton;
        TLabel *Label1;
        TMemo *Memo1;
        void __fastcall CloseButtonClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TAboutBox(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TAboutBox *AboutBox;
//---------------------------------------------------------------------------
#endif
