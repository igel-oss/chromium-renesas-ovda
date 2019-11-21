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
 * OMXR Extension header for video decoder common
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vdcmn.h
 */

#ifndef OMXR_EXTENSION_VDCMN_H
#define OMXR_EXTENSION_VDCMN_H

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
/* Flag indicating the cause of decoding error. */
#define OMXR_MC_VIDEO_DECODE_ERROR_FLAG_CAUTION  0x00000001 /**< If this value is set in u32DecodeError, it indicates that it has detected a minor error that have no effects on output picture data. */
#define OMXR_MC_VIDEO_DECODE_ERROR_FLAG_CONCEAL  0x00000002 /**< If this value is set in u32DecodeError, it indicates that it has detected a minor error that need error concealment. */

#define OMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITY_MAXIMUM_LEVEL ( 0xFFFFFFFFu )    /**< The value of the maximum level for eMaxLevel */

/**
* Extended Work Buffer Preference.
*/
#define OMXR_MC_VIDEO_WORKBUFFER_PREFERENCE_BUFFER_SIZE_MIN (4096)
#define OMXR_MC_VIDEO_WORKBUFFER_PREFERENCE_BUFFER_SIZE_FOR_2K (1572864)
#define OMXR_MC_VIDEO_WORKBUFFER_PREFERENCE_BUFFER_SIZE_FOR_4K (6291456)
#define OMXR_MC_VIDEO_WORKBUFFER_PREFERENCE_DEFAULT_NUM_BUFFERING_PIC (3)


enum {
    OMXR_MC_IndexParamVideoReorder                     = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0000),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.REORDER           */
    OMXR_MC_IndexParamVideoDeinterlaceMode             = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0001),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.DEINITERLACEMODE  */
    OMXR_MC_IndexParamVideoErrorConcealment            = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0002),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.ERROR.CONCEALMENT */
    OMXR_MC_IndexParamVideoDynamicPortReconfInDecoding = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0003),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.DYNAMIC.PORTRECONF.INDECODING */
    OMXR_MC_IndexParamVideoMaximumDecodeCapability     = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0004),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.MAXIMUM.DECODE.CAPABILITY */
    OMXR_MC_IndexParamVideoWorkBufferPreference        = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0005),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.WORKBUFFERPREFERENCE */
    OMXR_MC_IndexParamVideoBypassPostprocessing        = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0006),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.BYPASSPOSTPROCESSING */
    OMXR_MC_IndexParamVideoLossyCompression            = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0007),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.LOSSY.COMPRESSION */
    OMXR_MC_IndexConfigVideoSyntaxInfo                 = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0008),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.SYNTAXINFO */
    OMXR_MC_IndexParamVideoTimeStampMode               = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x0009),  /* OMX.RENESAS.INDEX.PARAM.VIDEO.TIMESTAMPMODE */
    OMXR_MC_IndexParamVideoEnableExtendedPSCEvent      = (OMXR_MC_IndexVendorBaseVideoDecoder + 0x000A)   /* OMX.RENESAS.INDEX.PARAM.VIDEO.ENABLEEXTENDEDPSCEVENT */
};

/**
 * Extended Deinterlace Mode Type.
 */
typedef enum OMXR_MC_VIDEO_DEINTERLACE_MODETYPE {
    OMXR_MC_VIDEO_DeinterlaceNone   = 0,            /**< Not applying IP */
    OMXR_MC_VIDEO_Deinterlace2DHalf = 1,            /**< 2D IP Conversion with half rate output */
    OMXR_MC_VIDEO_Deinterlace2DFull = 2,            /**< 2D IP Conversion with full rate output */
    OMXR_MC_VIDEO_Deinterlace3DHalf = 3,            /**< 3D IP Conversion with half rate output */
    OMXR_MC_VIDEO_Deinterlace3DFull = 4,            /**< 3D IP Conversion with full rate output */
    OMXR_MC_VIDEO_DeinterlaceEnd    = 0x7FFFFFFF    /**< terminate values */
} OMXR_MC_VIDEO_DEINTERLACE_MODETYPE;

