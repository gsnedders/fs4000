/*--------------------------------------------------------------

                        FsTiff.exe

        Build TIFF file from raw scan file from Fs4000.exe.

  Copyright (C) 2004  Steven Saunderson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston,
  MA 02111-1307, USA.

                StevenSaunderson at tpg.com.au

        2004-12-03      Original version.

--------------------------------------------------------------*/

#include <windows.h>
#include <math.h>

#define desc(x)         &x, sizeof (x)

        //====  Global fields

        HANDLE          hStdInput      = GetStdHandle (STD_INPUT_HANDLE);
        HANDLE          hConsoleInput  = CreateFile   ("CONIN$",
                                                       GENERIC_READ,
                                                       0,
                                                       NULL,
                                                       OPEN_EXISTING,
                                                       0,
                                                       NULL);
        HANDLE          hConsoleOutput = GetStdHandle (STD_OUTPUT_HANDLE);

        DWORD           ProgFlags = 0;
#define                   PF_BAD_SPECS          0x08000         //
        int             ReturnCode = 0;
#define                   RC_GOOD               0
#define                   RC_KBD_ABORT          255

        char            msg [256];


/*--------------------------------------------------------------

        setmem is now a macro and doesn't work with desc

--------------------------------------------------------------*/

void    SetMem                  (void *fld, int size, int value)
  {
  memset (fld, value, size);
  return;
  }


/*--------------------------------------------------------------

                Null fill area

--------------------------------------------------------------*/

void    NullMem                 (void *fld, int size)
  {
  memset (fld, 0x00, size);
  return;
  }


/*--------------------------------------------------------------

                Write text to screen

        We use WriteFile rather than WriteConsole in case
        the output is redirected to a file (WriteConsole
        fails if the handle is other than console output).

--------------------------------------------------------------*/

void    spout                   (char *msg = ::msg)
  {
        DWORD           chars = strlen (msg);

  if (!hConsoleOutput)
    return;
//WriteConsole (hConsoleOutput, msg, chars, &chars, NULL);
  WriteFile    (hConsoleOutput, msg, chars, &chars, NULL);
//dwLastSpout = GetTickCount ();
  return;
  }


/*--------------------------------------------------------------

                Get keystroke if any

--------------------------------------------------------------*/

int     GetAnyKeystroke         (void)
  {
        DWORD           dwRecs;
        INPUT_RECORD    buf;

  while (hConsoleInput)
    {
    GetNumberOfConsoleInputEvents (hConsoleInput, &dwRecs);
    if (dwRecs == 0)
      break;
    if (ReadConsoleInput (hConsoleInput, &buf, 1, &dwRecs) == 0)
      continue;
    if (buf.EventType != KEY_EVENT)
      continue;
    if (!buf.Event.KeyEvent.bKeyDown)
      continue;
    return (buf.Event.KeyEvent.uChar.AsciiChar);
    }
  return 0;
  }


/*--------------------------------------------------------------

                Check for abort request

--------------------------------------------------------------*/

BOOL    AbortWanted             (void)
  {
  if (GetAnyKeystroke () == 0x1B)
    ReturnCode = RC_KBD_ABORT;
  return (ReturnCode == RC_KBD_ABORT);
  }


/*--------------------------------------------------------------

                Get a keystroke

--------------------------------------------------------------*/

int     GetKeystroke            (void)
  {
        int             key;

  while (1)
    {
    WaitForSingleObject (hConsoleInput, INFINITE);
    if (NULL != (key = GetAnyKeystroke ()))
      return key;
    }
  }


/*--------------------------------------------------------------

                Debug display

--------------------------------------------------------------*/

int     ShowAndWait             (char *msg)
  {
        int             key;

  spout (msg);
  key = GetKeystroke ();
  spout ("\n");
  return key;
  }


/*--------------------------------------------------------------

                Find next spare filename

--------------------------------------------------------------*/

