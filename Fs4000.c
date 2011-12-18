/*--------------------------------------------------------------

                FS4000 SCAN UTILITY

        Software to drive Canon FS4000US scanner

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

                Steven Saunderson (steven at phelum.net)

        2004-12-03      Original version.

        2004-12-08      Check for USB FS4000 before ASPI.

        2005-01-26      Always use speed 2 for scan auto-exposure pass.

        2005-02-20      Max shutter 890 (was 1000).
                        Changed black position (old cols seem wrong).
                        Tried various approaches to black reads when
                        tuning.  Best is black col for holder, normal
                        exposure, min shutter, leave light on.

        2005-03-08      Added iMargin to globals.  Normally 120.  Setting
                        this to 0 stops the FS4000 sending the 40 col margin
                        before each line.  However, this wrecks the black
                        level readings so it seems the relevant scan mode data
                        bit has other effects.

        2005-03-12      Add standard deviation calcs to TuneRpt.
                        Emit CRLF at line end rather than just LF.
                        Calc digital offsets before each scan.

        2005-03-17      NegTune calcs changed to avoid Boost != 256.
                        TuneRpt StdDev code now uses values not errors.
                        Lamp on/off routines changed.

        2005-04-07      Get current path at BOJ.

        2005-06-09      Change gamma table load routine.
                        Fix standard deviation calcs.

        2007-05-18      Don't get digital offsets at each scan.

--------------------------------------------------------------*/

#define STRICT          1

#include "scsi_via_usb.h"       // USB scanner using various

#include <windows.h>
#include <math.h>
#include <strings.h>

#include "scsidefs.h"           // SCSI defines
//#include "wnaspi32.h"           // ASPI headers
//#include "scsi_via_aspi.h"      // SCSI scanner using ASPI
#include "fs4000-scsi.h"        // FS4000 I/O code
#include <tiffio.h>             // for TIFF library routines


#define desc(x)         &x, sizeof (x)

//  Calculating the digital offsets before each scan should reduce
//  dark level noise due to drifts.  But it doesn't seem to help and
//  I don't like doing it because turning the light on and off will
//  probably reduce its life.

#define GET_DIG_OFFS_AT_SCAN    0

        //====  Global fields

//        HANDLE          hStdInput      = GetStdHandle (STD_INPUT_HANDLE);
//        HANDLE          hConsoleInput  = CreateFile   ("CONIN$",
//                                                       GENERIC_READ,
//                                                       0,
//                                                       NULL,
//                                                       OPEN_EXISTING,
//                                                       0,
//                                                       NULL);
//        HANDLE          hConsoleOutput = GetStdHandle (STD_OUTPUT_HANDLE);

        BOOL            bIsExe = FALSE;
        DWORD           ProgFlags = 0;
#define                   PF_BAD_SPECS          0x08000
        int             ReturnCode = 0;
#define                   RC_GOOD               0
#define                   RC_SW_ABORT           254
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
#if 0
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
#endif

void spout() {}
/*--------------------------------------------------------------

                Get keystroke if any

--------------------------------------------------------------*/
#if 0
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
  return (ReturnCode != RC_GOOD);
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
  spout ("\r\n");
  return key;
  }


/*--------------------------------------------------------------

                Misc stuff

--------------------------------------------------------------*/

void    PrintBytes              (void *pEnt, int iSize)
  {
        BYTE    *pbEnt;
        int     x;

  for (pbEnt = (BYTE*) pEnt, x = 0; iSize--; pbEnt++, x++)
    {
    if (x == 16)
      {
      spout ("\r\n");
      x = 0;
      }
    wsprintf (msg, "%02X ", *pbEnt);
    spout ();
    }
  spout ("\r\n");
  return;
  }
#endif
/*------------------------------------------------------------*/
#if 0
WORD    SwapEndian2             (WORD wValue)
  {
  asm   mov     ax, wValue;
  asm   xchg    ah, al;
  asm   mov     wValue, ax;
  return wValue;
  }

/*------------------------------------------------------------*/

void    SwapEndian2             (void *pwEnt)
  {
//asm   mov     ecx, pwEnt;
//asm   mov     ax, [ecx]
//asm   xchg    ah, al;
//asm   mov     [ecx], ax;
  asm   mov     eax, pwEnt;
  asm   ror     word ptr [eax], 8;
  return;
  }

/*------------------------------------------------------------*/

DWORD   SwapEndian4             (DWORD dwValue)
  {
  asm   mov     eax, dwValue;
  asm   bswap   eax;
  asm   mov     dwValue, eax;
  return dwValue;
  }

/*------------------------------------------------------------*/

void    SwapEndian4             (void *pdwEnt)
  {
  asm   mov     ecx, pdwEnt;
  asm   mov     eax, [ecx];
  asm   bswap   eax;
  asm   mov     [ecx], eax;
  return;
  }
#endif
/*------------------------------------------------------------*/

int     RoundedDiv              (int iNum, int iDiv)
  {
  iNum += iDiv / 2;
  iNum /= iDiv;
  return iNum;
  }

/*------------------------------------------------------------*/

int     SumOfWords              (WORD *pNext, int iInc, int iCount)
  {
        int             iSum;

  for (iSum = 0; iCount--; pNext += iInc)
    iSum += *pNext;
  return iSum;
  }

/*------------------------------------------------------------*/

int     AvgOfWords              (WORD *pNext, int iInc, int iCount)
  {
  if (!iCount)
    return 0;
  return (RoundedDiv (SumOfWords (pNext, iInc, iCount), iCount));
  }


/*------------------------------------------------------------*/

int     MinOfWords              (WORD *pNext, int iInc, int iCount)
  {
        int             iMin;

  for (iMin = 65535; iCount--; pNext += iInc)
    if (iMin > *pNext)
      iMin   = *pNext;
  return iMin;
  }


/*------------------------------------------------------------*/

int     MaxOfWords              (WORD *pNext, int iInc, int iCount)
  {
        int             iMax;

  for (iMax = 0; iCount--; pNext += iInc)
    if (iMax < *pNext)
      iMax   = *pNext;
  return iMax;
  }


/*------------------------------------------------------------*/

//----  Average of values after dropping min and max entries

int     NormOfWords             (WORD *pNext, int iInc, int iCount)
  {
  if (iCount < 5)
    return AvgOfWords (pNext, iInc, iCount);
  return RoundedDiv (  SumOfWords (pNext, iInc, iCount)
                     - MinOfWords (pNext, iInc, iCount)
                     - MaxOfWords (pNext, iInc, iCount),
                     iCount - 2);
  }


/*------------------------------------------------------------*/