/**
 * Extended TimeStamp Mode Type.
 */
typedef enum OMXR_MC_VIDEO_TIMESTAMPMODETYPE {
    OMXR_MC_VIDEO_TimeStampModeDisplayOrder = 0,
    OMXR_MC_VIDEO_TimeStampModeDecodeOrder  = 1,
    OMXR_MC_VIDEO_TimeStampModeEnd          = 0x7FFFFFFF
} OMXR_MC_VIDEO_TIMESTAMPMODETYPE;

/**
 * Display information of output picture 
 */
typedef enum OMXR_PICTURETYPE{
    OMXR_MC_VIDEO_DispOrderProgressive     = 0,
    OMXR_MC_VIDEO_DispOrderTopFieldFirst   = 1,
    OMXR_MC_VIDEO_DispOrderBotFieldFirst   = 2,
    OMXR_MC_VIDEO_DispOrderTopField        = 3,
    OMXR_MC_VIDEO_DispOrderBotField        = 4,
    OMXR_MC_VIDEO_DispOrderEnd             = 0x7FFFFFFF
} OMXR_PICTURETYPE;

/**
 * Syntax status
 */
typedef enum {
	OMXR_DEC_SYNTAX_STATUS_NONE      = 0x00,
	OMXR_DEC_SYNTAX_STATUS_VALID     = 0x01,
	OMXR_DEC_SYNTAX_STATUS_CAUTION   = 0x10,
	OMXR_DEC_SYNTAX_STATUS_UNSUPPORT = 0x11,
	OMXR_DEC_SYNTAX_STATUS_ERROR     = 0x02
} OMXR_MC_VIDEO_SYNTAX_STATUS;

/**
 * Syntax index base
 */
enum {
	OMXR_MC_VideoSyntaxIndexBase = 0x00000
} ;

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
 * Extended Reorder Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_REORDERTYPE {
    OMX_U32                       nSize;            /**< Size of the structure */
    OMX_VERSIONTYPE               nVersion;         /**< OMX specification version info */
    OMX_U32                       nPortIndex;       /**< Target port index */
    OMX_BOOL                      bReorder;         /**< The reorder flags */
} OMXR_MC_VIDEO_PARAM_REORDERTYPE;

/**
 * Extended Decode Result Type.
 */
typedef struct tagOMXR_MC_VIDEO_EXTENDED_DECODERESULTTYPE {
    OMX_U32          nSize;
    OMX_PTR          pvVirtAddr[2][3];
    OMX_U32          u32HwipAddr[2][3];
    OMX_U32          u32HwipAncAddr[2][3];
    OMXR_PICTURETYPE eDispOrder;
    OMX_PTR          pvReserved;
    OMX_PTR          pvReserved2;
} OMXR_MC_VIDEO_EXTENDED_DECODERESULTTYPE;

/**
 * Decode Result Type.
 */
typedef struct tagOMXR_MC_VIDEO_DECODERESULTTYPE {
    OMX_U32 nSize;                                               /**< Size of the structure */
    OMX_PTR pvPhysImageAddressY;                                 /**< Top physical address of dacoded picture data */
    OMX_U32 u32PictWidth;                                        /**< The width of the decoded picture data */
    OMX_U32 u32PictHeight;                                       /**< The height of the decoded picture data */
    OMX_U32 u32DecodeError;                                      /**< The decoded error flags. */
    OMX_U32 u32PhyAddr;                                          /**< This member is not used */
    OMX_U32 u32Stride;                                           /**< This member is not used */
    OMX_U32 u32Area;                                             /**< This member is not used */
    OMXR_MC_VIDEO_EXTENDED_DECODERESULTTYPE sExtendDecodeResult; /**< Extended Decode Result information */
    OMX_PTR pvReserved;                                          /**< This member is not used */
} OMXR_MC_VIDEO_DECODERESULTTYPE;