int     GetNextSpareName        (char *cAnswer, char *cTemplate)
  {
        char            *pcIn, *pcSuffix, cFormat [16];
        int             iPrefixSize, iNumDigits, iNextValue;

  strcpy (cAnswer, cTemplate);
  pcSuffix = strchr (cTemplate, '+');                   // find '+'
  if (pcSuffix)
    {
    for (iNumDigits = 0, pcIn = pcSuffix; pcIn > cTemplate; iNumDigits++)
      {
      if (*--pcIn >= '0')                               // find start of
        if (*pcIn <= '9')                               // preceding digits
          continue;
      pcIn++;
      break;
      }
    iPrefixSize = pcIn - cTemplate;                     // get prefix size
    if (iNumDigits > 0)
      {
      iNextValue = atoi (pcIn);
      pcSuffix++;
      wsprintf (cFormat, "%%0%dd%%s", iNumDigits);      // build format
      for (;; iNextValue++)
        {
        wsprintf (&cAnswer [iPrefixSize],
                  cFormat,
                  iNextValue, pcSuffix);
        if (GetFileAttributes (cAnswer) == 0xFFFFFFFF)
          return 0;
        }
      }
    }
  return -1;
  }


//--------------------------------------------------------------

#include "tiffio.h"             // for TIFF library routines

//--------------------------------------------------------------

/*--------------------------------------------------------------

                FS4k (application specific code)

--------------------------------------------------------------*/

#include "Fs4000.h"             // structures

        BOOL            bShowProgress = TRUE;

        FS4K_BUF_INFO   rScan = {sizeof (FS4K_BUF_INFO), NULL, 0};

        BYTE            b16_822 [65536], b16_822i [65536], *pbXlate = NULL;
        BYTE            bXlateR [65536], bXlateG  [65536], bXlateB  [65536];


/*--------------------------------------------------------------

                Load gamma array for 16-bit to 8-bit conversion

        To reduce the effects of noise in low-level samples, the
        slope can be limited by specifying the min value that is
        not slope limited.  For a Dmax of 3.0 this value would be
        65536 / (10 ^ 3) for instance.

        When loading the table the code finds the first step
        after this minimum value, gets the previous step width,
        and then creates steps of this width back to the start of
        the table.  When the distance back to the start is not a
        multiple of the step width we shift the entries to fix
        this.  This shift is the XOffset.  Also, we have Y
        offsets (YOldBase and YNewBase) to compensate for the
        change in number of steps to this point.  This gets the
        values at the low end of the table correct.  Finally, we
        have a Y multiplication factor (dYMult) to negate the
        effect of the Y shift at the high end of the table.

--------------------------------------------------------------*/