int     SumOfWordDiffs          (WORD *pNext, int iInc, int iCount, int iSub)
  {
        int             iSum;
        int             iLast = *pNext;

  for (iSum = 0; iCount--; iLast = *pNext, pNext += iInc)
    iSum += max (abs (*pNext - iLast) -  iSub, 0);
  return iSum;
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


/*------------------------------------------------------------*/
//
//      Headers for transport and fs4000 I/O code.


/*------------------------------------------------------------*/

/*--------------------------------------------------------------

                FS4k (application specific code)

--------------------------------------------------------------*/

        int             (*pfnTransportClose) (void) = NULL;

//extern "C" BOOL FS4_Halt        (void);

#include "Fs4000.h"             // structures

        FS4K_GLOBALS    g;      // all our globals

/*--------------------------------------------------------------

                Status advise routines

        We advise Task, Step, Percent (-1 - 102)
        for Percent, < 0 = no info, 0 - 100 = progress, > 100 = done

--------------------------------------------------------------*/

int     fs4k_NewsPercent        (int iPercent)
  {
  if (iPercent == g.rNews.iPercent)
    return 0;

  g.rNews.iPercent = iPercent;
  if (g.pfnCallback)
    g.pfnCallback (&g.rNews);

  return 0;
  }


int     fs4k_NewsStep           (LPSTR pStep)
  {
  strcpy (g.rNews.cStep, pStep);
  g.rNews.iPercent = -2;
  return fs4k_NewsPercent (-1);
  }


int     fs4k_NewsTask           (LPSTR pTask)
  {
  strcpy (g.rNews.cTask, pTask);
  return fs4k_NewsStep ("");
  }


int     fs4k_NewsDone           (void)
  {
  if (g.rNews.iPercent < 0)
    return fs4k_NewsPercent (102);
  return fs4k_NewsPercent (101);
  }


/*--------------------------------------------------------------

                Reset calibration arry

--------------------------------------------------------------*/

void    fs4k_InitCalArray       (void)
  {
        int             x;

  for (x = 0; x < 12120; x++)
    {
    g.rCal [x].iOffset = 0;
    g.rCal [x].iMult   = 16384;
    }
  return;
  }


/*--------------------------------------------------------------

                Dump calibration info to disk

--------------------------------------------------------------*/

int     fs4k_DumpCalInfo        (LPSTR pFID /* = "-Fs4000.cal"*/)
  {
        FILE            *pFile;
        int             x;

  if (NULL == (pFile = fopen (pFID, "wb")))
    {
    wsprintf (msg, "%s  open failed\r\n", pFID);
    spout ();
    return -1;
    }
  fprintf (pFile, "        Calibration details\r\n");
  fprintf (pFile, "    Format is critical; do not change\r\n");
  fprintf (pFile, "Shutter %4d %4d %4d\r\n",
                  g.iShutter [0], g.iShutter [1], g.iShutter [2]);
  fprintf (pFile, "Gain    %4d %4d %4d\r\n",
                  g.iAGain   [0], g.iAGain   [1], g.iAGain   [2]);
  fprintf (pFile, "Offset  %4d %4d %4d\r\n",
                  g.iAOffset [0], g.iAOffset [1], g.iAOffset [2]);
  fprintf (pFile, "Boost   %4d %4d %4d\r\n",
                  g.iBoost   [0], g.iBoost   [1], g.iBoost   [2]);
  fprintf (pFile, "Speed   %4d\r\n", g.iSpeed);
  fprintf (pFile, "NegMode %s\r\n",  g.bNegMode ? "TRUE" : "FALSE");
  fprintf (pFile, "        Pixels numbered from bottom to top\r\n");
  fprintf (pFile, "                   Offset         Multiplier\r\n");
  fprintf (pFile, "               R     G     B     R     G     B\r\n");
  for (x = g.iMargin; x < g.iMargin + 12000; x += 3)
    {
    fprintf (pFile, "Pixel %4d, %5d %5d %5d  %5d %5d %5d\r\n",
             (x - g.iMargin) / 3,
             g.rCal [x].iOffset, g.rCal [x + 1].iOffset, g.rCal [x + 2].iOffset,
             g.rCal [x].iMult,   g.rCal [x + 1].iMult,   g.rCal [x + 2].iMult);
    }
  fclose (pFile);
  return 0;
  }


/*--------------------------------------------------------------

                Load calibration info from disk

--------------------------------------------------------------*/

int     fs4k_LoadCalInfo        (LPSTR pFID /* = "-Fs4000.cal"*/)
  {
        FILE            *pFile;
        int             x;

  if (NULL == (pFile = fopen (pFID, "rt")))
    {
    wsprintf (msg, "%s  open failed\r\n", pFID);
    spout ();
    return -1;
    }
  while (fgets (msg, sizeof (msg), pFile))
    {
    if (strncasecmp (msg, "Shutter ", 8) == 0)
      sscanf (&msg [8], " %d %d %d",
              &g.iShutter [0], &g.iShutter [1], &g.iShutter [2]);
    else if (strncasecmp (msg, "Gain    ", 8) == 0)
      sscanf (&msg [8], " %d %d %d",
              &g.iAGain   [0], &g.iAGain   [1], &g.iAGain   [2]);
    else if (strncasecmp (msg, "Offset  ", 8) == 0)
      sscanf (&msg [8], " %d %d %d",
              &g.iAOffset [0], &g.iAOffset [1], &g.iAOffset [2]);
    else if (strncasecmp (msg, "Boost   ", 8) == 0)
      sscanf (&msg [8], " %d %d %d",
              &g.iBoost   [0], &g.iBoost   [1], &g.iBoost   [2]);
    else if (strncasecmp (msg, "Speed ", 6) == 0)
      sscanf (&msg [6], " %d", &g.iSpeed);
    else if (strncasecmp (msg, "NegMode ", 8) == 0)
      g.bNegMode = (strncasecmp (&msg [8], "TRUE", 4) == 0);
    else if (strncasecmp (msg, "Pixel ", 6) == 0)
      {
      sscanf (&msg [6], " %d", &x);
      x = (x * 3) + g.iMargin;
      sscanf (&msg [11], " %d %d %d %d %d %d",
        &g.rCal [x].iOffset, &g.rCal [x + 1].iOffset, &g.rCal [x + 2].iOffset,
        &g.rCal [x].iMult,   &g.rCal [x + 1].iMult,   &g.rCal [x + 2].iMult);
      }
    }
  fclose (pFile);
  return 0;
  }


/*--------------------------------------------------------------

                Load gamma array for 16-bit to 8-bit conversion

        To reduce the effects of noise in low-level samples, the
        slope can be limited by specifying the maximum allowed.

--------------------------------------------------------------*/

int     fs4k_LoadGammaArray     (double dGamma, int iMaxSlope)
  {
        int             x, y;
        double          dYBase, dIn, dEnt;

  if (iMaxSlope < 2)                            // slope validation
    iMaxSlope   = 2;
  for (x = y = 0; y < 256; y++)                 // load table
    {
    dIn    = y + 1;
    dYBase = dIn / iMaxSlope;
    dEnt   = pow ((dIn / 256), dGamma);
    dEnt  *= (256.0 - dYBase);
    dEnt  += dYBase;
    dEnt  *= 256;                               // dEnt = max X for this Y
    while (x < dEnt)
      g.b16_822 [x++] = y;
    }
  while (x < 65536)                             // ensure all loaded
    g.b16_822 [x++] = 255;

//----  Load invert table for negatives.

  for (x = 0; x < 65536; x++)
    g.b16_822i [x] = 255 - g.b16_822 [x];

//----  Debug dump to list all steps in table.

#if 0                                           // list all steps
  for (x = y = z = 0; y < 256; x++)
    {
    if (x < 65536)
      if (g.b16_822 [x] == y)
        continue;
    wsprintf (msg, "%3d : %5d - %5d (%d)\r\n", y, z, x - 1, x - z);
    spout ();
    z = x;
    y++;
    }
#endif
  return 0;
  }


/*--------------------------------------------------------------

                Change debug setting for DB code

--------------------------------------------------------------*/

void    ChangeFS4000Debug       (BOOL bDebug)
  {
  g.ifs4000_debug <<= 1;
  g.ifs4000_debug += (fs4000_debug ? 1 : 0);
  fs4000_debug = bDebug;
  return;
  }


/*--------------------------------------------------------------

                Restore debug setting for DB code

--------------------------------------------------------------*/

int     RestoreFS4000Debug      (void)
  {
  fs4000_debug = g.ifs4000_debug & 0x01;
  g.ifs4000_debug >>= 1;
  return fs4000_debug;
  }


/*--------------------------------------------------------------

                Select 8, 14, or 16-bit input mode

--------------------------------------------------------------*/

int     fs4k_SetInMode          (int iNewMode)
  {
  if ((iNewMode != 8) && (iNewMode != 14) && (iNewMode != 16))
    return -1;
  g.iInMode = iNewMode;
  return 0;
  }


/*--------------------------------------------------------------

                Return bits/sample for window

--------------------------------------------------------------*/

int     fs4k_WBPS               (void)
  {
  return g.iInMode;
  }


/*--------------------------------------------------------------

                Return bits/sample for data

--------------------------------------------------------------*/

int     fs4k_DBPS               (void)
  {
  if (g.iInMode == 8)
    return 8;
  return 16;
  }


/*--------------------------------------------------------------

                Return config byte for ScanModeData

--------------------------------------------------------------*/

int     fs4k_U24                (void)
  {
  if (g.iInMode == 8)
    return 0x03;
  if (g.iInMode == 16)
    return 0x02;
  return 0x00;
  }


/*--------------------------------------------------------------

                Return analogue offset in AD9814 format

--------------------------------------------------------------*/

int     fs4k_AOffset            (int iOffset)
  {
  if (iOffset < -255)
    iOffset   = -255;
  if (iOffset >  255)
    iOffset   =  255;
  if (iOffset <    0)
    iOffset = 256 - iOffset;
  return iOffset;
  }


/*--------------------------------------------------------------

                Return AD9814 entry for (wanted gain * 100)
                (e.g. 100 = gain 1, 580 = gain 5.8)

--------------------------------------------------------------*/

int     fs4k_AGainEnt           (int iAGain)
  {
  if ((iAGain < 100) || (iAGain > 580))
    return -1;
  return ((613 * iAGain) - 60900) / (8 * iAGain);
  }


/*--------------------------------------------------------------

                Return gain * 100 for AD9814 entry
                (e.g. 100 = gain 1, 580 = gain 5.8)

--------------------------------------------------------------*/

int     fs4k_CalcAGain          (int iAGainEnt)
  {
  if ((iAGainEnt < 0) || (iAGainEnt > 63))
    return 0;
  return 60900 / (609 - (8 * iAGainEnt));
  }


/*--------------------------------------------------------------

                Get film status record

--------------------------------------------------------------*/

int     fs4k_GetFilmStatus      (FS4K_FILM_STATUS *pFS)
  {
  return fs4000_get_film_status_rec ((FS4000_GET_FILM_STATUS_DATA_IN_28*) pFS);
  }


/*--------------------------------------------------------------

                Get lamp info record

--------------------------------------------------------------*/

int     fs4k_GetLampInfo        (FS4K_LAMP_INFO *pLI)
  {
  return fs4000_get_lamp_rec ((FS4000_GET_LAMP_DATA_IN*) pLI);
  }


/*--------------------------------------------------------------

                Get scan mode info record

--------------------------------------------------------------*/

int     fs4k_GetScanModeInfo    (FS4K_SCAN_MODE_INFO *pSMI)
  {
  return fs4000_get_scan_mode_rec ((FS4000_GET_SCAN_MODE_DATA_IN_38*) pSMI);
  }


/*--------------------------------------------------------------

                Put scan mode info record

--------------------------------------------------------------*/

int     fs4k_PutScanModeInfo    (FS4K_SCAN_MODE_INFO *pSMI)
  {
  return fs4000_put_scan_mode_rec ((FS4000_DEFINE_SCAN_MODE_DATA_OUT*) pSMI);
  }


/*--------------------------------------------------------------

                Get window info record

--------------------------------------------------------------*/

int     fs4k_GetWindowInfo      (FS4K_WINDOW_INFO *pWI)
  {
  return fs4000_get_window_rec ((FS4000_GET_WINDOW_DATA_IN*) pWI);
  }


/*--------------------------------------------------------------

                Put window info record

--------------------------------------------------------------*/

int     fs4k_PutWindowInfo      (FS4K_WINDOW_INFO *pWI)
  {
  return fs4000_put_window_rec ((FS4000_SET_WINDOW_DATA_OUT*) pWI);
  }


/*--------------------------------------------------------------

                Update window info record

--------------------------------------------------------------*/

int     fs4k_SetWindow          (WORD   x_res,
                                 WORD   y_res,
                                 DWORD  x_upper_left,
                                 DWORD  y_upper_left,
                                 DWORD  width,
                                 DWORD  height,
                                 BYTE   bits_per_pixel)
  {
  return fs4000_set_window (x_res, y_res,
                            x_upper_left, y_upper_left,
                            width, height, bits_per_pixel);
  }


/*--------------------------------------------------------------

                Tasks at start-up

--------------------------------------------------------------*/

int     fs4k_BOJ                (DWORD dwMaxTransfer)
  {
        int             x;

  NullMem (desc (g));
  g.dwGlobalsSize       = sizeof (g);

  g.bShowProgress       = TRUE;
  g.bStepping           = FALSE;
  g.bTesting            = FALSE;
  g.bNegMode            = FALSE;
  g.bAutoExp            = FALSE;
  g.bSaveRaw            = FALSE;
  g.bMakeTiff           = TRUE;
  g.bUseHelper          = FALSE;

  g.bDisableShutters    = FALSE;                // changed when tuning

  g.dwMaxTransfer       = dwMaxTransfer;
  if (g.dwMaxTransfer > 65536)
    g.dwMaxTransfer   = 65536;
  //g.hEvent              = hEvent;

  g.iAGain    [0]       =   47;
  g.iAGain    [1]       =   36;
  g.iAGain    [2]       =   36;
  g.iAOffset  [0]       =  -25;
  g.iAOffset  [1]       =   -8;
  g.iAOffset  [2]       =   -5;
  g.iShutter  [0]       =  750;
  g.iShutter  [1]       =  352;
  g.iShutter  [2]       =  235;
  g.iInMode             =   14;
  g.iBoost    [0]       =  256;
  g.iBoost    [1]       =  256;
  g.iBoost    [2]       =  256;
  g.iSpeed              =    2;
  g.iFrame              =    0;
  g.ifs4000_debug       =    0;
  g.iMaxShutter         =  890;
  g.iAutoExp            =    2;
  g.iMargin             =  120; // 0 = no margin (stuffs black level !!!!)

  GetCurrentDirectory (sizeof (g.szOurPath), g.szOurPath);
  strcpy (g.szRawFID,   "-Raw0000+.dat");
  strcpy (g.szScanFID,  "-Scan000+.tif");
  strcpy (g.szThumbFID, "-Thumb00+.tif");

  fs4000_debug = FALSE;                         // off by default

  fs4k_InitCalArray     ();
  fs4k_LoadGammaArray   (2.2, 32);              // max slope = 32
  fs4000_cancel         ();
  fs4k_SetInMode        (14);

  while (fs4000_test_unit_ready ())
    if (AbortWanted ())
      return -1;

  if (fs4k_GetFilmStatus (&g.rFS))
    spout ("Error on BOJ GetFilmStatus\r\n");

  if (fs4k_GetLampInfo (&g.rLI))
    spout ("Error on BOJ GetLampInfo\r\n");

  if (fs4k_GetScanModeInfo (&g.rSMI))
    spout ("Error on BOJ GetScanModeInfo\r\n");
  NullMem (&g.rSMI.unknown1b [2], 9);
  g.rSMI.bySpeed      = g.iSpeed;
  g.rSMI.bySampleMods = 0;
  g.rSMI.unknown2a    = 0;
  g.rSMI.byImageMods  = 0;
  if (fs4k_PutScanModeInfo (&g.rSMI))
    spout ("Error on BOJ PutScanModeInfo\r\n");

  if (fs4k_GetWindowInfo (&g.rWI))
    spout ("Error on BOJ GetWindowInfo\r\n");
  if (fs4k_PutWindowInfo (&g.rWI))
    spout ("Error on BOJ PutWindowInfo\r\n");

  return 0;
  }


/*--------------------------------------------------------------

                Set Frame and remember latest setting

--------------------------------------------------------------*/

int     fs4k_SetFrame           (int iSetFrame)
  {
  fs4000_set_frame (iSetFrame);
  g.iFrame = iSetFrame;
  return 0;
  }


/*--------------------------------------------------------------

                Return film holder code

--------------------------------------------------------------*/

int     fs4k_GetHolderType      (void)
  {
  fs4k_GetFilmStatus (&g.rFS);
  return g.rFS.byHolderType;
  }


/*--------------------------------------------------------------

                Return position of black strip in holder

        This is okay for the slide holder but not completely dark
        with the negatives holder.  However, using this and disabling
        the shutters is very close to black and avoids the need to
        switch the lamp off.

--------------------------------------------------------------*/

int     fs4k_GetBlackPos        (void)
  {
  switch (fs4k_GetHolderType ())
    {
    case 1:     return      90;         // was 2 * 37;  // neg
    case 2:     return      798;        // was 2 * 387; // pos
    }
  return -1;
  }


/*--------------------------------------------------------------

                Return position of clear strip in holder

        This is used for white level calibration.  The focus
        should be set to 200 to minimise the light drop at the
        ends of the scan line.

--------------------------------------------------------------*/

int     fs4k_GetWhitePos        (void)
  {
  switch (fs4k_GetHolderType ())
    {
    case 1:     return      2 * 17;             // neg
    case 2:     return      2 * 337;            // pos
    }
  return -1;
  }


/*--------------------------------------------------------------

                Position holder (absolute)

--------------------------------------------------------------*/

int     fs4k_MoveHolder         (int    iPos)
  {
  fs4k_GetFilmStatus (&g.rFS);
  if (g.rFS.wHolderPosition == iPos)
    return 0;
  if (iPos > 0)
    return fs4000_move_position (1, 4, iPos);
  return fs4000_move_position (1, 0, 0);
  }


/*--------------------------------------------------------------

                Get current 'Frame' setting

--------------------------------------------------------------*/

int     fs4k_GetFrame           (void)
  {
  fs4k_GetFilmStatus (&g.rFS);
  return (g.rFS.unknown1 [0] >> 3);
  }


/*--------------------------------------------------------------

                Ensure lamp on for specified time
                (or ensure off if time == 0)
                (or return on time if -ve specified)

--------------------------------------------------------------*/

int     fs4k_LampTest           (void)
  {
        int             iResult = -1;

  fs4k_GetLampInfo (&g.rLI);
  if (g.rLI.byWhLampOn)
    iResult = g.rLI.dwWhLampTime;
  return iResult;
  }


int     fs4k_LampOff            (int iOffSecs)
  {
  fs4000_set_lamp (0, 0);
  if (iOffSecs > 0)
    Sleep (iOffSecs * 1000);
  return 0;
  }


int     fs4k_LampOn             (int iOnSecs)
  {
  do
    {
    fs4000_set_lamp (1, 0);
    fs4k_GetLampInfo (&g.rLI);
    if ((int) g.rLI.dwWhLampTime >= iOnSecs)
      break;
    wsprintf (msg, "Waiting for lamp (%d)  \r", iOnSecs - g.rLI.dwWhLampTime);
    fs4k_NewsStep (msg);
    if (g.bShowProgress)
      spout ();
    Sleep (500);
    }
  while (!AbortWanted ());
  return 0;
  }


/*--------------------------------------------------------------

                Update parts of ScanModeData record

--------------------------------------------------------------*/

int     fs4k_SetScanModeEx      (BYTE bySpeed,
                                 BYTE byUnk2_4,
                                 int  iUnk6)
  {
        int     x;

  fs4k_GetScanModeInfo (&g.rSMI);
  g.rSMI.bySpeed = bySpeed;
  g.rSMI.bySampleMods = byUnk2_4;               // 0-3 valid, 0 is bad if 8-bit
  if (g.iMargin == 0)
    g.rSMI.bySampleMods |= 0x20;                // stops margin
  for (x = 0; x < 3; x++)
    {
    g.rSMI.byAGain  [x] = g.iAGain   [x];       // analogue gain (AD9814)
    g.rSMI.wAOffset [x] = fs4k_AOffset (g.iAOffset [x]); // analogue offset
    g.rSMI.wShutter [x] = g.iShutter [x];       // CCD shutter pulse width
    if (g.bDisableShutters)                     // if special for tuning,
      g.rSMI.wShutter [x] = 0;
    }
  if (iUnk6 >= 0)
    g.rSMI.byImageMods = iUnk6;
  return fs4k_PutScanModeInfo (&g.rSMI);
  }


/*--------------------------------------------------------------

                Update parts of ScanModeData record

--------------------------------------------------------------*/

int     fs4k_SetScanMode        (BYTE bySpeed, BYTE byDunno)
  {
  return fs4k_SetScanModeEx (bySpeed, byDunno, -1);
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
                                 BYTE           bySamplesPerPixel,
                                 BYTE           byBitsPerSample)
  {
  fs4k_FreeBuf (pBI);
  pBI->dwHeaderSize = sizeof (FS4K_BUF_INFO);
  pBI->dwBufSize = pBI->dwLineBytes * pBI->dwLines;
  pBI->pBuf = (LPSTR) VirtualAlloc (NULL, pBI->dwBufSize,
                                    MEM_COMMIT, PAGE_READWRITE);
  pBI->dwBytesRead       = 0;
  pBI->dwLinesDone       = 0;
  pBI->bySamplesPerPixel = bySamplesPerPixel;
  pBI->byBitsPerSample   = byBitsPerSample;
  if (pBI->byBitsPerSample == 14)
    pBI->byBitsPerSample   =  16;
  pBI->byAbortReqd       = 0;
  pBI->byReadStatus      = 0;
  pBI->bySetFrame        = 0;
  pBI->byLeftToRight     = 0;
  pBI->byShift           = 0;
  pBI->wLPI              = 0;
  return (!pBI->pBuf);
  }


/*--------------------------------------------------------------

                Dump scan data to file

--------------------------------------------------------------*/

int     fs4k_DumpRaw            (LPSTR pFID, FS4K_BUF_INFO *pBI)
  {
        char            szRawFID [256];
        HANDLE          hFile;
        DWORD           dwBytesToWrite, dwBytesWritten;

  GetNextSpareName   (szRawFID, pFID);
  hFile = CreateFile (szRawFID,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    {
    wsprintf (msg, "Raw file (%s) create failed !!!\r\n", szRawFID);
    spout ();
    return -1;
    }
  pBI->dwHeaderSize = sizeof (FS4K_BUF_INFO);
  dwBytesToWrite    = sizeof (FS4K_BUF_INFO);
  WriteFile (hFile, pBI,       dwBytesToWrite, &dwBytesWritten, NULL);

  SetFilePointer (hFile, ((dwBytesWritten + 511) & -512), NULL, FILE_BEGIN);
  WriteFile (hFile, pBI->pBuf, pBI->dwBufSize, &dwBytesWritten, NULL);

  CloseHandle (hFile);
  wsprintf (msg, "** Raw file %s created\r\n", szRawFID);
  spout ();
  return 0;
  }


/*--------------------------------------------------------------

                Start scan build program

--------------------------------------------------------------*/

int     fs4k_StartMake          (LPSTR          pTifFID,
                                 LPSTR          pRawFID)
  {
        char                    cMsg [256];
        STARTUPINFO             rSI;
        PROCESS_INFORMATION     rPI;

  wsprintf (cMsg, "FsTiff.exe %s %s %s purge",
                  pTifFID,
                  pRawFID,
                  g.pbXlate == g.b16_822  ? "out8"  :
                  g.pbXlate == g.b16_822i ? "out8i" : "");
  NullMem (desc (rSI));
  rSI.cb = sizeof (rSI);
  rSI.dwFlags = STARTF_USESHOWWINDOW;           // want minimised console
  rSI.wShowWindow = SW_MINIMIZE;
  if (!CreateProcess (NULL,                     // app name below
                      cMsg,                     // app name and tail
                      NULL,                     // process attributes
                      NULL,                     // thread  attributes
                      TRUE,                     // inherit handles
                      CREATE_NEW_CONSOLE,       // creation flags
                      NULL,                     // environment
                      NULL,                     // current directory
                      &rSI,                     // startup info
                      &rPI))
    {
    wsprintf (msg, "CreateProcess failed (err = %d)", GetLastError ());
    spout ();
    }

  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Background scan data read routine

        This is the background thread that reads the scan data.

--------------------------------------------------------------*/

DWORD WINAPI
        fs4k_ReadThread         (void   *pInf)
  {
        FS4K_BUF_INFO           *pBI;
        DWORD                   dwToGo;
        char                    *pcBuf;

  pBI = (FS4K_BUF_INFO*) pInf;
  pBI->dwBytesRead = 0;
  pBI->dwReadBytes = pBI->dwBufSize;
  if (pBI->dwReadBytes > g.dwMaxTransfer)
    pBI->dwReadBytes =   g.dwMaxTransfer;
  pBI->dwReadBytes /= pBI->dwLineBytes;         // calc read size
  pBI->dwReadBytes *= pBI->dwLineBytes;
  pBI->byAbortReqd = 0;
  pBI->byReadStatus = SS_COMP;

  while (pBI->dwBytesRead < pBI->dwBufSize)
    {
    if (pBI->byAbortReqd)                       // user abort ?
      pBI->byReadStatus = SS_ABORTED;
    if (pBI->byReadStatus != SS_COMP)
      break;

    dwToGo = pBI->dwBufSize - pBI->dwBytesRead;
    if (pBI->dwReadBytes > dwToGo)              // short read at end ?
      pBI->dwReadBytes   = dwToGo;
    pcBuf = pBI->pBuf + pBI->dwBytesRead;       // calc buffer start
//  NullMem (pcBuf, pBI->dwReadBytes);          // ensure page(s) in mem

    pBI->byReadStatus = SS_PENDING;
    if (fs4000_read (pBI->dwReadBytes, (PIXEL*) pcBuf))
      pBI->byReadStatus = SS_ABORTED;
    else
      pBI->byReadStatus = SS_COMP;
    pBI->dwBytesRead += pBI->dwReadBytes;       // update read count
    }

  return 0;                                     // causes ExitThread (0)
  }


/*--------------------------------------------------------------

                Read scan data

        This routine starts the background reads and processes
        the scan data as required.

        Each scan line goes from the bottom up.
        For a thumbnail the first line is the leftmost.
        For a forward scan the first line is the rightmost.
        For a reverse scan the first line is the leftmost.

--------------------------------------------------------------*/

int     fs4k_ReadScan           (FS4K_BUF_INFO  *pBI,
                                 BYTE           bySamplesPerPixel,
                                 BYTE           byBitsPerSample,
                                 BOOL           bCorrectSamples,
                                 int            iLPI,
                                 LPSTR          pFID)
  {
        HANDLE          hFile = NULL, hThread;
        DWORD           dwThreadId, dwThreadExitCode;
        DWORD           dwBytesToWrite, dwBytesWritten;
        LPSTR           pNext;
        BYTE            *pbBuf;
        WORD            *pwBuf;
        DWORD           dwBytesDone, dwSamples, dwToDo, dwMax;
        LONGLONG        qwSum;
        int             iLineEnts, iLine, iCol, iSample, iColour, iIndex;
        int             iShift, iShift2, iOff [3];
        int             iLimit, iScale, iUnderflows, iOverflows;
        int             iWorstUnder, iWorstIndex, iWorstLine;

  fs4k_FreeBuf (pBI);
  ChangeFS4000Debug (0);
  if (fs4000_get_data_status (&pBI->dwLines, &pBI->dwLineBytes) ||
      fs4k_AllocBuf (pBI, bySamplesPerPixel, byBitsPerSample))
    {
    RestoreFS4000Debug ();
    fs4000_cancel ();
    spout ("Fatal error in ReadScan !!!\r\n");
    ReturnCode = RC_SW_ABORT;
    return -1;
    }

  pBI->wLPI = iLPI;
  pBI->bySetFrame = g.iFrame;
  pBI->byLeftToRight = (pBI->bySetFrame & 0x01);

  if (pFID && (iLPI > 0))                       // dump file reqd ?
    {
    hFile = CreateFile (pFID,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      {
      wsprintf (msg, "Raw file (%s) create failed !!!\r\n", pFID);
      spout ();
      hFile = NULL;
      }
    else
      {
      pBI->dwHeaderSize = 0;                    // will fix at close
      dwBytesToWrite = (sizeof (FS4K_BUF_INFO) + 511) & -512;
      NullMem (pBI->pBuf, dwBytesToWrite);
      memcpy  (pBI->pBuf, pBI, dwBytesToWrite);
      WriteFile (hFile, pBI->pBuf, dwBytesToWrite, &dwBytesWritten, NULL);
      }
    }

//      Shifting lines (de-interlacing) here isn't really worthwhile unless
//      we are doing colour correction as it is a processing overhead.
//      Shifting while saving the scan isn't an overhead as we are
//      rebuilding the lines anyway.
//
//      However, shifting here does give us a proper image now which may
//      assist code that wants to examine it.  So, we shift here unless we
//      are creating a raw dump file.  We can't shift when creating a raw
//      file as the file writing code doesn't wait for the shift so the
//      result is indeterminate.

  iLineEnts = pBI->dwLineBytes / ((byBitsPerSample + 7) >> 3);
  iShift = iShift2 = 0;
//if (pBI->wLPI ==  160) iShift = 0;
  if (pBI->wLPI ==  500) iShift = 1;
  if (pBI->wLPI == 1000) iShift = 2;
  if (pBI->wLPI == 2000) iShift = 4;
  if (pBI->wLPI == 4000) iShift = 8;
  if (hFile)                                    // no shift if raw output
    iShift = 0;
  pBI->byShift = iShift;
  iShift2 = iShift << 1;
  iOff [0] = 0;                                 // red
  iOff [1] = 0 - (iShift * iLineEnts);          // green
  iOff [2] = 0;                                 // blue
  if (pBI->byLeftToRight)                       // L2R
    iOff [0] -= (iShift2 * iLineEnts);
  else                                          // R2L
    iOff [2] -= (iShift2 * iLineEnts);
  pBI->dwLines -= iShift2;

#if 0
  wsprintf (msg, "LineEnts = %d, R off = %d, G off = %d, B off = %d\r\n",
                 iLineEnts,     iOff [0],   iOff [1],   iOff [2]);
  spout ();
#endif

  qwSum = 0;                                    // reset stats
  dwSamples = 0;
  pBI->wMin = 65535;
  pBI->wMax = 0;
  iUnderflows = iOverflows = 0;
  iWorstUnder = iWorstIndex = iWorstLine = 0;
  iLimit = 65535;

  hThread = CreateThread (NULL,                 // background reads
                          0,
                          fs4k_ReadThread,
                          &g.rScan,
                          0,
                          &dwThreadId);
  SetThreadPriority (hThread, THREAD_PRIORITY_ABOVE_NORMAL);

  pbBuf = (BYTE*) pBI->pBuf;
  pwBuf = (WORD*) pBI->pBuf;
  dwBytesDone = 0;
  iLine = 0;                                    // scan line #
  while (dwBytesDone < g.rScan.dwBufSize)
    {
    if (AbortWanted () || (g.rScan.byReadStatus == SS_ABORTED))
      {
      g.rScan.byAbortReqd = TRUE;               // tell thread to die
      while (hThread)
        {
        GetExitCodeThread (hThread, &dwThreadExitCode);
        if (dwThreadExitCode != STILL_ACTIVE)
          break;
        WaitForSingleObject (hThread, 1000);
        }
      CloseHandle   (hThread);
      fs4000_cancel ();                         // abort scan
      fs4k_FreeBuf  (pBI);
      if (hFile)
        CloseHandle (hFile);
      RestoreFS4000Debug ();
      return -1;
      }
    dwToDo = g.rScan.dwBytesRead - dwBytesDone;
    if (dwToDo == 0)                            // waiting for data ?
      {
      dwToDo = g.rScan.dwBytesRead - dwBytesDone;
      if (dwToDo == 0)                          // definitely waiting ?
        {
        continue;
        }
      }
    dwMax = g.rScan.dwLineBytes * 16;           // max data per loop (ensure
    if (dwToDo > dwMax)                         // frequent abort check and
      dwToDo   = dwMax;                         // display update
    pNext = g.rScan.pBuf + dwBytesDone;
    dwBytesToWrite = dwToDo;
    dwBytesDone += dwToDo;                      // premature update
    iCol = 0;                                   // scan col #
    iColour = 0;
    if (pBI->byBitsPerSample > 8)
      {
      dwToDo >>= 1;
      for ( ; dwToDo--; dwSamples++)            // for each sample
        {
        iSample = pwBuf [dwSamples];
        if (g.iInMode == 14)                    // change 14-bit to 16-bit
          iSample <<= 2;
        if ((WORD) iSample < pBI->wMin)
          pBI->wMin = iSample;
        if ((WORD) iSample > pBI->wMax)
          pBI->wMax = iSample;
        qwSum += iSample;
        if (bCorrectSamples && (iCol >= g.iMargin))   // if post margin
          {
          iSample += g.rCal [iCol].iOffset;     // add offset for sample
          if (iSample < 0)
            {
            if (iSample < iWorstUnder)
              {
              iWorstUnder = iSample;
              iWorstIndex = iCol;
              iWorstLine  = iLine;
              }
            iSample = 0;
            iUnderflows++;
            }
          else
            {
            iSample *= g.rCal [iCol].iMult;     // *= factor
            iSample += 8192;                    // for rounding
            iSample >>= 14;                     // /= 16384
            iScale = g.iBoost [iColour];
            if (iScale > 256)                   // if extra boost
              {
              iSample *= iScale;
              iSample >>= 8;
              }
            if (iSample > iLimit)
              {
              iSample = iLimit;
              iOverflows++;
              }
            }
          }
        iIndex = dwSamples + iOff [iColour];
        if (iIndex >= 0)
          pwBuf [iIndex] = iSample;
        if (++iCol == iLineEnts)                // EOL ?
          {
          iCol = 0;
          iLine++;
          }
        if (++iColour == 3)
          iColour = 0;
        }
      }
    else                                        // 8-bit input
      {
      for ( ; dwToDo--; dwSamples++)
        {
        iSample = pbBuf [dwSamples];
        if ((WORD) iSample < pBI->wMin)
          pBI->wMin = iSample;
        if ((WORD) iSample > pBI->wMax)
          pBI->wMax = iSample;
        qwSum += iSample;
        iIndex = dwSamples + iOff [iColour];    // slow code
        if (iIndex >= 0)                        // generated
          pbBuf [iIndex] = iSample;             // here !!!
        if (++iCol == iLineEnts)                // EOL ?
          {
          iCol = 0;
          iLine++;
          }
        if (++iColour == 3)
          iColour = 0;
        }
      }

    iIndex = iLine - iShift2;
    if (iIndex > 0)
      pBI->dwLinesDone = iIndex;

    if (hFile)
      WriteFile (hFile, pNext, dwBytesToWrite, &dwBytesWritten, NULL);

    if (pBI->dwLines > 100)                     // if serious read
      {
      iIndex = (pBI->dwLinesDone * 100) / pBI->dwLines;
      fs4k_NewsPercent (iIndex);
      wsprintf (msg, "Reading %d%%\r", iIndex);
      if (g.bShowProgress)
        spout ();
      }
    }

  RestoreFS4000Debug ();

  while (hThread)
    {
    GetExitCodeThread (hThread, &dwThreadExitCode);
    if (dwThreadExitCode != STILL_ACTIVE)
      break;
    WaitForSingleObject (hThread, 1000);
    }
  CloseHandle (hThread);

  pBI->wAverage = (qwSum / dwSamples);
  if (g.bStepping)
    {
    wsprintf (msg, "Samples = %d, min = %d, max = %d, average = %d\r\n",
                   dwSamples, pBI->wMin, pBI->wMax, pBI->wAverage);
    spout ();
    }

  if (hFile)                                    // finish and close raw file
    {
    SetFilePointer (hFile, 0, NULL, FILE_BEGIN);
    pBI->dwHeaderSize = sizeof (FS4K_BUF_INFO);
    dwBytesToWrite    = sizeof (FS4K_BUF_INFO);
    WriteFile (hFile, pBI, dwBytesToWrite, &dwBytesWritten, NULL);
    CloseHandle (hFile);
    wsprintf (msg, "** Raw file %s created\r\n", pFID);
    spout ();
    }

  if (iUnderflows)                              // common error
    {
    wsprintf (msg, "Underflows = %d (worst = %d at %d %s on line %d)\r\n",
                   iUnderflows, iWorstUnder,
                   (iWorstIndex - g.iMargin) / 3,
                   (iWorstIndex % 3 == 0) ? "red" :
                   (iWorstIndex % 3 == 1) ? "green" : "blue",
                   iWorstLine);
    spout ();
    }

  if (iOverflows)                               // common error
    {
    wsprintf (msg, "Overflows = %d\r\n", iOverflows);
    spout ();
    }

  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Write scan data to file

--------------------------------------------------------------*/

int     fs4k_SaveScanEx         (LPSTR          pFID,
                                 FS4K_BUF_INFO  *pBI,
                                 int            iLPI)
  {
        char                    szRawFID [256];

  if (pBI->wLPI == 0)
    pBI->wLPI = iLPI;
  GetNextSpareName (szRawFID, g.szRawFID);
  if (fs4k_DumpRaw (szRawFID, pBI) == 0)
    fs4k_StartMake (pFID, szRawFID);

//fs4k_FreeBuf (pBI);
  return AbortWanted ();
  }


int     fs4k_SaveScan           (LPSTR          pFID,
                                 FS4K_BUF_INFO  *pBI,
                                 int            iLPI)
  {
        char            cTifFID [256];
        int             iShift, iInBytesPerPixel;
        int             iInCols, iInRows, iInInc, iInMaxCol;
        int             iInRow,  iInCol,  iInEnt;
        int             iOutRow, iOutCol, iOutEnt;
        int             iOutBitsPerSample;
        BYTE            *pbSample, bLine [36000];
        WORD            *wLine = (WORD*) bLine;
        WORD            *pwSample;
        TIFF            *tif;

  if (g.bUseHelper)
    return fs4k_SaveScanEx (pFID, pBI, iLPI);

  if (pBI->wLPI == 0)                           // LPI not loaded ?
    pBI->wLPI = iLPI;

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
  iOutBitsPerSample = (pBI->byBitsPerSample + 7) & -8;
  if (g.pbXlate)
    iOutBitsPerSample = 8;                        // forces 8 bit output
  iInMaxCol = iInCols - (g.iMargin / 3);

  wsprintf (msg, "LPI = %d, cols = %d, rows = %d, shift = %d, inc = %d\r\n",
                  iLPI,     iInCols,   iInRows,   iShift,     iInInc);
  spout ();

  GetNextSpareName (cTifFID, pFID);
  tif = TIFFOpen   (cTifFID, "w");

  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, (uint16) iOutBitsPerSample);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL,(uint16) 3);
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, (uint32) iInRows);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, (uint32) iInMaxCol);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, (uint16) PLANARCONFIG_CONTIG );
  TIFFSetField (tif, TIFFTAG_COMPRESSION, (uint16) COMPRESSION_NONE );
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, (uint16) PHOTOMETRIC_RGB );
  TIFFSetField (tif, TIFFTAG_XRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_YRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, (uint16) 1);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (uint32) 1);

  pbSample = (BYTE*) pBI->pBuf;
  pwSample = (WORD*) pBI->pBuf;
  for (iOutRow = 0; iOutRow < (iInMaxCol); iOutRow++)
    {
    fs4k_NewsPercent ((iOutRow * 100) / (iInMaxCol));
    if ((g.bShowProgress) && ((iOutRow % 100) == 0))
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
          bLine [iOutEnt++] = g.pbXlate [pwSample [iInEnt]];
          iInEnt += iInInc;
          bLine [iOutEnt++] = g.pbXlate [pwSample [iInEnt]];
          iInEnt += iInInc;
          bLine [iOutEnt++] = g.pbXlate [pwSample [iInEnt]];
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
  wsprintf (msg, "** File %s created\r\n", cTifFID);
  spout ();
  fs4k_NewsStep (msg);
//fs4k_FreeBuf (pBI);
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Dump scan data to file

--------------------------------------------------------------*/

int     fs4k_DumpScan           (LPSTR pFID, FS4K_BUF_INFO *pBI)
  {
        int             iInBytesPerPixel;
        int             iInCols, iInRows, iOutRow;
        int             iOutBitsPerSample;
        BYTE            *pbSample;
        TIFF            *tif;

  iInBytesPerPixel = ((pBI->byBitsPerSample + 7) / 8) * pBI->bySamplesPerPixel;
  iInCols = pBI->dwLineBytes / iInBytesPerPixel;
  iInRows = pBI->dwLines;
  iOutBitsPerSample = (pBI->byBitsPerSample + 7) & 0xF8;

  tif = TIFFOpen (pFID, "w");

  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, (uint16) iOutBitsPerSample);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL,(uint16) 3);
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, (uint32) iInCols);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, (uint32) iInRows);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, (uint16) PLANARCONFIG_CONTIG );
  TIFFSetField (tif, TIFFTAG_COMPRESSION, (uint16) COMPRESSION_NONE );
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, (uint16) PHOTOMETRIC_RGB );
  TIFFSetField (tif, TIFFTAG_XRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_YRESOLUTION, (float) 4000);
  TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, (uint16) 1);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (uint32) 1);

  pbSample = (BYTE*) pBI->pBuf;
  iOutRow = 0;
  while ((CHAR*)pbSample < &pBI->pBuf [pBI->dwBufSize])
    {
    TIFFWriteScanline (tif, pbSample, iOutRow, 0);
    pbSample += pBI->dwLineBytes;
    iOutRow++;
    }
  TIFFClose (tif);
  wsprintf (msg, "** Dump file %s created\r\n", pFID);
  spout ();
  return 0;
  }


