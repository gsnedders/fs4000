#ifndef __FS4000_H__
#define __FS4000_H__

#include "fs4000-scsi.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
** Make sure structures are packed and undecorated.
*/

#ifdef __BORLANDC__
#pragma option -a1
#endif //__BORLANDC__

#ifdef _MSC_VER
#pragma pack(1)
#endif //__MSC_VER

typedef struct  /* FS4K_FILM_STATUS */
  {
        BYTE    byHolderType;
        BYTE    byNumFrames;
        WORD    wHolderPosition;
        BYTE    unknown1        [3];
                  //    [0]             // bits 7 - 3 = last setFrame
                  //                    // bits 2 - 0 = 0
                  //    [1]             // bits 7 - 0 = 0
                  //    [2]             // bits 7 - 0 = 0
        BYTE    byFocusPosition;
        WORD    wStartPixel;
        WORD    wLimitPixel;
        DWORD   dwDiffsSum      [3];    // sum of squares of diffs ?
        BYTE    unknown2;               // min/max recommended speed ?
        BYTE    byFocusPos      [3];    // best focus point for each colour ?
  }
  FS4K_FILM_STATUS;

typedef struct  /* FS4K_LAMP_INFO */
  {
        BYTE    byWhLampOn;
        DWORD   dwWhLampTime;
        BYTE    byIrLampOn;
        DWORD   dwIrLampTime;
  }
  FS4K_LAMP_INFO;

typedef struct  /* FS4K_SCAN_MODE_INFO */
  {
        BYTE    byLength;               // 0x25
        BYTE    unknown1a       [3];    // nulls
        BYTE    unknown1b       [11];   // 20 20 00 00 00 00 00 00 00 00 00
        BYTE    bySpeed;
        BYTE    unknown2        [4];    // 01 19 00 00
        BYTE    bySampleMods;
        BYTE    unknown2a;              // 00
        BYTE    byAGain         [3];
        WORD    wAOffset        [3];
        WORD    wShutter        [3];
        BYTE    byImageMods;
  }
  FS4K_SCAN_MODE_INFO;

typedef struct  /* FS4K_WINDOW_INFO */
  {
        SCSI_WINDOW_HEADER      header;
        SCSI_WINDOW_DESCRIPTOR  window  [1];
  }
  FS4K_WINDOW_INFO;

typedef struct  /* FS4K_BUF_INFO */
  {
        DWORD           dwHeaderSize;
        LPSTR           pBuf;
        DWORD           dwBufSize;
        DWORD           dwReadBytes;
        DWORD           dwBytesRead;
        UINT4           dwLines;
        UINT4           dwLineBytes;
        UINT4           dwLinesDone;
        BYTE            byBitsPerSample;
        BYTE            bySamplesPerPixel;
        BYTE            byAbortReqd;
        BYTE            byReadStatus;
        BYTE            bySetFrame;
        BYTE            byLeftToRight;
        BYTE            byShift;
        BYTE            bySpare;
        WORD            wLPI;
        WORD            wMin;
        WORD            wMax;
        WORD            wAverage;
  }
  FS4K_BUF_INFO;

typedef struct  /* FS4K_CAL_ENT */
  {
        int             iOffset;
        int             iMult;
  }
  FS4K_CAL_ENT;

typedef struct  /* FS4K_NEWS */
  {
        char            cTask [32];
        char            cStep [64];
        int             iPercent;
  }
  FS4K_NEWS;

