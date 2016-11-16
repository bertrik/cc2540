#include <stdbool.h>

#include <libusb.h>

#include <stdlib.h>
#include <stdio.h>

#define TIMEOUT 1000

static void setup(libusb_device_handle *dev)
{
    int ret;
    
    // ?
    ret = libusb_control_transfer(dev, 0x40, 0xC5, 0x00, 0x04, NULL, 0, TIMEOUT);
    printf("ret0 = %d\n", ret);
    
    // get status?
    uint8_t data;
    ret = libusb_control_transfer(dev, 0xC0, 0xC6, 0x00, 0x00, &data, 1, TIMEOUT);
    printf("ret1 = %d, data = %02X\n", ret, data);
    ret = libusb_control_transfer(dev, 0xC0, 0xC6, 0x00, 0x00, &data, 1, TIMEOUT);
    printf("ret2 = %d, data = %02X\n", ret, data);

    // ?
    ret = libusb_control_transfer(dev, 0x40, 0xC9, 0x00, 0x00, NULL, 0, TIMEOUT);
    printf("ret3 = %d\n", ret);

    // set capture channel
    data = 0x27;
    ret = libusb_control_transfer(dev, 0x40, 0xD2, 0x00, 0x00, &data, 1, TIMEOUT);
    printf("ret4 = %d\n", ret);
    data = 0x00;
    ret = libusb_control_transfer(dev, 0x40, 0xD2, 0x00, 0x01, &data, 1, TIMEOUT);
    printf("ret5 = %d\n", ret);

    // start capture?
    ret = libusb_control_transfer(dev, 0x40, 0xD0, 0x00, 0x00, NULL, 0, TIMEOUT);
    printf("ret6 = %d\n", ret);
}

static void bulk_read(libusb_device_handle *dev)
{
    int ret;
    int xfer;
    uint8_t data[1024];
    while (1) {
        xfer = 0;
        ret = libusb_bulk_transfer(dev, 0x83, data, sizeof(data), &xfer, TIMEOUT);
        if (ret == 0) {
            int i;
            for (i = 0; i < xfer; i++) {
                if ((i % 16) == 0) {
                    printf("\n%04X:", i);
                }
                printf(" %02X", data[i]);
            }
            printf("\n");
        }
    }
}

static void sniff(libusb_context *context, uint16_t pid, uint16_t vid)
{
    libusb_device_handle *dev = libusb_open_device_with_vid_pid(context, pid, vid);
    if (dev != NULL) {
        printf("Opened device %04X:%04X\n", pid, vid);
    
        setup(dev);
        bulk_read(dev);

        libusb_close(dev);
    } else {
        printf("device %04X:%04X not found!\n", pid, vid);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    libusb_context *context;
    libusb_init(&context);
    libusb_set_debug(context, LIBUSB_LOG_LEVEL_WARNING);
    sniff(context, 0x451, 0x16B3);
    libusb_exit(context);
    
    return 0;
}

