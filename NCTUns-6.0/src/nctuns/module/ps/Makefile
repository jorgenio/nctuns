#
# Makefile for the NCTUns engine module
#

obj-y	= \
	DRR/ \
	DS/ \
	FIFO/ \
	RED/ \
	WAN/

ifeq ($(CONFIG_GPRS),y)
  obj-$(CONFIG_GPRS) += GPRS_FIFO/
endif