typedef struct  /* FS4K_GLOBALS, FS4KG */
  {
        DWORD           dwGlobalsSize;  // for sanity checks

        BOOL            bShowProgress       ;// = TRUE;
        BOOL            bStepping           ;// = FALSE;
        BOOL            bTesting            ;// = FALSE;
        BOOL            bNegMode            ;// = FALSE;
        BOOL            bAutoExp            ;// = FALSE;
        BOOL            bSaveRaw            ;// = FALSE;
        BOOL            bMakeTiff           ;// = TRUE;
        BOOL            bUseHelper          ;// = FALSE;

        BOOL            bDisableShutters    ;// = FALSE;

        FS4K_FILM_STATUS        rFS         ;
        FS4K_LAMP_INFO          rLI         ;
        FS4K_SCAN_MODE_INFO     rSMI        ;
        FS4K_WINDOW_INFO        rWI         ;

        DWORD           dwMaxTransfer       ;// from transport code
        HANDLE          hEvent              ;// from transport code
        int             iAGain    [3]       ;// = {  47,   36,   36};
        int             iAOffset  [3]       ;// = { -25,   -8,   -5};
        int             iShutter  [3]       ;// = { 750,  352,  235};
        int             iInMode             ;// = 14;
        int             iBoost    [3]       ;// = { 256,  256,  256};
        int             iSpeed              ;// = 2;
        int             iFrame              ;// = 0;
        int             ifs4000_debug       ;// = 0;
        int             iMaxShutter         ;// = 890;
        int             iAutoExp            ;// = 2;
        int             iMargin             ;// = 120;

        FS4K_BUF_INFO   rScan               ;
        FS4K_CAL_ENT    rCal     [12120]    ;
        BYTE            b16_822  [65536]    ;
        BYTE            b16_822i [65536]    ;
        BYTE            *pbXlate            ;// = NULL;

        char            szOurPath  [256]    ;
        char            szRawFID   [256]    ;
        char            szScanFID  [256]    ;
        char            szThumbFID [256]    ;

        FS4K_NEWS       rNews;
        void            (*pfnCallback) (FS4K_NEWS*);
  }
  FS4K_GLOBALS, FS4KG;

typedef struct  /* TUNE_STATS */
  {
        DWORD   dwTotal;
        WORD    wMin;
        WORD    wMax;
        WORD    wSamples;
        WORD    wAverage;
  }
  TUNE_STATS;

typedef struct  /* TUNE_INFO */
  {
        int     iEntryLow;
        int     iEntryHigh;
        int     iErrorLow;
        int     iErrorHigh;
        int     iOldEntry;
        WORD    wBase;
        WORD    wWanted;
  }
  TUNE_INFO;

typedef struct  /* AE_BIN */
  {
        int     iFactor;
        int     iMin;
        int     iCount;
  }
  AE_BIN;

/*
** Restore compiler default packing and close off the C declarations.
*/

#ifdef __BORLANDC__
#pragma option -a.
#endif //__BORLANDC__

#ifdef _MSC_VER
#pragma pack()
#endif //_MSC_VER

//
//====          DLL exported functions
//

int     _export fs4k_DumpCalInfo   (LPSTR pFID = "-Fs4000.cal");
int     _export fs4k_LoadCalInfo   (LPSTR pFID = "-Fs4000.cal");
int     _export fs4k_GetHolderType (void);
int     _export FS4_BOJ            (void);
int     _export FS4_EOJ            (void);
int     _export FS4_Tune           (int iTuneSpeed = -1);
int     _export FS4_NegTune        (int iTuneSpeed = -1);
int     _export FS4_Thumbnail      (LPSTR pTifFID);
int     _export FS4_Scan           (LPSTR pTifFID, int iFrame, BOOL bAutoExp);
int     _export FS4_ProcessWord    (LPSTR pArg);
int     _export FS4_Process        (int argc, char **argv);
void    _export FS4_GetHandles     (HANDLE *phStdIn,
                                    HANDLE *phConIn,
                                    HANDLE *phConOut);
void    _export FS4_SetHandles     (HANDLE hStdIn,
                                    HANDLE hConIn,
                                    HANDLE hConOut);
FS4KG*  _export FS4_GlobalsLoc     (void);
FS4KG*  _export FS4_SetCallback    (void (*pfnCallback) (FS4K_NEWS*));
void    _export FS4_SetAbort       (void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__FS4000_H__
