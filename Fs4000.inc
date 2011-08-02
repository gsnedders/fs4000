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
  fs4k_ReadScan (&g.rScan, 3, 8);
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
    fs4k_ReadScan (&g.rScan, 3, byBitsPerPixel);
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
        fs4k_ReadScan (&g.rScan, 3, 14);
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
        fs4k_ReadScan (&g.rScan, 3,  8);
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
        fs4k_ReadScan (&g.rScan, 3, 8);
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
        fs4k_ReadScan (&g.rScan, 3, 16);
        RestoreFS4000Debug ();
        if (iUnk2 & 0x01)               // if LSB, data is big-endian
          for (DWORD x = 0; x < g.rScan.dwBufSize - 1; x += 2)
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
        fs4k_ReadScan (&g.rScan, 3,  8);
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



