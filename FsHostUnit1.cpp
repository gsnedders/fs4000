//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "FsHostUnit1.h"
#include "FsHostUnit2.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TBaseForm *BaseForm;

//---------------------------------------------------------------------------
//
//====          Code for background thread
//
        HANDLE          hFs4000         = NULL;
        DWORD           dwThreadId;
        LPSTR           pThreadArg      = NULL;
        LPSTR           pThreadText     = NULL;
        char            cText      [256];
        char            cLastToken [64];
        FS4K_NEWS       rNews;
        FS4K_NEWS       rLastNews;
        int             iNewsCtrIn      = 0;
        int             iNewsCtrOut     = 0;
        int             iHolderType     = -1;
        int             iLastErc;
        BOOL            bTaskDone       = FALSE;

void    Fs4000Display           (LPSTR pMsg)    // progress display
 {
  if (BaseForm->StatusBar->SimpleText != pMsg)
    BaseForm->StatusBar->SimpleText = pMsg;
  return;
 }


DWORD WINAPI Fs4000Thread       (void*)         // do scanner work
 {
  while (1)
   {
    pThreadArg = NULL;                          // = idle
    SuspendThread (hFs4000);
    bTaskDone = FALSE;
    do
     {
      if (pThreadText)                          // multi-word string ?
       {
        while (*pThreadText && (*pThreadText < '!'))
          pThreadText++;
        pThreadArg = pThreadText;               // word start
        if (!*pThreadText)
         {
          pThreadArg = pThreadText = NULL;
          break;                                // undo do
         }
        while (*pThreadText > ' ')              // find word end
          pThreadText++;
        if (!*pThreadText)                      // last word ?
          pThreadText = NULL;
        else
          *pThreadText++ = '\0';
       }
      iLastErc = FS4_ProcessWord (pThreadArg);  // do some work
      strcpy (cLastToken, pThreadArg);
      if (iLastErc)
        pThreadText = NULL;
     }
    while (pThreadText);
    bTaskDone = TRUE;
   }
//return 0;                                     // should never get here
 }


int     Fs4000DoWord            (LPSTR pArg)
 {
  while (pThreadArg)                            // wait if busy
    Sleep (100);
  iNewsCtrOut = iNewsCtrIn;
  Fs4000Display ("Scanner busy");
  BaseForm->ButtonAbort->Enabled = TRUE;
  BaseForm->ActiveControl = BaseForm->ButtonAbort;
  Application->Title = "FsHost - busy";
  pThreadArg = pArg;
  BaseForm->Timer1->Interval = 200;             // = busy
  BaseForm->Timer1->Enabled = TRUE;
  return ResumeThread (hFs4000);
 }


int     Fs4000DoText            (LPSTR pText)
 {
  while (pThreadArg)
    Sleep (100);
  pThreadText = pText;
  return Fs4000DoWord (pText);
 }


int     Fs4000StartThread       (void)
 {
  iNewsCtrIn = iNewsCtrOut = 0;
  hFs4000 = CreateThread (NULL,
                          0,
                          Fs4000Thread,
                          NULL,
                          NULL,
                          &dwThreadId);
  Sleep (10);                                   // let thread start and halt
  pThreadText = NULL;
  return Fs4000DoWord ("in16");
 }


void    Fs4000Callback          (FS4K_NEWS *pNews)
 {
  rNews = *pNews;                               // get news
  iNewsCtrIn++;
  return;
 }


//---------------------------------------------------------------------------

