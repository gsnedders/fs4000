//---------------------------------------------------------------------------
#ifndef FsHostUnit1H
#define FsHostUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
#include "Fs4000.h"
#include <Dialogs.hpp>
//---------------------------------------------------------------------------

struct  ThumBuf
  {
        int     iHolderType;
        int     iSpeed;
        int     iLines;
        int     iInputLines;
        int     iExtraLines;
        TColor  ent [160 * 1490];
  };

class TBaseForm : public TForm
  {
__published:	// IDE-managed Components
    TButton *ButtonOff;
    TButton *ButtonHome;
    TButton *ButtonTune;
    TButton *ButtonThumb;
    TButton *ButtonScan;
    TButton *ButtonEject;
    TButton *ButtonAbort;
    TCheckBox *CheckBoxAE;
    TCheckBox *CheckBoxNeg;
    TCheckBox *CheckBox1;
    TCheckBox *CheckBox2;
    TCheckBox *CheckBox3;
    TCheckBox *CheckBox4;
    TCheckBox *CheckBox5;
    TCheckBox *CheckBox6;
    TCheckBox *CheckBoxRpt;
    TMainMenu *mnu;
    TMenuItem *mnuFile;
    TMenuItem *mnuFileExit;
    TMenuItem *mnuHelp;
    TMenuItem *mnuHelpAbout;
    TPaintBox *PaintBox;
    TPanel *Panel1;
    TScrollBar *ScrollBar;
    TStaticText *StaticTextAE;
    TStaticText *StaticTextNeg;
    TStaticText *StaticText1;
    TStaticText *StaticText2;
    TStaticText *StaticText3;
    TStaticText *StaticText4;
    TStaticText *StaticText5;
    TStaticText *StaticText6;
    TStaticText *StaticTextRpt;
    TStatusBar *StatusBar;
    TTimer *Timer1;
    TTrackBar *BoostBar;
    TStaticText *StaticTextBoost;
        TMenuItem *mnuFileName;
        TSaveDialog *SaveDialog1;
    void __fastcall FormActivate(TObject *Sender);
    void __fastcall mnuFileNameClick(TObject *Sender);
    void __fastcall mnuFileExitClick(TObject *Sender);
    void __fastcall mnuHelpAboutClick(TObject *Sender);
    void __fastcall Timer1Timer(TObject *Sender);
    void __fastcall BoostBarChange(TObject *Sender);
    void __fastcall ButtonOffClick(TObject *Sender);
    void __fastcall ButtonHomeClick(TObject *Sender);
    void __fastcall ButtonTuneClick(TObject *Sender);
    void __fastcall ButtonThumbClick(TObject *Sender);
    void __fastcall ButtonScanClick(TObject *Sender);
    void __fastcall ButtonEjectClick(TObject *Sender);
    void __fastcall ButtonAbortClick(TObject *Sender);
    void __fastcall CheckBoxAEClick(TObject *Sender);
    void __fastcall CheckBoxnClick(TObject *Sender);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormResize(TObject *Sender);
    void __fastcall PaintBoxPaint(TObject *Sender);
    void __fastcall PaintBoxMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
    void __fastcall PaintBoxMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall CheckBoxNegClick(TObject *Sender);
    void __fastcall CheckBoxEnter(TObject *Sender);
    void __fastcall CheckBoxExit(TObject *Sender);
    void __fastcall ScrollBarScroll(TObject *Sender,
          TScrollCode ScrollCode, int &ScrollPos);
private:	// User declarations

        char            szOurPath [256];
        HANDLE          hLog;
        FS4K_GLOBALS    *pFSG;
        int             iThumbLines, iPaintCols, iPaintBoxLastX;
        int             iNowBoost;
        ThumBuf         *pThumbNail;

    LPSTR   CalFileID           (void);
    int     LoadCalInfo         (void);
    int     DumpCalInfo         (void);
    BOOL    NegMode             (void);
    int     SwitchButtons       (BOOL bSetting, int _iHolderType);
    void    CommandPrep         (void);
    void    CheckBoxToggle      (TCheckBox *pCheckBox);
    void    ButtonPush          (TButton *pButton);
    void    MakeActive          (TWinControl *pControl);
    void    PaintBoxUpdate      (int iMin, int iMax);
    void    PaintBoxScroll      (int iChange);
    void    ThumbNailBuild      (DWORD dwInputLines);
    void    AdjustBoost         (int iChange);
    void    AdjustScroll        (int iChange);

public:		// User declarations

        TWinControl     *pLastActive;

    __fastcall TBaseForm(TComponent* Owner);
  };
//---------------------------------------------------------------------------
extern PACKAGE TBaseForm *BaseForm;
//---------------------------------------------------------------------------
#endif

