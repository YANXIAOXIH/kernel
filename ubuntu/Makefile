#
# Makefile for the Linux kernel ubuntu supplied third-party device drivers.
#

#
# NOTE: to allow drivers to be added and removed without causing merge
# collisions you should add new entries in the middle of the six lines
# of ## at the bottom of the list.  Always add three lines of ## above
# your new entry and maintain the six lines below.
#

##
##
##
##
##
##
obj-y += media-i2c-it6510/
##
##
##
obj-y += mtk-apusys-driver/
##
##
##
obj-$(CONFIG_MTK_FSOURCE) += mtk-fsource-driver/
##
##
##
obj-y += mtk-mdp-driver/
##
##
##
obj-y += mtk-mt76/
##
##
##
obj-y += mtk-vcodec-driver/
##
##
##
obj-y += mtk-mali-gpu-driver-r43/
##
##
##
obj-y += mtk-vcu-driver/
##
##
##
obj-$(CONFIG_UBUNTU_HOST)      += ubuntu-host/
##
##
##
##
##
##

# This is a stupid trick to get kbuild to create ubuntu/built-in.o
obj- += foo.o
