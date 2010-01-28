#
# Makefile for the NCTUns engine module
#

obj-y	= \
	mac/ \
	phy/

ifeq ($(CONFIG_MobileWIMAX),y)
  obj-$(CONFIG_MobileWIMAX) += \
	mac_16e/ \
	phy_16e/
endif
ifeq ($(CONFIG_MobileRelayWIMAX),y)
  obj-$(CONFIG_MobileRelayWIMAX) += \
	mac_16j/ \
	phy_16j/
endif
ifeq ($(CONFIG_MR_WIMAX_NT),y)
  obj-$(CONFIG_MR_WIMAX_NT) += \
	mac_16j_NT/ \
	phy_16j_NT/ 
endif