/**
 * Extended Deinterlace Mode Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_DEINTERLACE_MODETYPE {
    OMX_U32                    nSize;                        /**< Size of the structure */
    OMX_VERSIONTYPE            nVersion;                     /**< OMX specification version info */
    OMX_U32                    nPortIndex;                   /**< Target port index */
    OMXR_MC_VIDEO_DEINTERLACE_MODETYPE    eDeinterlace;      /**< The de-interlace mode flags */
    OMX_BOOL                   bDisableInterpolateTimeStamp; /**< Disable interpolate timestamp */
    OMX_U32                    nTimeStampOffset;             /**< Offset of timestamp is used by OMXR_MC_VIDEO_Deinterlace2DFull or OMXR_MC_VIDEO_Deinterlace3DFull case */
} OMXR_MC_VIDEO_PARAM_DEINTERLACE_MODETYPE;

/**
 * Extended Error Concealment Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_ERROR_CONCEALMENTTYPE {
    OMX_U32            nSize;                               /**< Size of the structure */
    OMX_VERSIONTYPE    nVersion;                            /**< OMX specification version info */
    OMX_U32            nPortIndex;                          /**< Target port index */
    OMX_BOOL           bSkipInterframe;                     /**< Skip output until first valid frame has input. :     default:OMX_FALSE */
    OMX_BOOL           bSuppressErrorEventUntilValidFrame;  /**< Don't emit Error Event until first frame has input.: default:OMX_FALSE */
} OMXR_MC_VIDEO_PARAM_ERROR_CONCEALMENTTYPE;

/**
 * Extended Dynamic Port Reconf In Decoding Type.
 * - If the IL client has set the bEnable of "Dynamic Port Reconf in Decoding" index to TRUE, 
 *   Media Component notifies the PortSettingChanged Event in accordance with the change of the output port information, 
 *   and wait for the re-configuration of the port.
 * - Please the IL client processes a sequence of "Dynamic Port Reconfiguration" prescribed in OpenMAX IL standard.
 * - The output port information after the change will apply after PortDisable.
 * - The target output port information is nFrameWidth, nFrameHeight, nStride, nSliceHeight.
 *   nStride and nSliceHeight must not set it to 0 with this mode.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_DYNAMIC_PORT_RECONF_IN_DECODINGTYPE {
    OMX_U32                       nSize;            /**< Size of the structure */
    OMX_VERSIONTYPE               nVersion;         /**< OMX specification version info */
    OMX_U32                       nPortIndex;       /**< Target port index */
    OMX_BOOL                      bEnable;          /**< Enable dynamic port re-configuration in decoding : default:OMX_FALSE */
} OMXR_MC_VIDEO_PARAM_DYNAMIC_PORT_RECONF_IN_DECODINGTYPE;