/*--------------------------------------------------------------

                Tedious wait for stable lamp

--------------------------------------------------------------*/

int     fs4k_WaitForStableLamp  (int iTolerance)
  {
        int                     iFrameSave;
        FS4K_WINDOW_INFO        WI;
        FS4K_SCAN_MODE_INFO     SI, TSI;
        WORD                    wDiff [8];
        int                     iAverage, iPrevAverage, iLoops, iTemp;
        DWORD                   dwDelay;

  ChangeFS4000Debug (0);
  iFrameSave = fs4k_GetFrame ();
  fs4k_GetWindowInfo (&WI);
  fs4k_GetScanModeInfo (&SI);

  fs4k_LampOn (2);                              // lamp on
  if (iFrameSave != 12)
    fs4k_SetFrame (12);                         // 12 = no movement
  fs4k_SetWindow (4000,                         // UINT2 x_res,
                  4000,                         // UINT2 y_res,
                  0,                            // UINT4 x_upper_left,
                  0,                            // UINT4 y_upper_left,
                  4000,                         // UINT4 width,
                  16,                           // UINT4 height,
                  fs4k_WBPS ());                // BYTE bits_per_pixel);
  TSI = SI;
  TSI.bySpeed = 2;
  TSI.byAGain  [0] = TSI.byAGain  [1] = TSI.byAGain  [2] = 0;
  TSI.wAOffset [0] = TSI.wAOffset [1] = TSI.wAOffset [2] = 0;
  TSI.wShutter [0] = TSI.wShutter [1] = TSI.wShutter [2] = 500;
  fs4k_PutScanModeInfo (&TSI);

  for (iLoops = iPrevAverage = 0; !AbortWanted () ; iLoops++)
    {
    dwDelay = GetTickCount () + 250;
    fs4000_scan ();
    if (fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), FALSE, 0, NULL)) // get scan data
      break;
    iAverage = g.rScan.wAverage;
    iTemp = (iAverage - iPrevAverage);
    wsprintf (msg, "Lamp variation = %d     \r", iTemp);
    fs4k_NewsStep (msg);
    if (g.bShowProgress)
      spout ();
    wDiff [iLoops % 8] = abs (iTemp);
    if ((iLoops >= 8) && (iAverage >= 20000))
      if (SumOfWords (wDiff, 1, 8) <= (iTolerance * (iLoops - 8)))
        break;
    dwDelay -= GetTickCount ();
    if ((dwDelay > 10) && (dwDelay < 251))
      Sleep (dwDelay);
    iPrevAverage = iAverage;
    }

  if (iFrameSave != 12)
    fs4k_SetFrame (iFrameSave);
  fs4k_PutScanModeInfo (&SI);
  fs4k_PutWindowInfo (&WI);
  RestoreFS4000Debug ();

  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Set window and frame for tuning

--------------------------------------------------------------*/

int     fs4k_TuneSetWinFrame    (int iScanLines)
  {
  fs4k_SetWindow (4000,                         // UINT2 x_res,
                  4000,                         // UINT2 y_res,
                  0,                            // UINT4 x_upper_left,
                  0,                            // UINT4 y_upper_left,
                  4000,                         // UINT4 width,
                  iScanLines,                   // UINT4 height,
                  fs4k_WBPS ());                // BYTE bits_per_pixel);

//----  frame 12 = no advance during scan + no home after

  fs4k_SetFrame (12);
//fs4000_move_position (0, 0, 0);               // carriage home
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Setup for tuning

--------------------------------------------------------------*/

int     fs4k_TuneBegin          (int iTuneSpeed, int iScanLines)
  {
        int             iColour;

//fs4k_LampOn (1);
  fs4000_move_position (0, 0, 0);               // carriage home
  fs4k_TuneSetWinFrame (iScanLines);

//----  Reset offset and gain info

  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain   [iColour]  =    0;
    g.iAOffset [iColour]  =    0;
    g.iShutter [iColour]  =  500;
    g.iBoost   [iColour]  =  256;
    }
  fs4k_InitCalArray ();

//----  Position holder to clear area, wait for lamp
//----  Focus to 200 (closest to lamp) to minimise light drop
//----  on pixels at the ends of the line (this drop doesn't
//----  occur with film as it is closer to the lens).

  fs4k_MoveHolder (fs4k_GetWhitePos ());
  fs4000_execute_afae (2, 0, 0, 200, 0, 0);
  fs4k_SetScanMode (iTuneSpeed, fs4k_U24 ());
  fs4k_WaitForStableLamp (1);
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Do scan for tuning, load statistics

--------------------------------------------------------------*/

int     fs4k_TuneRead           (int            iSpeed,
                                 TUNE_STATS     Stat [6])
  {
        int             iColour, iSample, iSampleIndex, iLineSamples;
        WORD            *pwSample;
        TUNE_STATS      *pStat;

  ChangeFS4000Debug (0);
  fs4k_SetScanMode (iSpeed, fs4k_U24 ());       // load gains, etc
  fs4000_scan ();
  if (fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), FALSE, 0, NULL)) // get scan data
    return -1;
  RestoreFS4000Debug ();

  for (iColour = 0; iColour < 6; iColour++)     // reset stats
    {
    NullMem (desc (Stat [iColour]));
    Stat [iColour].wMin = 65535;
    }

  pwSample = (WORD*) g.rScan.pBuf;
  iLineSamples = g.rScan.dwLineBytes / 2;
  for (iSampleIndex = 0; iSampleIndex < iLineSamples; iSampleIndex++)
    {
    iSample = AvgOfWords (pwSample++, iLineSamples, g.rScan.dwLines);
    iColour = iSampleIndex % 3;
    if (iSampleIndex < g.iMargin)
      iColour += 3;
    pStat = &Stat [iColour];
    pStat->dwTotal += iSample;
    if (pStat->wMin > iSample)
      pStat->wMin   = iSample;
    if (pStat->wMax < iSample)
      pStat->wMax   = iSample;
    pStat->wSamples++;
    }
  for (iColour = 0; iColour < 6; iColour++)
    {
    pStat = &Stat [iColour];
    if (pStat->wSamples)
      pStat->wAverage = pStat->dwTotal / pStat->wSamples;
    }
  return 0;
  }


