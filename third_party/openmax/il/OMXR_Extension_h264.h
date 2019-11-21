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
 * OMXR Extension header for H.264 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h264.h
 * 
 */

#ifndef OMXR_EXTENSION_H264_H
#define OMXR_EXTENSION_H264_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_video.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_VendorBaseOffsetH264     (0x000001000)		/**< base offset for H.264 (internal use) */

#define OMXR_MC_IndexVendorBaseH264      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetH264)  /**< base value of extended OMX_INDEXTYPE for H.264  */
#define OMXR_MC_EventVendorBaseH264      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetH264)  /**< base value of extended OMX_EVENTTYPE for H.264  */
#define OMXR_MC_ErrorVendorBaseH264      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetH264)  /**< base value of extended OMX_ERRORTYPE for H.264  */

/** 
 * Enumeration of extended #OMX_VIDEO_AVCPROFILETYPE 
 */
enum {
    OMXR_MC_VIDEO_AVCProfileMultiviewHigh = OMX_VIDEO_AVCProfileVendorStartUnused,	/**< Multiview profile            */
    OMXR_MC_VIDEO_AVCProfileStereoHigh,												/**< Stereo High profile          */
    OMXR_MC_VIDEO_AVCProfileConstrainedBaseline,									/**< Constrained baseline profile */
    OMXR_MC_VIDEO_AVCProfileUnknown,												/**< unknown profile              */
    OMXR_MC_VIDEO_AVCProfileNone,                                                   /**< no profile                   */
    OMXR_MC_VIDEO_AVCProfileCustomizeStartUnused                                    /**< reserved to customize        */
};

/** 
 * Enumeration of extended #OMX_VIDEO_AVCLEVELTYPE 
 */
enum {
    OMXR_MC_VIDEO_AVCLevelUnknown = OMX_VIDEO_AVCLevelVendorStartUnused,			/**< unknown level         */
    OMXR_MC_VIDEO_AVCLevelNone,                                             		/**< no level              */
    OMXR_MC_VIDEO_AVCLevelCustomizeStartUnused                              		/**< reserved to customize */
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H264_H */