/**
 * Extended Muximum Decode Capability.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITY {
    OMX_U32         nSize;              /**< Size of the structure */
    OMX_VERSIONTYPE nVersion;           /**< OMX specification version info */
    OMX_U32         nPortIndex;         /**< Target port index */
    OMX_U32         nMaxDecodedWidth;   /**< Max width : default:1920(Full HD) / Set range w | 80 <= w <= 1920 ( 4096 if support 4K resolution ) */
    OMX_U32         nMaxDecodedHeight;  /**< Max height: default:1088(Full HD) / Set range h | 80 <= h <= 1920 ( 4096 if support 4K resolution ) */
    OMX_U32         eMaxLevel;          /**< Max level : default:OMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITY_MAXIMUM_LEVEL */
    OMX_BOOL        bForceEnable;       /**< If specified OMX_TRUE, ensure sufficient memory in order to decode with the given parameters.
                                             Otherwise, there's possibility of the memory shortage if the image size is changed during decoding.

                                              - The maximum value of OMX_VIDEO_PORTDEFINITIONTYPE.nSliceHeight is 1152 if all of the following conditions are satisfied:
                                                - bForceEnable == OMX_TRUE
                                                - OMXR_MC_VIDEO_PARAM_DEINTERLACE_MODETYPE.eDeinterlace == OMXR_MC_VIDEO_DeinterlaceNone,

                                              - The memory arrangement format is described in below if all of the following conditions are satisfied:
                                                - H264D
                                                - bForceEnable == OMX_TRUE
                                                - OMXR_MC_VIDEO_PARAM_DEINTERLACE_MODETYPE.eDeinterlace == OMXR_MC_VIDEO_DeinterlaceNone

                                              The memory arrangement changes according to the OMX_VIDEO_PORTDEFINITIONTYPE.eColorformat, as shown in the figures below:

                                              \image html YUV420_Planar_field.png "OMX_VIDEO_PORTDEFINITIONTYPE.eColorformat==OMX_COLOR_FormatYUV420Planar" 

                                              \image html YUV420_SemiPlanar_field.png "OMX_VIDEO_PORTDEFINITIONTYPE.eColorformat==OMX_COLOR_FormatYUV420SemiPlanar"
                                        */
} OMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITYTYPE;

/**
 * Extended Work Buffer Preference.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_WORKBUFFER_PREFERENCE {
    OMX_U32           nSize;
    OMX_VERSIONTYPE   nVersion;
    OMX_U32           nPortIndex;
    OMX_U32           nInputWorkbufferNum;
    OMX_U32           nInputWorkbufferSize;
    OMX_U32           nBufferingPicNum;
	OMX_U32           nDpbAdditionalNum;
	OMX_BOOL          bDisable3DIPConversion;
} OMXR_MC_VIDEO_PARAM_WORKBUFFER_PREFERENCETYPE;

/**
 * Extended Lossy compression mode type
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_LOSSY_COMPRESSION {
    OMX_U32          nSize;
    OMX_VERSIONTYPE  nVersion;
    OMX_U32          nPortIndex;
    OMX_BOOL         bEnable;
} OMXR_MC_VIDEO_PARAM_LOSSY_COMPRESSIONTYPE;

/**
 * Extended Time Stamp Mode Type.
 */
typedef struct tagOMXR_MC_VIDEO_PARAM_TIME_STAMP_MODETYPE {
    OMX_U32                                nSize;
    OMX_VERSIONTYPE                        nVersion;
    OMX_U32                                nPortIndex;
    OMXR_MC_VIDEO_TIMESTAMPMODETYPE        eTimeStampMode;
} OMXR_MC_VIDEO_PARAM_TIME_STAMP_MODETYPE;

/**
  * Extended PortSettingChanged Event Type.
*/
typedef struct tagOMXR_MC_VIDEO_PARAM_ENABLE_EXTENDED_PSC_EVENTTYPE {
    OMX_U32                       nSize;            /**< Size of the structure */
    OMX_VERSIONTYPE               nVersion;         /**< OMX specification version info */
    OMX_U32                       nPortIndex;       /**< Target port index */
    OMX_BOOL                      bEnable;          /**< Enable notification of Extended PSC event : default:OMX_FALSE */
} OMXR_MC_VIDEO_PARAM_ENABLE_EXTENDED_PSC_EVENTTYPE;


typedef struct tagOMXR_MC_VIDEO_PARAM_BYPASS_POSTPROCESSINGTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnable;
    OMX_BOOL        bLosslessCompression;
} OMXR_MC_VIDEO_PARAM_BYPASS_POSTPROCESSINGTYPE ;

/**
  * Extended SyntaxInfo Type.
*/
typedef struct tagOMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE {
    OMX_U32           nSize;
    OMX_VERSIONTYPE   nVersion;
    OMX_U32           nPortIndex;
    OMX_U32           u32SyntaxIndex;
    OMX_PTR           pvSyntaxInfo;
}OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE;

/***************************************************************************/
/* End of module                                                           */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VDCMN_H */