/*--------------------------------------------------------------

                Check readings, adjust entries for next scan

--------------------------------------------------------------*/

int     fs4k_TuneReact          (TUNE_INFO      Info [3],
                                 TUNE_STATS     Stat [3],
                                 int            iEntry [3],
                                 int            iType)
  {
        int             iReady, iColour, iTemp, iAdj;
        TUNE_INFO       *pInfo;
        TUNE_STATS      *pStat;
        int             *piEntry;

  for (iReady = iColour = 0; iColour < 3; iColour++)    // 3 colours
    {
    pInfo =   &Info   [iColour];                        // point to info, stats,
    pStat =   &Stat   [iColour];                        // and entry
    piEntry = &iEntry [iColour];
    pInfo->iOldEntry = *piEntry;
    if (iType <  0)                                     // get reqd value
      iTemp = pStat->wMin;
    if (iType == 0)
      iTemp = pStat->wAverage;
    if (iType >  0)
      iTemp = pStat->wMax;
    iTemp -= pInfo->wBase;
    iTemp -= pInfo->wWanted;
    if (iTemp <= 0)                                     // less or perfect
      {
      pInfo->iEntryLow = pInfo->iOldEntry;
      pInfo->iErrorLow = iTemp;
      }
    if (iTemp >= 0)                                     // greater or perfect
      {
      pInfo->iEntryHigh = pInfo->iOldEntry;
      pInfo->iErrorHigh = iTemp;
      }
    iAdj = pInfo->iEntryHigh - pInfo->iEntryLow;        // current range
    if (iAdj == 1)                                      // if range = 1
      {
      if (pInfo->iErrorLow == 0)                        // 0 = not tested
        {
        *piEntry = pInfo->iEntryLow;                    // test it
        continue;
        }
      if (pInfo->iErrorHigh == 0)                       // if upper not tested,
        {
        *piEntry = pInfo->iEntryHigh;                   // test it
        continue;
        }
      iTemp = pInfo->iErrorLow + pInfo->iErrorHigh;     // which ent is better ?
      if (iTemp > 0)                                    // if lower closer
        iTemp = pInfo->iEntryLow;
      else                                              // else use upper
        iTemp = pInfo->iEntryHigh;
      }
    else
      iTemp = pInfo->iEntryLow + iAdj / 2;              // calc next entry
    if (*piEntry != iTemp)                              // if change
      *piEntry    = iTemp;
    else                                                // if no change
      iReady++;
    }
  return iReady;                                        // 3 = all ready
  }


/*--------------------------------------------------------------

                Report on sloppiness of readings (depressing)

--------------------------------------------------------------*/

int     fs4k_TuneRpt            (int iSpeed, FS4K_BUF_INFO  *pBI)
  {
  typedef struct
    {
        int             iValueTotal;
        int             iRangeTotal;
        int             iMeanDevTotal;
        double          dStdDevTotal;
        int             iMaxRange;
        int             iMaxMeanDev;
        double          dMaxStdDev;
        int             iSamples;
    } RPT_STAT;

        RPT_STAT        rInf [3], *prInf;

        int             iColour, iSample, iSampleIndex, iLineSamples;
        int             iAverage, iMin, iMax, iRange, iLine, x, y;
        int             iError, iMeanDev, iWork, iTotal;
        double          dWork, dStdDev, dTotalSq;
        WORD            *pwSample, *pwEnt;

  for (iColour = 0; iColour < 3; iColour++)             // init totals
    NullMem (desc (rInf [iColour]));
  iLineSamples = pBI->dwLineBytes / 2;                  // # of sample points

  for (iSampleIndex = g.iMargin;                        // for each sample point
       iSampleIndex < iLineSamples;
       iSampleIndex++)
    {
    pwSample     = ((WORD*) pBI->pBuf) + iSampleIndex;  // point to first
    iColour      = iSampleIndex % 3;
    prInf        = &rInf [iColour];                     // point to totals

    iAverage     = AvgOfWords (pwSample, iLineSamples, pBI->dwLines);
    prInf->iValueTotal += iAverage;

    iError = iTotal = dTotalSq = 0;
    for (pwEnt = pwSample, x = pBI->dwLines;            // for each scan line
         x--;
         pwEnt += iLineSamples)
      {
      iWork     = abs (*pwEnt - iAverage);              // error
      iError   += iWork;
      iTotal   += *pwEnt;
      dWork     = *pwEnt;                               // value squared
      dTotalSq += dWork * dWork;
      }

    iMeanDev = RoundedDiv (iError, pBI->dwLines);       // mean deviation

//      sqrt((n.sum(x^2)-sum(x)^2)/(n*(n-1)))

    dWork    = iTotal;                                  // StdDev using values
    dWork   *= iTotal;
    dStdDev  = (dTotalSq * pBI->dwLines) - dWork;
    dStdDev /= pBI->dwLines * (pBI->dwLines - 1);
    dStdDev  = sqrt (dStdDev);

    if (prInf->iMaxMeanDev < iMeanDev)                  // mean dev info
      prInf->iMaxMeanDev   = iMeanDev;
    prInf->iMeanDevTotal  += iMeanDev;

    if (prInf->dMaxStdDev  < dStdDev)                   // std dev info
      prInf->dMaxStdDev    = dStdDev;
    prInf->dStdDevTotal   += dStdDev;

    iMin         = MinOfWords (pwSample, iLineSamples, pBI->dwLines);
    iMax         = MaxOfWords (pwSample, iLineSamples, pBI->dwLines);
    iRange = iMax - iMin;

    if (prInf->iMaxRange < iRange)                      // range info
      prInf->iMaxRange   = iRange;
    prInf->iRangeTotal += iRange;

    prInf->iSamples++;
    }

  wsprintf (msg, "  Exposure = %2d, Scanlines = %d\r\n", iSpeed, pBI->dwLines);
  spout ();
  //        xxxxx  xxxxx  xxxx/xxxx  xxxx/xxxx  xxxx/xxxx
  spout (" CCD      Avg    Peak-pk    Mean dev   Std dev  \r\n");
  spout ("cells    Value   Avg/Max    Avg/Max    Avg/Max  \r\n");
  for (iColour = 0; iColour < 3; iColour++)
    {
    prInf        = &rInf [iColour];
    wsprintf (msg, "%c %5d  %5d  %4d/%-4d  %4d/%-4d  %4d/%-4d\r\n",
              "RGB" [iColour],
              prInf->iSamples,
              prInf->iValueTotal   / prInf->iSamples,
              prInf->iRangeTotal   / prInf->iSamples,
              prInf->iMaxRange,
              prInf->iMeanDevTotal / prInf->iSamples,
              prInf->iMaxMeanDev,
              (int) (prInf->dStdDevTotal / prInf->iSamples),
              (int)  prInf->dMaxStdDev);
    spout ();
    }
  return 0;
  }


/*--------------------------------------------------------------

                Load best shutter settings

        Calc best exposure time (0 - 1000)

        The CCD light/reading ratio seems to drop off after about 10000
        (when 14-bit with no gain) so we try to set the exposure to get
        a reading of 10000.  Usually the red channel is too dark so its
        exposure gets set to 1000 (maximum) here and the analogue gain
        is increased in the next step.

        The black noise level seems to increase markedly when the exposure
        setting here approaches 1000 so now we limit it to 890.  The safe
        limit depends on the exposure speed.  Speeds 1 - 3 have no limit,
        speeds 4 - 8 have a limit of 890, speeds 10 & 12 have a limit of 910.
        The limit of 890 caters for any speed.

        Each colour channel starts at 0% shutter, unity gain, no offset.
        This step sets the shutter duty cycle for each colour.

--------------------------------------------------------------*/

