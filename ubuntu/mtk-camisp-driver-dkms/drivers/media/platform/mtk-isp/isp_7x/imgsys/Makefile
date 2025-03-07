# SPDX-License-Identifier: GPL-2.0
#

ccflags-y += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/mtk-hcp
ccflags-y += -I$(srctree)/include/linux/mailbox/
ccflags-y += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/imgsys/
ccflags-y += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/imgsys/platforms/
ccflags-$(CONFIG_VIDEO_MTK_ISP_70_IMGSYS) += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/imgsys/platforms/isp_70
ccflags-$(CONFIG_VIDEO_MTK_ISP_71_IMGSYS) += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/imgsys/platforms/isp_71
ccflags-y += -I$(top)/drivers/media/platform/mtk-isp/isp_7x/mtk-ipesys-me/
ccflags-y += -I$(srctree)/drivers/iommu
ccflags-y += -I$(srctree)/include/soc/mediatek

mtk_imgsys_util-objs := \
mtk_imgsys-dev.o \
mtk_imgsys-sys.o \
mtk_imgsys-v4l2.o \
mtk_imgsys-cmdq.o \
mtk_imgsys-worker.o \
mtk_imgsys-of.o \
mtk_imgsys-trace.o

ifeq ($(MTK_IMGSYS_VER), 71)
mtk_imgsys_hw_isp-objs := \
platforms/isp_71/mtk_imgsys-debug.o \
platforms/isp_71/modules/mtk_imgsys-dip.o \
platforms/isp_71/modules/mtk_imgsys-traw.o \
platforms/isp_71/modules/mtk_imgsys-pqdip.o \
platforms/isp_71/modules/mtk_imgsys-wpe.o \
platforms/isp_71/modules/mtk_imgsys-adl.o
else ifeq ($(MTK_IMGSYS_VER), 70)
mtk_imgsys_hw_isp-objs := \
platforms/isp_70/mtk_imgsys-debug.o \
platforms/isp_70/modules/mtk_imgsys-dip.o \
platforms/isp_70/modules/mtk_imgsys-traw.o \
platforms/isp_70/modules/mtk_imgsys-pqdip.o \
platforms/isp_70/modules/mtk_imgsys-wpe.o
endif

obj-$(CONFIG_VIDEO_MTK_ISP_7_IMGSYS) += mtk_imgsys_util.o
obj-$(CONFIG_VIDEO_MTK_ISP_7_IMGSYS) += mtk_imgsys_hw_isp.o
