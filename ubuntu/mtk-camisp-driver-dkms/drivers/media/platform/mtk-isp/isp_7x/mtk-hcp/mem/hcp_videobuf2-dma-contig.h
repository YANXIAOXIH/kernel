/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _HCP_MEDIA_VIDEOBUF2_DMA_CONTIG_H
#define _HCP_MEDIA_VIDEOBUF2_DMA_CONTIG_H

#include <media/videobuf2-v4l2.h>
#include <linux/dma-mapping.h>

int hcp_vb2_dma_contig_set_max_seg_size(struct device *dev, unsigned int size);
void hcp_vb2_dma_contig_clear_max_seg_size(struct device *dev);

extern const struct vb2_mem_ops hcp_vb2_dma_contig_memops;

#endif
