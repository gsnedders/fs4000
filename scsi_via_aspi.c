/*--------------------------------------------------------------

                ASPI INTERFACE CODE

        ASPI transport code for FS4000US scanner code

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

        2004-12-03      Original version from code by Dave Burns.

--------------------------------------------------------------*/

#define STRICT
#include <windows.h>
#include <stdio.h>
#include "scsidefs.h"
#include "scsi_via_aspi.h"
/*
        ASPI fields are split into global and per_user in case we want
        a multi-user version of this code.  Multi-user would allow the
        program to access multiple devices concurrently.  However, this
        enhancement requires an interface change as all calls would need
        to include a 'this' parameter pointing to the user fields.
        To avoid bother the current code is restricted to single-user.

        At start-up :
                call aspi_init
                load aspi.u.HA_Id
                load aspi.u.target

        At shut-down :
                call aspi_deinit

        ASPI user fields include an SRB and callback pointer so we can
        support callback mode.  For callback mode :-
        - load aspi.u.CallbackFunc
        - Or SRB_POSTING to data_dir in aspi_scsi_exec call
        - Callback routine should 'SetEvent (aspi.u.hEvent);' as this can
          be used to start another thread that is waiting.
        - See aspi_scsi_exec for further details
*/

#define Nullit(x) memset (&x, 0, sizeof (x))


        ASPI_FIELDS     aspi;

/*--------------------------------------------------------------

                ASPI user start

--------------------------------------------------------------*/

int
aspi_init_user          (ASPI_PER_USER *pUser)
{
  Nullit (*pUser);
  pUser->hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
  if (!pUser->hEvent)
    {
    printf("ERROR: Could not create the event object.\n");
    return -1;
    }
  pUser->HA_count = aspi.g.HA_count;
  return 0;
}


/*--------------------------------------------------------------

                ASPI user close

--------------------------------------------------------------*/

int
aspi_deinit_user        (ASPI_PER_USER *pUser)
{
  CloseHandle (pUser->hEvent);
  Nullit      (*pUser);
  return 0;
}


/*--------------------------------------------------------------

                ASPI start

--------------------------------------------------------------*/

int
aspi_init               (void)
{
        DWORD   dwSupportInfo;
        BYTE    byASPIStatus;

        // load WNASPI32.DLL

  aspi.g.WnASPI32DllHandle = LoadLibrary ("WNASPI32.DLL");
  if (aspi.g.WnASPI32DllHandle == 0)
    {
    printf ("ERROR: LoadLibrary: WNASPI32.DLL not found.\n");
    return -1;
    }

        // load the address of GetASPI32SupportInfo

  aspi.g.GetASPI32SupportInfoFunc = (DWORD (__cdecl*) (void))
     GetProcAddress (aspi.g.WnASPI32DllHandle, "GetASPI32SupportInfo");
  if (aspi.g.GetASPI32SupportInfoFunc == NULL)
    {
    printf ("ERROR: GetProcAddress: GetASPI32SupportInfo not found.\n");
    return -1;
    }

        // load the address of SendASPI32Command

  aspi.g.SendASPI32CommandFunc = (DWORD (__cdecl *) (LPSRB))
     GetProcAddress (aspi.g.WnASPI32DllHandle, "SendASPI32Command");
  if (aspi.g.SendASPI32CommandFunc == NULL)
    {
    printf ("ERROR: GetProcAddress: SendASPI32Command not found.\n");
    return -1;
    }

        // call GetASPI32SupportInfo

  dwSupportInfo   = aspi.g.GetASPI32SupportInfoFunc ();
  byASPIStatus    = HIBYTE (LOWORD (dwSupportInfo));
  aspi.g.HA_count = LOBYTE (LOWORD (dwSupportInfo));
  switch (byASPIStatus)
    {
      case SS_COMP: // ASPI support OK, now create the event object
           break; // everything OK !!!
      case SS_NO_ASPI:
           printf ("ERROR: Could not find the ASPI manager.\n");
           return -1;
      case SS_ILLEGAL_MODE:
           printf ("ERROR: ASPI for Windows does not support real mode.\n");
           return -1;
      case SS_OLD_MANAGER:
           printf ("ERROR: Old ASPI manager.\n");
           return -1;
      default:
           printf ("ERROR: Error initializing ASPI.\n");
           return -1;
    }

  return aspi_init_user (&aspi.u);
}


/*--------------------------------------------------------------

                ASPI close

--------------------------------------------------------------*/