int     fs4k_LoadGammaArray     (double dGamma, int iMin)
  {
        int             x, y, z;
        int             iSteps, iStepStart, iStepSize, iLinearEnd;
        int             iXOffset, iYOldBase, iYNewBase;
        double          dPower, dYMult, dIn, dEnt;

//iMin = 65536 / 2048;  // linear slope at low end (for very good scanner)
//iMin = 65536 / 64;    // linear slope at low end (ISO ??? standard)
//iMin = 65536 / 1024;  // linear slope at low end (is the FS this good ?)
//iMin = 0;             // no linear slope

  dPower      = 1.0 / dGamma;
  b16_822 [0] = 0;                              // always 0
  iXOffset    = 0;                              // reset adjustments
  iYOldBase   = 0;
  iYNewBase   = 0;
  iStepStart  = 0;
  iSteps      = 0;
  dYMult      = 1.0;
  iStepSize   = 0;                              // 0 = need slope mods
  for (x = 1; x < 65536; x++)
    {
    dIn = x;
    dIn += iXOffset;
    dEnt = pow ((dIn / 65536), dPower) * 256;
    dEnt -= iYOldBase;
    dEnt *= dYMult;
    dEnt += iYNewBase;
    y = dEnt;
    b16_822 [x] = (BYTE) y;
    if (iStepSize > 0)                          // back if mods calc'd
      continue;
    if (y == b16_822 [x - 1])                   // back if same level
      continue;
#if 0
    wsprintf (msg, "Step at %d\n", x);
    spout ();
#endif
    if (x <= iMin)                              // if step before min X
      iStepStart = x;                           //   remember
    else                                        // ready to calc mods.
      {
      iStepSize  = x - iStepStart;
      while (iStepStart > 0)                    // calc number of linear steps
        {
        iSteps++;
        iStepStart -= iStepSize;
        }
      iXOffset   = iStepStart;                  // to get equal sized steps
      iYOldBase  = y - 1;                       // Y base adjust ents
      iYNewBase  = iSteps;
      dYMult = (256.0 - iYNewBase) / (256.0 - iYOldBase);  // Y adjust factor
      iLinearEnd = iSteps * iStepSize;          // linear range size
      for (x = 0; x < iLinearEnd; x++)          // load linear range ents
        b16_822 [x] = x / iStepSize;
      x--;                                      // start point for calcs
#if 0
      wsprintf (msg,
        "Min = %d, Steps = %d, Size = %d, XOffset = %d, YOldBase = %d\n",
         iMin,     iSteps,     iStepSize, iXOffset,     iYOldBase);
      spout ();
#endif
      }
    }

//----  The slope mods reduce the X range available for the following
//----  entries.  This increases the slope of this following portion.
//----  Here we check entries from 0 upwards to make sure we haven't
//----  got a short step (slope too steep).  Once we hit a step that
//----  is longer than the minimum we can exit this test.

  for (x = y = 0; x < 65536; y++)
    {
    for (z = iStepSize; z--; x++)       // repeat value for step width
      b16_822 [x] = y;
    if (b16_822 [x] == y)               // break when step wider than reqd
      break;
    }

//----  Load invert table for negatives.

  for (x = 0; x < 65536; x++)
    b16_822i [x] = 255 - b16_822 [x];

//----  Debug dump to list all steps in table.

#if 0                                   // list all steps
  for (x = y = z = 0; y < 256; x++)
    {
    if (x < 65536)
      if (b16_822 [x] == y)
        continue;
    wsprintf (msg, "%3d : %5d - %5d (%d)\n", y, z, x - 1, x - z);
    spout ();
    z = x;
    y++;
    }
#endif
  return 0;
  }


/*--------------------------------------------------------------

                Tasks at start-up

--------------------------------------------------------------*/

void    fs4k_boj                (void)
  {
        int             x;

  fs4k_LoadGammaArray   (2.2, 64);

  return;
  }


/*--------------------------------------------------------------

                Release memory buffer

--------------------------------------------------------------*/

void    fs4k_FreeBuf            (FS4K_BUF_INFO  *pBI)
  {
  if (pBI->pBuf)
    VirtualFree (pBI->pBuf, 0, MEM_RELEASE);
  pBI->pBuf = NULL;
  pBI->dwBufSize = 0;
  return;
  }


/*--------------------------------------------------------------

                Get memory buffer

--------------------------------------------------------------*/

int     fs4k_AllocBuf           (FS4K_BUF_INFO  *pBI,
                                 BYTE           bySamplesPerPixel = 3,
                                 BYTE           byBitsPerSample = 14)
  {
  fs4k_FreeBuf (pBI);
  pBI->dwHeaderSize = sizeof (FS4K_BUF_INFO);
  pBI->dwBufSize = pBI->dwLineBytes * pBI->dwLines;
  pBI->pBuf = (LPSTR) VirtualAlloc (NULL, pBI->dwBufSize,
                                    MEM_COMMIT, PAGE_READWRITE);
  pBI->bySamplesPerPixel = bySamplesPerPixel;
  pBI->byBitsPerSample   = byBitsPerSample;
  if (pBI->byBitsPerSample == 14)
    pBI->byBitsPerSample   =  16;
  pBI->wLPI = 0;
  pBI->byShift = 0;
  return (!pBI->pBuf);
  }


/*--------------------------------------------------------------

                Copy default table to table for each colour

--------------------------------------------------------------*/

void    fs4k_CopyXlateTable     (void)
  {
  if (pbXlate)
    {
    memcpy (bXlateR, pbXlate, 65536);
    memcpy (bXlateG, pbXlate, 65536);
    memcpy (bXlateB, pbXlate, 65536);
    }
  return;
  }


