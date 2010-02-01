CC	:= gcc
CFLAGS	+= -Wall -g
TARGET	:= librtp.so
OBJ	:= \
	md5c.o \
	rtp_api.o \
	rtp_error.o \
	rtp_member_api.o \
	rtp_member_inner.o \
	rtp_network_api.o \
	rtp_network_inner.o \
	rtp_session_api.o \
	rtp_session_inner.o

all: $(TARGET)

$(TARGET): CFLAGS += -shared -fpic
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^
		
clean:
	$(RM) $(OBJ) $(TARGET)

.PHONY: all clean