int     fs4k_TuneShutters       (int iTuneSpeed, int iScanLines)
  {
        int             iWanted, iColour, iLoops, iReady;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_NewsStep ("Shutters");
  fs4k_TuneSetWinFrame (iScanLines);
  fs4k_LampOn (5);
  fs4k_MoveHolder (fs4k_GetWhitePos ());
  iWanted = (fs4k_DBPS () == 16 ? 40000 : 10000);
  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain   [iColour] = 0;
    g.iShutter [iColour] = 0;
    pInfo = &Info [iColour];
    pInfo->iEntryLow  = 0;
//  pInfo->iEntryHigh = 1000;
    pInfo->iEntryHigh = g.iMaxShutter;
    pInfo->iErrorLow  = 0;
    pInfo->iErrorHigh = 0;
    pInfo->wWanted    = iWanted;
    }
  for (iLoops = iReady = 0; iReady < 3; iLoops++)       // 3 = all OK
    {
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    if (iLoops == 0)                            // first scan gets base
      {                                         // entries for adjusting
      Info [0].wBase = Stat [0].wAverage;       // subsequent scans
      Info [1].wBase = Stat [1].wAverage;
      Info [2].wBase = Stat [2].wAverage;
      }
    iReady = fs4k_TuneReact (Info, Stat, g.iShutter, 0);
    if ((g.bTesting) || (iReady == 3))                          // display
      {
      wsprintf (msg, "Shutter = %4d %4d %4d, R = %5u, G = %5u, B = %5u\r\n",
                Info [0].iOldEntry, Info [1].iOldEntry, Info [2].iOldEntry,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
      spout ();
      }
    if (FS4_Halt ()) break;
    }
  g.iAGain [0] = g.iAGain [1] = g.iAGain [2] = 63;      // for next step
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Load AD9814 analogue offsets

        We adjust the three colour offsets here to get a slightly
        positive black reading.

        This step sets the analogue offset for each colour.

--------------------------------------------------------------*/

int     fs4k_TuneAOffsets       (int iTuneSpeed, int iScanLines)
  {
        int             iWanted, iColour, iLoops, iReady, x;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_NewsStep ("Analogue Offsets");
  fs4k_TuneSetWinFrame (iScanLines);
  fs4k_MoveHolder (fs4k_GetBlackPos ());
  g.bDisableShutters = TRUE;

  g.iAOffset [0] = g.iAOffset [1] = g.iAOffset [2] = 200;
  fs4k_TuneRead (iTuneSpeed, &Stat [0]);
  if (g.bTesting)
    {
    wsprintf (msg, "mins at 200  %5d %5d %5d\r\n",
                   Stat [0].wMin, Stat [1].wMin, Stat [2].wMin);
    spout ();
    }
  for (iColour = 0; iColour < 3; iColour++)
    {
    pInfo = &Info [iColour];
    pInfo->iEntryLow  = -255;
    pInfo->iEntryHigh = +255;
    pInfo->iErrorLow  = 0;
    pInfo->iErrorHigh = 0;
    pInfo->wBase      = 0;
    pInfo->wWanted    = Stat [iColour].wMin;
    }

  g.iAOffset [0] = g.iAOffset [1] = g.iAOffset [2] = 100;
  fs4k_TuneRead (iTuneSpeed, &Stat [0]);
  if (g.bTesting)
    {
    wsprintf (msg, "mins at 100  %5d %5d %5d\r\n",
                   Stat [0].wMin, Stat [1].wMin, Stat [2].wMin);
    spout ();
    }
  for (iColour = 0; iColour < 3; iColour++)
    {
    pInfo = &Info [iColour];
    x =  pInfo->wWanted;
    x -= Stat [iColour].wMin;
    if (x < 0)
      x = 0;
    pInfo->wWanted = RoundedDiv (x, 100);
    }
  if (g.bTesting)
    {
    wsprintf (msg, "wanted       %5d %5d %5d\r\n",
                   Info [0].wWanted, Info [1].wWanted, Info [2].wWanted);
    spout ();
    }
  g.iAOffset [0] = g.iAOffset [1] = g.iAOffset [2] = 0;
  for (iReady = 0; iReady < 3; )
    {
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
//  iReady = fs4k_TuneReact (Info, &Stat [0], g.iAOffset, -1);  // line mins
    iReady = fs4k_TuneReact (Info, &Stat [0], g.iAOffset,  0);  // line avg's
//  iReady = fs4k_TuneReact (Info, &Stat [3], g.iAOffset, -1);  // margin mins

    if ((g.bTesting) || (iReady == 3))
      {
      wsprintf (msg, "Offset  = %4d %4d %4d, R = %5u, G = %5u, B = %5u\r\n",
                Info [0].iOldEntry, Info [1].iOldEntry, Info [2].iOldEntry,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
//              Stat [0].wMin,      Stat [1].wMin,      Stat [2].wMin    );
      spout ();
      if (g.iMargin > 0)
        {
        wsprintf (msg, "Margin                    R = %5u, G = %5u, B = %5u\r\n",
                  Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
//                Stat [3].wMin,      Stat [4].wMin,      Stat [5].wMin    );
        spout ();
        }
      }
    if (FS4_Halt ()) break;
    }
  if (g.bTesting)
    fs4k_DumpScan ("-TAnOffs.tif", &g.rScan);
  g.bDisableShutters = FALSE;
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Load AD9814 analogue gains

        Calc best analogue amp gain.  We want a reading of about 15500 here
        in 14-bit mode or 62000 in 16-bit mode.

        This step sets the analogue gain for each colour.

--------------------------------------------------------------*/

int     fs4k_TuneAGains         (int iTuneSpeed, int iScanLines)
  {
        int             iWanted, iColour, iLoops, iReady;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_NewsStep ("Analogue Gains");
  fs4k_TuneSetWinFrame (iScanLines);
  fs4k_LampOn (5);
  fs4k_MoveHolder (fs4k_GetWhitePos ());
//iWanted = (fs4k_DBPS () == 16 ? 64000 : 16000);
  iWanted = (fs4k_DBPS () == 16 ? 62000 : 15500);
  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain [iColour]  = 32;
    pInfo = &Info [iColour];
    pInfo->iEntryLow  = 0;
    pInfo->iEntryHigh = 63;
    pInfo->iErrorLow  = 0;
    pInfo->iErrorHigh = 0;
    pInfo->wBase      = 0;
    pInfo->wWanted    = iWanted;
    }
  for (iReady = 0; iReady < 3; )                        // 3 = all OK
    {
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    iReady = fs4k_TuneReact (Info, Stat, g.iAGain, 0);
    if ((g.bTesting) || (iReady == 3))
      {
      wsprintf (msg, "Gain    = %4d %4d %4d, R = %5u, G = %5u, B = %5u\r\n",
                Info [0].iOldEntry, Info [1].iOldEntry, Info [2].iOldEntry,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
      spout ();
      if (g.iMargin > 0)
        {
        wsprintf (msg, "Margin                    R = %5u, G = %5u, B = %5u\r\n",
                  Stat [3].wAverage, Stat [4].wAverage, Stat [5].wAverage);
        spout ();
        }
      }
    if (FS4_Halt ()) break;
    }
  if (g.bTesting)
    fs4k_DumpScan ("-TAnGains.tif", &g.rScan);
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Load digital offset for each sample

        Load adjust array.  Adding these entries to scan data should
        cause black samples to have a value of 32.  This doesn't really
        work due to errors in the CCD output but at least we are trying.

        This step sets the digital offset for each sample.

--------------------------------------------------------------*/

int     fs4k_TuneDOffsets       (int iTuneSpeed, int iScanLines)
  {
        WORD            wSampleIndex, *pwSample, wLineSamples;
        int             iSample;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_NewsStep ("Digital Offsets");
  fs4k_TuneSetWinFrame (iScanLines);
  fs4k_GetLampInfo (&g.rLI);                          // lamp is normally on, but
  if (g.rLI.byWhLampOn)                         // off if called from scan
    {                                           // routine
    fs4k_MoveHolder (fs4k_GetBlackPos ());
    g.bDisableShutters = TRUE;
    }
  fs4k_TuneRead (iTuneSpeed, &Stat [0]);
  pwSample = (WORD*) g.rScan.pBuf;
  wLineSamples = g.rScan.dwLineBytes / 2;
  for (wSampleIndex = g.iMargin; wSampleIndex < wLineSamples; wSampleIndex++)
    {
    iSample = 32 - AvgOfWords (&pwSample [wSampleIndex],
                               wLineSamples,
                               g.rScan.dwLines);
    g.rCal [wSampleIndex].iOffset = iSample;
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);                  // precise as mud
  if (g.bTesting)
    fs4k_DumpScan ("-TDgOffs.tif", &g.rScan);
  g.bDisableShutters = FALSE;
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Load digital gain for each sample point

        Load factor array (output = input * factor / 16384) so
        the max reading from each sample point will be 64000.

        This step sets the digital multiplier for each sample.

--------------------------------------------------------------*/

int     fs4k_TuneDGains         (int iTuneSpeed, int iScanLines)
  {
        WORD            wSampleIndex, *pwSample, wLineSamples;
        int             iWanted, iSample;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_NewsStep ("Digital Gains");
  fs4k_TuneSetWinFrame (iScanLines);
  fs4k_LampOn (5);
  fs4k_MoveHolder (fs4k_GetWhitePos ());
  fs4k_TuneRead (iTuneSpeed, &Stat [0]);
  iWanted = (fs4k_DBPS () == 16 ? 64000 : 16000);
  pwSample = (WORD*) g.rScan.pBuf;
  wLineSamples = g.rScan.dwLineBytes / 2;
  for (wSampleIndex = g.iMargin; wSampleIndex < wLineSamples; wSampleIndex++)
    {
    iSample = AvgOfWords (&pwSample [wSampleIndex],
                          wLineSamples,
                          g.rScan.dwLines);
    iSample += g.rCal [wSampleIndex].iOffset;
    if (iSample < 10485)
      iSample   = 10485;
    g.rCal [wSampleIndex].iMult = RoundedDiv (iWanted * 16384, iSample);
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);
  if (g.bTesting)
    fs4k_DumpScan ("-TDgGains.tif", &g.rScan);
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Calc best speed for scan from preview

        This loads globals !!! -> g.iSpeed, g.iShutter [0 - 3]

--------------------------------------------------------------*/

int     fs4k_CalcSpeed          (FS4K_BUF_INFO *pBI)
  {
        int             x, y, z, iXStart, iXLimit, iYStart, iYLimit;
        WORD            *pwSample;
        int             iSample, iSamples, iReqd;
        AE_BIN          rAE  [15];
        static int      iKey [16] = { 1000,  1200,  1430,  1700,  2000,
                                      2449,  3000,  3464,  4000,  5000,
                                      6000,  6928,  8000, 10000, 12000,
                                     0x7FFFFFFF };
        static int      iSpeedEnt [9] = {1, 2, 3, 4, 5, 6, 8, 10, 12};
        int             iMult, iMax;

  if (pBI->dwLineBytes != (DWORD) (2 * (12000 + g.iMargin)))
    return g.iSpeed;

  iXStart = g.iMargin;                          // calc cropped X range
  iXLimit = pBI->dwLineBytes / 2;
  x       = (iXLimit - iXStart) / 20;
  iXStart += x;
  iXLimit -= x;

  iYStart = 0;                                  // calc cropped Y range
  iYLimit = pBI->dwLines;
  y       = (iYLimit - iYStart) / 20;
  iYStart += y;
  iYLimit -= y;

  for (x = 0; x < 15; x++)                      // initialise totals
    {
    rAE [x].iFactor = iKey [x];
    rAE [x].iMin    = 64000000 * g.iSpeed / iKey [x + 1];
    rAE [x].iCount  = 0;
    }

  iSamples = 0;
  for (y = iYStart; y < iYLimit; y++)
    {
    pwSample = (WORD*) &(pBI->pBuf [y * pBI->dwLineBytes]);
    pwSample += iXStart;
    for (x = iXStart; x < iXLimit; x++)
      {
      iSample = *pwSample++;                    // get sample
      for (z = 0; z < 14; z++)                  // determine total
        if (iSample >= rAE [z].iMin)
          break;
      rAE [z].iCount++;                         // update totals
      iSamples++;
      }
    }
  iReqd = iSamples / 256;

#if 0
  wsprintf (msg, "Samples = %d, want = %d\r\n", iSamples, iReqd);
  spout ();
  for (x = 0; x < 15; x++)
    {
    wsprintf (msg, "Bin [%d] = %d\r\n", x, rAE [x].iCount);
    spout ();
    }
#endif

  for (x = 0; x < 15; x++)                      // determine exposure
    if (0 >= (iReqd -= rAE [x].iCount))
      break;
  iMult = rAE [x].iFactor;

  iMax = g.iShutter [0];                        // find current max shutter
  if (iMax < g.iShutter [1])
    iMax =   g.iShutter [1];
  if (iMax < g.iShutter [2])
    iMax =   g.iShutter [2];

#if 0
  wsprintf (msg, "iMult = %d, iMax = %d\r\n", iMult, iMax);
  spout ();
#endif

  for (x = 0; x < 9; x++)                       // get speed setting
    {
    y = (iMult * iMax) / (iSpeedEnt [x] * 1000);
    if (y <= g.iMaxShutter)
      break;
    }

  g.iSpeed = iSpeedEnt [x];                     // load global speed
  for (x = 0; x < 3; x++)                       // load global shutters
    {
    g.iShutter [x] *= iMult;
    g.iShutter [x] /= (g.iSpeed * 1000);
    }

#if 0
  wsprintf (msg, "Speed = %d, shutters = %d %d %d\r\n", g.iSpeed,
            g.iShutter [0], g.iShutter [1], g.iShutter [2]);
  spout ();
#endif

  return g.iSpeed;
  }


//----------------------------------------------------------------------

//              Temp area for new code

int     fs4k_BlackTest          (int iScanLines)
  {
        int             iColour, iPos, iFocus;
        int             iTuneSpeed, iSpeed, iShutter;
        TUNE_STATS      Stat [6], *pStat;
        TUNE_INFO       Info [3], *pInfo;

  fs4k_TuneSetWinFrame (iScanLines);

//----  Reset offset and gain info

  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain   [iColour]  =   63;
    g.iAOffset [iColour]  =  -38; //  0;
    g.iShutter [iColour]  =  890;
    g.iBoost   [iColour]  =  256;
    }
  fs4k_InitCalArray ();


  fs4k_GetFilmStatus     (&g.rFS);
  if (g.rFS.byHolderType == 1)
    {
    fs4k_LampOn (5); 
//  fs4k_MoveHolder (fs4k_GetBlackPos ());
    iTuneSpeed = 2;
    for (iPos = 80; iPos < 100; iPos +=2)
      {
      fs4k_MoveHolder (iPos);
      fs4k_TuneRead (iTuneSpeed, Stat);
      wsprintf (msg, "Pos %3d, R %5u, G %5u, B %5u,  R %5u, G %5u, B %5u\n",
                iPos,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage,
                Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
      spout ();
      if (FS4_Halt ()) break;
      }
#if 0
    iPos = 88;
    for (iFocus = 75; iFocus <= 200; iFocus += 5)
      {
      fs4k_MoveHolder (iPos);
      fs4000_execute_afae (2, 0, 0, iFocus, 0, 0);
      fs4k_TuneRead (iTuneSpeed, Stat);
      wsprintf (msg, "Pos %3d, Focus %3d, R %5u, G %5u, B %5u,  R %5u, G %5u, B %5u\n",
                iPos, iFocus,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage,
                Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
      spout ();
      if (FS4_Halt ()) break;
      }
#endif
    }

  if (g.rFS.byHolderType == 2)
    {
    fs4k_LampOn (5);
//  fs4k_MoveHolder (fs4k_GetBlackPos ());
    iTuneSpeed = 2;
    for (iPos = 760; iPos < 840; iPos +=4)
      {
      fs4k_MoveHolder (iPos);
      fs4k_TuneRead (iTuneSpeed, Stat);
      wsprintf (msg, "Pos %3d, R %5u, G %5u, B %5u,  R %5u, G %5u, B %5u\n",
                iPos,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage,
                Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
      spout ();
      if (FS4_Halt ()) break;
      }
    }


  if (g.rFS.byHolderType == 0)
    fs4k_LampOff (1);
  for (iTuneSpeed = 1; iTuneSpeed < 13; iTuneSpeed++)
    {
    if ((iTuneSpeed > 6) && (iTuneSpeed % 2))
      continue;
#if 1
//  for (iShutter = 800; iShutter < 1000; iShutter += 20)
    for (iShutter =  50; iShutter < 1000; iShutter += 900)
      {
      g.iShutter [0]  =  g.iShutter [1]  =  g.iShutter [2]  =  iShutter;
      fs4k_TuneRead (iTuneSpeed, Stat);
      if (g.iMargin > 0)
        wsprintf (msg, "Exp %2d.%02d, R %5u, G %5u, B %5u,  R %5u, G %5u, B %5u\n",
                  iTuneSpeed, iShutter / 10,
                  Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage,
                  Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
      else
        wsprintf (msg, "Exp %2d.%02d, R %5u, G %5u, B %5u\n",
                  iTuneSpeed, iShutter / 10,
                  Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
      spout ();
      if (FS4_Halt ()) break;
      }
#endif
#if 0
//  if (iTuneSpeed < 12)
//    continue;
    for (iSpeed = 0; iSpeed < 1000; iSpeed += 100)
      {
      g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = iSpeed;
      fs4k_TuneRead (iTuneSpeed, Stat);
      wsprintf (msg, "Exp %2d.%03d, R %5u, G %5u, B %5u,  R %5u, G %5u, B %5u\n",
                iTuneSpeed, iSpeed,
                Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage,
                Stat [3].wAverage,  Stat [4].wAverage,  Stat [5].wAverage);
      spout ();
      if (FS4_Halt ()) break;
      }
#endif
    if (FS4_Halt ()) break;
    }
bye:
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Start-up code.

--------------------------------------------------------------*/
#if 0
int     FS4_FindScanner         (BYTE   byStartHaId,
                                 BYTE   byStartTarget,
                                 BOOL   bReScanBus)
  {
  aspi.u.HA_Id  = byStartHaId;
  aspi.u.target = byStartTarget;
  aspi.u.LUN    = 0;
  return aspi_find_device (DTYPE_SCAN, "CANON", "IX-40015G", bReScanBus);
  }


int     FS4_ScannerAnnounce     (void)
  {
  wsprintf (msg, "%d:%d:0 Scanner %.28s\r\n",
                 aspi.u.HA_Id, aspi.u.target,
                 aspi.u.LUN_inq.vendor);
  spout ();
  wsprintf (msg, "  MaxTransfer  = %u\r\n", aspi.u.max_transfer);
  spout ();
  return 0;
  }
#endif

int     FS4_BOJ                 (void)
  {
        int             erc;

  erc = usb_init ();
  if (erc == 0)
    {
    fs4000_do_scsi    = usb_scsi_exec;
    pfnTransportClose = usb_deinit;

    erc = fs4k_BOJ (65536);

    return erc;
    }
#if 0
  erc = aspi_init ();
  if (erc == 0)
    if ((FS4_FindScanner (2, 0, 0)) &&
        (FS4_FindScanner (0, 0, 0)) &&
        (FS4_FindScanner (0, 0, 1)) )
      {
      spout ("Scanner not found\r\n");
      aspi_deinit ();
      erc = 1;
      }

  if (erc == 0)
    {
    fs4000_do_scsi    = aspi_scsi_exec;
    pfnTransportClose = aspi_deinit;

    FS4_ScannerAnnounce ();

    erc = fs4k_BOJ (aspi.u.max_transfer, aspi.u.hEvent);
    }
#endif
  return erc;
  }


/*--------------------------------------------------------------

                Close-down code.

--------------------------------------------------------------*/

int     FS4_EOJ                 (void)
  {
  if (pfnTransportClose)
    pfnTransportClose ();
  return 0;
  }


/*--------------------------------------------------------------

                Keyboard halt if stepping mode,
                check for keyboard abort if not

--------------------------------------------------------------*/

BOOL    FS4_Halt                (void)
  {
  if (g.bStepping)
    if (ShowAndWait ("--- ") == 0x1B)           // ESC = quit
      ReturnCode = RC_KBD_ABORT;
  return (AbortWanted ());
  }


/*--------------------------------------------------------------

                Calc gains and offsets

        'Fs4000.exe tune dump'

ScanModeData.unknown3 [3] = {60, 60, 60};       // 0 - 63       analogue gain
ScanModeData.unknown4 [3] = {280, 280, 280};    // 0 - 511      analogue offset
ScanModeData.unknown5 [3] = {750, 750, 750};    // 0 - 1000     shutter time

The CCD samples are based on the light and the shutter time.

The CCD samples are adjusted by :-
a) an offset (unk4, 0 - 255 = add 0 - 255, 256 - 511 = sub 0 - 255)
b) programmable gain (unk3, 0 - 63 = 1 - 5.8 gain)

Analogue gain (unknown3)
- gain = 5.8 / (1.0 + (4.8 * ((63 - entry) / 63)))
       = 609 / (609 - (8 * entry));
- entry = (609 * (gain - 1)) / (8 * gain)

Analogue offset (unknown4)
- 9-bit entry (sign + 8-bit adjust)
- if sign = 0, input += adjust
- if sign = 1, input -= adjust
- 16-bit effect = (adjust * 19) approx when analogue gain = 1

Shutter time (unknown5)
- Entry (0 - 1000) specifies shutter open portion of exposure period.
  0 is slightly more than no shutter (about 1).


        Order of operations.

a) Wait for stable lamp.

b) Set shutters for white balance.

c) Set black level analogue offsets.

d) Set white level analogue gains.

e) Set black level digital offset for each sample.

f) Set white level digital gain for each sample.

--------------------------------------------------------------*/

int     FS4_Tune                (int iTuneSpeed)
  {
        int             iScanLines = 16;        // Vuescan uses 80
                                        // above 16 is slower but no better
  fs4k_NewsTask          ("Tuning");
  fs4000_reserve_unit    ();
  fs4000_control_led     (2);
  fs4000_test_unit_ready ();
  ChangeFS4000Debug (0);
  if (iTuneSpeed < 0)
    iTuneSpeed = g.iSpeed;
  if (fs4k_WBPS () < 14)
    {
    spout ("Can't TUNE when bits/sample < 14\r\n");
    goto release;
    }
  if (fs4k_GetBlackPos () < 0)
    {
    spout ("No film holder\r\n");
    goto release;
    }

  if (fs4k_TuneBegin    (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneShutters (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneAOffsets (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneAGains   (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneDOffsets (iTuneSpeed, 100       ) ) goto release;
  if (fs4k_TuneDGains   (iTuneSpeed, 100       ) ) goto release;

  g.bNegMode = FALSE;

release:
  RestoreFS4000Debug  ();
  fs4000_control_led  (0);
  fs4000_release_unit ();
  fs4k_NewsDone       ();
  return AbortWanted  ();
  }


/*--------------------------------------------------------------

                Calc gains and offsets for negatives

        'Fs4000.exe tuneg dump'

--------------------------------------------------------------*/

int     FS4_NegTune             (int iTuneSpeed)
  {
        WORD            wColour, wLine, *pwSample;
        int             iSample, iTemp;
        int             iScanLines = 16;
        int             iLimit [3], iBestValue [4], iBestLine [4];
        int             iCurrentGain, iWantedGain;
        int             iShutterMax, iGainMax, iAGainEnt;
        int             iShutterLimit = g.iMaxShutter;
        int             iGainLimit;
        double          dTemp;


  fs4k_NewsTask          ("Tuning");
  fs4000_reserve_unit    ();
  fs4000_control_led     (2);
  fs4000_test_unit_ready ();
  ChangeFS4000Debug      (0);
  if (iTuneSpeed < 0)
    iTuneSpeed = g.iSpeed;
  fs4k_GetFilmStatus     (&g.rFS);
  if (g.rFS.byHolderType != 1)
    {
    spout ("No negative film holder\r\n");
    goto release;
    }
  if (fs4k_WBPS () < 14)
    {
    spout ("Can't TUNE when bits/sample < 14\r\n");
    goto release;
    }

//----  Load basic info so we can change it

  if (fs4k_TuneBegin    (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneShutters (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneAOffsets (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneAGains   (iTuneSpeed, iScanLines) ) goto release;
  if (fs4k_TuneDOffsets (iTuneSpeed, 100       ) ) goto release;
  if (fs4k_TuneDGains   (iTuneSpeed, 100       ) ) goto release;

//----  Set limits for brightest we accept as film

  iLimit [0] = 64000 * 3 / 4;
  iLimit [1] = 64000 * 1 / 3;
  iLimit [2] = 64000 * 1 / 4;

//----  Find an unexposed film strip.  Check between frames 1 and 2.

  fs4k_NewsStep ("Determine base colour");
  fs4k_SetScanMode (iTuneSpeed, fs4k_U24 ());
  for (wColour = 0; wColour < 4; wColour++)
    iBestValue [wColour] = 0;
  fs4000_set_thumbnail_window (4000,            // UINT2 x_res,
                               160,             // UINT2 y_res,
                               0,               // UINT4 x_upper_left,
                               0,               // UINT4 y_upper_left,
                               4000,            // UINT4 width,
                               312,             // UINT4 height,
                               fs4k_WBPS ());   // BYTE bits_per_pixel);
  fs4000_scan_for_thumbnail ();
  fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), FALSE, 0, NULL);
  for (wLine = 0; wLine < (WORD) g.rScan.dwLines; wLine++)
    {
    if (wLine < 280) continue;
    if ((wLine > 300) && (wLine < 746)) continue;

    pwSample = (WORD*) &g.rScan.pBuf [wLine * g.rScan.dwLineBytes];
    pwSample [0] = AvgOfWords (&pwSample [g.iMargin  ], 3, 4000);
    pwSample [1] = AvgOfWords (&pwSample [g.iMargin+1], 3, 4000);
    pwSample [2] = AvgOfWords (&pwSample [g.iMargin+2], 3, 4000);
//  wsprintf (msg, "Line %3d, %5d %5d %5d\r\n", wLine,
//                 pwSample [0], pwSample [1], pwSample [2]);
//  spout ();
    iTemp = 0;
    for (wColour = 0; wColour < 3; wColour++)
      {
      if (pwSample [wColour] > iLimit [wColour])
        pwSample [wColour] = 0;
      if (pwSample [wColour] > iBestValue [wColour])
        {
        iBestValue [wColour] = pwSample [wColour];
        iBestLine  [wColour] = wLine;
        }
      iTemp += pwSample [wColour];
      }
    if (iTemp > iBestValue [3])
      {
      iBestValue [3] = iTemp;
      iBestLine  [3] = wLine;
      }
    }

  wsprintf (msg, "Line %3d %3d %3d, %5d %5d %5d\r\n",
                 iBestLine  [0], iBestLine  [1], iBestLine  [2],
                 iBestValue [0], iBestValue [1], iBestValue [2]);
  spout ();
  wsprintf (msg, "Total %3d, %5d\r\n",
                 iBestLine  [3],
                 iBestValue [3]);
  spout ();

//      Find the maximum wanted gain so we can reduce all wanted gains
//      if required to avoid any of them exceeding the limit.

  iGainLimit = iShutterLimit * fs4k_CalcAGain (63);     // max possible
  iGainMax = iGainLimit;
  for (wColour = 0; wColour < 3; wColour++)             // will any exceed
    {                                                   // the max ?
    dTemp        = g.iShutter [wColour];
    dTemp       *= fs4k_CalcAGain (g.iAGain [wColour]);
    dTemp       *= 64000;
    dTemp       /= iBestValue [wColour];
    iWantedGain  = dTemp;
    if (iWantedGain > iGainMax)
      iGainMax = iWantedGain;
    }

//      Calc wanted gain and shutter setting for each colour.

  for (wColour = 0; wColour < 3; wColour++)
    {
    iCurrentGain = g.iShutter [wColour] *               // what we have
                   fs4k_CalcAGain (g.iAGain [wColour]);
    dTemp        = iCurrentGain;                        // what we want
    dTemp       *= 64000;
    dTemp       /= iBestValue [wColour];
    dTemp       *= iGainLimit;
    dTemp       /= iGainMax;
    iWantedGain  = dTemp;
    iShutterMax  = g.iShutter [wColour] * 40000 / iBestValue [wColour];
    if (iShutterMax > iShutterLimit)
      iShutterMax   = iShutterLimit;

//  wsprintf (msg, "%c  CurrentGain %6d, WantedGain %6d, ShutterMax %4d\r\n",
//                  "RGB" [wColour],
//                     iCurrentGain,    iWantedGain,    iShutterMax);
//  spout ();

    for (iAGainEnt = 0; iAGainEnt < 63; iAGainEnt++)
      if (fs4k_CalcAGain (iAGainEnt) >= iWantedGain / iShutterMax)
        break;

    g.iAGain   [wColour] = iAGainEnt;
    g.iShutter [wColour] = iWantedGain / fs4k_CalcAGain (iAGainEnt);
    g.iBoost   [wColour] = 256;
    if (g.iShutter [wColour] > iShutterLimit)
      {
      g.iBoost  [wColour] *= g.iShutter [wColour];
      g.iBoost  [wColour] /= iShutterLimit;
      g.iShutter [wColour] = iShutterLimit;
      if (g.iBoost [wColour] != 256)            // shouldn't happen now !!!
        spout ("Oops, boost reqd !!!\r\n");
      }
    }

  wsprintf (msg, "Shutter %4d %4d %4d\r\n",
                 g.iShutter [0], g.iShutter [1], g.iShutter [2]);
  spout ();
  wsprintf (msg, "AGain   %4d %4d %4d\r\n",
                 g.iAGain   [0], g.iAGain   [1], g.iAGain   [2]);
  spout ();
  wsprintf (msg, "Boost   %4d %4d %4d\r\n",
                 g.iBoost   [0], g.iBoost   [1], g.iBoost   [2]);
  spout ();

//      Don't get digital offsets here as we get them before each scan

#if !GET_DIG_OFFS_AT_SCAN

  if (fs4k_TuneDOffsets (iTuneSpeed, 100       )        ) goto release;

#endif

  g.bNegMode = TRUE;

release:
  RestoreFS4000Debug  ();
  fs4000_control_led  (0);
  fs4000_release_unit ();
  fs4k_NewsDone       ();
  return AbortWanted  ();
  }


/*--------------------------------------------------------------

                Thumbnail scan with TIF output

        'Fs4000.exe tune out8 thumb'

        The normal approach here is to scan in 14-bit mode with
        output set to 8-bit gamma adjusted.

--------------------------------------------------------------*/

int     FS4_Thumbnail           (LPSTR pTifFID)
  {
        int             iLines;

  fs4k_NewsTask          ("Thumbnail");
  fs4000_reserve_unit    ();
  fs4000_control_led     (2);
  fs4000_test_unit_ready ();

  fs4k_GetFilmStatus (&g.rFS);
  if (g.rFS.byHolderType == 0)
    {
    spout ("No film holder\r\n");
    goto release;
    }
  iLines = (g.rFS.byHolderType == 1 ? 1480 : 1448);

  fs4k_LampOn (15);
  if (FS4_Halt ()) goto release;

  fs4k_SetFrame (1);                            // L2R (for app not scanner)
  fs4000_move_position (0,                      // BYTE unknown1,
                        0,                      // BYTE unknown2,
                        0);                     // UINT2 position);
  if (FS4_Halt ()) goto release;

//fs4k_MoveHolder (394 * 2);                    // mid of frame #2
//fs4k_SetScanMode (iSpeed, fs4k_U24 ());
//fs4000_execute_afae (1, 0, 0, 0, 500, 3500);
  fs4000_execute_afae (2, 0, 0, 128, 0, 0);     // default focus

  fs4000_set_thumbnail_window (160,             // UINT2 x_res,
                               160,             // UINT2 y_res,
                               0,               // UINT4 x_upper_left,
                               0,               // UINT4 y_upper_left,
                               4000,            // UINT4 width,
                               iLines,          // UINT4 height,
                               fs4k_WBPS ());   // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  if (fs4k_SetScanMode (g.iSpeed, fs4k_U24 ()))
    goto release;
  if (FS4_Halt ()) goto release;

  fs4k_NewsStep ("Reading");
  fs4000_scan_for_thumbnail ();
  if (fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), TRUE, 160, NULL) == 0)
    {
    fs4k_NewsStep ("Processing");
    if (g.bSaveRaw)
      fs4k_DumpRaw (g.szRawFID, &g.rScan);
    if (g.bMakeTiff)
      fs4k_SaveScan (pTifFID, &g.rScan, 160);
    }

  if (FS4_Halt ()) goto release;

release:
  fs4000_control_led  (0);
  fs4000_release_unit ();
  fs4k_NewsDone       ();
  return AbortWanted  ();
  }


/*--------------------------------------------------------------

                Scan a frame

        'Fs4000.exe tune out8 ae scan 1'

For best scan quality :-
a) Gamma table is now linear from 0 to 32 (0 to 8 in 8-bit output).
b) 14-bit input is now converted to 16-bit in read stage.
c) Setting reads to 16-bit (rather than 14) reduces chance of gappy histogram.
d) General boost now reduces scan speed to achieve effect as far as possible
   and then adjusts multipliers to achieve final effect.
e) Individual boost (R or G or B) uses multiplier to achieve effect.

We use gamma 2.2 for 8-bit output as it is *the* standard.  Gamma 1.5 would be
better given the realistic Dmax (about 3) of the scanner.

It is better to achieve exposure and colour balance by specifying boosts here
rather than adjusting the scan file in Photoshop, etc.

The holder offsets listed below for each frame are all 36 more than the
offset of the frame in a thumbnail view.  This is probably because the
carriage advances a bit (past the home sensor) before the scan data
collection starts.

--------------------------------------------------------------*/

int     FS4_Scan                (LPSTR pTifFID, int iFrame, BOOL bAutoExp)
  {
        int             iNegOff [6] = { 600, 1080, 1558, 2038, 2516, 2996};
        int             iPosOff [4] = { 552, 1330, 2110, 2883};
        int             iOffset, iSaveSpeed, iSaveShutter [3];
        int             iSetFrame;
        char            szRawFID [256], *pRawFID;

  wsprintf (msg, "Scan frame %d", iFrame + 1);
  fs4k_NewsTask (msg);

  iSaveSpeed = g.iSpeed;                        // save globals
  iSaveShutter [0] = g.iShutter [0];
  iSaveShutter [1] = g.iShutter [1];
  iSaveShutter [2] = g.iShutter [2];

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_test_unit_ready ();
  fs4k_GetFilmStatus (&g.rFS);
  switch (g.rFS.byHolderType)
    {
    case 1:     if (iFrame > 5)
                  goto release;
                iOffset = iNegOff [iFrame];
                break;

    case 2:     if (iFrame > 3)
                  goto release;
                iOffset = iPosOff [iFrame];
                break;

    default:    spout ("No film holder\r\n");
                fs4000_set_lamp (0, 0);
                goto release;
    }

  fs4k_LampOn (15);                             // lamp on for 15 secs min
  if (FS4_Halt ()) goto release;

  fs4k_SetFrame (0);                            // may help home func below
  fs4000_move_position (0, 0, 0);               // carriage home
  fs4000_move_position (1, 4, iOffset - 236);   // focus position
  if (FS4_Halt ()) goto release;

  fs4k_NewsStep ("Focussing");
  fs4k_SetScanMode (4, fs4k_U24 ());            // mid exposure
  fs4000_execute_afae (1, 0, 0, 0, 500, 3500);
  if (FS4_Halt ()) goto release;

  fs4000_move_position (1, 4, iOffset);

  iSetFrame = 0;                                // R2L
  if (bAutoExp)                                 // exposure pre-pass
    {
    fs4k_SetFrame ( 8);                         // R2L, no return after
    fs4k_SetWindow (4000,                       // UINT2 x_res,
                    500,                        // UINT2 y_res,
                    0,                          // UINT4 x_upper_left,
                    0,                          // UINT4 y_upper_left,
                    4000,                       // UINT4 width,
                    5904,                       // UINT4 height,
                    fs4k_WBPS ());              // BYTE bits_per_pixel);
    g.iSpeed = g.iAutoExp;
    fs4k_SetScanMode (g.iSpeed, fs4k_U24 ());
    if (FS4_Halt ()) goto release;

    fs4k_NewsStep ("Exposure pass");
    fs4000_scan ();
    if (fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), TRUE, 500, NULL))
      goto release;
    if (FS4_Halt ()) goto release;
    fs4k_NewsStep ("Calc exposure");
    fs4k_CalcSpeed (&g.rScan);
    iSetFrame = 1;                              // L2R
    }

#if GET_DIG_OFFS_AT_SCAN

  fs4k_LampOff (1);
  if (fs4k_TuneDOffsets (g.iSpeed, 100)) goto release;
  fs4k_LampOn  (3);

#endif

  fs4k_SetFrame (iSetFrame);                    // L2R or R2L

  wsprintf (msg, "Frame %d, speed = %d, red = %d, green = %d, blue = %d\r\n",
                 iFrame + 1, g.iSpeed, g.iShutter [0], g.iShutter [1], g.iShutter [2]);
  spout ();

  fs4k_SetScanMode (g.iSpeed, fs4k_U24 ());
  fs4k_SetWindow (4000,                         // UINT2 x_res,
                  4000,                         // UINT2 y_res,
                  0,                            // UINT4 x_upper_left,
                  0,                            // UINT4 y_upper_left,
                  4000,                         // UINT4 width,
                  5904,                         // UINT4 height,
                  fs4k_WBPS ());                // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  GetNextSpareName (szRawFID, g.szRawFID);
  pRawFID = szRawFID;

  fs4k_NewsStep ("Reading");
  fs4000_scan ();

  if (!(g.bSaveRaw || g.bUseHelper))
    pRawFID = NULL;
  if (fs4k_ReadScan (&g.rScan, 3, fs4k_WBPS (), TRUE, 4000, pRawFID) == 0)
    {
    fs4k_NewsStep ("Processing");
    if (g.bMakeTiff)
      if (g.bUseHelper)
        {
        fs4k_FreeBuf (&g.rScan);
        fs4k_StartMake (pTifFID, szRawFID);
        }
      else
        fs4k_SaveScan (pTifFID, &g.rScan, 4000);
    }
  FS4_Halt ();

release:
  fs4k_SetFrame        (0);
  fs4000_move_position (0, 0, 0);
  fs4000_control_led   (0);
  fs4000_release_unit  ();

  g.iSpeed = iSaveSpeed;                        // restore globals
  g.iShutter [0] = iSaveShutter [0];
  g.iShutter [1] = iSaveShutter [1];
  g.iShutter [2] = iSaveShutter [2];

  fs4k_NewsDone      ();
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Code for testing and debug

--------------------------------------------------------------*/

/*--------------------------------------------------------------

        Position holder (absolute and relative)

unknown1        0 = carriage, 1 = holder
unknown2        0 = home
                1 = eject (only for holder)
                2 = rel position (+offset)
                3 = rel position (-offset)
                4 = abs position (=offset)

Carriage position is in 8000dpi increments.
Holder position is in 360dpi increments.
Reverse relative pos to pos < 50 causes fs4000 to eject holder.
- fs4000 freaks if home sensed during positioning.
Warning :- if carriage not homed before holder homed, holder hits case.
Note :- can't relatively position carriage when it is homed.
        can   relatively position holder   when it is homed.
Don't know how to get current carriage position.

--------------------------------------------------------------*/

int     FS4_Position            (void)
  {
        BYTE            byFilmHolder;
        BYTE            byNumFrames;
        WORD            uwFilmPosition;
        BYTE            byFocusPosition;

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_test_unit_ready ();
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (byFilmHolder == 0)
    {
    spout ("No film holder\n");
    goto release;
    }

//----  Carriage home

  fs4000_move_position (0,                      // BYTE unknown1,
                        0,                      // BYTE unknown2,
                        0);                     // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder home

  fs4000_move_position (1,                      // BYTE unknown1,
                        0,                      // BYTE unknown2,
                        0);                     // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder out +1001

  fs4000_move_position (1,                      // BYTE unknown1,
                        2,                      // BYTE unknown2,
                        1001);                  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder out +1001 (again)

  fs4000_move_position (1,                      // BYTE unknown1,
                        2,                      // BYTE unknown2,
                        1001);                  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Carriage test

        int     x, y;

  fs4000_move_position (0, 2,   0);     // rejected as pos = 0
  fs4000_move_position (0, 2, 100);     // rel adj not allowed when pos = 0
  fs4000_move_position (0, 4, 100);     // abs adj OK
  fs4000_move_position (0, 2,   0);     // allowed as pos > 0 (does nothing)
  if (FS4_Halt ()) goto release;

  for (x = 20000, y = 100; x > 0; )             // find maximum pos
    {
//  if (fs4000_move_position (0, 4, y + x))
    if (fs4000_move_position (0, 2, x))
      x /= 2;
    else
      y += x;
    }
  wsprintf (msg, "y = %d\n", y);
  spout ();
  if (FS4_Halt ()) goto release;


//----  Carriage in 5000

  fs4000_move_position (0,                      // BYTE unknown1,
                        4,                      // BYTE unknown2,
                        5000);                  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Carriage in another 5000

  fs4000_move_position (0,                      // BYTE unknown1,
                        2,                      // BYTE unknown2,
                        5000);                  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Carriage in another 1807 (11807 = max allowed)

  fs4000_move_position (0,                      // BYTE unknown1,
                        2,                      // BYTE unknown2,
                        1807);                  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Carriage out 11307 (back to 500)

  fs4000_move_position (0,                      // BYTE unknown1,
                        3,                      // BYTE unknown2,
                        11307);                 // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder back to 100

  fs4000_move_position (1,                      // BYTE unknown1,
                        3,                      // BYTE unknown2,
                        uwFilmPosition - 100);  // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder back to 50

  fs4000_move_position (1,                      // BYTE unknown1,
                        3,                      // BYTE unknown2,
                         50);                   // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

//----  Holder back to home

  fs4000_move_position (1,                      // BYTE unknown1,
                        0,                      // BYTE unknown2,
                        0);                     // UINT2 position);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

release:
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


/*--------------------------------------------------------------

        Test for frame, etc

SetFrame.unknown1
  bits 7-5     don't care; probably LUN
  bit  4       must be 0
  bit  3       1 = don't home carriage after scan
  bits 2-0     0 = R2L scan
               1 = L2R scan
               2 = scan rightmost line (width times)
               3 = scan leftmost  line (width times)
               4 = scan current   line (width times)
        2 & 3 position carriage before scan but don't move during scan,
        4 doesn't move carriage at all.

--------------------------------------------------------------*/

int     FS4_FrameTest           (WORD wPrePos, BYTE byFrame)
  {
        BYTE            byFilmHolder;
        BYTE            byNumFrames;
        WORD            uwFilmPosition;
        BYTE            byFocusPosition;
        BYTE            byWhLampOn;
        UINT4           dwWhLampTime;
        BYTE            byIRLampOn;
        UINT4           dwIRLampTime;

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_set_lamp (1, 0);
  fs4000_move_position (1, 4, 590);
  fs4000_move_position (0, 4, wPrePos);         // pos carriage
  fs4000_set_frame (byFrame);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  fs4k_SetScanMode (0, 3);
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1000,                      // UINT4 height,
                     8 );                       // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  fs4000_scan ();
  fs4k_ReadScan (&g.rScan, 3, 8, FALSE, 0, NULL);
//  fs4k_SaveScan (pTifFID, &g.rScan, 500, (!(byFrame & 0x01)));
  FS4_Halt ();

release:
  fs4000_move_position (0, 0, 0);
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


/*--------------------------------------------------------------

        Auto-focus, auto-exposure test

FS4000_EXECUTE_AFAE_DATA_OUT
  BYTE unknown1 [0]     bits 7-3 don't care
                        bits 2-0 0 = apparently do nothing      ???
                                 1 = get AE longs, calc focus
                                 2 = get AE longs, focus = focus_position
                                 3 = invalid                    ???
  BYTE unknown1 [1]     ???
  BYTE unknown1 [2]     ???
  BYTE focus_position
  WORD unknown2         first pixel in line for AF/AE input
  WORD unknown3         last pixel + 1 in line for AF/AE input
                        (e.g. 500 - 3500 = check from 500 to 3499)

FS4000_GET_FILM_STATUS_DATA_IN_28
  BYTE film_holder_type
  BYTE num_frames
  WORD film_position
  BYTE unknown1 [3]
  BYTE focus_position
  WORD unknown2         AF/AE start pixel
  WORD unknown3         AF/AE end   pixel
  LONG unknown4 [3]     RGB totals from last AF/AE      ???
  BYTE unknown5 [1]     min exposure (4 bits) / max exposure (4 bits) ???
  BYTE unknown6 [3]     RGB best focus from last AF/AE with calc focus


RGB totals could be sum of samples for each colour in AF/AE area.
- this seems to be the case
RGB totals could be min/max sample for each colour in AF/AE area.
- values seen conflict with this idea

--------------------------------------------------------------*/

int     FS4_AfAeTest            (void)
  {
        BYTE            byFilmHolder;
        BYTE            byNumFrames;
        WORD            uwFilmPosition;
        BYTE            byFocusPosition;
        BYTE            byWhLampOn;
        UINT4           dwWhLampTime;
        BYTE            byIRLampOn;
        UINT4           dwIRLampTime;
        WORD            *pInSample, *pOutSample;
        int             x, y, z, iSample;
//      UINT2           wBlackPos, wWhitePos;
        UINT2           wXRes;                  // UINT2 *x_res,
        UINT2           wYRes;                  // UINT2 *y_res,
        UINT4           dwXStart;               // UINT4 *x_upper_left,
        UINT4           dwYStart;               // UINT4 *y_upper_left,
        UINT4           dwWidth;                // UINT4 *width,
        UINT4           dwHeight;               // UINT4 *height,
        BYTE            byBitsPerPixel;         // BYTE *bits_per_pixel);
        int             iTotal [3];


  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_test_unit_ready ();
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  switch (byFilmHolder)
    {
    case 1: //  wBlackPos = 2 * 37;
            //  wWhitePos = 2 * 16;
                break;

    case 2: //  wBlackPos = 2 * 0;
            //  wWhitePos = 2 * 335;
                break;

    default:    spout ("No film holder\n");
                fs4000_set_lamp (0, 0);
                goto release;
    }
  fs4000_set_lamp (1,                           // BYTE visible,
                   0);                          // BYTE infrared);
  fs4000_get_lamp (&byWhLampOn,                 // BYTE *is_visible_lamp_on,
                   &dwWhLampTime,               // UINT4 *visible_lamp_duration,
                   &byIRLampOn,                 // BYTE *is_infrared_lamp_on,
                   &dwIRLampTime);              // UINT4 *infrared_lamp_duration);
  if (dwWhLampTime < 3)
    Sleep (1000 * (3 - dwWhLampTime));

  fs4000_move_position (0, 0, 0);               // carriage home
//fs4000_move_position (1, 0, 0);               // holder home
//if (FS4_Halt ()) goto release;

  fs4000_move_position (1, 4, 670); // 1332 - 260);
//fs4000_move_position (1, 4, wWhitePos);
//fs4000_move_position (1, 4, wBlackPos);
//fs4000_move_position (1, 0, 0);               // holder home
//if (FS4_Halt ()) goto release;

#if 0
  for (int x = 0; x <  32; x++)
    {
    fs4000_set_frame ( x);                      // 5 bit param
    fs4k_SetScanMode (0, fs4k_U24 ());
    fs4000_execute_afae (2, x, x+1, 75, 500, 3500); // 75 = min focus
    fs4k_ShowFilmStatus ();
    if (FS4_Halt ()) goto release;
    }
#endif

#if 0
  for (int x = 0; x < 13; x++)
    {
    if ((x % 2) && (x > 6))
      continue;
    fs4k_SetScanMode (x, fs4k_U24 ());          // speed affects afae
    fs4000_execute_afae (1, 0, 0, 0, 500, 3500);
    fs4k_ShowFilmStatus ();
    if (FS4_Halt ()) goto release;
    }
#endif

  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  fs4000_execute_afae (2, 0, 0, byFocusPosition,   0,    1);
#if 0
  fs4000_execute_afae (0,   0,   0,   0,    1,    1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  fs4000_execute_afae (1,   0,   0,   0,    1,    1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (3, 120, 130, 140,  150,  160);
  if (FS4_Halt ()) goto release;
  fs4000_execute_afae (2,   0,   0,   0,    0,    1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;
#endif

  fs4000_get_window (&wXRes,                    // UINT2 *x_res,
                     &wYRes,                    // UINT2 *y_res,
                     &dwXStart,                 // UINT4 *x_upper_left,
                     &dwYStart,                 // UINT4 *y_upper_left,
                     &dwWidth,                  // UINT4 *width,
                     &dwHeight,                 // UINT4 *height,
                     &byBitsPerPixel);          // BYTE *bits_per_pixel);
  fs4000_set_window (wXRes,                     // UINT2 x_res,
                     wYRes,                     // UINT2 y_res,
            10, //   500,                       // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
            20, //   3000,                      // UINT4 width,
                     1,                         // UINT4 height,
              14);// byBitsPerPixel);           // BYTE bits_per_pixel);
  fs4000_get_window (&wXRes,                    // UINT2 *x_res,
                     &wYRes,                    // UINT2 *y_res,
                     &dwXStart,                 // UINT4 *x_upper_left,
                     &dwYStart,                 // UINT4 *y_upper_left,
                     &dwWidth,                  // UINT4 *width,
                     &dwHeight,                 // UINT4 *height,
                     &byBitsPerPixel);          // BYTE *bits_per_pixel);
  fs4000_set_frame (12);

  for (x = 0; x < 1001; x+=250)
    {
    g.iAGain [0] =  0;
    g.iAGain [1] =  0;
    g.iAGain [2] =  0;
    g.iAOffset [0] = g.iAOffset [1] = g.iAOffset [2] = 0;
    g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = x;

    ChangeFS4000Debug (0);
    fs4k_SetScanMode (0, 0);
    fs4000_scan ();
    fs4k_ReadScan (&g.rScan, 3, byBitsPerPixel, FALSE, 0, NULL);
    RestoreFS4000Debug ();

    wsprintf (msg, "SensorGain = %d/1000\n", g.iShutter [0]);
    spout ();
    wsprintf (msg, "margin %08X  %08X  %08X  = %08X  %08X  %08X\n",
                 SumOfWords (&((WORD*) g.rScan.pBuf) [  0], 3, 40),
                 SumOfWords (&((WORD*) g.rScan.pBuf) [  1], 3, 40),
                 SumOfWords (&((WORD*) g.rScan.pBuf) [  2], 3, 40),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [  0], 3, 40),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [  1], 3, 40),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [  2], 3, 40));
    spout ();
    wsprintf (msg, "scanned %08X  %08X  %08X  = %08X  %08X  %08X\n",
                 SumOfWords (&((WORD*) g.rScan.pBuf) [120], 3, dwWidth),
                 SumOfWords (&((WORD*) g.rScan.pBuf) [121], 3, dwWidth),
                 SumOfWords (&((WORD*) g.rScan.pBuf) [122], 3, dwWidth),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [120], 3, dwWidth),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [121], 3, dwWidth),
                 AvgOfWords (&((WORD*) g.rScan.pBuf) [122], 3, dwWidth));
    spout ();
    wsprintf (msg, "Min/max       %04X/%04X %04X/%04X %04X/%04X\n",
                    MinOfWords (&((WORD*) g.rScan.pBuf) [120], 3, dwWidth),
                    MaxOfWords (&((WORD*) g.rScan.pBuf) [120], 3, dwWidth),
                    MinOfWords (&((WORD*) g.rScan.pBuf) [121], 3, dwWidth),
                    MaxOfWords (&((WORD*) g.rScan.pBuf) [121], 3, dwWidth),
                    MinOfWords (&((WORD*) g.rScan.pBuf) [122], 3, dwWidth),
                    MaxOfWords (&((WORD*) g.rScan.pBuf) [122], 3, dwWidth));
    spout ();
    for (y = 0; y <  1; y++)
      {
      wsprintf (msg, "Diff %d totals  %08X  %08X  %08X\n", y,
                   SumOfWordDiffs (&((WORD*) g.rScan.pBuf) [120], 3, dwWidth, y),
                   SumOfWordDiffs (&((WORD*) g.rScan.pBuf) [121], 3, dwWidth, y),
                   SumOfWordDiffs (&((WORD*) g.rScan.pBuf) [122], 3, dwWidth, y));
      spout ();
      }

    fs4000_execute_afae (2, 0, 0, byFocusPosition, dwXStart, dwXStart + dwWidth);
//  fs4000_execute_afae (2, 0, 0, byFocusPosition, 0, 4000);
    fs4000_get_film_status (0,                    // int shorter,
                            &byFilmHolder,        // BYTE *film_holder,
                            &byNumFrames,         // BYTE *num_frames,
                            &uwFilmPosition,      // UINT2 *film_position,
                            &byFocusPosition);    // BYTE *focus_position);
    if (FS4_Halt ()) goto release;
    }
#if 0
//fs4000_execute_afae (2, 0, 0, byFocusPosition, 4000, 4000);
//fs4000_execute_afae (2, 0, 0, byFocusPosition, 4000, 5000);
//fs4000_execute_afae (2, 0, 0, byFocusPosition, 5000, 4000);
//if (FS4_Halt ()) goto release;

  fs4000_execute_afae (1,   0,   0,   0,    0, 1000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (0,   0,   0,   0, 1000, 2000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (0, 120, 130, 140, 1000, 2000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (0, 151, 152, 153, 0, 1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (1, 0, 0, 100, 0, 1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae ( 2, 82, 83, 100, 0, 1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition,   0,    1);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition,    0, 2000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition, 2000, 4000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition,    0, 2000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;

  fs4000_execute_afae (2, 0, 0, byFocusPosition,    0, 2000);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
  if (FS4_Halt ()) goto release;
#endif
release:
  fs4000_move_position (0, 0, 0);
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


/*--------------------------------------------------------------

        Test for SetScanMode

  Make scan files (portion of slide 2) using different Unknown6
  and Unknown2 [4] settings.

  Unknown2 [4] specifies the processing on each sample.  It seems that each
  pixel is read 4 times and the results are summed giving 3 16-bit samples.
  Each 16-bit little-endian sample can be shifted right 2 to create a 14-bit
  sample (14-bit mode) or byte-swapped so the leftmost byte is output (8-bit
  mode).

  BYTE unknown1 [0]     0x00 - 0xFF valid  (always 0x25 on read)
                [1]                        (always 0x00 on read)
                [2]                        (always 0x00 on read)
                [3]                        (always 0x00 on read)
                [4]     0x20        valid
                [5]     0x20        valid
                [6]     0x00 - 0x03 valid
                        0x80 - 0x83 valid
                [7]     0x00        valid
                [8]     anything    valid
                [9]     anything    valid
                [10]    0x00 - 0x03 valid
                        0x10 - 0x13 valid
                        0x20 - 0x23 valid
                        0x30 - 0x33 valid
                [11]    anything    valid
                [12]    anything    valid
                [13]    anything    valid
                [14]    0x00 - 0x03 valid (always 0x00 on read)

  BYTE unknown2 [0]     0x00, 0x01  valid (always 0x01 on read)
                [1]     0x00 - 0xFF valid (always 0x19 on read)
                [2]     0  valid
                [3]     0  valid
                [4] b0  1 = swap bytes (create big-endian)
                    b1  1 = don't shift right 2
                    b4  1 = sample = min (sample << 2, 0xFFFF)
                    b5  1 = no black margin, reduce sample values slightly

                    when 8-bit mode
                        x00     noisy, perhaps low 8 bits of 14 bit sample
                        x03     normal
                        x10     noisy, different from 00
                        x13     normal
                        x20     invert ???                      no black margin
                        x23     as per 03, samples -= 10        no black margin
                        x30     invert ???                      no black margin
                        x33     as per 03, samples -= 10        no black margin
                    when 14-bit mode
                        x00     normal
                        x01     big-endian, 16383 max
                        x02     normal * 4
                        x03     big-endian * 4
                        x10     normal * 4 but limited to 16383
                        x11     big-endian, 16383 max
                        x12     normal * 16, 65532 max
                        x13     same as 03
                        x20     as per 10, samples -= 686       no black margin
                        x21     as per 11, samples -= 686       no black margin
                        x22     as per 12, samples -= 686       no black margin
                        x23     as per 13, samples -= 686       no black margin
                        x30     as per 10, samples -= 686       no black margin
                        x31     as per 11, samples -= 686       no black margin
                        x32     as per 12, samples -= 686       no black margin
                        x33     as per 13, samples -= 686       no black margin
                [5]     0 - 255 valid
  BYTE unknown6
                when 8-bit mode
                        0   = normal
                        1   = output sample = input sample / 2
                        2   = output sample = input sample / 2 * 2      ???
                when 14-bit mode
                        0   = output sample = 14-bit sample         0 - 16383
                        1   = output sample = dual 14-bit sample    0 - 32766
                        2   = output sample = quad 14-bit sample    0 - 65532
                when either mode
                        129 = image softening (min)
                        130 = image softening (more)
                        131 = image softening (more++)
                        132 = image softening (max)

--------------------------------------------------------------*/

int     FS4_ScanModeTest        (LPSTR pFID)
  {
        BYTE            byFilmHolder;
        BYTE            byNumFrames;
        WORD            uwFilmPosition;
        BYTE            byFocusPosition;

        UINT2           wXRes;
        UINT2           wYRes;
        UINT4           dwXStart;
        UINT4           dwYStart;
        UINT4           dwWidth;
        UINT4           dwHeight;
        BYTE            byBitsPerPixel;

        BYTE            byWhLampOn;
        UINT4           dwWhLampTime;
        BYTE            byIRLampOn;
        UINT4           dwIRLampTime;

        char            cFID [256];
        int             iUnk2, iUnk6;

	DWORD           x;

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_set_lamp (1, 0);
//fs4000_move_position (1, 4,  700 - 260);
  fs4000_move_position (1, 4, 1332 - 260);
  fs4000_move_position (0, 0, 0);
  fs4000_get_lamp (&byWhLampOn,                 // BYTE *is_visible_lamp_on,
                   &dwWhLampTime,               // UINT4 *visible_lamp_duration,
                   &byIRLampOn,                 // BYTE *is_infrared_lamp_on,
                   &dwIRLampTime);              // UINT4 *infrared_lamp_duration);
  if (dwWhLampTime < 3)
    Sleep (1000 * (3 - dwWhLampTime));
  fs4000_set_frame (0);
  fs4000_execute_afae (1, 0, 0, 0, 500, 3500);
  fs4000_get_film_status (0,                    // int shorter,
                          &byFilmHolder,        // BYTE *film_holder,
                          &byNumFrames,         // BYTE *num_frames,
                          &uwFilmPosition,      // UINT2 *film_position,
                          &byFocusPosition);    // BYTE *focus_position);
#if 1
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1016,                      // UINT4 height,
                     14);                       // BYTE bits_per_pixel);

  for (iUnk2 = 0, iUnk6 = 0; iUnk6 < 256; iUnk6++)
    {
    wsprintf (cFID, "%sEx%02Xx%02X.tif", pFID, iUnk2, iUnk6);
    if (GetFileAttributes (cFID) != 0xFFFFFFFF)
      continue;
    if (fs4k_SetScanModeEx (0, iUnk2, iUnk6) == 0)
      {
      if (fs4000_scan () == 0)
        {
        ChangeFS4000Debug (0);
        fs4k_ReadScan (&g.rScan, 3, 14, FALSE, 0, NULL);
        RestoreFS4000Debug ();
        fs4k_SaveScan (cFID, &g.rScan, 4000);
        }
      if (FS4_Halt ()) goto release;
      }
    }
#endif
#if 1
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1016,                      // UINT4 height,
                     8 );                       // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  for (iUnk2 = 3, iUnk6 = 0; iUnk6 < 256; iUnk6++)
    {
    wsprintf (cFID, "%s8x%02Xx%02X.tif", pFID, iUnk2, iUnk6);
    if (GetFileAttributes (cFID) != 0xFFFFFFFF)
      continue;
    if (fs4k_SetScanModeEx (0, iUnk2, iUnk6) == 0)
      {
      if (fs4000_scan () == 0)
        {
        ChangeFS4000Debug (0);
        fs4k_ReadScan (&g.rScan, 3,  8, FALSE, 0, NULL);
        RestoreFS4000Debug ();
        fs4k_SaveScan (cFID, &g.rScan, 4000);
        }
      if (FS4_Halt ()) goto release;
      }
    }
#endif
#if 1
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1016,                      // UINT4 height,
                     8 );                       // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  for (iUnk2 = 0, iUnk6 = 0; iUnk2 < 256; iUnk2++)
    {
    wsprintf (cFID, "%s8x%02Xx%02X.tif", pFID, iUnk2, iUnk6);
    if (GetFileAttributes (cFID) != 0xFFFFFFFF)
      continue;
    if (fs4k_SetScanModeEx (0, iUnk2, iUnk6) == 0)
      {
      if (fs4000_scan () == 0)
        {
        ChangeFS4000Debug (0);
        fs4k_ReadScan (&g.rScan, 3, 8, FALSE, 0, NULL);
        RestoreFS4000Debug ();
#if 0                   // bit order reverse
        if ((iUnk2 & 0x0F) == 0)
          for (DWORD x = 0; x < g.rScan.dwBufSize; x++)
            {
            BYTE byB = g.rScan.pBuf [x];
            for (int y = 0; y < 8; y++)
              {
              g.rScan.pBuf [x] <<= 1;
              g.rScan.pBuf [x] |= (byB & 0x01);
              byB >>= 1;
              }
            }
#endif
        fs4k_SaveScan (cFID, &g.rScan, 4000);
        }
      if (FS4_Halt ()) goto release;
      }
    }
#endif
#if 1
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1016,                      // UINT4 height,
                     16);   // same as 14       // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  for (iUnk2 = 0, iUnk6 = 0; iUnk2 < 256; iUnk2++)
    {
    wsprintf (cFID, "%sEx%02Xx%02X.tif", pFID, iUnk2, iUnk6);
    if (GetFileAttributes (cFID) != 0xFFFFFFFF)
      continue;
    if (fs4k_SetScanModeEx (0, iUnk2, iUnk6) == 0)
      {
      if (fs4000_scan () == 0)
        {
        ChangeFS4000Debug (0);
        fs4k_ReadScan (&g.rScan, 3, 16, FALSE, 0, NULL);
        RestoreFS4000Debug ();
        if (iUnk2 & 0x01)               // if LSB, data is big-endian
          for (x = 0; x < g.rScan.dwBufSize - 1; x += 2)
            *(WORD*) (&g.rScan.pBuf [x]) = (WORD) g.rScan.pBuf [x+1] +
                                          ((WORD) g.rScan.pBuf [x] * 256);
        fs4k_SaveScan (cFID, &g.rScan, 4000);
        }
      if (FS4_Halt ()) goto release;
      }
    }
#endif

#if 0
        //      Unknown2 [0]    0, 1    accepted
        //               [1]    0 - 255 accepted
        //               [2]    0       accepted
        //               [3]    0       accepted
        //               [4]    tested above
        //               [5]    0 - 255 accepted


        FS4000_DEFINE_SCAN_MODE_DATA_OUT        ScanModeData;
        int             iUnk;

  NullMem (desc (ScanModeData));
  ScanModeData.unknown1 [4] = 0x20;
  ScanModeData.unknown1 [5] = 0x20;
  ScanModeData.unknown3 [0] = iAGain   [0];     // analogue gain
  ScanModeData.unknown3 [1] = iAGain   [1];
  ScanModeData.unknown3 [2] = iAGain   [2];
  ScanModeData.unknown4 [0] = iAOffset [0];     // analogue offset
  ScanModeData.unknown4 [1] = iAOffset [1];
  ScanModeData.unknown4 [2] = iAOffset [2];
  ScanModeData.unknown5 [0] = iShutter [0];     // CCD gain
  ScanModeData.unknown5 [1] = iShutter [1];
  ScanModeData.unknown5 [2] = iShutter [2];


  for (iUnk = 0; iUnk < 256; iUnk++)
    {
    ScanModeData.unknown2 [0] = iUnk;
    ChangeFS4000Debug (0);
    if (fs4000_define_scan_mode (&ScanModeData))
      continue;
    wsprintf (msg, "Unknown2 [0] = %02X accepted\n", iUnk);
    spout ();
    if (FS4_Halt ()) goto release;
    }
#endif

#if 0             // testing set_window unknown; 02, 12, 22, 32 allowed
  fs4000_set_window (4000,                      // UINT2 x_res,
                     4000,                      // UINT2 y_res,
                     0,                         // UINT4 x_upper_left,
                     0,                         // UINT4 y_upper_left,
                     4000,                      // UINT4 width,
                     1016,                      // UINT4 height,
                     8 );                       // BYTE bits_per_pixel);
  if (FS4_Halt ()) goto release;

  fs4k_CopyXlateTable ();
  for (iUnk2 = 0; iUnk2 < 4; iUnk2++)
    {
    wsprintf (cFID, "%sGamma%02X.tif", pFID, iUnk2);
    if (GetFileAttributes (cFID) != 0xFFFFFFFF)
      continue;
    fs4000_set_window (4000,                      // UINT2 x_res,
                       4000,                      // UINT2 y_res,
                       0,                         // UINT4 x_upper_left,
                       0,                         // UINT4 y_upper_left,
                       4000,                      // UINT4 width,
                       1016,                      // UINT4 height,
                       8,                         // BYTE bits_per_pixel);
                       iUnk2);                    // BYTE test
    if (fs4k_SetScanModeEx (2, 0x03, 0) == 0)
      {
      if (fs4000_scan () == 0)
        {
        fs4000_debug--;
        fs4k_ReadScan (&g.rScan, 3,  8, FALSE, 0, NULL);
        fs4000_debug++;
        fs4k_SaveScan (cFID, &g.rScan, 4000, 1);
        }
      }
    if (FS4_Halt ()) goto release;
    }
#endif

release:
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


int     FS4_Test                (void)
  {
        int             iColour, iIndex;
        int             iScanLines = 16;
        int             iTuneSpeed = 2;
        TUNE_STATS      Stat [6], *pStat;

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_test_unit_ready ();
  fs4k_TuneSetWinFrame (iScanLines);

//----  Reset offset and gain info

  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain   [iColour]  =    0;
    g.iAOffset [iColour]  =    0;
    g.iShutter [iColour]  =    0;
    }

  fs4k_LampOff (1);
  fs4k_MoveHolder (fs4k_GetBlackPos ());
  for (iIndex = 0; iIndex <= 1000; iIndex += 100)
    {
    g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = iIndex;
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    wsprintf (msg, "Shutter = %4d, R = %5u, G = %5u, B = %5u\n", iIndex,
              Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
    spout ();
    if (FS4_Halt ()) break;
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);

  fs4k_LampOn (5);
  for (iIndex = 0; iIndex <= 1000; iIndex += 100)
    {
    g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = iIndex;
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    wsprintf (msg, "Shutter = %4d, R = %5u, G = %5u, B = %5u\n", iIndex,
              Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
    spout ();
    if (FS4_Halt ()) break;
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);

  fs4k_LampOff (1);
  fs4k_MoveHolder (fs4k_GetWhitePos ());
  for (iIndex = 0; iIndex <= 1000; iIndex += 100)
    {
    g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = iIndex;
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    wsprintf (msg, "Shutter = %4d, R = %5u, G = %5u, B = %5u\n", iIndex,
              Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
    spout ();
    if (FS4_Halt ()) break;
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);

  fs4k_LampOn (5);
  for (iIndex = 0; iIndex <= 1000; iIndex += 100)
    {
    g.iShutter [0] = g.iShutter [1] = g.iShutter [2] = iIndex;
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    wsprintf (msg, "Shutter = %4d, R = %5u, G = %5u, B = %5u\n", iIndex,
              Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
    spout ();
    if (FS4_Halt ()) break;
    }
  fs4k_TuneRpt (iTuneSpeed, &g.rScan);

release:
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


int     FS4_Test1               (void)
  {
        int             iColour, iIndex;
        int             iScanLines = 16;
        int             iTuneSpeed = 2;
        TUNE_STATS      Stat [6], *pStat;

  fs4000_reserve_unit ();
  fs4000_control_led (2);
  fs4000_test_unit_ready ();
  ChangeFS4000Debug (0);
  fs4k_TuneSetWinFrame (iScanLines);

//----  Reset offset and gain info

  for (iColour = 0; iColour < 3; iColour++)
    {
    g.iAGain   [iColour]  =    0;
    g.iAOffset [iColour]  =    0;
    g.iShutter [iColour]  =  500;
    }

  fs4k_LampOn (5);
  fs4k_MoveHolder (300); // (fs4k_GetBlackPos ());
  for (iIndex = 75; iIndex <= 200; iIndex++)
    {
    fs4000_execute_afae (2, 0, 0, iIndex, 0, 0);
    fs4k_TuneRead (iTuneSpeed, &Stat [0]);
    wsprintf (msg, "Focus = %4d, R = %5u, G = %5u, B = %5u\n", iIndex,
              Stat [0].wAverage,  Stat [1].wAverage,  Stat [2].wAverage);
    spout ();
    if (FS4_Halt ()) break;
    }

release:
  RestoreFS4000Debug ();
  fs4000_control_led (0);
  fs4000_release_unit ();
  return 0;
  }


/*--------------------------------------------------------------

                Code to get and process arguments

--------------------------------------------------------------*/
#if 0
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
  return pChar;
  }


int     FS4_ProcessWord         (LPSTR pArg)
  {
static  BOOL            bNeedSpeed = FALSE;
static  BOOL            bNeedFrame = FALSE;
        int             x, y, z;

  wsprintf (msg, "--Token: %s\r\n", pArg);
  spout ();
  ReturnCode = RC_GOOD;                         // reset abort

//----          Parameter for last word ?

  if (bNeedSpeed)
    {
    bNeedSpeed = FALSE;
    y = atoi (pArg);
    if ((y > 0) && (y < 13) && (y != 7) && (y != 9) && (y != 11))
      g.iSpeed = y;
    else
      spout ("Speed must be 1 - 6, 8, 10, 12\r\n");
    goto bye;
    }

  if (bNeedFrame)
    {
    bNeedFrame = FALSE;
    y = atoi (pArg);
    if (y > 0)
      y--;
    FS4_Scan (g.szScanFID, y, g.bAutoExp);
    goto bye;
    }

//----          Modifiers

  if (strcasecmp (pArg, "ae") == 0)
    g.bAutoExp = TRUE;
  if (strcasecmp (pArg, "ae-") == 0)
    g.bAutoExp = FALSE;
  if (pArg [0] == '+')
    {
    y = (atoi (pArg) + 100) * 256 / 100;
    y *= g.iSpeed / 2;
    for (g.iSpeed = 2; y > 511; y -= 256)
      g.iSpeed += 2;
    for (z = 0; z < 3; z++)
      {
      g.iBoost [z] *= y;
      g.iBoost [z] /= 256;
      }
    }
  if (strncasecmp (pArg, "R+", 2) == 0)
    {
    pArg += 2;
    y = (atoi (pArg) + 100) * 256 / 100;
    g.iBoost [0] *= y;
    g.iBoost [0] /= 256;
    }
  if (strncasecmp (pArg, "G+", 2) == 0)
    {
    pArg += 2;
    y = (atoi (pArg) + 100) * 256 / 100;
    g.iBoost [1] *= y;
    g.iBoost [1] /= 256;
    }
  if (strncasecmp (pArg, "B+", 2) == 0)
    {
    pArg += 2;
    y = (atoi (pArg) + 100) * 256 / 100;
    g.iBoost [2] *= y;
    g.iBoost [2] /= 256;
    }
  if (strcasecmp (pArg, "debug") == 0)
    fs4000_debug = TRUE;
  if (strcasecmp (pArg, "export") == 0)
    g.bUseHelper = TRUE;
  if (strcasecmp (pArg, "export-") == 0)
    g.bUseHelper = FALSE;
  if (strcasecmp (pArg, "in8") == 0)
    fs4k_SetInMode (8);
  if (strcasecmp (pArg, "in14") == 0)
    fs4k_SetInMode (14);
  if (strcasecmp (pArg, "in16") == 0)
    fs4k_SetInMode (16);
  if (strcasecmp (pArg, "out16") == 0)
    g.pbXlate = NULL;
  if (strcasecmp (pArg, "out8") == 0)
    g.pbXlate = g.b16_822;
  if (strcasecmp (pArg, "out8i") == 0)
    g.pbXlate = g.b16_822i;
  if (strcasecmp (pArg, "raw") == 0)
    g.bSaveRaw = TRUE;
  if (strcasecmp (pArg, "raw-") == 0)
    g.bSaveRaw = FALSE;
  if (strcasecmp (pArg, "speed") == 0)
    bNeedSpeed = TRUE;
  if (strcasecmp (pArg, "step") == 0)
    g.bStepping = TRUE;
  if (strcasecmp (pArg, "testing") == 0)
    g.bTesting = TRUE;
  if (strcasecmp (pArg, "tif") == 0)
    g.bMakeTiff = TRUE;
  if (strcasecmp (pArg, "tif-") == 0)
    g.bMakeTiff = FALSE;

//----          Normal functions

  if (strcasecmp (pArg, "dump") == 0)
    fs4k_DumpCalInfo ("calinfo");
  if (strcasecmp (pArg, "eject") == 0)
    {
    fs4000_move_position (0, 0, 0);
    fs4000_move_position (1, 1, 0);
    }
  if (strcasecmp (pArg, "home") == 0)
    {
    fs4000_move_position (0, 0, 0);
    fs4k_MoveHolder (0);
    }
  if (strcasecmp (pArg, "load") == 0)
    fs4k_LoadCalInfo ("calinfo");
  if (strcasecmp (pArg, "off") == 0)
    {
    fs4000_move_position (0, 0, 0);
    fs4k_LampOff    (0);
    fs4k_MoveHolder (0);
    }
  if (strcasecmp (pArg, "scan") == 0)
    bNeedFrame = TRUE;
  if (strcasecmp (pArg, "thumb") == 0)
    FS4_Thumbnail (g.szThumbFID);
  if (strcasecmp (pArg, "tune") == 0)
    FS4_Tune (-1);
  if (strcasecmp (pArg, "tuneg") == 0)
    FS4_NegTune (-1);

//----          Test functions

  if (strcasecmp (pArg, "afae") == 0)
    FS4_AfAeTest ();
  if (strcasecmp (pArg, "frame") == 0)
    for (x = 0; x < 5; x++)
      FS4_FrameTest (4000, x);
  if (strcasecmp (pArg, "modes") == 0)
    FS4_ScanModeTest ("-mode");
  if (strcasecmp (pArg, "mytest") == 0)
    FS4_Test ();
  if (strcasecmp (pArg, "mytest1") == 0)
    FS4_Test1 ();
  if (strcasecmp (pArg, "pos") == 0)
    FS4_Position ();
  if (strcasecmp (pArg, "tunetest") == 0)
    for (x = 0; x < 7; x++)
      {
      wsprintf (msg, "**** Speed = %d\r\n", x);
      spout ();
      if (FS4_Tune (x))
        break;
      }
  if (strcasecmp (pArg, "blacktest") == 0)
    fs4k_BlackTest (100);

bye:
  return AbortWanted ();
  }


int     FS4_Process             (int argc, char **argv)
  {
        DWORD           dwRecs;
        HANDLE          hFile    = NULL,
                        hPipeIn  = NULL,
                        hPipeOut = NULL;
        char            *pArg;
        DWORD           dwBytes;

  fs4000_debug = FALSE;

//      if no commands, and re-directed input, use StdIn

  if (bIsExe && (argc == 1))
    if (GetNumberOfConsoleInputEvents (hStdInput, &dwRecs) == 0)
      hFile = hStdInput;

  while (!AbortWanted ())
    {
    if (hPipeOut)
      WriteFile (hPipeOut, pArg, strlen (pArg), &dwBytes, NULL);
    if (hFile)
      pArg = FS4_GetWord (argc, argv, &hFile);
    else
      pArg = FS4_GetWord (argc, argv, &hPipeIn);
    if (!pArg)
      break;

//----          IPC functions

    if (pArg [0] == '@')
      {
      hFile = CreateFile (pArg + 1,
                          GENERIC_READ,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL);
      if (hFile == INVALID_HANDLE_VALUE)
        {
        wsprintf (msg, "%s open failed\r\n", pArg + 1);
        spout ();
        hFile = NULL;
        }
      continue;
      }
    if (strcasecmp (pArg, "close") == 0)
      {
      if (hFile)
        {
        CloseHandle (hFile);
        hFile = NULL;
        }
      else
        spout ("No input file open\r\n");
      continue;
      }
    if (strncasecmp (pArg, "pipe=", 5) == 0)
      {
      if (hPipeIn)
        CloseHandle (hPipeIn);
      if (hPipeOut)
        CloseHandle (hPipeOut);
      hPipeIn  = (HANDLE) atoi (&pArg [5]);
      hPipeOut = (HANDLE) atoi (strchr (&pArg [5], ',') + 1);
      continue;
      }

//----          Other words

    FS4_ProcessWord (pArg);
    }
  return AbortWanted ();
  }


/*--------------------------------------------------------------

                Code for DLL

        For user program to use Fs4000.dll:

- include Fs4000.h
- link Fs4000imp.lib
- At BOJ,
    use FS4_GetHandles and FS4_SetHandles to get console output, etc
    call FS4_BOJ () and check result is 0
    can call FS4_GlobalsLoc () to get ptr to our globals
    can call FS4_SetCallback (&rtn) to get progress info
- Call FS4_ProcessWord (arg) for each command word
    and / or
  Call FS4_Process (argc, argv []) to process array of words
    and / or
  Call FS4_xxxxxxx (...) directly to tune, scan, etc
- Can call FS4_SetAbort (via another thread) to kill task in progress

void    HostCallback            (FS4K_NEWS *p)
  {
  react to news
  }

--------------------------------------------------------------*/

void    FS4_GetHandles          (HANDLE *phStdIn,
                                 HANDLE *phConIn,
                                 HANDLE *phConOut)
  {
  *phStdIn  = hStdInput;
  *phConIn  = hConsoleInput;
  *phConOut = hConsoleOutput;
  return;
  }


void    FS4_SetHandles          (HANDLE hStdIn,
                                 HANDLE hConIn,
                                 HANDLE hConOut)
  {
  hStdInput      = hStdIn;
  hConsoleInput  = hConIn;
  hConsoleOutput = hConOut;
  return;
  }
#endif

FS4KG*  FS4_GlobalsLoc          (void)
  {
  return &g;                                    // info for voyeur
  }


FS4KG*  FS4_SetCallback         (void (*pfnCallback) (FS4K_NEWS*))
  {
  g.pfnCallback = pfnCallback;                  // remember
  return FS4_GlobalsLoc ();                     // innards pointer
  }


void    FS4_SetAbort            (void)
  {
  ReturnCode = RC_KBD_ABORT;
  return;
  }

#if 0
BOOL WINAPI DllEntryPoint       (HINSTANCE    /*hinstDLL*/,
                                 DWORD          fdwReason,
                                 LPVOID       /*lpReserved*/)
  {
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
      fs4000_debug = FALSE;
      break;

    case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
      break;

    case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
      break;

    case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
      FS4_EOJ ();
      break;
    }
  return TRUE;
  }
#endif

/*--------------------------------------------------------------

                Get specs, do work

--------------------------------------------------------------*/
#if 0
int main                        (int argc, char **argv)
  {
        int             erc;

  bIsExe = TRUE;

  if (0 < (erc = FS4_BOJ ()))
    goto bye;

  erc = FS4_Process (argc, argv);

bye:
  FS4_EOJ ();
  return erc;
  }
#endif

