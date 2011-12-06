#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define SRB_DIR_IN                  0x08        // Transfer from SCSI target to host
#define SRB_DIR_OUT                 0x10        // Transfer from host to SCSI target

typedef struct  /* LUN_INQUIRY */
{
        byte            reserved [8];
        byte            vendor   [8];
        byte            product  [16];
        byte            release  [4];
}
  LUN_INQUIRY;

typedef struct  /* USB_GLOBALS */
{
        libusb_device_handle  *pUdev;
        BYTE            byInterfaceNumber;
}
  USB_GLOBALS;

typedef struct  /* USB_FIELDS */
{
        USB_GLOBALS     g;
}
  USB_FIELDS;

extern  USB_FIELDS      usb;

int     usb_init                (void);

int     usb_deinit              (void);

int     usb_do_request          (DWORD          dwValue,
                                 BOOL           bInput,
                                 void           *pBuf,
                                 DWORD          dwBufLen);

int     usb_scsi_exec           (void         *cdb,
                                 unsigned int cdb_length,
                                 int          mode_and_dir,
                                 void         *data_buf,
                                 unsigned int data_buf_len);

int     usb_unit_inquiry        (LUN_INQUIRY *pLI);

#ifdef __cplusplus
}
#endif //__cplusplus