__fastcall TBaseForm::TBaseForm(TComponent* Owner) : TForm(Owner)
 {
  hLog        = NULL;
  iThumbLines = 0;
  iPaintCols  = 0;
  pFSG        = NULL;
  pThumbNail  = NULL;
  iNowBoost   = 2;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::FormActivate(TObject *Sender)
 {
        char            cLine [256];
        SYSTEMTIME      sDT;
        DWORD           dwDone;

  GetCurrentDirectory (sizeof (szOurPath), szOurPath);
  hLog = CreateFile ("-FsHost.log",
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_ALWAYS,
                     0,
                     NULL);
  if (hLog == INVALID_HANDLE_VALUE)
    hLog = NULL;
  else
   {
    SetFilePointer (hLog, 0, NULL, FILE_END);
    GetLocalTime (&sDT);
    wsprintf (cLine, "\r\n**** Start %04d-%02d-%02d  %2d:%02d:%02d\r\n",
                sDT.wYear, sDT.wMonth,  sDT.wDay,
                sDT.wHour, sDT.wMinute, sDT.wSecond);
    WriteFile (hLog, cLine, strlen (cLine), &dwDone, NULL);
   }
  FS4_SetHandles (0, 0, hLog);                  // log all output
  if (FS4_BOJ () == 0)
    if (FS4_GlobalsLoc ()->dwGlobalsSize == sizeof (FS4KG))
     {
      pFSG = FS4_SetCallback (Fs4000Callback);
      pFSG->bShowProgress = FALSE;
      pLastActive = ButtonOff;
      Fs4000StartThread ();
      LoadCalInfo ();
      mnuFileName->Enabled = TRUE;
     }
    else
      StatusBar->SimpleText = "Incorrect DLL version !!!";
  else
   {
    SwitchButtons (FALSE, 0);
    StatusBar->SimpleText = "Scanner ERROR !!!";
   }
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::mnuFileNameClick(TObject *Sender)
 {
  SaveDialog1->FileName = pFSG->szScanFID;
  if (SaveDialog1->Execute ())
    strcpy (pFSG->szScanFID, SaveDialog1->FileName.c_str ());
  SetCurrentDirectory (szOurPath);
//StatusBar->SimpleText = pFSG->cScanFID;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::mnuFileExitClick(TObject *Sender)
 {
  Close ();
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::mnuHelpAboutClick(TObject *Sender)
 {
        TAboutBox*      AboutBox = new TAboutBox (this);

  AboutBox->Left = PaintBox->ClientOrigin.x +
                   ((PaintBox->Width  - AboutBox->Width ) / 2);
  AboutBox->Top  = PaintBox->ClientOrigin.y +
                   ((PaintBox->Height - AboutBox->Height) / 2);
  AboutBox->ShowModal ();
  delete AboutBox;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::Timer1Timer(TObject *Sender)
 {
        char            cMsg [256];
        int             iTemp;

  if (pThreadArg)                               // doing something ?
   {
    iTemp = iNewsCtrOut;                        // care while accessing
    while (iNewsCtrIn != iNewsCtrOut)           // shared memory
     {
      iNewsCtrOut = iNewsCtrIn;
      rLastNews = rNews;
     }
    if (iNewsCtrOut != iTemp)                   // news ?
     {
      strcpy (cMsg, rLastNews.cTask);
      if (cMsg [0])
       {
        if (rLastNews.cStep [0])
         {
          strcat (cMsg, ", ");
          strcat (cMsg, rLastNews.cStep);
          if ((rLastNews.iPercent >= 0) && (rLastNews.iPercent < 102))
            if (rLastNews.iPercent < 101)
              wsprintf (strchr (cMsg, 0), " %d%%", rLastNews.iPercent);
            else
              strcat (cMsg, " done");
         }
        Fs4000Display (cMsg);
       }
     }
   }
  else                                          // doing nothing
   {
    if (bTaskDone)                              // just finished ?
     {
      bTaskDone = FALSE;                        // seen it
      Application->Title = "FsHost";
      Fs4000Display ("Scanner ready");
      Timer1->Interval = 2000;
      if (iLastErc == 0)                        // if no error
       {
        if ((strcmpi (cLastToken, "tune")  == 0) ||     // if tune
            (strcmpi (cLastToken, "tuneg") == 0) )      // save it
          DumpCalInfo ();
        if (strcmpi (cLastToken, "thumb")  == 0)        // if thumb
         {
          if (pThumbNail)                               // if our buf
            pThumbNail->iExtraLines = 5;                // margin at end
          pLastActive = ScrollBar;                      // default control
         }
       }
     }
    SwitchButtons (TRUE, fs4k_GetHolderType ());
    if (pLastActive)                            // restore focus ?
     {
      if ((ActiveControl == ButtonAbort) ||
          (ActiveControl == NULL)        )
        if (pLastActive->Enabled)
          ActiveControl = pLastActive;
        else
          ActiveControl = ButtonOff;
      pLastActive = NULL;
      ButtonAbort->Enabled = FALSE;
     }
    if (CheckBoxRpt->Checked && ButtonScan->Enabled)
      ButtonScanClick (this); // was (Sender);
   }
  iThumbLines = 0;                              // thumbnail ?
  if (pFSG->rScan.pBuf)
    if (pFSG->rScan.wLPI == 160)
      ThumbNailBuild (pFSG->rScan.dwLinesDone);
  if (pThumbNail)                               // our buffer ?
    iThumbLines = pThumbNail->iLines + pThumbNail->iExtraLines;
  if (iThumbLines == 0)
   {
    if (ScrollBar->Enabled)
     {
      ScrollBar->Enabled = FALSE;
      ScrollBar->Max = 0;
     }
   }
  else
   {
    iTemp = iThumbLines - PaintBox->Width;
    if (iTemp < 0)
      iTemp = 0;
    ScrollBar->Max = iTemp;
    if (!ScrollBar->Enabled)
      ScrollBar->Enabled = TRUE;
   }
  if (ScrollBar->Position > ScrollBar->Max)
    ScrollBar->Position = ScrollBar->Max;
  if (iThumbLines != iPaintCols)
    PaintBoxUpdate (iPaintCols, iThumbLines);
  iTemp = 2000;
  if (pThreadArg || bTaskDone)
    iTemp = 200;
  if (Timer1->Interval != (DWORD) iTemp)
    Timer1->Interval = iTemp;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::BoostBarChange(TObject *Sender)
 {
        int             iBoost [] = {12, 10, 8, 6, 5, 4, 3, 2, 1};
        char            msg [32];

  iNowBoost = iBoost [BoostBar->Position];
  if (pFSG->iSpeed != iNowBoost)                // if change
   {
    pFSG->iSpeed = iNowBoost;                   // change driver field
    wsprintf (msg, "Exp %d", pFSG->iSpeed);
    StaticTextBoost->Caption = msg;             // update display
    if (pThumbNail)                             // if buffered thumbnail
     {
      iPaintCols = 0;                           //   = all display
      Timer1->Interval = 250;                   //   refresh display soon
     }
   }
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonOffClick(TObject *Sender)
 {
  CommandPrep ();
  Fs4000DoWord ("off");
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonHomeClick(TObject *Sender)
 {
  CommandPrep ();
  Fs4000DoWord ("home");
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonTuneClick(TObject *Sender)
 {
        BOOL            bNegMode;

  BoostBar->Position = 7;                       // force default speed
  BoostBarChange (this);
  bNegMode = (CheckBoxNeg->Enabled && CheckBoxNeg->Checked);
  strcpy (cText, "speed 2 tune");
  if (bNegMode)
    strcat (cText, "g");                        // "tuneg" = tune for negs
  CommandPrep ();
  Fs4000DoText (cText);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonThumbClick(TObject *Sender)
 {
        int             index, limit;

  if (!pThumbNail)                              // get buffer if reqd
    pThumbNail = new ThumBuf;
  if (pThumbNail)
   {
    memset (pThumbNail, 0, (byte*) pThumbNail->ent - (byte*) pThumbNail);
    pThumbNail->iHolderType = iHolderType;
    pThumbNail->iSpeed      = pFSG->iSpeed;
    limit = sizeof (pThumbNail->ent) / sizeof (pThumbNail->ent [0]);
    for (index = 0; index < limit; index++)
      pThumbNail->ent [index] = clWindow;
   }
  pFSG->rScan.dwLinesDone = 0;                  // no lines yet
  if (NegMode ())
    strcpy (cText, "out8i");
  else
    strcpy (cText, "out8");
  strcat (cText, " tif- thumb");
  CommandPrep ();
  Fs4000DoText (cText);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonScanClick(TObject *Sender)
 {
  if (NegMode ())
    strcpy (cText, "out8i");
  else
    strcpy (cText, "out8");

  strcat (cText, " tif");

  if (CheckBox1->Enabled && CheckBox1->Checked)
    strcat (cText, " scan 1");
  if (CheckBox2->Enabled && CheckBox2->Checked)
    strcat (cText, " scan 2");
  if (CheckBox3->Enabled && CheckBox3->Checked)
    strcat (cText, " scan 3");
  if (CheckBox4->Enabled && CheckBox4->Checked)
    strcat (cText, " scan 4");
  if (CheckBox5->Enabled && CheckBox5->Checked)
    strcat (cText, " scan 5");
  if (CheckBox6->Enabled && CheckBox6->Checked)
    strcat (cText, " scan 6");

  if (CheckBoxRpt->Checked)
    strcat (cText, " eject");

  CommandPrep ();
  Fs4000DoText (cText);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonEjectClick(TObject *Sender)
 {
  CommandPrep ();
  Fs4000DoWord ("eject");
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ButtonAbortClick(TObject *Sender)
 {
  FS4_SetAbort ();
  ButtonAbort->Enabled = FALSE;
  CheckBoxRpt->Checked = FALSE;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::CheckBoxAEClick(TObject *Sender)
 {
  pFSG->bAutoExp = CheckBoxAE->Checked;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::CheckBoxNegClick(TObject *Sender)
 {
  pFSG->bNegMode = CheckBoxNeg->Checked;
  LoadCalInfo ();
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::CheckBoxnClick(TObject *Sender)
 {
  SwitchButtons ((pThreadArg == NULL), iHolderType);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
 {
  switch (Key)
   {
    case VK_ESCAPE:
      if (ButtonAbort->Enabled)
        ButtonAbortClick (this);   // was (Sender);
      else
        if (!pThreadArg)
          Close ();
      break;
    case '1':         CheckBoxToggle (CheckBox1);   break;
    case '2':         CheckBoxToggle (CheckBox2);   break;
    case '3':         CheckBoxToggle (CheckBox3);   break;
    case '4':         CheckBoxToggle (CheckBox4);   break;
    case '5':         CheckBoxToggle (CheckBox5);   break;
    case '6':         CheckBoxToggle (CheckBox6);   break;
    case VK_MULTIPLY: CheckBoxToggle (CheckBoxAE);  break;
    case VK_SUBTRACT: CheckBoxToggle (CheckBoxNeg); break;
    case VK_ADD:      CheckBoxToggle (CheckBoxRpt); break;
    case 'O':         ButtonPush     (ButtonOff);   break;
    case 'M':         ButtonPush     (ButtonHome);  break;
    case 'T':         ButtonPush     (ButtonTune);  break;
    case 'U':         ButtonPush     (ButtonThumb); break;
    case 'S':         ButtonPush     (ButtonScan);  break;
    case 'E':         ButtonPush     (ButtonEject); break;
    case 'A':         ButtonPush     (ButtonAbort); break;
    case 'V':         MakeActive     (BoostBar);    break;
    case 'H':         MakeActive     (ScrollBar);   break;
    case VK_UP:       AdjustBoost    (-1);          break;
    case VK_DOWN:     AdjustBoost    (+1);          break;
    case VK_LEFT:     AdjustScroll   (-16);         break;
    case VK_RIGHT:    AdjustScroll   (+16);         break;
    default:          return;
   }
  Key = 0;                                      // 0 = we've used it
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::FormResize(TObject *Sender)
 {
        int             iReqdHeight = 329;
        int             iMinWidth   = 405;

  if ((Height != iReqdHeight) || (Width < iMinWidth))
   {
    if (Height != iReqdHeight)
      Height = iReqdHeight;
    if (Width < iMinWidth)
      Width = iMinWidth;
    return;
   }
  PaintBox->Width  = ClientWidth;
  ScrollBar->Width = ClientWidth;
  PaintBoxPaint (this);  // was (Sender);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::PaintBoxPaint(TObject *Sender)
 {
//PaintBoxUpdate (0, PaintBox->Width);

//      must do paint via timer routine

  iPaintCols = 0;
  Timer1->Interval = 100;                       // refresh display soon
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::ScrollBarScroll(TObject *Sender,
      TScrollCode ScrollCode, int &ScrollPos)
 {
        int             iChange;
        TRect           rDest;
        TRect           rSrc;

  iChange = ScrollPos - ScrollBar->Position;

//      Zero change may be due to up/down key that has been nulled after
//      adjusting BoostBar.  Doing anything here wrecks the pending paint.

  if (iChange)                                  // if change
    PaintBoxScroll (iChange);
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::PaintBoxMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
 {
  if (Shift.Contains (ssLeft))
    iPaintBoxLastX = X;
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::PaintBoxMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
 {
        int             iChange;
        int             iMax;
        int             iPos;
        int             iNewPos;

  if (Shift.Contains (ssLeft))
   {
    iChange = iPaintBoxLastX - X;
    iPaintBoxLastX = X;
    iMax = ScrollBar->Max;
    iPos = ScrollBar->Position;
    iNewPos = iPos + iChange;
    if (iNewPos < 0   ) iNewPos = 0;
    if (iNewPos > iMax) iNewPos = iMax;
    if (iNewPos != iPos)
      PaintBoxScroll (iNewPos - iPos);
   }
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::CheckBoxEnter(TObject *Sender)
 {
  switch (((TCheckBox*) Sender)->Tag)
   {
    case 0: StaticTextAE ->BorderStyle = sbsSunken; break;
    case 1: StaticTextNeg->BorderStyle = sbsSunken; break;
    case 2: StaticText1  ->BorderStyle = sbsSunken; break;
    case 3: StaticText2  ->BorderStyle = sbsSunken; break;
    case 4: StaticText3  ->BorderStyle = sbsSunken; break;
    case 5: StaticText4  ->BorderStyle = sbsSunken; break;
    case 6: StaticText5  ->BorderStyle = sbsSunken; break;
    case 7: StaticText6  ->BorderStyle = sbsSunken; break;
    case 8: StaticTextRpt->BorderStyle = sbsSunken; break;
   }
  return;
 }
//---------------------------------------------------------------------------

void __fastcall TBaseForm::CheckBoxExit(TObject *Sender)
 {
  switch (((TCheckBox*) Sender)->Tag)
   {
    case 0: StaticTextAE ->BorderStyle = sbsNone; break;
    case 1: StaticTextNeg->BorderStyle = sbsNone; break;
    case 2: StaticText1  ->BorderStyle = sbsNone; break;
    case 3: StaticText2  ->BorderStyle = sbsNone; break;
    case 4: StaticText3  ->BorderStyle = sbsNone; break;
    case 5: StaticText4  ->BorderStyle = sbsNone; break;
    case 6: StaticText5  ->BorderStyle = sbsNone; break;
    case 7: StaticText6  ->BorderStyle = sbsNone; break;
    case 8: StaticTextRpt->BorderStyle = sbsNone; break;
   }
  return;
 }
//---------------------------------------------------------------------------

LPSTR   TBaseForm::CalFileID    (void)
 {
        static char     szCalFID [256];

  strcpy (szCalFID, szOurPath);
  strcat (szCalFID, "\\-FsHost");
  strcat (szCalFID, (pFSG->bNegMode ? "n.cal" : "p.cal"));
  return (szCalFID);
 }
//---------------------------------------------------------------------------

int     TBaseForm::LoadCalInfo  (void)
 {
  return fs4k_LoadCalInfo (CalFileID ());
 }
//---------------------------------------------------------------------------

int     TBaseForm::DumpCalInfo  (void)
 {
  return fs4k_DumpCalInfo (CalFileID ());
 }
//---------------------------------------------------------------------------

BOOL    TBaseForm::NegMode      (void)
 {
        BOOL            bOldMode;
        BOOL            bNewMode;

  bOldMode = pFSG->bNegMode;
  bNewMode = (CheckBoxNeg->Enabled && CheckBoxNeg->Checked);
  if (bNewMode != bOldMode)
    {
    pFSG->bNegMode = bNewMode;
    LoadCalInfo ();
    }
  return pFSG->bNegMode;
 }
//---------------------------------------------------------------------------

int     TBaseForm::SwitchButtons (BOOL bSetting, int _iHolderType)
 {
        BOOL            bTemp;

  if (_iHolderType != iHolderType)              // film holder change ?
   {
    iHolderType = _iHolderType;
//  bTemp = (iHolderType > 0);
    bTemp = TRUE;
    if (CheckBox1->Enabled != bTemp)
      CheckBox1->Enabled = bTemp;
    if (CheckBox2->Enabled != bTemp)
      CheckBox2->Enabled = bTemp;
    if (CheckBox3->Enabled != bTemp)
      CheckBox3->Enabled = bTemp;
    if (CheckBox4->Enabled != bTemp)
      CheckBox4->Enabled = bTemp;
//  bTemp = (iHolderType == 1);
    bTemp = (iHolderType < 2);
    if (CheckBox5->Enabled != bTemp)
      CheckBox5->Enabled = bTemp;
    if (CheckBox6->Enabled != bTemp)
      CheckBox6->Enabled = bTemp;
   }

  if (BoostBar->Enabled != bSetting)
    BoostBar->Enabled   = bSetting;

  if (ButtonOff->Enabled != bSetting)
    ButtonOff->Enabled   = bSetting;

  bTemp = bSetting && (iHolderType > 0);
  if (ButtonTune->Enabled != bTemp)
    ButtonTune->Enabled   = bTemp;
  if (ButtonHome->Enabled != bTemp)
    ButtonHome->Enabled   = bTemp;
  if (ButtonThumb->Enabled != bTemp)
    ButtonThumb->Enabled   = bTemp;
  if (ButtonEject->Enabled != bTemp)
    ButtonEject->Enabled   = bTemp;

  bTemp = FALSE;
  if (iHolderType > 0)                          // any film holder ?
   {
    bTemp |= CheckBox1->Checked;
    bTemp |= CheckBox2->Checked;
    bTemp |= CheckBox3->Checked;
    bTemp |= CheckBox4->Checked;
   }
  if (iHolderType == 1)                         // neg film holder ?
   {
    bTemp |= CheckBox5->Checked;
    bTemp |= CheckBox6->Checked;
   }

  bTemp &= bSetting;
  if (ButtonScan->Enabled != bTemp)
    ButtonScan->Enabled   = bTemp;

  bTemp |= CheckBoxRpt->Checked;
  if (CheckBoxRpt->Enabled != bTemp)
    CheckBoxRpt->Enabled   = bTemp;

  if (CheckBoxAE->Enabled != bSetting)
    CheckBoxAE->Enabled   = bSetting;

  bTemp = bSetting && (iHolderType == 1);
  if (CheckBoxNeg->Enabled != bTemp)
    CheckBoxNeg->Enabled   = bTemp;

  return 0;
 }
//---------------------------------------------------------------------------

void    TBaseForm::CommandPrep  (void)
 {
  pLastActive = ActiveControl;
  SwitchButtons (FALSE, iHolderType);
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::CheckBoxToggle (TCheckBox *pCheckBox)
 {
  if (pCheckBox->Enabled)
   {
    pCheckBox->Checked = !pCheckBox->Checked;
    if (!pThreadArg)                            // idle ?
      SwitchButtons (TRUE, iHolderType);
   }
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::ButtonPush   (TButton *pButton)
 {
  if (pButton->Enabled)
    pButton->OnClick (pButton);
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::MakeActive   (TWinControl *pControl)
 {
  if (pControl->Enabled)
    ActiveControl = pControl;
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::PaintBoxUpdate (int iMin, int iMax)
 {
        int             iColour, x, y, z, iTemp;
        WORD            *pwEnt;
        BYTE            *pbXlate;
        BYTE            *pbEnt;
        TRect           rRect;
        BYTE            byConv [256];

  if (iMax < iMin)                              // ensure valid params
   {
    iMin = iMax;
    iMax = PaintBox->Width;
   }
  if (iMin > PaintBox->Width)
    iMin = PaintBox->Width;
  if (iMin > iThumbLines)
    iMin = iThumbLines;
  iPaintCols = iThumbLines;

  iTemp = 0;                                    // left col for fill
  if (pFSG)
   {
    iTemp = iThumbLines - ScrollBar->Position;
    if (iTemp > PaintBox->Width)
      iTemp = PaintBox->Width;
    if (iMax > iTemp)
      iMax = iTemp;
    if (pThumbNail)                             // if our buffer
     {
      if (pThumbNail->iSpeed != iNowBoost)      // adjust reqd ?
       {
        pbXlate = pFSG->pbXlate;
        if (pbXlate [0] < pbXlate [65535])      // positives mode ?
         {
          for (x = y = 0; x < 256; x++)         // calc conversion ents
           {
            while (pbXlate [y] < x)
              y++;
            z = y * iNowBoost / pThumbNail->iSpeed;
            if (z > 65535)
              byConv [x] = 255;
            else
              byConv [x] = pbXlate [z];
           }
         }
        else                                    // negatives mode
         {
          for (x = 0, y = 65535; x < 256; x++)  // calc conversion ents
           {
            while (pbXlate [y] != x)
              y--;
            z = y * iNowBoost / pThumbNail->iSpeed;
            if (z > 65535)
              z = 65535;
            byConv [x] = pbXlate [z];
           }
         }
       }
      for (x = iMin; x < iMax; x++)             // load display
       {
        z = (x + ScrollBar->Position) * 160;
        for (y = PaintBox->Height; y--; )
         {
          if (pThumbNail->iSpeed == iNowBoost)
            PaintBox->Canvas->Pixels [x] [y] = pThumbNail->ent [z + y];
          else
           {
            iColour = (int) pThumbNail->ent [z + y];
            pbEnt = (BYTE*) &iColour;
            if (pbEnt [3] == 0)                 // 0 = RGB ent
             {
              *pbEnt++ = byConv [*pbEnt];
              *pbEnt++ = byConv [*pbEnt];
              *pbEnt   = byConv [*pbEnt];
             }
            PaintBox->Canvas->Pixels [x] [y] = (TColor) iColour;
           }
         }
       }
     }
    else                                        // direct from scan buf
     {
      pbXlate = pFSG->pbXlate;
      pwEnt = (WORD*) pFSG->rScan.pBuf;
      pwEnt += (iMin + ScrollBar->Position) * (480 + pFSG->iMargin);
      iColour = 0;
      pbEnt = (BYTE*) &iColour;
      for (x = iMin; x < iMax; x++)
       {
        pwEnt += (480 + pFSG->iMargin) - (3 * PaintBox->Height);
        for (y = PaintBox->Height; y--; )
         {
          pbEnt [0] = pbXlate [*pwEnt++];
          pbEnt [1] = pbXlate [*pwEnt++];
          pbEnt [2] = pbXlate [*pwEnt++];
          PaintBox->Canvas->Pixels [x] [y] = (TColor) iColour;
         }
       }
     }
   }
  rRect.Left   = iTemp;                         // fill rest of area
  rRect.Top    = 0;
  rRect.Right  = PaintBox->Width;
  rRect.Bottom = PaintBox->Height;
  PaintBox->Canvas->Brush->Color = PaintBox->Color;
  PaintBox->Canvas->FillRect (rRect);
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::PaintBoxScroll (int iChange)
 {
        TRect           rDest;
        TRect           rSrc;
        int             iAbsChange;

  if ((ScrollBar->Position + iChange) < 0)
    iChange = ScrollBar->Position;
  if ((ScrollBar->Position + iChange) > ScrollBar->Max)
    iChange = ScrollBar->Max - ScrollBar->Position;
  rDest = PaintBox->ClientRect;
  rSrc  = PaintBox->ClientRect;
  if (iChange > 0)
   {
    rSrc.Left   += iChange;
    rDest.Right -= iChange;
    iAbsChange   = iChange;
   }
  else
   {
    rDest.Left  -= iChange;
    rSrc.Right  += iChange;
    iAbsChange   = 0 - iChange;
   }
  ScrollBar->Position += iChange;
  if (iAbsChange >= PaintBox->Width)
    PaintBoxUpdate (0, PaintBox->Width);
  else
   {
    PaintBox->Canvas->CopyRect (rDest, PaintBox->Canvas, rSrc);
    if (rDest.Left == 0)
      PaintBoxUpdate (rDest.Right, rSrc.Right);
    else
      PaintBoxUpdate (0, rDest.Left);
   }
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::ThumbNailBuild (DWORD dwInputLines)
 {
        WORD            *pwEnt;
        BYTE            *pbXlate;
        BYTE            *pbEnt;
        int             iOutLine, iOutIndex, x;

  if (!pThumbNail)
   {
    iThumbLines = pFSG->rScan.dwLinesDone;
    return;
   }
  for (pbXlate = pFSG->pbXlate;
       pThumbNail->iInputLines < (int) dwInputLines;
       pThumbNail->iInputLines++)
   {
    iOutLine = pThumbNail->iInputLines;
    if (pThumbNail->iHolderType == 1)                   // negative
     {
      if      ((iOutLine >=   44) && (iOutLine <  284))         // frame 1
        iOutLine -= 39;
      else if ((iOutLine >=  284) && (iOutLine <  524))         // frame 2
        iOutLine -= 34;
      else if ((iOutLine >=  524) && (iOutLine <  764))         // frame 3
        iOutLine -= 29;
      else if ((iOutLine >=  764) && (iOutLine < 1004))         // frame 4
        iOutLine -= 24;
      else if ((iOutLine >= 1004) && (iOutLine < 1244))         // frame 5
        iOutLine -= 19;
      else if ((iOutLine >= 1244) && (iOutLine < 1484))         // frame 6
        iOutLine -= 14;
      else                                                      // ignore
        continue;
     }
    else                                                // slide
     {
      if      ((iOutLine >=   21) && (iOutLine <  255))         // frame 1
        iOutLine -= 16;
      else if ((iOutLine >=  411) && (iOutLine <  645))         // frame 2
        iOutLine -= 167;
      else if ((iOutLine >=  801) && (iOutLine < 1035))         // frame 3
        iOutLine -= 318;
      else if ((iOutLine >= 1191) && (iOutLine < 1425))         // frame 4
        iOutLine -= 469;
      else                                                      // ignore
        continue;
     }
    pwEnt = (WORD*) pFSG->rScan.pBuf;
    if (pFSG->iMargin == 120)
      pwEnt += (pThumbNail->iInputLines * 600) + 120;     // 600 words/line
    else
      pwEnt += (pThumbNail->iInputLines * 480);           // 480 words/line
    iOutIndex = ++iOutLine * 160;                       // 160 ents/line
    for (x = 0; x < 160; x++)
     {
      pbEnt = (byte*) &pThumbNail->ent [--iOutIndex];
      *pbEnt++ = pbXlate [*pwEnt++];
      *pbEnt++ = pbXlate [*pwEnt++];
      *pbEnt++ = pbXlate [*pwEnt++];
      *pbEnt   = 0;
     }
    pThumbNail->iLines = iOutLine;
   }
  iThumbLines = pThumbNail->iLines + 5;
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::AdjustBoost  (int iChange)
 {
        int             iOldPos, iPos;

  if (BoostBar->Enabled)
   {
    iOldPos = BoostBar->Position;
    iPos = iOldPos + iChange;
    if (iPos < BoostBar->Min)
      iPos = BoostBar->Min;
    if (iPos > BoostBar->Max)
      iPos = BoostBar->Max;
    iChange = iPos - iOldPos;
    if (iChange)
     {
      BoostBar->Position = iPos;
      BoostBar->OnChange (this);
     }
   }
  return;
 }
//---------------------------------------------------------------------------

void    TBaseForm::AdjustScroll (int iChange)
 {
        int             iOldPos, iPos;

  if (ScrollBar->Enabled)
   {
    iOldPos = ScrollBar->Position;
    iPos = iOldPos + iChange;
    if (iPos < 0)
      iPos = 0;
    if (iPos > ScrollBar->Max)
      iPos = ScrollBar->Max;
    iChange = iPos - iOldPos;
    if (iChange)
      PaintBoxScroll (iChange);
   }
  return;
 }
//---------------------------------------------------------------------------

