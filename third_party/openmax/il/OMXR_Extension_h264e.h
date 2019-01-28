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
 * OMXR Extension header for H.264 encoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h264e.h
 */
#ifndef OMXR_EXTENSION_H264E_H
#define OMXR_EXTENSION_H264E_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vecmn.h"
#include "OMXR_Extension_h264.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseH264Encoder     (OMXR_MC_IndexVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetH264)		/**< base value of extended OMX_INDEXTYPE for H.264 encoder  */
#define OMXR_MC_EventVendorBaseH264Encoder     (OMXR_MC_EventVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetH264)		/**< base value of extended OMX_EVENTTYPE for H.264 encoder  */
#define OMXR_MC_ErrorVendorBaseH264Encoder     (OMXR_MC_ErrorVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetH264)		/**< base value of extended OMX_ERRORTYPE for H.264 encoder  */

/**
 * extended #OMX_INDEXTYPE for H.264 encoder
 */
enum {
    OMXR_MC_IndexParamVideoSequenceHeaderWithIDR     = (OMXR_MC_IndexVendorBaseH264Encoder + 0x0000),  /**< OMX.RENESAS.INDEX.PARAM.VIDEO.SEQUENCE.HEADER.WITH.IDR */
    OMXR_MC_IndexParamVideoAVCSyntaxOption           = (OMXR_MC_IndexVendorBaseH264Encoder + 0x0001),  /**< OMX.RENESAS.INDEX.PARAM.VIDEO.AVC.SYNTAX.OPTION */
    OMXR_MC_IndexParamVideoAVCVuiProperty            = (OMXR_MC_IndexVendorBaseH264Encoder + 0x0002)   /**< OMX.RENESAS.INDEX.PARAM.VIDEO.AVC.VUI.PROPERTY */
};


/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * Extended Sequence Header With IDR Type. 
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_SEQUENCE_HEADER_WITH_IDRTYPE{
    OMX_U32             nSize;                  /**< size of the structure */
    OMX_VERSIONTYPE     nVersion;               /**< OMX specification version info */
    OMX_U32             nPortIndex;             /**< target port index */
    OMX_BOOL            bSequenceHeaderWithIDR; /**< flag indicating the insertion of sequence header */
} OMXR_MC_VIDEO_PARAM_SEQUENCE_HEADER_WITH_IDRTYPE;

/**
 * Extended AVC Syntax Option Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_AVC_SYNTAX_OPTION{
    OMX_U32           nSize;            /**< size of the structure  */
    OMX_VERSIONTYPE   nVersion;         /**< OMX specification version info */
    OMX_U32           nPortIndex;       /**< target port index */
    OMX_BOOL          bAUDEnable;       /**< flag indicating the insertion of AUD */
    OMX_BOOL          bVUIEnable;       /**< flag indicating the insertion of VUI */
    OMX_U32           u32PPSNum;        /**< flag indicating the number of insertion of PPS */
} OMXR_MC_VIDEO_PARAM_AVC_SYNTAX_OPTION;

/**
* Extended AVC YUI Property Type.
*/
typedef struct tagOMXR_MC_VIDEO_PARAM_AVC_VUI_PROPERTY {
    OMX_U32           nSize;                         /**< size of the structure  */
    OMX_VERSIONTYPE   nVersion;                      /**< OMX specification version info */
    OMX_U32           nPortIndex;                    /**< target port index */
    OMX_BOOL          bAspectRatioInfoPresentFlag;   /**< aspect_ratio_info_present_flag */
    OMX_U32           u32AspectRatioIdc;             /**< aspect_ratio_idc */
    OMX_U32           u32SarWidth;                   /**< sar_width */
    OMX_U32           u32SarHeight;                  /**< sar_height */
    OMX_BOOL          bVideoSignalTypePresentFlag;   /**< video_signal_type_present_flag */
    OMX_U32           u32VideoFormat;                /**< video_format */
    OMX_BOOL          bVideoFullRangeFlag;           /**< video_full_range_flag */
    OMX_BOOL          bColourDescriptionPresentFlag; /**< colour_description_present_flag */
    OMX_U32           u32ColourPrimaries;            /**< colour_primaries */
    OMX_U32           u32TransferCharacteristics;    /**< transfer_characteristics */
    OMX_U32           u32MatrixCoefficients;         /**< matrix_coefficients */
    OMX_BOOL          bTimingInfoPresentFlag;        /**< timing_info_present_flag */
    OMX_U32           u32NumUnitsInTick;             /**< num_units_in_tick */
    OMX_U32           u32TimeScale;                  /**< time_scale */
    OMX_BOOL          bFixedFrameRateFlag;           /**< fixed_frame_rate_flag */
} OMXR_MC_VIDEO_PARAM_AVC_VUI_PROPERTY;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H264E_H */
