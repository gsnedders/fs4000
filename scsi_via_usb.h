#include <libusb.h>

#define SRB_DIR_IN                  0x08        // Transfer from SCSI target to host
#define SRB_DIR_OUT                 0x10        // Transfer from host to SCSI target

typedef struct  /* USB_GLOBALS */
{
        libusb_device_handle  *pUdev;
        int            byInterfaceNumber;
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

int     usb_do_request          (uint16_t       wValue,
                                 bool           bInput,
                                 unsigned char           *pBuf,
                                 uint16_t       dwBufLen);

int     usb_scsi_exec           (unsigned char         *cdb,
                                 unsigned int cdb_length,
                                 int          mode_and_dir,
                                 unsigned char         *data_buf,
                                 unsigned int data_buf_len);