int
aspi_deinit             (void)
{
        // release default user fields

  aspi_deinit_user (&aspi.u);

        // unload WNASPI32.DLL

  FreeLibrary (aspi.g.WnASPI32DllHandle);
  aspi.g.WnASPI32DllHandle = NULL;

  return 0;
}


/*--------------------------------------------------------------

                Provide error message

--------------------------------------------------------------*/

const char *
scsi_decode_asc         (unsigned char asc,
                         unsigned char ascq)
{
        unsigned short code = (asc << 8) & ascq;

  if (asc == 0x40)
    return "DIAGNOSTIC FAILURE ON COMPONENT NN (80H-FFH)";

  switch (code)
    {
    case 0x0004: return "BEGINNING-OF-PARTITION/MEDIUM DETECTED";
    case 0x3f02: return "CHANGED OPERATING DEFINITION";
    case 0x4A00: return "COMMAND PHASE ERROR";
    case 0x2C00: return "COMMAND SEQUENCE ERROR";
    case 0x2F00: return "COMMANDS CLEARED BY ANOTHER INITIATOR";
    case 0x2B00: return "COPY CANNOT EXECUTE SINCE HOST CANNOT DISCONNECT";
    case 0x4B00: return "DATA PHASE ERROR";
    case 0x0005: return "END-OF-DATA DETECTED";
    case 0x0002: return "END-OF-PARTITION/MEDIUM DETECTED";
    case 0x0A00: return "ERROR LOG OVERFLOW";
    case 0x1102: return "ERROR TOO LONG TO CORRECT";
    case 0x0006: return "I/O PROCESS TERMINATED";
    case 0x4800: return "INITIATOR DETECTED ERROR MESSAGE RECEIVED";
    case 0x3F03: return "INQUIRY DATA HAS CHANGED";
    case 0x4400: return "INTERNAL TARGET FAILURE";
    case 0x3D00: return "INVALID BITS IN IDENTIFY MESSAGE";
    case 0x2C02: return "INVALID COMBINATION OF WINDOWS SPECIFIED";
    case 0x2000: return "INVALID COMMAND OPERATION CODE";
    case 0x2400: return "INVALID FIELD IN CDB";
    case 0x2600: return "INVALID FIELD IN PARAMETER LIST";
    case 0x4900: return "INVALID MESSAGE ERROR";
    case 0x6000: return "LAMP FAILURE";
    case 0x5B02: return "LOG COUNTER AT MAXIMUM";
    case 0x5B00: return "LOG EXCEPTION";
    case 0x5B03: return "LOG LIST CODES EXHAUSTED";
    case 0x2A02: return "LOG PARAMETERS CHANGED";
    case 0x0800: return "LOGICAL UNIT COMMUNICATION FAILURE";
    case 0x0802: return "LOGICAL UNIT COMMUNICATION PARITY ERROR";
    case 0x0801: return "LOGICAL UNIT COMMUNICATION TIME-OUT";
    case 0x0500: return "LOGICAL UNIT DOES NOT RESPOND TO SELECTION";
    case 0x4C00: return "LOGICAL UNIT FAILED SELF-CONFIGURATION";
    case 0x3E00: return "LOGICAL UNIT HAS NOT SELF-CONFIGURED YET";
    case 0x0401: return "LOGICAL UNIT IS IN PROCESS OF BECOMING READY";
    case 0x0400: return "LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE";
    case 0x0402: return "LOGICAL UNIT NOT READY, INITIALIZING COMMAND REQUIRED";
    case 0x0403: return "LOGICAL UNIT NOT READY, MANUAL INTERVENTION REQUIRED";
    case 0x2500: return "LOGICAL UNIT NOT SUPPORTED";
    case 0x1501: return "MECHANICAL POSITIONING ERROR";
    case 0x5300: return "MEDIA LOAD OR EJECT FAILED";
    case 0x3A00: return "MEDIUM NOT PRESENT";
    case 0x4300: return "MESSAGE ERROR";
    case 0x3F01: return "MICROCODE HAS BEEN CHANGED";
    case 0x2A01: return "MODE PARAMETERS CHANGED";
    case 0x0700: return "MULTIPLE PERIPHERAL DEVICES SELECTED";
    case 0x1103: return "MULTIPLE READ ERRORS";
    case 0x0000: return "NO ADDITIONAL SENSE INFORMATION";
    case 0x2800: return "NOT READY TO READY TRANSITION, MEDIUM MAY HAVE CHANGED";
    case 0x5A00: return "OPERATOR REQUEST OR STATE CHANGE INPUT (UNSPECIFIED)";
    case 0x6102: return "OUT OF FOCUS";
    case 0x4E00: return "OVERLAPPED COMMANDS ATTEMPTED";
    case 0x1A00: return "PARAMETER LIST LENGTERROR";
    case 0x2601: return "PARAMETER NOT SUPPORTED";
    case 0x2602: return "PARAMETER VALUE INVALID";
    case 0x2A00: return "PARAMETERS CHANGED";
    case 0x0300: return "PERIPHERAL DEVICE WRITE FAULT";
    case 0x3B0C: return "POSITION PAST BEGINNING OF MEDIUM";
    case 0x3B0B: return "POSITION PAST END OF MEDIUM";
    case 0x2900: return "POWER ON, RESET, OR BUS DEVICE RESET OCCURRED";
    case 0x1500: return "RANDOM POSITIONING ERROR";
    case 0x3B0A: return "READ PAST BEGINNING OF MEDIUM";
    case 0x3B09: return "READ PAST END OF MEDIUM";
    case 0x1101: return "READ RETRIES EXHAUSTED";
    case 0x1400: return "RECORDED ENTITY NOT FOUND";
    case 0x1700: return "RECOVERED DATA WITNO ERROR CORRECTION APPLIED";
    case 0x1701: return "RECOVERED DATA WITRETRIES";
    case 0x3700: return "ROUNDED PARAMETER";
    case 0x3900: return "SAVING PARAMETERS NOT SUPPORTED";
    case 0x6200: return "SCAN HEAD POSITIONING ERROR";
    case 0x4700: return "SCSI PARITY ERROR";
    case 0x4500: return "SELECT OR RESELECT FAILURE";
    case 0x1B00: return "SYNCHRONOUS DATA TRANSFER ERROR";
    case 0x3F00: return "TARGET OPERATING CONDITIONS HAVE CHANGED";
    case 0x5B01: return "THRESHOLD CONDITION MET";
    case 0x2603: return "THRESHOLD PARAMETERS NOT SUPPORTED";
    case 0x2C01: return "TOO MANY WINDOWS SPECIFIED";
    case 0x6101: return "UNABLE TO ACQUIRE VIDEO";
    case 0x1100: return "UNRECOVERED READ ERROR";
    case 0x4600: return "UNSUCCESSFUL SOFT RESET";
    case 0x6100: return "VIDEO ACQUISITION ERROR";
    case 0x0C00: return "WRITE ERROR";
    default:     return NULL;
    }
}


