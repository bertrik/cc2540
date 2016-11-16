LIBUSB = libusb-1.0

CFLAGS = -W -Wall -O
CFLAGS += $(shell pkg-config $(LIBUSB) --cflags)
LDFLAGS = $(shell pkg-config $(LIBUSB) --libs)

all: rawsniff

