/*--------------------------------------------------------------

                USB INTERFACE CODE

        USB transport code for FS4000US scanner code

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

        2004-12-08      Original version.

        2004-12-10      Add access using LibUSB.

        2004-12-12      File access code now works with W98.

--------------------------------------------------------------*/

#include <inttypes.h>
#include <libusb.h>
#include <stdio.h>
#include <stdbool.h>

#include "scsidefs.h"
#include "scsi_via_usb.h"

/*
        USB fields are split into global and per_user because I did this
        with the ASPI code.  Probably no value here.

        At start-up :
                call usb_init

        At shut-down :
                call usb_deinit
*/

#define Nullit(x) memset (&x, 0, sizeof (x))

USB_FIELDS      usb;


/*--------------------------------------------------------------

                USB start

--------------------------------------------------------------*/

int
usb_init                (void)
{
    libusb_device_handle          *udev;

    // XXX: error handling
    libusb_init (NULL);
    udev = libusb_open_device_with_vid_pid(NULL, 0x04A9, 0x3042);
    int configuration = 0;
    libusb_get_configuration(udev, &configuration);
    libusb_set_configuration(udev, configuration);
    usb.g.pUdev = udev;
    usb.g.byInterfaceNumber = 0; // originally dev->config[0].interface[0].altsetting[0].bInterfaceNumber;
    libusb_claim_interface(udev, usb.g.byInterfaceNumber);
    return 0;
}


/*--------------------------------------------------------------

                USB close

--------------------------------------------------------------*/

int
usb_deinit              (void)
{
    // close handle

    libusb_release_interface (usb.g.pUdev, usb.g.byInterfaceNumber);
    libusb_close (usb.g.pUdev);

    return 0;
}


/*--------------------------------------------------------------

                Execute USB request

--------------------------------------------------------------*/

int
usb_do_request          (uint16_t       wValue,
                         bool           bInput,
                         unsigned char  *pBuf,
                         uint16_t       dwBufLen)
{
    uint8_t            byRequest, byRequestType;

    // bit 7        0 = output, 1 = input
    // bits 6-5     2 = vendor special
    // bits 4-0     0 = recipient is device
    byRequestType = bInput ? 0xC0 : 0x40;

    // loaded by driver according to ddk
    byRequest = (dwBufLen < 2) ? 0x0C : 0x04;     // is this significant ?

    libusb_control_transfer (usb.g.pUdev,              // libusb_device_handle *dev,
                             byRequestType,            // uint8_t requesttype,
                             byRequest,                // uint8_t request,
                             wValue,                   // uint16_t wValue,
                             0,                        // uint16_t index,
                             pBuf,                     // unsigned char *bytes,
                             dwBufLen,                 // uint_16 size,
                             0);                       // unsigned int timeout);
    return 0;
}


/*--------------------------------------------------------------

                Execute USB SCSI command

--------------------------------------------------------------*/

int
usb_scsi_exec           (unsigned char           *cdb,
                         unsigned int   cdb_length,
                         int            mode_and_dir,
                         unsigned char           *pdb,
                         unsigned int   pdb_len)
{
    void            *save_pdb    = pdb;
    uint16_t           save_pdb_len = pdb_len;
    int             dwBytes;
    uint16_t          x;
    unsigned char            byNull = 0, byOne = 1;
    unsigned char   *pbyCmd = cdb;
    bool            bInput;
    uint16_t           dwValue, dwValueC5 = 0xC500, dwValue03 = 0x0300;
    unsigned char            byStatPDB [4];          // from C500 request
    unsigned char            bySensPDB [14];         // from 0300 request

    if (!pdb_len)                         // if no data, use dummy output
    {
        mode_and_dir &= ~SRB_DIR_IN;
        mode_and_dir |= SRB_DIR_OUT;
        pdb = &byNull;                              // default output
        pdb_len = 1;
        if (*pbyCmd == 0x00)                        // change for some
            pdb = &byOne;
        else if (*pbyCmd == 0xE4)
            pdb = &byOne;
        else if (*pbyCmd == 0xE6)
        {
            pdb = pbyCmd + 1;
            pdb_len = 5;
        }
        else if (*pbyCmd == 0xE7)
        {
            pdb = pbyCmd + 2;
            pdb_len = 2;
        }
        else if (*pbyCmd == 0xE8)
            pdb = pbyCmd + 1;
    }
    if (*pbyCmd == 0x28)                          // if read
    {
        mode_and_dir &= ~SRB_DIR_IN;                // output not input
        mode_and_dir |= SRB_DIR_OUT;
        pdb = pbyCmd + 6;                           // send buf size
        pdb_len = 3;
    }

    dwValue =  pbyCmd [0] << 8;                   // build parameters
    if (cdb_length > 2)
        if ((*pbyCmd == 0x12) || (*pbyCmd == 0xD5))
            dwValue += pbyCmd [2];
    bInput = (mode_and_dir & SRB_DIR_IN) > 0;

    if (usb_do_request (dwValue, bInput, pdb, pdb_len))   // SCSI via USB
        return -1;

    if (*pbyCmd == 0x28)                          // if read, get bulk data
    {
        libusb_bulk_transfer (usb.g.pUdev, 0x81, save_pdb, save_pdb_len, &dwBytes, 0);

        if (dwBytes != save_pdb_len)
            return -1;
    }

    if (usb_do_request (dwValueC5, true, &(byStatPDB[0]), 4))  // get status
        return -1;

    if (byStatPDB [0] != *pbyCmd)
        if ((byStatPDB [0] != 0) || ((*pbyCmd != 0x16) && (*pbyCmd != 0x17)))
            printf ("cmd mismatch %02X %02X\n", *pbyCmd, byStatPDB [0]);

    if (byStatPDB [1] & 0xFF)                             // sense data ?
    {
      usb_do_request (dwValue03, true, &(bySensPDB[0]), 14);   // get sense
        printf ("sense");
        for (x = 0; x < 14; x++)
            printf (" %02X", bySensPDB [x]);
        printf ("\n");
    }

    return 0;
}