/*--------------------------------------------------------------

                Display sense description

--------------------------------------------------------------*/

void
scsi_decode_sense       (unsigned char sense,
                         unsigned char asc,
                         unsigned char ascq)
{
        char *sense_desc[] = {
                "NO SENSE",
                "RECOVERED ERROR",
                "NOT READY",
                "MEDIUM ERROR",
                "HARDWARE ERROR",
                "ILLEGAL REQUEST",
                "UNIT ATTENTION",
                "DATA PROTECT",
                "BLANK CHECK",
                "VENDOR-SPECIFIC",
                "COPY ABORTED",
                "ABORTED COMMAND",
                "EQUAL",
                "VOLUME OVERFLOW",
                "MISCOMPARE",
                "RESERVED"
        };
        const char *asc_desc;

  asc_desc = scsi_decode_asc(asc, ascq);
  if (asc_desc != NULL)
    printf ("SENSE: %s - %s\n", sense_desc[sense], asc_desc);
  else
    printf ("SENSE: %s, ASC %02x, ASCQ %02x\n", sense_desc[sense], asc, ascq);
}


/*--------------------------------------------------------------

                Execute SCSI command

  The mode_and_dir parameter controls this routine.  It can contain just the
  SCSI direction bits for default mode or it can contain extra bits for
  special modes.

  Normally we start the I/O and wait for the result.  In this mode the event
  handle is reset and we wait for the ASPI manager to set it signifying I/O
  complete.  This default mode is selected if no special bits are set in
  mode_and_dir.

  Modification bits include the 2 SCSI bits (POSTING for callback mode, and
  EVENT_NOTIFY for event mode) and the 4 special bits defined by this routine.
  If any of the special bits are set we do exactly what the caller specifies.
  If none of the special bits are set we set some based on the 2 SCSI bits.
  This supports simple calls that just want a synchronous I/O performed, smart
  calls that specify a particular mode, and finicky calls for special code.

  For callback mode, the caller should reset the event handle before the first
  I/O and set it in the callback routine.  This can be used to resume another
  thread.  This other thread can reset the event handle if it wants to wait
  again.

  SCSI I/O bits :-
        SRB_POSTING             Callback mode
        SRB_EVENT_NOTIFY        Set event on I/O complete

  Special bits :-
        SVAF_RESET_EVENT        Reset event handle
        SVAF_WANT_IO            Initiate I/O
        SVAF_WAIT_EVENT         Wait for event
        SVAF_CHECK_RESULT       Report if bad result

--------------------------------------------------------------*/

