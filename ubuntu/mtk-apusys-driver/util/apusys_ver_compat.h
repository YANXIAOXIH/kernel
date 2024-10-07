/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _APUSYS_VERSION_COMPAT_H_
#define _APUSYS_VERSION_COMPAT_H_

#include <linux/version.h>

#if KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE
#define class_create_compat(module, name) \
    class_create(name)
#else
#define class_create_compat(module, name) \
    class_create(module, name)
#endif /* KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE */

#if KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE
#define PDE_DATA(inode) \
    pde_data(inode)
#endif /* KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE */

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE

#define iommu_map_sg_compat(domain, iova, sg, nents, prot) \
    iommu_map_sg(domain, iova, sg, nents, prot, GFP_KERNEL)

#define iommu_map_compat(domain, iova, paddr, size, prot) \
    iommu_map(domain, iova, paddr, size, prot, GFP_KERNEL)

#else

#define iommu_map_sg_compat(domain, iova, sg, nents, prot) \
    iommu_map_sg(domain, iova, sg, nents, prot)

#define iommu_map_compat(domain, iova, paddr, size, prot) \
    iommu_map(domain, iova, paddr, size, prot)

#endif /* KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE */

#endif /* _APUSYS_VERSION_COMPAT_H_ */