#include <stdbool.h>

#include <libusb.h>

#include <stdlib.h>
#include <stdio.h>
#include <pcap.h>

#define TIMEOUT 1000

// from https://github.com/andrewdodd/ccsniffpiper/blob/master/ccsniffpiper.py
#define GET_IDENT   0xC0
#define SET_POWER   0xC5
#define GET_POWER   0xC6
#define SET_START   0xD0
#define SET_END     0xD1
#define SET_CHAN    0xD2

#define POWER_RETRIES 10

static int get_ident(libusb_device_handle *dev)
{
    uint8_t ident[32];
    int ret;
    
    ret = libusb_control_transfer(dev, 0xC0, GET_IDENT, 0x00, 0x00, ident, sizeof(ident), TIMEOUT);
    if (ret > 0) {
        int i;
        printf("IDENT:");
        for (i = 0; i < ret; i++) {
            printf(" %02X", ident[i]);
        }
        printf("\n");
    }
    return ret;
}

static int set_power(libusb_device_handle *dev, uint8_t power, int retries)
{
    int ret;

   // set power
    ret = libusb_control_transfer(dev, 0x40, SET_POWER, 0x00, power, NULL, 0, TIMEOUT);
    
    // get power until it is the same as configured in set_power
    int i;
    for (i = 0; i < retries; i++) {
        uint8_t data;
        ret = libusb_control_transfer(dev, 0xC0, GET_POWER, 0x00, 0x00, &data, 1, TIMEOUT);
        if (ret < 0) {
            return ret;
        }
        if (data == power) {
            return 0;
        }
    }
    return ret;
}

static int set_channel(libusb_device_handle *dev, uint8_t channel)
{
    int ret;
    uint8_t data;

    data = channel & 0xFF;
    ret = libusb_control_transfer(dev, 0x40, SET_CHAN, 0x00, 0x00, &data, 1, TIMEOUT);
    if (ret < 0) {
        printf("setting channel (LSB) failed!\n");
        return ret;
    }
    data = (channel >> 8) & 0xFF;
    ret = libusb_control_transfer(dev, 0x40, SET_CHAN, 0x00, 0x01, &data, 1, TIMEOUT);
    if (ret < 0) {
        printf("setting channel (LSB) failed!\n");
        return ret;
    }

    return ret;
}

static int setup(libusb_device_handle *dev, int channel)
{
    int ret;
    
    // read ident
    ret = get_ident(dev);
    if (ret < 0) {
        printf("getting identity failed!\n");
        return ret;
    }
    ret = libusb_claim_interface(dev, 0);
    if (ret < 0) {
        printf("claim interface failed!\n");
        exit(1);
        // return ret;
    }

    ret = libusb_set_configuration( dev, 1);
    if (ret < 0) {
        printf("setting configuraiton failed!\n");
        // return ret;
    }
    
    // set power
    ret = set_power(dev, 0x04, POWER_RETRIES);
    if (ret < 0) {
        printf("setting power failed!\n");
        // return ret;
    }

    // ?
    ret = libusb_control_transfer(dev, 0x40, 0xC9, 0x00, 0x00, NULL, 0, TIMEOUT);
    if (ret < 0) {
        printf("setting reg 0xC9 failed!\n");
        return ret;
    }

    // set capture channel
    ret = set_channel(dev, channel);
    if (ret < 0) {
        printf("setting channel failed!\n");
        return ret;
    }

    // start capture?
    ret = libusb_control_transfer(dev, 0x40, SET_START, 0x00, 0x00, NULL, 0, TIMEOUT);

    return ret;
}

void bulk_read(pcap_dumper_t* pd, libusb_device_handle *dev)
{
    uint8_t data[200];
    while (1) {
        int xfer = 0;
        int ret = libusb_bulk_transfer(dev, 0x83, data, sizeof(data), &xfer, TIMEOUT);
        if (ret == 0) {
           /* int i;
            for (i = 0; i < xfer; i++) {
                if ((i % 16) == 0) {
                    printf("\n%04X:", i);
                }
                printf(" %02X", data[i]);
            }
            printf("\n");
*/
            struct pcap_pkthdr packet_header;
            struct timeval ts;
/*
            if (data[0] != 0) {
              printf("LEN TOO LARGE");
              return;
            }
            int len = data[1];
            if (len != xfer - 3) {
               printf("LEN inconsistent");
              return;
            }
            int plen = data[7];
            if (plen != len - 5) {
              printf("plen %d inconsistent %d\n", plen, len);
              return;
            }*/
            printf(".");
            if (xfer > 8 ) {
            gettimeofday(&packet_header.ts, NULL);
            packet_header.caplen = xfer - 8 -2;
            packet_header.len = xfer -8 -2;
            pcap_dump( (u_char * ) pd, &packet_header, data+8);
            }
            
        } 
    }
}


static void sniff(libusb_context *context, uint16_t pid, uint16_t vid, int channel)
{
    libusb_device_handle *dev = libusb_open_device_with_vid_pid(context, pid, vid);
    if (dev != NULL) {
        printf("Opened USB device %04X:%04X\n", pid, vid);

        int ret;
        ret = setup(dev, channel);
        if (ret < 0) {
            printf("Sniffer setup failed!\n");
        } else {
            pcap_t *pcapt = pcap_open_dead( 251, 100);
            pcap_dumper_t *pd = pcap_dump_open( pcapt, "out.pcap");
            bulk_read(pd, dev);
        }

        libusb_close(dev);
    } else {
        printf("USB device %04X:%04X not found!\n", pid, vid);
    }
}

int main(int argc, char *argv[])
{
    // channels 37,38,39 are the advertisement channels
    int channel = 37;
    if (argc > 1) {
        channel = atoi(argv[1]);
    }
    printf("Sniffing BLE traffic on channel %d\n", channel);

    libusb_context *context;
    int ret = libusb_init(&context);
    if (ret != 0) {
       printf("init failed!\n");
       exit(1);
    }
    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
    sniff(context, 0x451, 0x16B3, channel);
    libusb_exit(context);
    
    return 0;
}