int
aspi_scsi_exec          (void           *cdb,
                         unsigned int   cdb_length,
                         int            mode_and_dir,
                         void           *data_buf,
                         unsigned int   data_buf_len)
{
  if (0 == (mode_and_dir & SVAF_ANY_FLAGS))             // if no specs,
    mode_and_dir |= SVAF_EXTRA_MODS | SRB_EVENT_NOTIFY; // use default
  else
    if (0 == (mode_and_dir & SVAF_EXTRA_MODS))          // if no extras,
      if (mode_and_dir & SRB_EVENT_NOTIFY)              // if event mode,
        mode_and_dir |= SVAF_EXTRA_MODS;                // use default
      else
        if (mode_and_dir & SRB_POSTING)                 // if callback,
          mode_and_dir |= SVAF_WANT_IO;                 // need I/O

  if (mode_and_dir & SVAF_RESET_EVENT)                  // generally true
    ResetEvent (aspi.u.hEvent);

  if (mode_and_dir & SVAF_WANT_IO)                      // invariably true
    {
    Nullit (aspi.u.srbExec);
    aspi.u.srbExec.SRB_Cmd        = SC_EXEC_SCSI_CMD;
    aspi.u.srbExec.SRB_HaId       = aspi.u.HA_Id;
    aspi.u.srbExec.SRB_Flags      = mode_and_dir;
    aspi.u.srbExec.SRB_Target     = aspi.u.target;
    aspi.u.srbExec.SRB_Lun        = aspi.u.LUN;
    aspi.u.srbExec.SRB_BufLen     = data_buf_len;
    aspi.u.srbExec.SRB_BufPointer = (char*) data_buf;
    aspi.u.srbExec.SRB_SenseLen   = SENSE_LEN;
    aspi.u.srbExec.SRB_CDBLen     = cdb_length;
    memcpy (aspi.u.srbExec.CDBByte, cdb, cdb_length);

    if (mode_and_dir & SRB_POSTING)                     // want callback
      aspi.u.srbExec.SRB_PostProc = (LPVOID) aspi.u.CallbackFunc;

    if (mode_and_dir & SRB_EVENT_NOTIFY)                // want event
      aspi.u.srbExec.SRB_PostProc = (LPVOID) aspi.u.hEvent;

    aspi.g.SendASPI32CommandFunc (&aspi.u.srbExec);     // init I/O
    }

  if (mode_and_dir & SVAF_WAIT_EVENT)                   // if wait wanted,
    while (aspi.u.srbExec.SRB_Status == SS_PENDING)
      {
      WaitForSingleObject (aspi.u.hEvent, INFINITE);
      if (aspi.u.srbExec.SRB_Status == SS_PENDING)      // shouldn't happen
        ResetEvent (aspi.u.hEvent);
      }

  if (mode_and_dir & SVAF_CHECK_RESULT)                 // if report wanted,
    if (aspi.u.srbExec.SRB_Status != SS_COMP)
      {
      scsi_decode_sense (aspi.u.srbExec.SenseArea[2] & 0xf,
                         aspi.u.srbExec.SenseArea[12],
                         aspi.u.srbExec.SenseArea[13]);
      return -1;
      }

  return 0;
}


/*--------------------------------------------------------------

                Get LUN inquiry info

--------------------------------------------------------------*/

int
aspi_unit_inquiry       (LUN_INQUIRY *pLI)
{
        BYTE            CDB [6];

  Nullit (CDB);
  CDB [0] = SCSI_INQUIRY;
  CDB [4] = 36;
  return aspi_scsi_exec (CDB, 6, SRB_DIR_IN, pLI, sizeof (*pLI));
}


/*--------------------------------------------------------------

                Search for a device

--------------------------------------------------------------*/

