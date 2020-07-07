LIBUSB = libusb-1.0
LIBPCAP = libpcap

CFLAGS = -W -Wall -O -g
CFLAGS += $(shell pkg-config $(LIBUSB) --cflags) $(shell pkg-config $(LIBPCAP) --cflags)
LDFLAGS = $(shell pkg-config $(LIBUSB) --libs)  $(shell pkg-config $(LIBPCAP) --libs) 


all: rawsniff

clean:
	rm -f rawsniff