/*--------------------------------------------------------------

                Load raw file into memory buffer

--------------------------------------------------------------*/

int     fs4k_LoadRaw            (LPSTR pFID, FS4K_BUF_INFO *pBI)
  {
        HANDLE          hFile;
        DWORD           dwBytesToRead, dwBytesRead;

  hFile = CreateFile (pFID,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    {
    spout ("Raw file read failed !!!\n");
    return -1;
    }
  fs4k_FreeBuf (pBI);

  dwBytesToRead = sizeof (FS4K_BUF_INFO);
  ReadFile (hFile, pBI,       dwBytesToRead,  &dwBytesRead, NULL);
  if ((dwBytesRead != sizeof (FS4K_BUF_INFO))   ||
      (dwBytesRead != pBI->dwHeaderSize)        )
    {
    pBI->pBuf = NULL;
    CloseHandle (hFile);
    spout ("Raw file header read failed !!!\n");
    return -1;
    }

  dwBytesRead +=  511;
  dwBytesRead &= -512;
  SetFilePointer (hFile, dwBytesRead, NULL, FILE_BEGIN);

  pBI->pBuf = (LPSTR) VirtualAlloc (NULL, pBI->dwBufSize,
                                    MEM_COMMIT, PAGE_READWRITE);
  ReadFile (hFile, pBI->pBuf, pBI->dwBufSize, &dwBytesRead, NULL);
  CloseHandle (hFile);
  return 0;
  }


/*--------------------------------------------------------------

                Calc gamma table entry

--------------------------------------------------------------*/

double  fs4k_GammaEnt           (int iNum, int iDiv, double dGamma)
  {
        double          dNum, dDiv, dAns;

  dNum = iNum;
  dDiv = iDiv;
  dAns = (256 * pow (dNum / dDiv, dGamma));
  if (dAns > 255)
    dAns = 255;
  return dAns;
  }


/*--------------------------------------------------------------

                Load a gamma table

--------------------------------------------------------------*/

void    fs4k_LoadNegXlate       (LPSTR pbMin, LPSTR pbMax, double dGamma)
  {
        int             iRange, iBigRange, x;
        double          dMult, dAns;
        LPSTR           pEnt;

  iRange = pbMax - pbMin;
//iBigRange = iRange * 6 / 5;
//dMult = 256.0 / fs4k_GammaEnt (iRange, iBigRange, dGamma);
  for (pEnt = pbMax, x = 0; x < iRange; x++)
    {
//  dAns = (dMult * fs4k_GammaEnt (x, iBigRange, dGamma));
    dAns = fs4k_GammaEnt (x, iRange, dGamma);
    if (dAns > 255)
      dAns = 255;
    *--pEnt = (byte) dAns;
    }
  return;
  }


/*--------------------------------------------------------------

                Prepare to process negative scan

--------------------------------------------------------------*/

int     fs4k_PrepForNeg         (FS4K_BUF_INFO *pBI)
  {
        int             x, y, z, iXStart, iXLimit, iYStart, iYLimit;
        WORD            *pwSample;
        int             iSample, iSamples, iReqd, iCount [256];
        int             iMin, iMax, iAdd, iDiv;

  if (pBI->dwLineBytes != 24240)
    return -1;

  iXStart = 120;                                // calc cropped X range
  iXLimit = pBI->dwLineBytes / 2;
  x       = (iXLimit - iXStart) / 20;
  iXStart += x;
  iXLimit -= x;

  iYStart = 0;                                  // calc cropped Y range
  iYLimit = pBI->dwLines;
  y       = (iYLimit - iYStart) / 20;
  iYStart += y;
  iYLimit -= y;

  for (x = 0; x < 256; x++)                     // initialise totals
    iCount [x] = 0;

  iSamples = 0;
  for (y = iYStart; y < iYLimit; y++)
    {
    pwSample = (WORD*) &(pBI->pBuf [y * pBI->dwLineBytes]);
    pwSample += iXStart;
    for (x = iXStart; x < iXLimit; x++)
      {
      iSample = *pwSample++;                    // get sample
      z = b16_822 [iSample];
      iCount [z] ++;
      iSamples++;
      }
    }

  iReqd = iSamples / 256;
  for (iMin = 0; iMin < 120; iMin++)
    if (0 >= (iReqd -= iCount [iMin]))
      break;
  for (x = 0; x < 32768; x++)
    if (b16_822 [x] >= iMin)
      break;
  iMin = x;

  iReqd = iSamples / 256;
  for (iMax = 255; iMax > 120; iMax--)
    if (0 >= (iReqd -= iCount [iMax]))
      break;
  for (x = 65535; x > 32768; x--)
    if (b16_822 [x] < iMax)
      break;
  iMax = x;

#if 1
  wsprintf (msg, "iMin = %d, iMax = %d\n", iMin, iMax);
  spout ();
#endif

  for (x = 0; x < iMin; x++)
    bXlateR [x] = 255;
  for (x = iMax; x < 65536; x++)
    bXlateR [x] = 0;

  for (x = iMin; x < iMax; x++)
    bXlateR [x] = 255 - ((255 * (x - iMin)) / (iMax - iMin));

  memcpy (bXlateG, bXlateR, 65536);
  memcpy (bXlateB, bXlateR, 65536);

  fs4k_LoadNegXlate (&bXlateR [iMin], &bXlateR [iMax], 2.0);
  fs4k_LoadNegXlate (&bXlateG [iMin], &bXlateR [iMax], 2.2);
  fs4k_LoadNegXlate (&bXlateB [iMin], &bXlateR [iMax], 2.4);

  return 0;
  }


/*--------------------------------------------------------------

                Write scan data to file

--------------------------------------------------------------*/

int     fs4k_SaveScan           (LPSTR          pFID,
                                 FS4K_BUF_INFO  *pBI)
  {
        int             iShift, iInBytesPerPixel;
        int             iInCols, iInRows, iInInc;
        int             iInRow,  iInCol,  iInEnt;
        int             iOutRow, iOutCol, iOutEnt;
        int             iOutBitsPerSample;
        BYTE            *pbSample, bLine [36000];
        WORD            *wLine = (WORD*) bLine;
        WORD            *pwSample;
        TIFF            *tif;

  iShift = 0;
  if (pBI->byShift == 0)                        // 0 = shift not done
    {
//  if (pBI->wLPI ==  160) iShift = 0;
    if (pBI->wLPI ==  500) iShift = 1;
    if (pBI->wLPI == 1000) iShift = 2;
    if (pBI->wLPI == 2000) iShift = 4;
    if (pBI->wLPI == 4000) iShift = 8;
    pBI->byShift = iShift;
    }
  iInBytesPerPixel = ((pBI->byBitsPerSample + 7) / 8) * pBI->bySamplesPerPixel;
  iInCols = pBI->dwLineBytes / iInBytesPerPixel;
  iInRows = pBI->dwLines - (2 * iShift);
  iInInc  = 1 + (3 * iShift * iInCols);
  if (pBI->byLeftToRight)
    iInInc = 2 - iInInc;
  iOutBitsPerSample = (pBI->byBitsPerSample + 7) & 0xF8;
  if (pbXlate)
    iOutBitsPerSample = 8;                        // forces 8 bit output

  wsprintf (msg, "LPI = %d, cols = %d, rows = %d, shift = %d, inc = %d\n",
                 pBI->wLPI, iInCols,   iInRows,   iShift,     iInInc);
  spout ();

  tif = TIFFOpen (pFID, "w");

  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, (uint16) iOutBitsPerSample);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL,(uint16) 3);
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, (uint32) iInRows);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, (uint32) iInCols - 40);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, (uint16) PLANARCONFIG_CONTIG );
  TIFFSetField (tif, TIFFTAG_COMPRESSION, (uint16) COMPRESSION_NONE );
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, (uint16) PHOTOMETRIC_RGB );
  TIFFSetField (tif, TIFFTAG_XRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_YRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, (uint16) 1);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (uint32) 1);

  pbSample = (BYTE*) pBI->pBuf;
  pwSample = (WORD*) pBI->pBuf;
  for (iOutRow = 0; iOutRow < iInCols - 40; iOutRow++)
    {
    if ((bShowProgress) && ((iOutRow % 100) == 0))
      {
      wsprintf (msg, "line = %4d\r", iOutRow);
      spout ();
      }
    iInRow = (2 * iShift);
    if (!pBI->byLeftToRight)
      iInRow = iInRows;
    iInCol = iInCols - 1 - iOutRow;
    for (iOutCol = iOutEnt = 0; iOutCol < iInRows; iOutCol++)
      {
      if (!pBI->byLeftToRight)
        iInRow--;
      iInEnt = ((iInRow * iInCols) + iInCol) * 3;
      if (pBI->byBitsPerSample > 8)                     // 16-bit input
        {
        if (iOutBitsPerSample <= 8)                     // 8-bit output
          {
#if 1
          bLine [iOutEnt++] = bXlateR [pwSample [iInEnt]];
          iInEnt += iInInc;
          bLine [iOutEnt++] = bXlateG [pwSample [iInEnt]];
          iInEnt += iInInc;
          bLine [iOutEnt++] = bXlateB [pwSample [iInEnt]];
#else           // stupid attempt at colour correction
        int             iValue [3], iOut [3], iLimit = 65535;

          iValue [0] = pwSample [iInEnt];
          iInEnt += iInInc;
          iValue [1] = pwSample [iInEnt];
          iInEnt += iInInc;
          iValue [2] = pwSample [iInEnt];
          iOut [0] = (iValue [0] * 18 / 10)
                   - (iValue [1] *  7 / 10)
                   - (iValue [2] *  1 / 10);
          if (iOut [0] < 0)
            iOut [0]   = 0;
          if (iOut [0] > iLimit)
            iOut [0]   = iLimit;
          iOut [1] = (iValue [1] * 18 / 10)
                   - (iValue [0] *  3 / 10)
                   - (iValue [2] *  5 / 10);
          if (iOut [1] < 0)
            iOut [1]   = 0;
          if (iOut [1] > iLimit)
            iOut [1]   = iLimit;
          iOut [2] = (iValue [2] * 21 / 10)
                   - (iValue [1] *  8 / 10)
                   - (iValue [0] *  3 / 10);
          if (iOut [2] < 0)
            iOut [2]   = 0;
          if (iOut [2] > iLimit)
            iOut [2]   = iLimit;
          bLine [iOutEnt++] = bXlateR [iOut [0]];
          bLine [iOutEnt++] = bXlateG [iOut [1]];
          bLine [iOutEnt++] = bXlateB [iOut [2]];
#endif
          }
        else                                            // 16-bit output
          {
          wLine [iOutEnt++] = pwSample [iInEnt];
          iInEnt += iInInc;
          wLine [iOutEnt++] = pwSample [iInEnt];
          iInEnt += iInInc;
          wLine [iOutEnt++] = pwSample [iInEnt];
          }
        }
      else                                              // 8-bit input
        {
        bLine [iOutEnt++] = pbSample [iInEnt];
        iInEnt += iInInc;
        bLine [iOutEnt++] = pbSample [iInEnt];
        iInEnt += iInInc;
        bLine [iOutEnt++] = pbSample [iInEnt];
        }
      if (pBI->byLeftToRight)
        iInRow++;
      }
    TIFFWriteScanline (tif, bLine, iOutRow, 0);
    if (AbortWanted ())
      break;
    }
  TIFFClose (tif);
  wsprintf (msg, "File %s created\n", pFID);
  spout ();
  fs4k_FreeBuf (pBI);
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Start-up code.