int
aspi_find_device        (byte   byDeviceType,
                         LPSTR  pbVendor,
                         LPSTR  pbProduct,
                         BOOL   bReScanBus)
{
        SRB_RescanPort  srbRescanPort;
        SRB_GDEVBlock   srbGDEVBlock;
        int             iLen;

  for ( ; ; aspi.u.HA_Id++, aspi.u.target = 0)
    {

    Nullit (aspi.u.HA_inq);                             // get HA info
    aspi.u.HA_inq.SRB_Cmd = SC_HA_INQUIRY;
    aspi.u.HA_inq.SRB_HaId = aspi.u.HA_Id;
    aspi.g.SendASPI32CommandFunc ((LPSRB) &aspi.u.HA_inq);

    if (aspi.u.HA_inq.SRB_Status != SS_COMP)
      return 256 + aspi.u.HA_inq.SRB_Status;

    aspi.u.max_transfer = aspi.u.HA_inq.HA_MaxTransferLength;

    if (bReScanBus)
      {
      Nullit (srbRescanPort);
      srbRescanPort.SRB_Cmd = SC_RESCAN_SCSI_BUS;
      srbRescanPort.SRB_HaId = aspi.u.HA_Id;
      aspi.g.SendASPI32CommandFunc ((LPSRB) &srbRescanPort);
      if (srbRescanPort.SRB_Status != SS_COMP)
        return 512 + srbRescanPort.SRB_Status;
      }

    for ( ; aspi.u.target < aspi.u.HA_inq.HA_MaxTargets; aspi.u.target++)
      {

      Nullit (srbGDEVBlock);                            // get dev info
      srbGDEVBlock.SRB_Cmd = SC_GET_DEV_TYPE;
      srbGDEVBlock.SRB_HaId = aspi.u.HA_Id;
      srbGDEVBlock.SRB_Target = aspi.u.target;
      aspi.g.SendASPI32CommandFunc ((LPSRB) &srbGDEVBlock);

      if (srbGDEVBlock.SRB_Status != SS_COMP)
        continue;

      if (aspi_unit_inquiry (&aspi.u.LUN_inq))
        continue;

      if (byDeviceType < 32)                            // check type ?
        if (srbGDEVBlock.SRB_DeviceType != byDeviceType)
          continue;

      iLen = strlen (pbVendor);                         // check vendor ?
      if (iLen)
        if (memcmp (aspi.u.LUN_inq.vendor, pbVendor, iLen))
          continue;

      iLen = strlen (pbProduct);                        // check product ?
      if (iLen)
        if (memcmp (aspi.u.LUN_inq.product, pbProduct, iLen))
          continue;

      return 0;                                         // found it !!!
      }
    }
//return -1;
}


/*--------------------------------------------------------------

                Produce bus/device report

--------------------------------------------------------------*/

