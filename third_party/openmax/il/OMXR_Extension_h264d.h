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
 * OMXR Extension header for H.264 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h264d.h
 * 
 */

#ifndef OMXR_EXTENSION_H264D_H
#define OMXR_EXTENSION_H264D_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_h264.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseH264Decoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH264)        /**< base value of extended OMX_INDEXTYPE for H.264 decoder  */
#define OMXR_MC_EventVendorBaseH264Decoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH264)        /**< base value of extended OMX_EVENTTYPE for H.264 decoder  */
#define OMXR_MC_ErrorVendorBaseH264Decoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH264)        /**< base value of extended OMX_ERRORTYPE for H.264 decoder  */

#define OMXR_MC_SyntaxIndexBaseH264Decoder     (OMXR_MC_VideoSyntaxIndexBase + OMXR_MC_VendorBaseOffsetH264)

#define OMXR_MC_VIDEO_NOEL_AVC_CONST     (6u)
#define OMXR_MC_VIDEO_NOEL_AVC_NRF_POC   (255u)
#define OMXR_MC_VIDEO_NOEL_AVC_HRD       (32u)

/**
 * extended #OMX_INDEXTYPE for H.264 decoder
 */
enum {
    OMXR_MC_IndexParamVideoStreamStoreUnit = (OMXR_MC_IndexVendorBaseH264Decoder + 0x0000),   /**< stream store unit. parameter name:OMX.RENESAS.INDEX.PARAM.VIDEO.STREAM.STORE.UNIT */
    OMXR_MC_IndexParamVideoAvcIgnoreLowDelayHint = (OMXR_MC_IndexVendorBaseH264Decoder + 0x0001)   /**< stream store unit. parameter name:OMX.RENESAS.INDEX.PARAM.VIDEO.IGNORE.LOW.DELAY.HINT */
};

enum {
    OMXR_MC_IndexConfigVideoInfoAvcSyntax = (OMXR_MC_SyntaxIndexBaseH264Decoder + 0x0000)
};

/**
 * stream store unit type
 */
typedef enum OMXR_MC_VIDEO_STOREUNITTYPE {
    OMXR_MC_VIDEO_StoreUnitEofSeparated       = 0,            /**< EOF separated mode        */
    OMXR_MC_VIDEO_StoreUnitTimestampSeparated = 1,            /**< time stamp separated mode */
    OMXR_MC_VIDEO_StoreUnitEnd                = 0x7FFFFFFF    /**< definition end            */
} OMXR_MC_VIDEO_STOREUNITTYPE;


/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * Structure to Set/Get stream store unit type
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_STREAM_STORE_UNITTYPE {
    OMX_U32                       nSize;             /**< Size of the structure in bytes   */
    OMX_VERSIONTYPE               nVersion;          /**< OMX specification version info   */
    OMX_U32                       nPortIndex;        /**< Port that this struct applies to */
    OMXR_MC_VIDEO_STOREUNITTYPE   eStoreUnit;        /**< Stream store unit type           */
} OMXR_MC_VIDEO_PARAM_STREAM_STORE_UNITTYPE;

/**
* Ignore low-delay hint type
*
* Ignore low-delay hint in input stream, and use this specified parameter instead.
* This parameter works well  when input stream contains wrong low-delay hint information.
*/
typedef struct tagOMXR_MC_VIDEO_PARAM_IGNORE_LOW_DELAY_HINTTYPE {
    OMX_U32           nSize;                /**< Size of the structure */
    OMX_VERSIONTYPE   nVersion;             /**< OMX specification version info */
    OMX_U32           nPortIndex;           /**< Target port index */
    OMX_BOOL          bEnable;              /**< Default: False, Use max_num_reorder_frames in stream. OMX_TRUE: Ignore max_num_reorder_frames in stream .  */
}OMXR_MC_VIDEO_PARAM_IGNORE_LOW_DELAY_HINTTYPE;

