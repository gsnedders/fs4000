#ifndef _SCSI_VIA_ASPI_H_
#define _SCSI_VIA_ASPI_H_

#include "wnaspi32.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//efine SVAF_POSTING            0x01    /* same as SRB_POSTING          */
//efine SVAF_EVENT_NOTIFY       0x40    /* same as SRB_EVENT_NOTIFY     */
#define SVAF_IO_MODS            (SRB_POSTING | SRB_EVENT_NOTIFY)

#define SVAF_RESET_EVENT        0x0100  /* Reset event handle           */
#define SVAF_WANT_IO            0x0200  /* Initiate I/O                 */
#define SVAF_WAIT_EVENT         0x0400  /* Wait for event               */
#define SVAF_CHECK_RESULT       0x0800  /* Report if bad result         */
#define SVAF_EXTRA_MODS         0x0F00  /* Any of our flags             */
#define SVAF_ANY_FLAGS          (SVAF_IO_MODS | SVAF_EXTRA_MODS)


typedef struct  /* LUN_INQUIRY */
{
        byte            reserved [8];
        byte            vendor   [8];
        byte            product  [16];
        byte            release  [4];
}
  LUN_INQUIRY;

typedef struct  /* ASPI_GLOBALS */
{
        HINSTANCE       WnASPI32DllHandle;
        DWORD           (*GetASPI32SupportInfoFunc)     (void);
        DWORD           (*SendASPI32CommandFunc)        (LPSRB);
        int             HA_count;
}
  ASPI_GLOBALS;

typedef struct  /* ASPI_PER_USER */
{
        HANDLE          hEvent;
        BYTE            HA_count;
        BYTE            HA_Id;
        BYTE            target;
        BYTE            LUN;
        SRB_HAInquiry   HA_inq;
        DWORD           max_transfer;
        LUN_INQUIRY     LUN_inq;
        SRB_ExecSCSICmd srbExec;
        void            (*CallbackFunc) (SRB_ExecSCSICmd *);
}
  ASPI_PER_USER;

typedef struct  /* ASPI_FIELDS */
{
        ASPI_GLOBALS    g;
        ASPI_PER_USER   u;
}
  ASPI_FIELDS;


extern  ASPI_FIELDS     aspi;

int     aspi_init_user          (ASPI_PER_USER *pUser);

int     aspi_deinit_user        (ASPI_PER_USER *pUser);

int     aspi_init               (void);

int     aspi_deinit             (void);

const char *scsi_decode_asc     (unsigned char asc,
                                 unsigned char ascq);

void    scsi_decode_sense       (unsigned char sense,
                                 unsigned char asc,
                                 unsigned char ascq);

int     aspi_scsi_exec          (void         *cdb,
                                 unsigned int cdb_length,
                                 int          mode_and_dir,
                                 void         *data_buf,
                                 unsigned int data_buf_len);

int     aspi_unit_inquiry       (LUN_INQUIRY *pLI);

int     aspi_find_device        (byte   byDeviceType,
                                 LPSTR  pbVendor,
                                 LPSTR  pbProduct,
                                 BOOL   bReScanBus);

void    aspi_bus_scan           (void);

        /* LEGACY */

int     scsiaspi_init           (void);

int     scsi_deinit             (void);

void    scsi_bus_scan           (void);

int     scsi_do_command         (void         *cdb,
                                 unsigned int cdb_length,
                                 int          data_dir,
                                 void         *data_buf,
                                 unsigned int data_buf_len);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* _SCSI_VIA_ASPI_H_ */