void
aspi_bus_scan           (void)
{
        char data_out_buf[300];
        BYTE HA_Count;
        BYTE HA_num;

        // PHASE 1: determine how many host adapters are installed and the
        //          name and version of the ASPI manager

        // initialize the SRB for the first inquiry

  SRB_HAInquiry srbHAInquiry;
  memset (&srbHAInquiry, 0, sizeof (SRB_HAInquiry));
  srbHAInquiry . SRB_Cmd = SC_HA_INQUIRY;
  srbHAInquiry . SRB_HaId = 0;

        // get the number of host adapters installed

  aspi.g.SendASPI32CommandFunc ((LPSRB) &srbHAInquiry);
  if (srbHAInquiry.SRB_Status != SS_COMP)
    {
    printf ("ERROR: Host adapter inquiry failed.\n");
    return;
    }
  HA_Count = srbHAInquiry.HA_Count;

        // display ASPI manager information

  srbHAInquiry.HA_ManagerId [ 16 ] = 0;
  printf ("Installed ASPI manager: %s\n", srbHAInquiry.HA_ManagerId);

        // PHASE 2: describe each host adapter and any attached device

        // for each host adapter...

   for (HA_num = 0; HA_num < HA_Count; HA_num++)
       {
         char szHA_num [ 10 ];
         itoa ( ( int ) HA_num, szHA_num, 10 );
         memset ( &srbHAInquiry, 0, sizeof ( SRB_HAInquiry ) );
         srbHAInquiry . SRB_Cmd = SC_HA_INQUIRY;
         srbHAInquiry . SRB_HaId = HA_num;
         aspi.g.SendASPI32CommandFunc ( ( LPSRB ) &srbHAInquiry );
         if ( srbHAInquiry . SRB_Status != SS_COMP )
            {
              char szMsg [ 60 ] = "Host adapter number ";
              strcat ( szMsg, szHA_num );
              strcat ( szMsg, ": inquiry failed." );
              printf("ERROR: FS 007: %s\n", szMsg);
            }
         else
            {
                                BYTE SCSI_Id;
              char szMsg [ 60 ] = "Host adapter n. ";
              strcat ( szMsg, szHA_num );
              strcat ( szMsg, ":" );
              srbHAInquiry.HA_Identifier [ 16 ] = 0;
              printf("%s: %s\n", szMsg, srbHAInquiry.HA_Identifier);
           // for each SCSI id...
              for ( SCSI_Id = 0; SCSI_Id < 8; SCSI_Id++ )
                  {
                                          BYTE SCSI_Lun;
                    char szSCSI_Id [ 10 ];
                    char szDescription [ 32 ];

                                        itoa ( ( int ) SCSI_Id, szSCSI_Id, 10 );

                                        // for each logical unit...
                    for ( SCSI_Lun = 0; SCSI_Lun < 8; SCSI_Lun++ )
                        {
                          char szSCSI_Lun [ 10 ];
                          char szDevice [ 60 ] = "Unknown device type.";
                          char szMsg [ 60 ] = "SCSI id ";
                          SRB_GDEVBlock srbGDEVBlock;

                          itoa ( ( int ) SCSI_Lun, szSCSI_Lun, 10 );

                                                  strcat ( szMsg, szSCSI_Id );
                          strcat ( szMsg, " lun " );
                          strcat ( szMsg, szSCSI_Lun );

                                                  memset ( &srbGDEVBlock, 0, sizeof ( srbGDEVBlock ) );
                          srbGDEVBlock.SRB_Cmd = SC_GET_DEV_TYPE;
                          srbGDEVBlock.SRB_HaId = HA_num;
                          srbGDEVBlock.SRB_Target = SCSI_Id;
                          srbGDEVBlock.SRB_Lun = SCSI_Lun;
                          aspi.g.SendASPI32CommandFunc ( ( LPSRB ) &srbGDEVBlock );
                          if ( srbGDEVBlock.SRB_Status != SS_COMP ) continue;
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_DASD )
                             strcpy ( szDevice, "Direct access storage device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_SEQD )
                             strcpy ( szDevice, "Sequntial access storage device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_PRNT )
                             strcpy ( szDevice, "Printer device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_PROC )
                             strcpy ( szDevice, "Processor device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_WORM )
                             strcpy ( szDevice, "WORM device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM )
                             strcpy ( szDevice, "CD-ROM device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_SCAN )
                             strcpy ( szDevice, "Scanner device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_OPTI )
                             strcpy ( szDevice, "Optical memory device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_JUKE )
                             strcpy ( szDevice, "Medium changer device" );
                          if ( srbGDEVBlock.SRB_DeviceType == DTYPE_COMM )
                             strcpy ( szDevice, "Communication device" );
                          strcat ( szMsg, " - " );
                          strcat ( szMsg, szDevice );
                          {
                          unsigned char cdb[6] = { 0x12, 0, 0, 0, 0x24, 0 };
                          aspi_scsi_exec (cdb, sizeof(cdb), SRB_DIR_IN, data_out_buf, sizeof(data_out_buf));
                          }
                          strncpy ( szDescription, &data_out_buf[8], 28 );
                          szDescription[28] = 0;
                          printf("%s: %s\n", szMsg, szDescription);
                        }
                  }
           }
      }
   return;
}


/*--------------------------------------------------------------

                Legacy routines

--------------------------------------------------------------*/

int
scsiaspi_init           (void)
{
        int             erc;

  erc = aspi_init ();
  aspi.u.HA_Id  = 3;                            // defaults for
  aspi.u.target = 4;                            // Dave Burns
  return erc;
}


int
scsi_deinit             (void)
{
  return aspi_deinit ();
}


void
scsi_bus_scan           (void)
{
  aspi_bus_scan ();
  return;
}


int
scsi_do_command         (void         *cdb,
                         unsigned int cdb_length,
                         int          data_dir,
                         void         *data_buf,
                         unsigned int data_buf_len)
{
  return aspi_scsi_exec (cdb, cdb_length, data_dir, data_buf, data_buf_len);
}