--------------------------------------------------------------*/

int     FS4_BOJ                 (void)
  {
        int             erc;

  fs4k_boj ();

  return 0;
  }


/*--------------------------------------------------------------

                Close-down code.

--------------------------------------------------------------*/

int     FS4_EOJ                 (void)
  {
  return 0;
  }


/*--------------------------------------------------------------

                Code to get and process arguments

--------------------------------------------------------------*/

LPSTR   FS4_GetWord             (int argc, char **argv, HANDLE *phInput)
  {
        static int      iArgX = 1;
        static char     cWord [256];
        char            *pChar;
        DWORD           dwCount;

  pChar = cWord;
  while (*phInput)
    {
    if ((ReadFile (*phInput, pChar, 1, &dwCount, NULL) == 0) ||
        (dwCount != 1))
      {
      CloseHandle (*phInput);
      *phInput = NULL;
      *pChar = NULL;
      }
    if ((*pChar < '!') || (*pChar > '~'))
      if (pChar == cWord)
        continue;
      else
        {
        *pChar = NULL;
        pChar = cWord;
        goto got_ent;
        }
    if (pChar < &cWord [255])
      pChar++;
    }

  pChar = NULL;
  if (iArgX < argc)
    pChar = argv [iArgX++];

got_ent:
//if (pChar) { spout (pChar); spout ("\n"); }
  return pChar;
  }


