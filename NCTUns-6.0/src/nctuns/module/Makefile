#
# Makefile for the NCTUns engine module
#

obj-y	= \
	arp/ \
	hub/ \
	its/ \
	mac/ \
	misc/ \
	nctuns-dep/ \
	mobile/ \
	phy/ \
	ps/ \
	route/ \
	sw/ \
	tcpdump/ \
	80211p/

ifeq ($(CONFIG_GPRS),y)
  obj-$(CONFIG_GPRS) += gprs/
endif

ifeq ($(CONFIG_MESH),y)
  obj-$(CONFIG_MESH) += mesh/
endif

ifeq ($(CONFIG_OPTICAL),y)
  obj-$(CONFIG_OPTICAL) += opt/
endif

ifeq ($(CONFIG_SATELLITE),y)
  obj-$(CONFIG_SATELLITE) += satellite/
endif

ifeq ($(CONFIG_WIMAX),y)
  obj-$(CONFIG_WIMAX) += wimax/
endif