/**
* vui syntax information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_AVC_VUI_SYNTAX {
    OMX_U32    aspect_ratio_info_present_flag;
    OMX_U32    aspect_ratio_idc;
    OMX_U32    sar_width;
    OMX_U32    sar_height;
    OMX_U32    overscan_info_present_flag;
    OMX_U32    overscan_appropriate_flag;
    OMX_U32    video_signal_type_present_flag;
    OMX_U32    video_format;
    OMX_U32    video_full_range_flag;
    OMX_U32    colour_description_present_flag;
    OMX_U32    colour_primaries;
    OMX_U32    transfer_characteristics;
    OMX_U32    matrix_coefficients;
    OMX_U32    chroma_loc_info_present_flag;
    OMX_U32    chroma_sample_loc_type_top_field;
    OMX_U32    chroma_sample_loc_type_bottom_field;
    OMX_U32    timing_info_present_flag;
    OMX_U32    num_units_in_tick;
    OMX_U32    time_scale;
    OMX_U32    fixed_frame_rate_flag;
    OMX_U32    nal_hrd_parameters_present_flag;
    OMX_U32    nal_hrd_cpb_cnt_minus1;
    OMX_U32    nal_hrd_bit_rate_scale;
    OMX_U32    nal_hrd_cpb_size_scale;
    OMX_U32    nal_hrd_bit_rate_value_minus1[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    nal_hrd_cpb_size_value_minus1[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    nal_hrd_cbr_flag[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    nal_hrd_initial_cpb_removal_delay_length_minus1;
    OMX_U32    nal_hrd_cpb_removal_delay_length_minus1;
    OMX_U32    nal_hrd_dpb_output_delay_length_minus1;
    OMX_U32    nal_hrd_time_offset_length;
    OMX_U32    vcl_hrd_parameters_present_flag;
    OMX_U32    vcl_hrd_cpb_cnt_minus1;
    OMX_U32    vcl_hrd_bit_rate_scale;
    OMX_U32    vcl_hrd_cpb_size_scale;
    OMX_U32    vcl_hrd_bit_rate_value_minus1[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    vcl_hrd_cpb_size_value_minus1[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    vcl_hrd_cbr_flag[OMXR_MC_VIDEO_NOEL_AVC_HRD];
    OMX_U32    vcl_hrd_initial_cpb_removal_delay_length_minus1;
    OMX_U32    vcl_hrd_cpb_removal_delay_length_minus1;
    OMX_U32    vcl_hrd_dpb_output_delay_length_minus1;
    OMX_U32    vcl_hrd_time_offset_length;
    OMX_U32    low_delay_hrd_flag;
    OMX_U32    pic_struct_present_flag;
    OMX_U32    bitstream_restriction_flag;
    OMX_U32    motion_vectors_over_pic_boundaries_flag;
    OMX_U32    max_bytes_per_pic_denom;
    OMX_U32    max_bits_per_mb_denom;
    OMX_U32    log2_max_mv_length_horizontal;
    OMX_U32    log2_max_mv_length_vertical;
    OMX_U32    max_num_reorder_frames;
    OMX_U32    max_dec_frame_buffering;
} OMXR_MC_VIDEO_INFO_AVC_VUI_SYNTAXTYPE;

/**
* sps syntax information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_AVC_SPS_SYNTAX {
    OMX_U32    struct_size;
    OMXR_MC_VIDEO_SYNTAX_STATUS    status;
    OMX_U32    nal_ref_idc;
    OMX_U32    nal_unit_type;
    OMX_U32    profile_idc;
    OMX_U32    constraint_setx_flag[OMXR_MC_VIDEO_NOEL_AVC_CONST];
    OMX_U32    level_idc;
    OMX_U32    seq_parameter_set_id;
    OMX_U32    chroma_format_idc;
    OMX_U32    log2_max_frame_num_minus4;
    OMX_U32    pic_order_cnt_type;
    OMX_U32    log2_max_pic_order_cnt_lsb_minus4;
    OMX_U32    delta_pic_order_always_zero_flag;
    OMX_S32    offset_for_non_ref_pic;
    OMX_S32    offset_for_top_to_bottom_field;
    OMX_U32    num_ref_frames_in_pic_order_cnt_cycle;
    OMX_S32    offset_for_ref_frame[OMXR_MC_VIDEO_NOEL_AVC_NRF_POC];
    OMX_U32    max_num_ref_frames;
    OMX_U32    gaps_in_frame_num_value_allowed_flag;
    OMX_U32    pic_width_in_mbs_minus1;
    OMX_U32    pic_height_in_map_units_minus1;
    OMX_U32    frame_mbs_only_flag;
    OMX_U32    mb_adaptive_frame_field_flag;
    OMX_U32    direct_8x8_inference_flag;
    OMX_U32    frame_cropping_flag;
    OMX_U32    frame_crop_left_offset;
    OMX_U32    frame_crop_right_offset;
    OMX_U32    frame_crop_top_offset;
    OMX_U32    frame_crop_bottom_offset;
    OMX_U32    vui_parameters_present_flag;
    OMXR_MC_VIDEO_INFO_AVC_VUI_SYNTAXTYPE vui;
    OMX_PTR    reserved;
}OMXR_MC_VIDEO_INFO_AVC_SPS_SYNTAXTYPE;


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H264D_H */
