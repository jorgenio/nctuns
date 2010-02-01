TARGET	:= adapt_bw rtprecvonly rtpsendrecv adapt_sessionbw

LIBRTP	:= ../../../lib/librtp/
CPPFLAGS += -I$(LIBRTP)
CFLAGS	+= -D_GNU_SOURCE
LDLIBS := -L$(LIBRTP) -lrtp

all: rtp

rtp:
	$(MAKE) -C $(LIBRTP)

include ../../scripts/Makefile.tools
