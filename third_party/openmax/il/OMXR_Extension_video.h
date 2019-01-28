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
 * OMXR Extension header for video common
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_video.h
 */

#ifndef OMXR_EXTENSION_VIDEO_H
#define OMXR_EXTENSION_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension.h"
#include "OMX_VideoExt.h"
#include "OMX_IndexExt.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/**
 * Base offset of the vendor-defined extensions for video (OMX_INDEXTYPE)
 */
enum {
    OMXR_MC_IndexVendorBaseVideoDecoder = OMXR_MC_IndexVendorBaseVideo + 0x00004000,    /**< Video decoder domain */
    OMXR_MC_IndexVendorBaseVideoEncoder = OMXR_MC_IndexVendorBaseVideo + 0x00008000     /**< Video encoder domain */
};

/**
 * Base offset of the vendor-defined extensions for video (OMX_EVENTTYPE)
 */
enum {
    OMXR_MC_EventVendorBaseVideoDecoder = OMXR_MC_EventVendorBaseVideo + 0x00004000,    /**< Video decoder domain */
    OMXR_MC_EventVendorBaseVideoEncoder = OMXR_MC_EventVendorBaseVideo + 0x00008000     /**< Video encoder domain */
};

/**
 * Base offset of the vendor-defined extensions for video (OMX_ERRORTYPE)
 */
enum {
    OMXR_MC_ErrorBaseVideoDecoder = OMXR_MC_ErrorBaseVideo + 0x00004000,                /**< Video decoder domain */
    OMXR_MC_ErrorBaseVideoEncoder = OMXR_MC_ErrorBaseVideo + 0x00008000                 /**< Video encoder domain */
};


/**
 * Enumeration defining possible uncompressed video formats
 */
enum {
    OMX_COLOR_FormatYVU420Planar               = OMX_COLOR_FormatVendorStartUnused + 0x00000000,  /**< YVU Planar format */
    OMX_COLOR_FormatYVU420SemiPlanar           = OMX_COLOR_FormatVendorStartUnused + 0x00000001,  /**< YVU Semi-Planar format */
    OMX_COLOR_FormatYUV420PlanarMultiPlane     = OMX_COLOR_FormatVendorStartUnused + 0x00000002,
    OMX_COLOR_FormatYUV420SemiPlanarMultiPlane = OMX_COLOR_FormatVendorStartUnused + 0x00000003
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
* Extended physical image address Type.
*/

#define OMXR_EXTEND_ADDRESS_PLANE_NUM	3

typedef struct tagOMXR_MC_VIDEO_EXTEND_ADDRESSTYPE {
    OMX_U32    nSize;                                            /**< Size of the structure */
	OMX_PTR    pvVirtAddr[OMXR_EXTEND_ADDRESS_PLANE_NUM];        /**< Top address of Virtual Address */
	OMX_U32    u32HwipAddr[OMXR_EXTEND_ADDRESS_PLANE_NUM];       /**< Top address of HWIP Address */
	OMX_U32    u32AllocateSize[OMXR_EXTEND_ADDRESS_PLANE_NUM];   /**< Allocate Size */
} OMXR_MC_VIDEO_EXTEND_ADDRESSTYPE;


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VIDEO_H */