int     FS4_Process             (int argc, char **argv)
  {
        DWORD           dwRecs;
        char            cTifFID [256], cRawFID [256];
        HANDLE          hFile    = NULL,
                        hPipeIn  = NULL,
                        hPipeOut = NULL;
        char            *pArg;
        DWORD           dwBytes;
        BOOL            bPurge;

//      if no commands, and re-directed input, use StdIn

  if (argc == 1)
    if (GetNumberOfConsoleInputEvents (hStdInput, &dwRecs) == 0)
      hFile = hStdInput;

  cTifFID [0] = 0;
  cRawFID [0] = 0;
  bPurge      = FALSE;
  pbXlate     = NULL;
  while (!AbortWanted ())
    {
    if (hPipeOut)
      WriteFile (hPipeOut, pArg, strlen (pArg), &dwBytes, NULL);

    if (hFile)
      pArg = FS4_GetWord (argc, argv, &hFile);
    else
      pArg = FS4_GetWord (argc, argv, &hPipeIn);

    if (!pArg)
      pArg = ";";

//----          IPC functions

    if (memicmp (pArg, "pipe=", 5) == 0)
      {
      if (hPipeIn)
        CloseHandle (hPipeIn);
      if (hPipeOut)
        CloseHandle (hPipeOut);
      hPipeIn  = (HANDLE) atoi (&pArg [5]);
      hPipeOut = (HANDLE) atoi (strchr (&pArg [5], ',') + 1);
      }

//----          File names and delimiter

    if (*pArg == ';')                           // delimiter
      {
      if (cRawFID [0] == 0)
        break;
      if (fs4k_LoadRaw (cRawFID, &rScan))
        return 2;
      fs4k_CopyXlateTable ();
//    if (pbXlate == b16_822i)
//      fs4k_PrepForNeg (&rScan);
      fs4k_SaveScan (cTifFID, &rScan);
      if (bPurge)
        DeleteFile (cRawFID);
      cTifFID [0] = 0;
      cRawFID [0] = 0;
      bPurge      = FALSE;
      pbXlate     = NULL;
      continue;
      }

    if (cTifFID [0] == 0)                       // output name first
      {
      GetNextSpareName (cTifFID, pArg);
      continue;
      }

    if (cRawFID [0] == 0)                       // input name second
      {
      strcpy (cRawFID, pArg);
      continue;
      }

//----          Modifiers come after file names and before ';'

    if (strcmpi (pArg, "out16") == 0)
      pbXlate = NULL;
    if (strcmpi (pArg, "out8") == 0)
      pbXlate = b16_822;
    if (strcmpi (pArg, "out8i") == 0)
      pbXlate = b16_822i;
    if (strcmpi (pArg, "purge") == 0)
      bPurge = TRUE;
    }

  return 0;
  }


/*--------------------------------------------------------------

                Get specs, do work

--------------------------------------------------------------*/

int main                        (int argc, char **argv)
  {
        int             erc;

  if (0 < (erc = FS4_BOJ ()))
    goto bye;

  erc = FS4_Process (argc, argv);

bye:
  FS4_EOJ ();
  return erc;
  }


