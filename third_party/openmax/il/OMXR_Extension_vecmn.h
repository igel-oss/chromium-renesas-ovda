/*
 * Copyright(C) 2015-2017 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 */
/**
 * OMXR Extension header for video encoder common
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vecmn.h
 */

#ifndef OMXR_EXTENSION_VECMN_H
#define OMXR_EXTENSION_VECMN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#include "OMXR_Extension_video.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/**
 * extended #OMX_INDEXTYPE for video encoder
 */
enum {
    OMXR_MC_IndexParamVideoPictureMemoryAlloc               = (OMXR_MC_IndexVendorBaseVideoEncoder + 0x0000),   /**< OMX.RENESAS.INDEX.PARAM.VIDEO.PICTURE.MEMORY.ALLOC */
    OMXR_MC_IndexParamVideoRatecontrolPreference            = (OMXR_MC_IndexVendorBaseVideoEncoder + 0x0001),   /**< OMX.RENESAS.INDEX.PARAM.VIDEO.RATECONTROL.PREFERENCE */
    OMXR_MC_IndexParamVideoHierarchicalEncodingPreference   = (OMXR_MC_IndexVendorBaseVideoEncoder + 0x0002),   /**< OMX.RENESAS.INDEX.PARAM.VIDEO.HIERARCHICAL.ENCODING.PREFERENCE */
    OMXR_MC_IndexParamVideoIntraRefresh                     = (OMXR_MC_IndexVendorBaseVideoEncoder + 0x0003)    /**< OMX.RENESAS.INDEX.PARAM.VIDEO.INTRA.REFRESH */
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * Extended MemoryAlloc Mode.
 */
typedef enum OMXR_MC_VIDEO_MEMORYALLOCTYPE {
	OMXR_MC_VIDEO_MemAllocFrame        = 0,         /**< frame structure */
	OMXR_MC_VIDEO_MemAllocFrameTff     = 1,         /**< frame structure and top filed first */
	OMXR_MC_VIDEO_MemAllocFrameBff     = 2,         /**< frame structure and bottom filed first */
	OMXR_MC_VIDEO_MemAllocField        = 3,         /**< filed structure */
	OMXR_MC_VIDEO_MemAllocFieldTff     = 4,         /**< filed structure and top filed first */
	OMXR_MC_VIDEO_MemAllocFieldBff     = 5,         /**< filed structure and bottom filed first */
	OMXR_MC_VIDEO_MemAllocFrameOrField = 6,         /**< frame or filed structure */
	OMXR_MC_VIDEO_MemAllocEnd          = 0x7FFFFFFF /**< type end */
} OMXR_MC_VIDEO_MEMORYALLOCTYPE;

/**
 * Extended MemoryAlloc Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_PICTURE_MEMORY_ALLOCTYPE {
    OMX_U32                       nSize;            /**< size of the structure  */
    OMX_VERSIONTYPE               nVersion;         /**< OMX specification version info */
    OMX_U32                       nPortIndex;       /**< target port index */
    OMXR_MC_VIDEO_MEMORYALLOCTYPE eMemoryAlloc;     /**< memory structure */
    OMX_BOOL                      bIPConvert;       /**< (not support member) */
    OMX_BOOL                      bTLConvert;       /**< (not support member) */
} OMXR_MC_VIDEO_PARAM_PICTURE_MEMORY_ALLOCTYPE;

/**
 * Extended RatecontrolPreference Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_RATECONTROL_PREFERENCETYPE {
    OMX_U32         nSize;            /**< size of the structure  */
    OMX_VERSIONTYPE nVersion;         /**< OMX specification version info */
    OMX_U32         nPortIndex;       /**< target port index */
    OMX_U32         nMinQpI;          /**< the minimum value of quantization for I picture. Supported range is relied on codec type. */
    OMX_U32         nMinQpP;          /**< the minimum value of quantization for P picture. Supported range is relied on codec type. */
    OMX_U32         nMinQpB;          /**< the minimum value of quantization for B picture. Supported range is relied on codec type. */
    OMX_U32         nMaxQpI;          /**< the maximum value of quantization for I picture. Supported range is relied on codec type. */
    OMX_U32         nMaxQpP;          /**< the maximum value of quantization for P picture. Supported range is relied on codec type. */
    OMX_U32         nMaxQpB;          /**< the maximum value of quantization for B picture. Supported range is relied on codec type. */
    OMX_U32         nMaxPicBitsI;     /**< Set the largest bit amount about target picture type. If this parameter was set 0 (default value), this library does not clip amount of bitstream by this value. The valid range of this value is 0, 200 to 40000000. */
    OMX_U32         nMaxPicBitsP;     /**< Set the largest bit amount about target picture type. If this parameter was set 0 (default value), this library does not clip amount of bitstream by this value. The valid range of this value is 0, 200 to 40000000. */
    OMX_U32         nMaxPicBitsB;     /**< Set the largest bit amount about target picture type. If this parameter was set 0 (default value), this library does not clip amount of bitstream by this value. The valid range of this value is 0, 200 to 40000000. */
    OMX_U32         nMaxEsBytes;      /**< the maximum elementary stream size in bytes of a picture */
} OMXR_MC_VIDEO_PARAM_RATECONTROL_PREFERENCETYPE;

/**
* Extended HierarchicalEncodingPreference Type.
*/
typedef struct tagOMXR_MC_VIDEO_PARAM_HIERARCHICAL_ENCODING_PREFERENCETYPE {
	OMX_U32         nSize;             /**< size of the structure  */
	OMX_VERSIONTYPE nVersion;          /**< OMX specification version info */
	OMX_U32         nPortIndex;        /**< target port index */
	OMX_U32         nLevel;            /**< Set the level of hierarchical encoding. 0: Non-hierarchical coding (default), 1: Hierarchical encoding level 1, 2: Hierarchical encoding level 2.  */
	OMX_U32         nNumNonRefPFrames; /**< Set the number of non-reference P-frames for hierarchical encoding. 0: Non-hierarchical coding(default) */
} OMXR_MC_VIDEO_PARAM_HIERARCHICAL_ENCODING_PREFERENCETYPE;

/**
 * Extended IntraRefresh type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_INTRA_REFRESHTYPE {
       OMX_U32         nSize;             /**< size of the structure  */
       OMX_VERSIONTYPE nVersion;          /**< OMX specification version info */
       OMX_U32         nPortIndex;        /**< target port index */
       OMX_U32         nIntraRefreshCycle;/**< intra refresh cycle */
} OMXR_MC_VIDEO_PARAM_INTRA_REFRESHTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VECMN_H */
